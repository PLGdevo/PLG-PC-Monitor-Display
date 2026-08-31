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

void MONITOR_STATUS()
{
    if (last_status != status)
    {
        myTFT.TFTdrawLine(0, 23, 320, 23, UI_BORDER);
        last_status = status;
    }

    draw_battery(20, 60, battery1, last_battery1);   // pin remote
    draw_battery(100, 140, battery2, last_battery2); // pin may

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
    ChartLayout c[5];
    get_chart_layout(c);

    if (last_desktop != desktop)
    {
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        for (int i = 0; i < 5; i++)
            draw_chart_frame(c[i].x, c[i].y, c[i].w, c[i].h);
        last_desktop = desktop;
        taskmanager_dirty = true;
        reset_taskmanager_clock_cache(); // buoc ve lai gio sau khi xoa man hinh
    }

    if (taskmanager_dirty)
    {
        draw_chart_data(c[0].x, c[0].y, c[0].w, c[0].h, "CPU", chart_cpu, UI_CPU);
        draw_chart_data(c[1].x, c[1].y, c[1].w, c[1].h, "RAM", chart_ram, UI_RAM, 90, "BOOST NOW");
        draw_chart_data(c[2].x, c[2].y, c[2].w, c[2].h, "GPU", chart_gpu, UI_GPU);
        // "MEM" (3 ky tu, khong phai "VRAM" 4 ky tu): o khung ben phai (x~164) TFTdrawText dung
        // con tro X kieu uint8_t (toi da 255) - nhan dai hon 1 ky tu la du lam tran/wrap ve dau man hinh
        draw_chart_data(c[3].x, c[3].y, c[3].w, c[3].h, "MEM", chart_gpumem, UI_GPUMEM);
        draw_chart_data(c[4].x, c[4].y, c[4].w, c[4].h, "WIFI", chart_wifi, UI_WIFI);

        taskmanager_dirty = false;
    }

    MONITOR_STATUS();
    draw_taskmanager_clock();
}

/*------------------- Menu SETTING -------------------*/
// vi tri + kich thuoc 1 dong trong menu SETTING, tinh 1 lan dung chung cho ve khung
// va cho cap nhat khi doi lua chon (rowH/rowGap nho lai de FUNTION_MODE_COUNT dong
// deu vua man hinh, khong bi tran ra ngoai / bi den do ve qua vung hien thi)
static const int16_t MENU_ROW_X = 55, MENU_ROW_W = 220, MENU_ROW_H = 28, MENU_ROW_GAP = 4, MENU_START_Y = 26;

static void draw_menu_row(int8_t i, bool selected)
{
    const char *labels[FUNTION_MODE_COUNT] = {"PLAYER", "FUNTION", "MODE", "CLOCK", "COLOR", "TASK"};
    int16_t rowY = MENU_START_Y + i * (MENU_ROW_H + MENU_ROW_GAP);
    uint16_t bg = selected ? UI_PANEL_HI : UI_PANEL;
    uint16_t border = selected ? UI_ACCENT : UI_BORDER;
    uint16_t fg = selected ? UI_ACCENT : UI_TEXT_DIM;
    myTFT.TFTfillRoundRect(MENU_ROW_X, rowY, MENU_ROW_W, MENU_ROW_H, 4, bg);
    myTFT.TFTdrawRoundRect(MENU_ROW_X, rowY, MENU_ROW_W, MENU_ROW_H, 4, border);
    myTFT.TFTdrawText(MENU_ROW_X + 15, rowY + 7, (char *)labels[i], fg, bg, 2);
}

