#include "PLG_screens.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "PLG_state.h"
#include "PLG_theme.h"
#include "PLG_pins.h"
#include "PLG_display.h"
#include "PLG_charts.h"
#include "PLG_serial_link.h"
#include "PLG_lang.h"
#include "PLG_logo.hpp"

void drawLogoFull(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *img)
{
    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            myTFT.TFTdrawPixel(x + i, y + j, img[j * w + i]);
        }
    }
}

/*------------------- Splash khoi dong -------------------*/
// nen den tuyet doi, dong bo voi nen den cua logo -> khong con khung sang phia sau logo
static const uint16_t SPLASH_BG = RGB565(0, 0, 0);

// ve 1 khung loading (thanh bar + chu "Loading...N%"), dung chung cho ca doan chay
// nhanh 0-90% va doan cho du lieu dao dong 90-98%
static void draw_loading_frame(int16_t barX, int16_t barY, int16_t barW, int16_t barH, int a)
{
    char load[20] = "";
    // %3d: do dai chuoi luon co dinh (khong doi tu 1 sang 2, 3 chu so) -> khong can
    // fillRect xoa ca vung truoc khi ve lai, tranh hien tuong chop den moi lan cap nhat
    snprintf(load, sizeof(load), "Loading...%3d%%", a);
    myTFT.TFTdrawText(76, 208, load, UI_TEXT, SPLASH_BG, 2);

    int16_t fillW = (barW - 4) * a / 100;
    myTFT.TFTfillRect(barX + 2, barY + 2, fillW, barH - 4, UI_ACCENT);
}

void MONITOR_BEGIN()
{
    myTFT.TFTfillScreen(SPLASH_BG);
    drawLogoFull(100, 50, 120, 100, PLG_logo);

    // thanh loading ngay duoi logo, chu "Loading...N%" nam duoi thanh
    int16_t barX = 60, barY = 180, barW = 200, barH = 12;
    myTFT.TFTdrawRoundRect(barX, barY, barW, barH, 4, UI_BORDER);

    for (int a = 0; a <= 90; a++)
    {
        draw_loading_frame(barX, barY, barW, barH, a);
        // Doc Serial ngay ca trong doan chay nhanh nay (khong doi den vong cho ben
        // duoi) de tra loi bat tay "PLG_ID?" cang som cang tot ngay sau khi USB CDC
        // san sang - rut ngan thoi gian PC nhan dien lai board sau khi rut/cam.
        read_taskmanager_serial();
        sleep_ms(10);
    }

    // Doan cho du lieu that tu PC (monitor.py/.exe): chay cham dan tu 90% len 98% roi
    // dung lai o 98% (chi cap nhat lai hinh, khong lui xuong) cho den khi:
    //  - nhan duoc du lieu that qua Serial (taskmanager_dirty = true), hoac
    //  - nguoi dung nhan nut encoder 2 lan de bo qua thu cong (vd khi chua muon bat monitor.py)
    // Tranh truong hop vao thang man hinh chinh voi du lieu cu/gia (-1) khi PC chua gui gi.
    bool last_btn = gpio_get(button);
    int click_count = 0;
    int a = 90;
    while (!taskmanager_dirty && click_count < 2)
    {
        read_taskmanager_serial();

        bool now_btn = gpio_get(button);
        if (last_btn == 1 && now_btn == 0) // canh nhan xuong = 1 lan click
            click_count++;
        last_btn = now_btn;

        draw_loading_frame(barX, barY, barW, barH, a);
        if (a < 98)
            a++;

        sleep_ms(250);
    }

    for (int a2 = 99; a2 <= 100; a2++)
    {
        draw_loading_frame(barX, barY, barW, barH, a2);
        sleep_ms(40);
    }
    // giu hinh 100% hien ro mot nhip truoc khi chuyen man hinh, tranh cam giac "ket o 99%"
    sleep_ms(400);
    myTFT.TFTfillScreen(UI_BG);
}

/*------------------- Status bar (pin + ket noi) -------------------*/
// ve 1 icon pin (4 vach) + so % tai vi tri iconX, dung chung cho battery1 va battery2
static void draw_battery(int16_t iconX, int16_t textX, int8_t &value, int8_t &lastValue)
{
    if (value < 0)
        value = 0;
    if (value > 100)
        value = 100;
    if (lastValue != value)
    {
        myTFT.TFTfillRect(textX, 5, 40, 17, UI_BG);
        lastValue = value;
    }

    char text[4] = "";
    itoa(value, text, 10);
    myTFT.TFTdrawText(textX, 5, text, UI_TEXT, UI_BG, 2);

    uint16_t col = (value <= 20) ? UI_DANGER : UI_CPU; // luon xanh la, khong doi theo UI_ACCENT
    myTFT.TFTdrawRoundRect(iconX, 5, 34, 15, 3, UI_BORDER);
    myTFT.TFTfillRect(iconX + 34, 8, 4, 8, UI_BORDER);

    int bars = (value <= 25) ? 1 : (value <= 50) ? 2
                               : (value <= 75)   ? 3
                                                  : 4;
    for (int i = 0; i < 4; i++)
    {
        myTFT.TFTfillRoundRect(iconX + 1 + i * 8, 6, 7, 13, 2, (i < bars) ? col : UI_PANEL);
    }
}

// icon banh rang (settings): dich sat gan goc phai tren cung (truoc icon ket noi bat dau x288,
// chua r=7+2 rang(3px) cua icon nen dat cx=272 de khong de len icon ket noi), chi hien khi dang
// o trong SETTING (moi man hinh con nhu COLOR/FONT/LANGUAGE/TASK deu giu nguyen
// desktop_state == DESKTOP_SETING nen icon luon hien xuyen suot)
static const int16_t GEAR_CX = 272, GEAR_CY = 12;
static bool last_settings_icon_shown = false;

