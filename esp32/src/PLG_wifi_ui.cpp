#include "PLG_wifi_ui.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include "PLG_state.h"
#include "PLG_theme.h"
#include "PLG_display.h"
#include "PLG_lang.h"
#include "PLG_screens.h"
#include "PLG_transport_wifi.h"

/*==================== Trang thai wizard ====================*/

enum WifiUiStep : uint8_t
{
    STEP_SCANNING,   // dang quet mang (khong chan), cho ket qua
    STEP_PICK_SSID,  // danh sach mang quet duoc, xoay de cuon
    STEP_PASSWORD,   // wheel-picker nhap tung ky tu
    STEP_CONNECTING, // da goi WiFi.begin(), dang cho vao mang
    STEP_FAILED,     // sai mat khau / het thoi gian cho
    STEP_DONE        // da vao mang, hien IP de nhap vao monitor.py
};

static WifiUiStep wizard_step = STEP_SCANNING;
static WifiUiStep last_drawn_step = STEP_DONE; // khac wizard_step ban dau -> lan render dau luon ve lai
static bool needs_full_draw = true;

static int ssid_count = 0;
static int ssid_index = 0;
static int last_drawn_ssid_index = -1;
static char chosen_ssid[33] = "";

// WPA2 cho phep mat khau toi da 63 ky tu.
static char password[64] = "";
static uint8_t password_len = 0;
static int char_index = 0;
static int last_drawn_char_index = -1;
static uint8_t last_drawn_password_len = 255; // 255 = chua ve lan nao

static uint32_t connect_started_ms = 0;
// 15s: du cho DHCP cham; qua han thi coi nhu sai mat khau/mang khong toi duoc, quay lai wizard
// thay vi treo man hinh mai.
static const uint32_t CONNECT_TIMEOUT_MS = 15000;

// Lan ket noi nay dung cau hinh da luu (true) hay do nguoi dung vua go tay (false)?
// Quyet dinh 2 thu: that bai thi tu quay lai buoc quet (thay vi dung man hinh loi), va o man
// hinh IP thi nut giu mang nghia "quen mang" thay vi "luu IP tinh".
static bool using_saved_config = false;

// Da bam luu trong phien nay chua - de doi dong huong dan sang "giu nut = quen mang" ngay,
// khong bat nguoi dung thoat ra vao lai moi thay.
static bool just_saved = false;