void MONITOR_FUNTION()
{
    MONITOR_STATUS();

    // cuon vong: cuoi danh sach quay len dau va nguoc lai (thay vi dung khung o 2 dau khi quay nhanh).
    // dung % thay vi if de xu ly dung ca khi quay rat nhanh khien funtion_mode nhay qua nhieu buoc cung luc.
    funtion_mode = ((funtion_mode % FUNTION_MODE_COUNT) + FUNTION_MODE_COUNT) % FUNTION_MODE_COUNT;

    if (menu_needs_full_draw)
    {
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        for (int i = 0; i < FUNTION_MODE_COUNT; i++)
        {
            draw_menu_row(i, i == funtion_mode);
        }
        last_funtion_mode = funtion_mode;
        menu_needs_full_draw = false;
    }
    else if (last_funtion_mode != funtion_mode)
    {
        // chi ve lai 2 dong bi anh huong (dong cu bo chon + dong moi duoc chon)
        // thay vi xoa toan bo man hinh -> chuyen doi giua cac muc muot, khong nhap nhay
        draw_menu_row(last_funtion_mode, false);
        draw_menu_row(funtion_mode, true);
        last_funtion_mode = funtion_mode;
    }
}

/*------------------- Man hinh dong ho -------------------*/
// man hinh dong ho: hien gio + ngay hien tai (nhan tu monitor.py qua serial, truong TIME/DATE)
// giao dien toi gian: chi 2 dong chu can giua, khong khung khong tieu de
void MONITOR_CLOCK()
{
    MONITOR_STATUS();

    if (last_show_clock != show_clock)
    {
        last_show_clock = show_clock;
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        strcpy(last_time_str, "");
        strcpy(last_date_str, "");
        clock_dirty = true;
    }

    if (clock_dirty)
    {
        if (strcmp(last_time_str, current_time_str) != 0)
        {
            myTFT.TFTfillRect(0, 100, 320, 45, UI_BG);
            // can giua: size4, moi ky tu chiem size*(5+1)=24px. "HH:MM:SS" (8 ky tu) rong 192px
            // -> x=(320-192)/2=64. Dung size4 (thay vi 5) de vi tri ky tu cuoi cung (64+7*24=232)
            // nam an toan duoi nguong 255 cua con tro X kieu uint8_t trong TFTdrawText, tranh tran/wrap.
            myTFT.TFTdrawText(64, 108, current_time_str, UI_ACCENT, UI_BG, 4);
            strcpy(last_time_str, current_time_str);
        }
        if (strcmp(last_date_str, current_date_str) != 0)
        {
            myTFT.TFTfillRect(0, 155, 320, 25, UI_BG);
            // x=70 de can giua "DD/MM/YYYY" (10 ky tu, size 3 -> rong ~177px tren man 320px).
            // Khong duoc dat x qua ~90: TFTdrawText dung con tro X kieu uint8_t, neu x + do rong
            // vuot qua 255 no se tran so va ky tu cuoi nhay ve sat le trai man hinh.
            myTFT.TFTdrawText(70, 158, current_date_str, UI_TEXT_DIM, UI_BG, 3);
            strcpy(last_date_str, current_date_str);
        }
        clock_dirty = false;
    }
}

/*------------------- Man hinh chon mau -------------------*/
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

    if (last_color_index != color_index)
    {
        last_color_index = color_index;

        myTFT.TFTfillRect(0, 40, 320, 130, UI_BG);

        const int16_t swW = 26, swGap = 6, startX = 12, swY = 55, swH = 26;
        for (uint8_t i = 0; i < UI_ACCENT_PRESET_COUNT; i++)
        {
            int16_t sx = startX + i * (swW + swGap);
            bool selected = (i == color_index);
            myTFT.TFTfillRoundRect(sx, swY, swW, swH, 4, UI_ACCENT_PRESETS[i]);
            myTFT.TFTdrawRoundRect(sx, swY, swW, swH, 4, selected ? UI_TEXT : UI_BORDER);
            if (selected)
                myTFT.TFTdrawRoundRect(sx - 2, swY - 2, swW + 4, swH + 4, 5, UI_TEXT);
        }

        myTFT.TFTdrawText(100, 110, (char *)UI_ACCENT_NAMES[color_index], UI_ACCENT_PRESETS[color_index], UI_BG, 3);
        myTFT.TFTdrawText(40, 145, (char *)"Nhan nut de ap dung", UI_TEXT_DIM, UI_BG, 1);
    }
}