static void draw_gear_icon(int16_t cx, int16_t cy, uint16_t color)
{
    myTFT.TFTfillCircle(cx, cy, 7, color);
    myTFT.TFTfillCircle(cx, cy, 3, UI_BG); // lo tron giua banh rang
    // 8 rang nho quanh vien (goc N/S/E/W + 4 goc cheo), toa do co dinh, khong dung luong giac
    myTFT.TFTfillRect(cx - 1, cy - 9, 3, 3, color);
    myTFT.TFTfillRect(cx - 1, cy + 6, 3, 3, color);
    myTFT.TFTfillRect(cx - 9, cy - 1, 3, 3, color);
    myTFT.TFTfillRect(cx + 6, cy - 1, 3, 3, color);
    myTFT.TFTfillRect(cx - 6, cy - 6, 3, 3, color);
    myTFT.TFTfillRect(cx + 4, cy - 6, 3, 3, color);
    myTFT.TFTfillRect(cx - 6, cy + 4, 3, 3, color);
    myTFT.TFTfillRect(cx + 4, cy + 4, 3, 3, color);
}

void MONITOR_STATUS()
{
    if (last_status != status)
    {
        myTFT.TFTdrawLine(0, 23, 320, 23, UI_BORDER);
        last_status = status;
    }

    draw_battery(20, 60, battery1, last_battery1);   // pin remote
    draw_battery(100, 140, battery2, last_battery2); // pin may

    bool showGear = (desktop_state == DESKTOP_SETING);
    if (showGear != last_settings_icon_shown)
    {
        myTFT.TFTfillRect(GEAR_CX - 10, 0, 20, 22, UI_BG);
        if (showGear)
            draw_gear_icon(GEAR_CX, GEAR_CY, UI_ACCENT);
        last_settings_icon_shown = showGear;
    }

    /*---------------------status connected------------------------------*/
    if (LAST_CONNECT_STATUS != CONNECT_STATUS)
    {
        myTFT.TFTfillRect(288, 0, 32, 20, UI_BG);
        if (CONNECT_STATUS)
        {
            myTFT.TFTfillRoundRect(290, 15, 8, 5, 0, UI_ACCENT);  // song min
            myTFT.TFTfillRoundRect(300, 10, 8, 10, 0, UI_ACCENT); // song mid
            myTFT.TFTfillRoundRect(310, 5, 8, 15, 0, UI_ACCENT);  // song max
        }
        else
        {
            myTFT.TFTdrawLine(290, 5, 318, 15, UI_DANGER);
            myTFT.TFTdrawLine(318, 5, 290, 15, UI_DANGER);
        }
        LAST_CONNECT_STATUS = CONNECT_STATUS;
    }
}

/*------------------- Task Manager (5 chart + dong ho goc tren-phai) -------------------*/
// gio hien tai o goc tren-phai thanh trang thai, dat vao khoang trong giua pin may
// (ket thuc ~x174) va icon ket noi (bat dau x288) -> khong chong len noi dung pin
static char last_time_str_top[9] = "";

void reset_taskmanager_clock_cache()
{
    strcpy(last_time_str_top, "");
}

// khai bao truoc: dung trong draw_clock_cpu_ram (dinh nghia ben duoi, sau get_clock_char_metrics)
void reset_clock_cpu_ram_cache();
static void draw_clock_cpu_ram();

static void draw_taskmanager_clock()
{
    if (strcmp(last_time_str_top, current_time_str) != 0)
    {
        // size2, chi hien HH:MM (bo giay) de vua khoang trong: 5 ky tu * size*(5+1)=12px = 60px,
        // x=190 -> ket thuc 250px, khong vuot nguong tran/wrap cua TFTdrawText va khong de len pin may
        char hm[6] = "";
        strncpy(hm, current_time_str, 5);
        hm[5] = '\0';
        myTFT.TFTfillRect(188, 4, 65, 17, UI_BG);
        myTFT.TFTdrawText(190, 5, hm, UI_ACCENT, UI_BG, 2); // dong bo mau voi mau nhan dien da chon
        strncpy(last_time_str_top, current_time_str, sizeof(last_time_str_top) - 1);
        last_time_str_top[sizeof(last_time_str_top) - 1] = '\0';
    }
}

void MONITOR_TASKMANAGER()
{
    ChartLayout c[6];
    get_chart_layout(c);

    if (last_desktop != desktop)
    {
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        for (int i = 0; i < 6; i++)
            draw_chart_frame(c[i].x, c[i].y, c[i].w, c[i].h);
        last_desktop = desktop;
        taskmanager_dirty = true;
        reset_taskmanager_clock_cache(); // buoc ve lai gio sau khi xoa man hinh
        reset_chart_cache();              // buoc 6 chart ve lai (man hinh vua bi xoa trang)
    }

    if (taskmanager_dirty)
    {
        draw_chart_data(c[0].x, c[0].y, c[0].w, c[0].h, "CPU", chart_cpu, UI_CPU, 101, nullptr, 0);
        draw_chart_data(c[1].x, c[1].y, c[1].w, c[1].h, "RAM", chart_ram, UI_RAM, 90, "BOOST NOW", 1);
        draw_chart_data(c[2].x, c[2].y, c[2].w, c[2].h, "GPU", chart_gpu, UI_GPU, 101, nullptr, 2);
        // "MEM" (3 ky tu, khong phai "VRAM" 4 ky tu): o khung ben phai (x~164) TFTdrawText dung
        // con tro X kieu uint8_t (toi da 255) - nhan dai hon 1 ky tu la du lam tran/wrap ve dau man hinh
        draw_chart_data(c[3].x, c[3].y, c[3].w, c[3].h, "MEM", chart_gpumem, UI_GPUMEM, 101, nullptr, 3);
        // WIFI chia doi voi TEMP (nhiet do CPU) tren cung 1 hang, moi o rong bang nua nhu truoc.
        // O nay nam o nua PHAI man hinh (x~164) - giong ly do "MEM" duoc chon thay vi "VRAM" o
        // tren: TFTdrawText dung con tro X kieu uint8_t (toi da 255), nhan "TEMP" (4 ky tu) +
        // "  NNC" se lam ky tu cuoi tinh ra vi tri >255 va tran ve dau man hinh - dung "TMP" (3
        // ky tu, bang do dai voi "RAM"/"MEM" da an toan) va KHONG dung warnText (vd "HOT") vi
        // ngay ca voi nhan ngan, chuoi canh bao dai them se lai vuot qua nguong an toan nay.
        draw_chart_data(c[4].x, c[4].y, c[4].w, c[4].h, "WIFI", chart_wifi, UI_WIFI, 101, nullptr, 4);
        draw_chart_data(c[5].x, c[5].y, c[5].w, c[5].h, "TMP", chart_temp, UI_TEMP, 101, nullptr, 5, "C");

        taskmanager_dirty = false;
    }

    MONITOR_STATUS();
    draw_taskmanager_clock();
}

