#include "PLG_lang.h"
#include "PLG_state.h"

// thu vien font chi ho tro ASCII (khong dau) -> ban VI o day la tieng Viet khong dau, giu nguyen
// text goc da dung truoc khi co tinh nang doi ngon ngu; ban EN la tieng Anh tuong duong.
static const char *const MENU_LABELS_VI[FUNTION_MODE_COUNT] = {
    "PLAYER", "FUNTION", "MODE", "CLOCK", "COLOR", "FONT", "TASK", "LANGUAGE"};
static const char *const MENU_LABELS_EN[FUNTION_MODE_COUNT] = {
    "PLAYER", "FUNCTION", "MODE", "CLOCK", "COLOR", "FONT", "TASK", "LANGUAGE"};

static const char *const LANG_NAMES_VI[UI_LANG_COUNT] = {"TIENG VIET", "TIENG ANH"};
static const char *const LANG_NAMES_EN[UI_LANG_COUNT] = {"VIETNAMESE", "ENGLISH"};

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

// nhan 1 ky tu (khong phai "CPU"/"RAM" day du): hien thi cung hang voi thanh pin, khe rat hep
// (phai ket thuc truoc x=255 do gioi han uint8_t cua TFTdrawText, xem draw_clock_cpu_ram)
const char *lang_label_cpu() { return "C"; }
const char *lang_label_ram() { return "R"; }
