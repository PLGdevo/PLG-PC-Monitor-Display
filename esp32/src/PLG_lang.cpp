#include "PLG_lang.h"
#include "PLG_state.h"

// thu vien font chi ho tro ASCII (khong dau) -> ban VI o day la tieng Viet khong dau, giu nguyen
// text goc da dung truoc khi co tinh nang doi ngon ngu; ban EN la tieng Anh tuong duong.
static const char *const MENU_LABELS_VI[FUNTION_MODE_COUNT] = {
    "PLAYER", "FUNTION", "MODE", "CLOCK", "COLOR", "FONT", "TASK", "LANGUAGE", "CLOCK STYLE", "CONNECTION"};
static const char *const MENU_LABELS_EN[FUNTION_MODE_COUNT] = {
    "PLAYER", "FUNCTION", "MODE", "CLOCK", "COLOR", "FONT", "TASK", "LANGUAGE", "CLOCK STYLE", "CONNECTION"};

static const char *const LANG_NAMES_VI[UI_LANG_COUNT] = {"TIENG VIET", "TIENG ANH"};
static const char *const LANG_NAMES_EN[UI_LANG_COUNT] = {"VIETNAMESE", "ENGLISH"};

static const char *const CLOCK_STYLE_NAMES_VI[CLOCK_STYLE_COUNT] = {"SO DIEN TU", "KIM CO DIEN", "KIM TOI GIAN", "KIM DAM"};
static const char *const CLOCK_STYLE_NAMES_EN[CLOCK_STYLE_COUNT] = {"DIGITAL", "ANALOG CLASSIC", "ANALOG MINIMAL", "ANALOG BOLD"};

const char *const *lang_menu_labels()
{
    return (ui_language == UI_LANG_EN) ? MENU_LABELS_EN : MENU_LABELS_VI;
}

const char *lang_hint_choose_size()
{
    return (ui_language == UI_LANG_EN) ? "Press button to choose size" : "Nhan nut de chon co chu";
}

const char *lang_hint_apply()
{
    return (ui_language == UI_LANG_EN) ? "Press button to apply" : "Nhan nut de ap dung";
}

const char *lang_name(int8_t index)
{
    if (index < 0 || index >= UI_LANG_COUNT)
        return "";
    return (ui_language == UI_LANG_EN) ? LANG_NAMES_EN[index] : LANG_NAMES_VI[index];
}

const char *lang_clock_style_name(int8_t index)
{
    if (index < 0 || index >= CLOCK_STYLE_COUNT)
        return "";
    return (ui_language == UI_LANG_EN) ? CLOCK_STYLE_NAMES_EN[index] : CLOCK_STYLE_NAMES_VI[index];
}

// nhan 1 ky tu (khong phai "CPU"/"RAM" day du): hien thi cung hang voi thanh pin, khe rat hep
// (phai ket thuc truoc x=255 do gioi han uint8_t cua TFTdrawText, xem draw_clock_cpu_ram)
const char *lang_label_cpu() { return "C"; }
const char *lang_label_ram() { return "R"; }

const char *lang_ble_title()
{
    return (ui_language == UI_LANG_EN) ? "BLUETOOTH DEVICE" : "THIET BI BLUETOOTH";
}

const char *lang_status_waiting()
{
    return (ui_language == UI_LANG_EN) ? "Waiting for connection..." : "Dang cho ket noi...";
}

const char *lang_status_connected()
{
    return (ui_language == UI_LANG_EN) ? "Connected" : "Da ket noi";
}

const char *lang_hint_back()
{
    return (ui_language == UI_LANG_EN) ? "Press button to go back" : "Nhan nut de quay lai";
}

const char *lang_wifi_scanning()
{
    return (ui_language == UI_LANG_EN) ? "Scanning networks..." : "Dang quet mang...";
}

const char *lang_wifi_no_network()
{
    return (ui_language == UI_LANG_EN) ? "No network found" : "Khong tim thay mang nao";
}

const char *lang_wifi_pick_network()
{
    return (ui_language == UI_LANG_EN) ? "CHOOSE WIFI NETWORK" : "CHON MANG WIFI";
}

const char *lang_wifi_password()
{
    return (ui_language == UI_LANG_EN) ? "ENTER PASSWORD" : "NHAP MAT KHAU";
}

const char *lang_wifi_connecting()
{
    return (ui_language == UI_LANG_EN) ? "Connecting..." : "Dang ket noi...";
}

const char *lang_wifi_failed()
{
    return (ui_language == UI_LANG_EN) ? "Connection failed" : "Ket noi that bai";
}

const char *lang_wifi_connected()
{
    return (ui_language == UI_LANG_EN) ? "WIFI CONNECTED" : "DA KET NOI WIFI";
}

const char *lang_wifi_hint_password()
{
    return (ui_language == UI_LANG_EN) ? "Hold button = delete" : "Giu nut = xoa lui";
}

const char *lang_wifi_done()
{
    return (ui_language == UI_LANG_EN) ? "[DONE]" : "[XONG]";
}

const char *lang_wifi_cancel()
{
    return (ui_language == UI_LANG_EN) ? "[CANCEL]" : "[HUY]";
}