/*------------------- Menu SETTING (dang "wheel/carousel" cuon doc) -------------------*/
// 5 "khe" hien thi cung luc quanh muc dang chon: khe giua (dang chon) to nhat + noi bat, 2 khe ke
// nho hon, 2 khe ngoai cung chi la chu mo nhat (gia cam giac banh xe cuon). Danh sach cuon vong
// (FUNTION_MODE_COUNT muc), nen moi lan doi muc phai ve lai toan bo 5 khe (khac voi menu dang list
// cu chi can ve lai 2 dong) vi noi dung TUNG khe deu doi theo huong cuon.
// to hon ban dau (size 1/2/3 -> 2/3/4) va chi xoa dung vung khoi wheel (thay vi ca vung noi dung
// 216px) truoc khi ve lai -> giam dien tich + thoi gian ve moi lan doi muc, chuyen muot hon
static const int16_t CAR_ROW_H[5] = {20, 30, 40, 30, 20};   // chieu cao tung khe: ngoai->giua->ngoai
static const uint8_t CAR_ROW_SIZE[5] = {2, 3, 4, 3, 2};     // co chu tung khe (TFTdrawText size)
static const int16_t CAR_GAP = 4;                            // khoang cach giua cac khe
static const int16_t CAR_BOX_X = 50, CAR_BOX_W = 220;         // khung nen chi ve cho khe giua + ke

// vi tri Y cua tung khe khong doi theo funtion_mode (chi NOI DUNG/chu ben trong doi) - nen khung
// nen (box fill+border) chi can ve 1 LAN duy nhat luc vao menu; moi lan doi muc chi can xoa+ve lai
// phan CHU ben trong khung (vung nho hon nhieu so voi xoa+ve lai ca khoi) -> giam giat/nhap nhay
// ro ret so voi cach cu (xoa toan khoi + ve lai het khung moi lan cuon).
static int16_t car_row_y[5]; // cache vi tri Y da tinh (dung chung giua ve khung va ve chu)

static void car_compute_layout()
{
    int16_t total = CAR_GAP * 4;
    for (int i = 0; i < 5; i++)
        total += CAR_ROW_H[i];
    int16_t y = 24 + (216 - total) / 2; // can giua khoi wheel trong vung noi dung (24..240)
    for (int slot = 0; slot < 5; slot++)
    {
        car_row_y[slot] = y;
        y += CAR_ROW_H[slot] + CAR_GAP;
    }
}

// ve khung nen (fill + border) ca 5 khe - goi 1 lan duy nhat khi vua vao menu (menu_needs_full_draw)
static void draw_menu_carousel_frame()
{
    int16_t total = CAR_GAP * 4;
    for (int i = 0; i < 5; i++)
        total += CAR_ROW_H[i];
    myTFT.TFTfillRect(0, car_row_y[0] - 2, 320, total + 4, UI_BG);

    for (int slot = 0; slot < 5; slot++)
    {
        int16_t y = car_row_y[slot], h = CAR_ROW_H[slot];
        if (slot == 2) // khe giua (dang chon)
        {
            myTFT.TFTfillRoundRect(CAR_BOX_X, y, CAR_BOX_W, h, 6, UI_PANEL_HI);
            myTFT.TFTdrawRoundRect(CAR_BOX_X, y, CAR_BOX_W, h, 6, UI_ACCENT);
        }
        else if (slot == 1 || slot == 3) // 2 khe ke
        {
            myTFT.TFTfillRoundRect(CAR_BOX_X, y, CAR_BOX_W, h, 4, UI_PANEL);
            myTFT.TFTdrawRoundRect(CAR_BOX_X, y, CAR_BOX_W, h, 4, UI_BORDER);
        }
        // khe ngoai cung (slot 0/4): khong co khung, chi chu mo tren nen UI_BG
    }
}