/*==================== Bang ky tu cua wheel-picker ====================*/
// Thu tu dat theo tan suat go mat khau WiFi: chu thuong -> chu hoa -> so -> ky tu dac biet.
// Dau cach de o cuoi nhom ky tu dac biet vi rat it mat khau dung toi.
static const char WIFI_CHARSET[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    "!@#$%^&*()-_=+[]{};:,.<>/?~ ";

static const int CHARSET_LEN = (int)(sizeof(WIFI_CHARSET) - 1); // tru '\0'
// 2 muc ao noi tiep ngay sau bang ky tu that: xoay qua het chu so/ky tu dac biet la toi.
static const int IDX_DONE = CHARSET_LEN;
static const int IDX_CANCEL = CHARSET_LEN + 1;
static const int WHEEL_LEN = CHARSET_LEN + 2;

// Nhan hien thi cua 1 muc trong banh xe: ky tu thuong thi la chinh no, 2 muc cuoi la [XONG]/[HUY].
static const char *wheel_label(int index)
{
    static char one_char[2] = {0, 0};
    if (index == IDX_DONE)
        return lang_wifi_done();
    if (index == IDX_CANCEL)
        return lang_wifi_cancel();
    one_char[0] = WIFI_CHARSET[index];
    return one_char;
}

/*==================== Ve man hinh ====================*/

static void draw_line(int16_t y, const char *text, uint16_t color, uint8_t size, int16_t clearH)
{
    int16_t textW = (int16_t)strlen(text) * size * (5 + 1);
    int16_t x = (320 - textW) / 2;
    if (x < 0)
        x = 0;
    myTFT.TFTfillRect(0, y - 3, 320, clearH, UI_BG);
    myTFT.TFTdrawText(x, y, (char *)text, color, UI_BG, size);
}

static void clear_content()
{
    myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
}

// Danh sach mang: 5 dong quanh muc dang chon, muc giua noi bat - cung ngon ngu hinh anh voi
// carousel cua menu SETTING de nguoi dung khong phai hoc cach dieu khien moi.
static const int16_t LIST_ROWS = 5;
static const int16_t LIST_ROW_H = 26;
static const int16_t LIST_TOP = 66;

static void draw_ssid_list()
{
    char line[40];
    for (int row = 0; row < LIST_ROWS; row++)
    {
        int idx = ssid_index - 2 + row;
        int16_t y = LIST_TOP + row * LIST_ROW_H;
        bool center = (row == 2);

        myTFT.TFTfillRect(0, y - 2, 320, LIST_ROW_H - 2, UI_BG);
        if (idx < 0 || idx >= ssid_count)
            continue; // ngoai danh sach: de trong, khong cuon vong (de biet dang o dau/cuoi)

        // RSSI (dBm) kem theo de chon duoc mang khoe nhat khi trung ten (vd nha co 2 router).
        snprintf(line, sizeof(line), "%s%-20.20s %ddBm", center ? "> " : "  ",
                 wifi_scan_ssid(idx), wifi_scan_rssi(idx));
        myTFT.TFTdrawText(10, y, line, center ? UI_ACCENT : UI_TEXT_FAINT, UI_BG, center ? 2 : 1);
    }
}

// Banh xe ky tu: 7 o ngang, o giua la ky tu dang chon.
static const int WHEEL_SLOTS = 7;
static const int16_t WHEEL_Y = 130;

static void draw_password_wheel()
{
    const int16_t slotW = 320 / WHEEL_SLOTS;
    myTFT.TFTfillRect(0, WHEEL_Y - 6, 320, 40, UI_BG);

    for (int slot = 0; slot < WHEEL_SLOTS; slot++)
    {
        // Cuon vong: het bang ky tu thi quay lai dau, khong bi "ket" o 2 dau banh xe.
        int idx = ((char_index - 3 + slot) % WHEEL_LEN + WHEEL_LEN) % WHEEL_LEN;
        bool center = (slot == 3);
        const char *label = wheel_label(idx);

        // [XONG]/[HUY] dai hon 1 ky tu nen phai thu nho de khong tran sang o ben canh.
        uint8_t size = center ? 3 : 2;
        int16_t textW = (int16_t)strlen(label) * size * (5 + 1);
        while (textW > slotW - 2 && size > 1)
        {
            size--;
            textW = (int16_t)strlen(label) * size * (5 + 1);
        }

        int16_t x = slot * slotW + (slotW - textW) / 2;
        if (x < 0)
            x = 0;
        myTFT.TFTdrawText(x, WHEEL_Y + (center ? 0 : 6), (char *)label,
                          center ? UI_ACCENT : UI_TEXT_FAINT, UI_BG, size);
    }

    // Khung sang danh dau o dang chon
    myTFT.TFTdrawRoundRect(3 * slotW, WHEEL_Y - 6, slotW, 36, 4, UI_ACCENT);
}

static void draw_password_text()
{
    // Hien mat khau dang ro (khong che bang '*'): day la thiet bi cam tay, khong co man hinh
    // nguoi khac nhin trom nhu ATM, ma go bang encoder rat de sai - thay duoc minh vua go gi
    // quan trong hon nhieu so voi che di.
    char shown[28];
    const char *src = password;
    // Chuoi dai hon o hien thi: cuon theo duoi, luon thay duoc phan vua go.
    if (password_len > sizeof(shown) - 2)
        src = password + (password_len - (sizeof(shown) - 2));
    snprintf(shown, sizeof(shown), "%s_", src); // '_' = con tro dang go

    myTFT.TFTfillRect(0, 84, 320, 24, UI_BG);
    int16_t textW = (int16_t)strlen(shown) * 2 * (5 + 1);
    int16_t x = (320 - textW) / 2;
    if (x < 0)
        x = 0;
    myTFT.TFTdrawText(x, 86, shown, UI_TEXT, UI_BG, 2);
}

/*==================== Chuyen buoc ====================*/

static void go_to(WifiUiStep next)
{
    wizard_step = next;
    needs_full_draw = true;
}

// Bat dau lai tu buoc QUET (bo qua cau hinh da luu). Dung khi nguoi dung chu dong quen mang,
// hoac khi ket noi bang cau hinh da luu that bai.
static void start_scan_wizard()
{
    password[0] = '\0';
    password_len = 0;
    char_index = 0;
    ssid_index = 0;
    ssid_count = 0;
    chosen_ssid[0] = '\0';
    last_drawn_ssid_index = -1;
    last_drawn_char_index = -1;
    last_drawn_password_len = 255;
    using_saved_config = false;
    just_saved = false;

    wifi_scan_start();
    go_to(STEP_SCANNING);
}

void wifi_ui_enter()
{
    // Da co cau hinh luu tu lan truoc -> ket noi thang bang dia chi tinh, bo qua toan bo
    // wizard. Day chinh la muc dich cua viec luu: khong phai go lai mat khau moi lan boot.
    if (wifi_config_exists() && wifi_connect_saved())
    {
        strncpy(chosen_ssid, wifi_config_ssid(), sizeof(chosen_ssid) - 1);
        chosen_ssid[sizeof(chosen_ssid) - 1] = '\0';
        using_saved_config = true;
        just_saved = false;
        connect_started_ms = millis();
        go_to(STEP_CONNECTING);
        return;
    }

    start_scan_wizard();
}

void wifi_ui_exit()
{
    wifi_scan_clear();
    show_wifi_ui = false;
    menu_needs_full_draw = true; // buoc menu SETTING ve lai tu dau khi quay ra
}

/*==================== Ve + tu tien trien theo buoc ====================*/

void wifi_ui_render()
{
    MONITOR_STATUS();

    bool full = needs_full_draw || (last_drawn_step != wizard_step);
    if (full)
    {
        clear_content();
        last_drawn_step = wizard_step;
        needs_full_draw = false;
        last_drawn_ssid_index = -1;
        last_drawn_char_index = -1;
        last_drawn_password_len = 255;
    }

    switch (wizard_step)
    {
    case STEP_SCANNING:
    {
        if (full)
            draw_line(120, lang_wifi_scanning(), UI_TEXT_DIM, 2, 24);

        int status = wifi_scan_status();
        if (status == WIFI_SCAN_RUNNING)
            break;
        if (status == WIFI_SCAN_FAILED || status == 0)
        {
            ssid_count = 0;
            go_to(STEP_PICK_SSID); // man hinh danh sach se bao "khong tim thay mang nao"
            break;
        }
        ssid_count = status;
        ssid_index = 0;
        go_to(STEP_PICK_SSID);
        break;
    }

    case STEP_PICK_SSID:
        if (full)
        {
            draw_line(36, lang_wifi_pick_network(), UI_TEXT_DIM, 1, 16);
            if (ssid_count == 0)
                draw_line(120, lang_wifi_no_network(), UI_DANGER, 2, 24);
            else
                draw_line(210, lang_hint_apply(), UI_TEXT_DIM, 1, 16);
        }
        if (ssid_count > 0 && ssid_index != last_drawn_ssid_index)
        {
            draw_ssid_list();
            last_drawn_ssid_index = ssid_index;
        }
        break;

    case STEP_PASSWORD:
        if (full)
        {
            draw_line(36, lang_wifi_password(), UI_TEXT_DIM, 1, 16);
            draw_line(56, chosen_ssid, UI_ACCENT, 1, 16);
            draw_line(200, lang_wifi_hint_password(), UI_TEXT_DIM, 1, 16);
        }
        if (password_len != last_drawn_password_len)
        {
            draw_password_text();
            last_drawn_password_len = password_len;
        }
        if (char_index != last_drawn_char_index)
        {
            draw_password_wheel();
            last_drawn_char_index = char_index;
        }
        break;

    case STEP_CONNECTING:
    {
        if (full)
        {
            draw_line(100, lang_wifi_connecting(), UI_TEXT_DIM, 2, 24);
            draw_line(140, chosen_ssid, UI_ACCENT, 2, 24);
        }

        if (wifi_is_online())
        {
            go_to(STEP_DONE);
            break;
        }
        if (millis() - connect_started_ms > CONNECT_TIMEOUT_MS)
        {
            if (using_saved_config)
            {
                // Mang da luu gio khong con (doi mat khau, di cho khac...): tu chay lai wizard
                // thay vi dung o man hinh loi bat nguoi dung tu mo lai.
                Serial.println("PLG_>>>> WIFI: mang da luu khong ket noi duoc, quet lai");
                start_scan_wizard();
            }
            else
            {
                go_to(STEP_FAILED);
            }
        }
        break;
    }

    case STEP_FAILED:
        if (full)
        {
            draw_line(100, lang_wifi_failed(), UI_DANGER, 2, 24);
            draw_line(150, lang_hint_apply(), UI_TEXT_DIM, 1, 16);
        }
        break;

    case STEP_DONE:
        if (full)
        {
            draw_line(60, lang_wifi_connected(), UI_TEXT_DIM, 1, 16);
            draw_line(90, chosen_ssid, UI_TEXT, 2, 24);
            // IP la thu nguoi dung phai go sang PC ("monitor.py --wifi <ip>") nen ve to nhat man hinh.
            draw_line(130, wifi_local_ip_str(), UI_ACCENT, 3, 30);
            char port_line[32];
            snprintf(port_line, sizeof(port_line), "TCP %d", WIFI_TCP_PORT);
            draw_line(175, port_line, UI_TEXT_DIM, 1, 16);
            // Da luu roi thi nut giu dung de QUEN mang; chua luu thi de LUU thanh IP tinh.
            bool saved = using_saved_config || just_saved;
            draw_line(196, saved ? lang_wifi_hint_forget() : lang_wifi_hint_save(), UI_TEXT_DIM, 1, 16);
            draw_line(214, lang_hint_back(), UI_TEXT_DIM, 1, 16);
        }
        break;
    }
}

/*==================== Thao tac nguoi dung ====================*/

void wifi_ui_on_rotate(int32_t delta)
{
    if (delta == 0)
        return;

    switch (wizard_step)
    {
    case STEP_PICK_SSID:
        if (ssid_count <= 0)
            break;
        // Kep trong danh sach (khong cuon vong): de nguoi dung biet minh dang o dau/cuoi.
        ssid_index += (int)delta;
        if (ssid_index < 0)
            ssid_index = 0;
        if (ssid_index >= ssid_count)
            ssid_index = ssid_count - 1;
        break;

    case STEP_PASSWORD:
        char_index = (int)(((char_index + delta) % WHEEL_LEN + WHEEL_LEN) % WHEEL_LEN);
        break;

    default:
        break; // cac buoc con lai chi hien thong tin, khong co gi de xoay
    }
}

void wifi_ui_on_short_press()
{
    switch (wizard_step)
    {
    case STEP_SCANNING:
        break; // dang quet, chua co gi de chon

    case STEP_PICK_SSID:
        if (ssid_count <= 0)
        {
            wifi_ui_exit(); // khong co mang nao -> thoat wizard, khong bat nguoi dung ket o day
            break;
        }
        strncpy(chosen_ssid, wifi_scan_ssid(ssid_index), sizeof(chosen_ssid) - 1);
        chosen_ssid[sizeof(chosen_ssid) - 1] = '\0';
        Serial.printf("PLG_>>>> WIFI: chon mang \"%s\"\n", chosen_ssid);
        go_to(STEP_PASSWORD);
        break;

    case STEP_PASSWORD:
        if (char_index == IDX_CANCEL)
        {
            Serial.println("PLG_>>>> WIFI: huy wizard");
            wifi_ui_exit();
        }
        else if (char_index == IDX_DONE)
        {
            Serial.printf("PLG_>>>> WIFI: ket noi \"%s\" (mat khau %d ky tu)\n", chosen_ssid, password_len);
            wifi_connect(chosen_ssid, password);
            connect_started_ms = millis();
            go_to(STEP_CONNECTING);
        }
        else if (password_len < sizeof(password) - 1)
        {
            password[password_len++] = WIFI_CHARSET[char_index];
            password[password_len] = '\0';
            // KHONG dat lai char_index ve 0: mat khau hay co cum ky tu gan nhau trong bang
            // (vd chu thuong lien tiep), giu nguyen vi tri giup go nhanh hon nhieu.
        }
        break;

    case STEP_CONNECTING:
        break; // dang ket noi, de no chay het thoi gian cho

    case STEP_FAILED:
        // Thu lai tu dau: quet lai mang. Dung start_scan_wizard chu khong phai wifi_ui_enter -
        // neu goi wifi_ui_enter thi no lai lay cau hinh da luu ra thu tiep, lap vo tan.
        start_scan_wizard();
        break;

    case STEP_DONE:
        wifi_ui_exit();
        break;
    }
}

void wifi_ui_on_long_press()
{
    // Nut giu mang y nghia khac nhau theo buoc; o cac buoc khong liet ke thi nuot luon thao tac
    // nay de khong vo tinh chuyen tab HOME/SETTING giua chung wizard.
    if (wizard_step == STEP_PASSWORD)
    {
        if (password_len > 0)
            password[--password_len] = '\0';
        return;
    }

    if (wizard_step == STEP_DONE)
    {
        if (using_saved_config || just_saved)
        {
            // Quen mang: xoa cau hinh roi chay lai wizard tu buoc quet, de doi sang mang khac.
            wifi_config_clear();
            start_scan_wizard();
        }
        else if (wifi_config_save(chosen_ssid, password))
        {
            just_saved = true;
            needs_full_draw = true; // ve lai man hinh: dong huong dan doi sang "giu nut = quen mang"
            // Bao da luu ngay tren man hinh - ghi NVS khong co dau hieu nhin thay nao khac.
            draw_line(140, lang_wifi_saved(), UI_CPU, 2, 24);
            delay(700); // du de doc dong bao truoc khi man hinh ve lai
        }
    }
}