// chi xoa + ve lai CHU trong tung khe (khung nen giu nguyen) - goi moi khi doi muc dang chon
static void draw_menu_carousel_labels()
{
    const char *const *labels = lang_menu_labels();

    for (int slot = 0; slot < 5; slot++)
    {
        int8_t idx = (int8_t)(((funtion_mode - 2 + slot) % FUNTION_MODE_COUNT + FUNTION_MODE_COUNT) % FUNTION_MODE_COUNT);
        int16_t y = car_row_y[slot], h = CAR_ROW_H[slot];
        uint8_t size = CAR_ROW_SIZE[slot];
        bool isCenter = (slot == 2);
        bool isAdjacent = (slot == 1 || slot == 3);
        uint16_t bg = isCenter ? UI_PANEL_HI : (isAdjacent ? UI_PANEL : UI_BG);
        uint16_t fg = isCenter ? UI_ACCENT : UI_TEXT_FAINT; // mo hon nua so voi UI_TEXT_DIM truoc day

        // xoa vung chu cu (chua cham vien khung, chi phan ruot ben trong)
        myTFT.TFTfillRect(CAR_BOX_X + 3, y + 2, CAR_BOX_W - 6, h - 4, bg);

        int16_t charW = (int16_t)(6 * size);
        int16_t textW = (int16_t)strlen(labels[idx]) * charW;
        int16_t tx = 160 - textW / 2; // can giua theo truc doc man hinh (320/2)
        if (tx < 0)
            tx = 0;
        int16_t ty = y + (h - 8 * size) / 2;
        myTFT.TFTdrawText(tx, ty, (char *)labels[idx], fg, bg, size);
    }
}

void MONITOR_FUNTION()
{
    MONITOR_STATUS();

    // cuon vong: cuoi danh sach quay len dau va nguoc lai (thay vi dung khung o 2 dau khi quay nhanh).
    // dung % thay vi if de xu ly dung ca khi quay rat nhanh khien funtion_mode nhay qua nhieu buoc cung luc.
    funtion_mode = ((funtion_mode % FUNTION_MODE_COUNT) + FUNTION_MODE_COUNT) % FUNTION_MODE_COUNT;

    if (menu_needs_full_draw)
    {
        car_compute_layout();
        draw_menu_carousel_frame();
        draw_menu_carousel_labels();
        last_funtion_mode = funtion_mode;
        menu_needs_full_draw = false;
    }
    else if (last_funtion_mode != funtion_mode)
    {
        // khung nen da ve san, vi tri khong doi -> chi can ve lai chu, muot hon nhieu so voi
        // xoa+ve lai ca khoi (xem draw_menu_carousel_labels)
        draw_menu_carousel_labels();
        last_funtion_mode = funtion_mode;
    }
}

/*------------------- Man hinh dong ho -------------------*/
// 5 HO CHU (font family) co the chon cho dong ho, chon theo 2 buoc trong SETTING > FONT:
// buoc 1 chon ho chu (CLOCK_FONT_COUNT o PLG_state.h), buoc 2 chon co chu (size) rieng cho ho
// do. Deu la font co ban (1-6) cua thu vien, ho tro phong to tuy y qua tham so size cua
// TFTdrawChar - khac voi Bignum/Mednum (font 7-8) chi co 1 kich thuoc co dinh, khong scale duoc.
// CLOCK_FONT_MAX_SIZE gioi han rieng cho tung ho de 8 ky tu "HH:MM:SS" luon vua man hinh rong
// 320px (size*(baseW+1)*8 <= 320).
static const char *CLOCK_FONT_NAMES[CLOCK_FONT_COUNT] = {"DEFAULT", "THICK", "7-SEGMENT", "WIDE", "HOMESPUN"};
static const int16_t CLOCK_FONT_BASE_W[CLOCK_FONT_COUNT] = {5, 7, 4, 8, 7};
static const int8_t CLOCK_FONT_MAX_SIZE[CLOCK_FONT_COUNT] = {6, 5, 6, 4, 5};
// co chu nho nhat cho phep chon (duoi muc nay chu qua nho, kho doc tren man 240x320)
static const int8_t CLOCK_FONT_MIN_SIZE = 2;

static int8_t clamp_clock_size(int8_t style, int8_t size)
{
    if (size < CLOCK_FONT_MIN_SIZE)
        return CLOCK_FONT_MIN_SIZE;
    if (size > CLOCK_FONT_MAX_SIZE[style])
        return CLOCK_FONT_MAX_SIZE[style];
    return size;
}
// so muc co chu co the chon cho 1 ho chu, va gia tri size thuc te ung voi 1 muc (index)
static int8_t get_clock_size_count(int8_t style) { return CLOCK_FONT_MAX_SIZE[style] - CLOCK_FONT_MIN_SIZE + 1; }
// khong static: PLG_input.cpp can goi khi ap dung lua chon size (xem PLG_screens.h)
int8_t get_clock_size_value(int8_t style, int8_t index) { return clamp_clock_size(style, (int8_t)(CLOCK_FONT_MIN_SIZE + index)); }
// tra ve index (0-based) ung voi 1 gia tri size cho truoc, dung de khoi tao lai vi tri duyet
// trong man hinh chon CO CHU tu gia tri active_clock_size hien tai (xem PLG_input.cpp)
int8_t clock_size_value_to_index(int8_t style, int8_t sizeValue)
{
    return (int8_t)(clamp_clock_size(style, sizeValue) - CLOCK_FONT_MIN_SIZE);
}

// kich thuoc 1 ky tu ("HH:MM:SS") theo ho chu + co chu, dung chung cho ve dong ho va xem truoc
static void get_clock_char_metrics(int8_t style, int8_t size, int16_t &charW, int16_t &charH)
{
    int8_t s = clamp_clock_size(style, size);
    charW = s * (CLOCK_FONT_BASE_W[style] + 1);
    charH = s * 8;
}

// ve 1 ky tu gio tai (x,y) theo ho chu + co chu. Chuyen font roi tra ve font mac dinh ngay sau
// do vi TFTFontNum() la trang thai dung chung toan bo thu vien - cac man hinh/dong khac (status
// bar, ngay, menu, AM/PM...) deu gia dinh dang o font mac dinh.
static void draw_clock_font_char(int8_t style, int8_t size, int16_t x, int16_t y, char c)
{
    switch (style)
    {
    case 1:
        myTFT.TFTFontNum(myTFT.TFTFont_Thick);
        break;
    case 2:
        myTFT.TFTFontNum(myTFT.TFTFont_Seven_Seg);
        break;
    case 3:
        myTFT.TFTFontNum(myTFT.TFTFont_Wide);
        break;
    case 4:
        myTFT.TFTFontNum(myTFT.TFTFont_HomeSpun);
        break;
    default:
        myTFT.TFTFontNum(myTFT.TFTFont_Default);
        break;
    }
    myTFT.TFTdrawChar(x, y, (uint8_t)c, UI_ACCENT, UI_BG, clamp_clock_size(style, size));
    myTFT.TFTFontNum(myTFT.TFTFont_Default);
}

static const int16_t TIME_LEN = 8; // "HH:MM:SS"
// Neo tren cua khoi gio, chon du thap de ho/co chu lon nhat co the chon (48px cao) + dong AM/PM
// + dong ngay ben duoi van vua vung noi dung (y=24..240) - xem cach tinh dateY trong MONITOR_CLOCK.
static const int16_t TIME_Y = 60;

// xoa + ve lai DUNG 1 ky tu gio tai vi tri idx, theo ho/co chu dang ap dung
// (active_clock_font/active_clock_size). Cap nhat tung ky tu (thay vi ca chuoi) giup hieu ung
// doi giay muot hon: da so lan chi doi 1-2 ky tu cuoi (giay), cac ky tu khac (gio/phut/dau ':')
// dung nguyen, khong chop lai ca dong.
static void draw_time_char(int idx, char c)
{
    int16_t charW, charH;
    get_clock_char_metrics(active_clock_font, active_clock_size, charW, charH);
    int16_t x = (320 - TIME_LEN * charW) / 2 + idx * charW;
    myTFT.TFTfillRect(x, TIME_Y, charW, charH, UI_BG);
    draw_clock_font_char(active_clock_font, active_clock_size, x, TIME_Y, c);
}

// chuyen "HH:MM:SS" 24h (tu monitor.py) sang hien thi 12h + co AM/PM; MM:SS giu nguyen.
// out12 phai cung do dai voi in24 (9 byte, 8 ky tu + '\0').
static void format_time_12h(const char *in24, char *out12, bool *isPM)
{
    int hh = (in24[0] - '0') * 10 + (in24[1] - '0');
    *isPM = (hh >= 12);
    int hh12 = hh % 12;
    if (hh12 == 0)
        hh12 = 12;
    out12[0] = '0' + (hh12 / 10);
    out12[1] = '0' + (hh12 % 10);
    strncpy(out12 + 2, in24 + 2, 6); // ":MM:SS" (6 ky tu) giu nguyen tu chuoi goc
    out12[8] = '\0';
}

// nhan AM/PM can giua ngay duoi chu gio (khong dat ben phai): voi cac ho chu rong (vd WIDE),
// dat ben phai se day x qua nguong 255 cua con tro X kieu uint8_t trong TFTdrawText, khien nhan
// bi tran so va ve sai vi tri (nhay ve gan le trai man hinh) - dat o giua ben duoi tranh duoc
// van de nay cho moi ho/co chu, dong thoi don gian va can doi hon.
// Chi ve lai khi doi giua AM/PM (2 lan/ngay) - dung tri-state (-1 = chua ve) de ep ve lai lan dau.
static int8_t last_ampm_shown = -1;
static void draw_ampm(bool isPM)
{
    int8_t val = isPM ? 1 : 0;
    if (val == last_ampm_shown)
        return;
    last_ampm_shown = val;

    int16_t charW, charH;
    get_clock_char_metrics(active_clock_font, active_clock_size, charW, charH);
    int16_t y = TIME_Y + charH + 4;
    const int16_t ampmW = 2 * 2 * (5 + 1); // size2, 2 ky tu ("AM"/"PM")
    int16_t x = (320 - ampmW) / 2;

    myTFT.TFTfillRect(0, y, 320, 20, UI_BG);
    myTFT.TFTdrawText(x, y, (char *)(isPM ? "PM" : "AM"), UI_ACCENT, UI_BG, 2);
}

/*------------------- CPU/RAM duoi dong ngay thang, man hinh dong ho -------------------*/
// Vi tri Y phu thuoc chieu cao ky tu gio dang chon (active_clock_font/active_clock_size), tinh
// giong cach tinh dateY trong MONITOR_CLOCK ben duoi de luon nam sat duoi dong ngay (DD/MM/YYYY)
// bat ke ho/co chu nao dang ap dung.
// Nhan/gia tri van can tuan thu gioi han uint8_t cua con tro X trong TFTdrawText (toi da 255):
// clamp gia tri hien thi toi da 99% de giu do rong chuoi "C:99%" co dinh (toi da 5 ky tu).
static int16_t get_clock_stat_y()
{
    int16_t charW, charH;
    get_clock_char_metrics(active_clock_font, active_clock_size, charW, charH);
    // TIME_Y + charH + 4 (gap AM/PM) + 20 (cao AM/PM) + 8 (gap) + 25 (cao dong ngay) + 6 (gap)
    return TIME_Y + charH + 4 + 20 + 8 + 25 + 6;
}

static const int16_t CLOCK_STAT_H = 26;
static const int16_t CLOCK_STAT_GAP = 14; // khoang cach giua 2 nhan CPU/RAM
// can giua theo dung chieu rong man hinh (320), giong moi thanh phan khac trong file nay
// (gio, ngay, AM/PM...). x cuoi cung van duoc clamp phia duoi de khong bao gio tran uint8_t.
static const int16_t SCREEN_W = 320;
static int8_t last_clock_cpu_shown = -2, last_clock_ram_shown = -2; // -2 = chua ve lan nao

void reset_clock_cpu_ram_cache()
{
    myTFT.TFTfillRect(0, get_clock_stat_y(), 320, CLOCK_STAT_H, UI_BG);
    last_clock_cpu_shown = -2;
    last_clock_ram_shown = -2;
}

static void draw_clock_cpu_ram()
{
    // gia tri -1 = "chua co du lieu that" (xem PLG_state.h), hien thi 0 thay vi so am;
    // clamp toi da 99 de gioi han do rong chuoi toi da, tranh tran x=255
    int8_t cpu = chart_cpu[CHART_SAMPLES - 1];
    int8_t ram = chart_ram[CHART_SAMPLES - 1];
    if (cpu < 0)
        cpu = 0;
    if (cpu > 99)
        cpu = 99;
    if (ram < 0)
        ram = 0;
    if (ram > 99)
        ram = 99;

    // ve lai ca cap khi 1 trong 2 gia tri doi, vi vi tri (can giua theo tong do rong) phu thuoc ca 2
    if (last_clock_cpu_shown == cpu && last_clock_ram_shown == ram)
        return;
    last_clock_cpu_shown = cpu;
    last_clock_ram_shown = ram;

    char cpuText[8], ramText[8];
    snprintf(cpuText, sizeof(cpuText), "%s:%d%%", lang_label_cpu(), cpu);
    snprintf(ramText, sizeof(ramText), "%s:%d%%", lang_label_ram(), ram);

    const int16_t charW = 3 * (5 + 1); // size3
    int16_t cpuW = (int16_t)strlen(cpuText) * charW;
    int16_t ramW = (int16_t)strlen(ramText) * charW;
    int16_t totalW = cpuW + CLOCK_STAT_GAP + ramW;
    int16_t startX = (SCREEN_W - totalW) / 2;
    if (startX < 0)
        startX = 0;
    if (startX + totalW > 255) // TFTdrawText nhan x kieu uint8_t, khong bao gio de tran
        startX = 255 - totalW;
    if (startX < 0)
        startX = 0;

    int16_t y = get_clock_stat_y();
    myTFT.TFTfillRect(0, y, 320, CLOCK_STAT_H, UI_BG);
    myTFT.TFTdrawText(startX, y, cpuText, UI_CPU, UI_BG, 3);                      // dong bo mau voi bieu do CPU
    myTFT.TFTdrawText(startX + cpuW + CLOCK_STAT_GAP, y, ramText, UI_RAM, UI_BG, 3); // dong bo mau voi bieu do RAM
}

// man hinh dong ho: hien gio (12h + AM/PM) + ngay hien tai (nhan tu monitor.py qua serial,
// truong TIME/DATE). Gio ve theo ho/co chu dang chon trong SETTING > FONT
// (active_clock_font/active_clock_size).
void MONITOR_CLOCK()
{
    MONITOR_STATUS();

    if (last_show_clock != show_clock)
    {
        last_show_clock = show_clock;
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        // memset ve 0 (khong phai strcpy("")): can moi phan tu trong mang deu khac ky tu that
        // (so/':') de vong lap so sanh tung ky tu ben duoi bat buoc ve lai TOAN BO 8 ky tu ngay
        // sau khi man hinh vua bi xoa trang - strcpy("") chi dat byte dau ve 0, cac byte con lai
        // van giu gia tri cu va co the trung voi gio hien tai, khien ky tu do bi bo qua (khong
        // ve lai) va de trong mot khoang den tren man hinh. Dieu nay cung xoa sach vung gio cu
        // neu nguoi dung vua doi kieu chu (kich thuoc ky tu khac nhau giua cac kieu).
        memset(last_time_str, 0, sizeof(last_time_str));
        last_ampm_shown = -1;
        strcpy(last_date_str, "");
        clock_dirty = true;
        reset_clock_cpu_ram_cache(); // xoa vung CPU/RAM cu (vd gio Task Manager) + buoc ve lai

        // xoa gio nho HH:MM cua Task Manager con sot lai tren thanh trang thai (o day y<24,
        // nam ngoai vung noi dung vua xoa o tren) khi chuyen tu Task Manager sang Clock
        myTFT.TFTfillRect(188, 4, 65, 17, UI_BG);
        reset_taskmanager_clock_cache(); // buoc ve lai gio do khi quay lai Task Manager
    }

    draw_clock_cpu_ram();

    if (clock_dirty)
    {
        char display_time[9];
        bool isPM;
        format_time_12h(current_time_str, display_time, &isPM);

        for (int i = 0; i < TIME_LEN; i++)
        {
            if (last_time_str[i] != display_time[i])
                draw_time_char(i, display_time[i]);
        }
        strcpy(last_time_str, display_time);
        draw_ampm(isPM);

        if (strcmp(last_date_str, current_date_str) != 0)
        {
            // vi tri dong ngay phu thuoc chieu cao ky tu gio (charH thay doi theo ho/co chu) -
            // tinh dong de khong bi dong AM/PM de len khi dung co chu lon nhat (cao nhat)
            int16_t charW, charH;
            get_clock_char_metrics(active_clock_font, active_clock_size, charW, charH);
            int16_t dateY = TIME_Y + charH + 4 /*gap den AM/PM*/ + 20 /*cao dong AM/PM*/ + 8 /*gap*/;

            myTFT.TFTfillRect(0, dateY, 320, 25, UI_BG);
            // x=70 de can giua "DD/MM/YYYY" (10 ky tu, size 3 -> rong ~177px tren man 320px).
            // Khong duoc dat x qua ~90: TFTdrawText dung con tro X kieu uint8_t, neu x + do rong
            // vuot qua 255 no se tran so va ky tu cuoi nhay ve sat le trai man hinh.
            myTFT.TFTdrawText(70, dateY + 3, current_date_str, UI_TEXT_DIM, UI_BG, 3);
            strcpy(last_date_str, current_date_str);
        }
        clock_dirty = false;
    }
}

/*------------------- Man hinh chon HO CHU / CO CHU cho dong ho (2 buoc) -------------------*/
// xem truoc 1 mau gio co dinh ("12:34:56") theo ho chu style + co chu size, can giua man hinh
static void draw_clock_font_sample(int8_t style, int8_t size)
{
    const char *sample = "12:34:56";
    int16_t charW, charH;
    get_clock_char_metrics(style, size, charW, charH);
    int16_t x = (320 - TIME_LEN * charW) / 2;
    for (int i = 0; sample[i] != '\0'; i++)
    {
        draw_clock_font_char(style, size, x + i * charW, 70, sample[i]);
    }
}

// co chu dung de xem truoc o buoc 1 (chon HO CHU): co dinh, du nho de an toan voi moi ho chu
// (nho nhat trong CLOCK_FONT_MAX_SIZE la 4 - WIDE) -> chon 3 de con chenh lech voi min-size(2)
static const int8_t CLOCK_FONT_PREVIEW_SIZE = 3;

// buoc 1/2: dung encoder duyet qua 5 HO CHU (xem truoc mau gio "12:34:56" o co chu co dinh),
// nhan nut de chuyen sang buoc 2 chon CO CHU. Cau truc tuong tu MONITOR_COLOR.
void MONITOR_FONT()
{
    MONITOR_STATUS();

    clock_font_index = (int8_t)(((clock_font_index % CLOCK_FONT_COUNT) + CLOCK_FONT_COUNT) % CLOCK_FONT_COUNT);

    if (last_show_font != show_font)
    {
        last_show_font = show_font;
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        last_clock_font_index = -1;
    }

    if (last_clock_font_index != clock_font_index)
    {
        last_clock_font_index = clock_font_index;

        // vung xem truoc thay doi kich thuoc theo tung ho chu -> xoa ca vung roi ve lai mau +
        // ten kieu; day la man hinh cai dat, chon it thay doi, chop nhe luc doi ho chu la chap nhan duoc
        myTFT.TFTfillRect(0, 40, 320, 130, UI_BG);

        draw_clock_font_sample(clock_font_index, CLOCK_FONT_PREVIEW_SIZE);

        const char *name = CLOCK_FONT_NAMES[clock_font_index];
        int16_t nameW = (int16_t)strlen(name) * 3 * (5 + 1); // size3
        myTFT.TFTdrawText((320 - nameW) / 2, 130, (char *)name, UI_ACCENT, UI_BG, 3);

        // x tinh dong theo do dai chuoi (khac nhau giua VI/EN), size1 -> 6px/ky tu
        const char *hint1 = lang_hint_choose_size();
        int16_t hint1X = (320 - (int16_t)strlen(hint1) * 6) / 2;
        myTFT.TFTdrawText(hint1X, 155, (char *)hint1, UI_TEXT_DIM, UI_BG, 1);
    }
}

// buoc 2/2: dung encoder duyet qua cac co chu (size) hop le cua HO CHU da chon o buoc 1 (xem
// truoc mau gio "12:34:56" dung dung co chu dang duyet), nhan nut de ap dung ca hai (luu flash)
// va quay lai menu SETTING.
void MONITOR_FONT_SIZE()
{
    MONITOR_STATUS();

    int8_t sizeCount = get_clock_size_count(active_clock_font);
    clock_size_index = (int8_t)(((clock_size_index % sizeCount) + sizeCount) % sizeCount);

    if (last_show_font_size != show_font_size)
    {
        last_show_font_size = show_font_size;
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        last_clock_size_index = -1;
    }

    if (last_clock_size_index != clock_size_index)
    {
        last_clock_size_index = clock_size_index;
        int8_t size = get_clock_size_value(active_clock_font, clock_size_index);

        // vung xem truoc thay doi kich thuoc theo tung co chu -> xoa ca vung roi ve lai mau + nhan size
        myTFT.TFTfillRect(0, 40, 320, 130, UI_BG);

        draw_clock_font_sample(active_clock_font, size);

        char sizeLabel[10];
        snprintf(sizeLabel, sizeof(sizeLabel), "Size %d", size);
        int16_t labelW = (int16_t)strlen(sizeLabel) * 3 * (5 + 1); // size3
        myTFT.TFTdrawText((320 - labelW) / 2, 130, sizeLabel, UI_ACCENT, UI_BG, 3);

        // x tinh dong theo do dai chuoi (khac nhau giua VI/EN), size1 -> 6px/ky tu
        const char *hint2 = lang_hint_apply();
        int16_t hint2X = (320 - (int16_t)strlen(hint2) * 6) / 2;
        myTFT.TFTdrawText(hint2X, 155, (char *)hint2, UI_TEXT_DIM, UI_BG, 1);
    }
}

/*------------------- Man hinh chon mau -------------------*/
// vi tri + kich thuoc 1 o mau, dung chung cho ve khung lan dau va cap nhat khi doi lua chon
static const int16_t COLOR_SW_W = 26, COLOR_SW_GAP = 6, COLOR_SW_START_X = 12, COLOR_SW_Y = 55, COLOR_SW_H = 26;

// ve 1 o mau tai vi tri i; xoa truoc vung vien-noi-bat (outer highlight) cua lan chon truoc
// do -2px so voi o mau, tranh con sot vien khi chuyen tu trang thai selected sang khong selected
static void draw_color_swatch(uint8_t i, bool selected)
{
    int16_t sx = COLOR_SW_START_X + i * (COLOR_SW_W + COLOR_SW_GAP);
    myTFT.TFTfillRect(sx - 3, COLOR_SW_Y - 3, COLOR_SW_W + 6, COLOR_SW_H + 6, UI_BG);
    myTFT.TFTfillRoundRect(sx, COLOR_SW_Y, COLOR_SW_W, COLOR_SW_H, 4, UI_ACCENT_PRESETS[i]);
    myTFT.TFTdrawRoundRect(sx, COLOR_SW_Y, COLOR_SW_W, COLOR_SW_H, 4, selected ? UI_TEXT : UI_BORDER);
    if (selected)
        myTFT.TFTdrawRoundRect(sx - 2, COLOR_SW_Y - 2, COLOR_SW_W + 4, COLOR_SW_H + 4, 5, UI_TEXT);
}

// xoa + ve lai chi dong ten mau (thay doi moi lan doi lua chon), khong dung ca vung nhu truoc
// -> tranh chop man hinh khi xoay encoder nhanh.
// Font mac dinh: rong 5px + 1px cach, cao 8px -> voi size 3 la 18px/ky tu, cao 24px; truoc day
// vung xoa chi cao 24px bat dau tu y=105 (den y=129) trong khi chu ve tu y=110 den y=134,
// cat mat 5px duoi cung -> chu moi ngan hon de sot lai net chu cu. Tang vung xoa len 34px
// (het khoang y=134) de xoa sach; can giua theo do dai ten mau (khac nhau giua cac mau).
static void draw_color_name(int8_t idx)
{
    const int16_t charW = 3 * (5 + 1); // size3 * (font width + khoang cach)
    int16_t textW = (int16_t)strlen(UI_ACCENT_NAMES[idx]) * charW;
    int16_t x = (320 - textW) / 2;
    myTFT.TFTfillRect(0, 105, 320, 34, UI_BG);
    myTFT.TFTdrawText(x, 110, (char *)UI_ACCENT_NAMES[idx], UI_ACCENT_PRESETS[idx], UI_BG, 3);
}

// man hinh chinh mau giao dien: dung encoder chon 1 trong cac mau nhan dien co san,
// nhan nut de ap dung va quay lai menu SETTING
void MONITOR_COLOR()
{
    MONITOR_STATUS();

    // cuon vong nhu menu SETTING; dung % de dung ca khi quay rat nhanh nhay qua nhieu buoc
    color_index = (int8_t)(((color_index % (int8_t)UI_ACCENT_PRESET_COUNT) + (int8_t)UI_ACCENT_PRESET_COUNT) % (int8_t)UI_ACCENT_PRESET_COUNT);

    if (last_show_color != show_color)
    {
        last_show_color = show_color;
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        last_color_index = -1;
    }

    if (last_color_index < 0)
    {
        // vao man hinh lan dau (hoac quay lai): ve toan bo 1 lan, gom ca dong huong dan tinh
        for (uint8_t i = 0; i < UI_ACCENT_PRESET_COUNT; i++)
            draw_color_swatch(i, i == color_index);
        draw_color_name(color_index);
        // x tinh dong theo do dai chuoi (khac nhau giua VI/EN), size1 -> 6px/ky tu
        const char *hint = lang_hint_apply();
        int16_t hintX = (320 - (int16_t)strlen(hint) * 6) / 2;
        myTFT.TFTdrawText(hintX, 145, (char *)hint, UI_TEXT_DIM, UI_BG, 1);
        last_color_index = color_index;
    }
    else if (last_color_index != color_index)
    {
        // chi ve lai 2 o mau bi anh huong (o cu bo chon + o moi duoc chon) va dong ten mau,
        // thay vi xoa lai toan bo vung -> chuyen doi giua cac mau muot, khong nhap nhay
        draw_color_swatch(last_color_index, false);
        draw_color_swatch(color_index, true);
        draw_color_name(color_index);
        last_color_index = color_index;
    }
}

/*------------------- Man hinh chon ngon ngu giao dien (VI/EN) -------------------*/
// ve 1 dong ten ngon ngu (idx: 0=VI, 1=EN), can giua man hinh; xoa vung chu cu truoc khi ve lai
static const int16_t LANG_ROW_Y[UI_LANG_COUNT] = {90, 130};
static void draw_language_row(int8_t idx, bool selected)
{
    const char *name = lang_name(idx);
    int16_t textW = (int16_t)strlen(name) * 3 * (5 + 1); // size3
    int16_t x = (320 - textW) / 2;
    uint16_t fg = selected ? UI_ACCENT : UI_TEXT_DIM;
    myTFT.TFTfillRect(0, LANG_ROW_Y[idx] - 3, 320, 30, UI_BG);
    myTFT.TFTdrawText(x, LANG_ROW_Y[idx], (char *)name, fg, UI_BG, 3);
}

// dung encoder duyet qua VI/EN (xem ten ngon ngu bang chinh ngon ngu dang duyet), nhan nut de
// ap dung va quay lai menu SETTING. Cau truc tuong tu MONITOR_COLOR/MONITOR_FONT.
void MONITOR_LANGUAGE()
{
    MONITOR_STATUS();

    language_index = (int8_t)(((language_index % UI_LANG_COUNT) + UI_LANG_COUNT) % UI_LANG_COUNT);

    if (last_show_language != show_language)
    {
        last_show_language = show_language;
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        last_language_index = -1;
    }

    if (last_language_index < 0)
    {
        for (int8_t i = 0; i < UI_LANG_COUNT; i++)
            draw_language_row(i, i == language_index);
        const char *hint = lang_hint_apply();
        int16_t hintX = (320 - (int16_t)strlen(hint) * 6) / 2;
        myTFT.TFTdrawText(hintX, 175, (char *)hint, UI_TEXT_DIM, UI_BG, 1);
        last_language_index = language_index;
    }
    else if (last_language_index != language_index)
    {
        draw_language_row(last_language_index, false);
        draw_language_row(language_index, true);
        last_language_index = language_index;
    }
}
