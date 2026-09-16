#include "PLG_flash_settings.h"
#include <Preferences.h>
#include "PLG_theme.h"
#include "PLG_state.h"

// Ten namespace NVS toi da 15 ky tu.
static const char *NVS_NAMESPACE = "plg_ui";

// Gia tri tra ve khi khoa chua ton tai. Dung 0xFF (khong phai 0) de phan biet "chua luu bao gio"
// voi "da luu va gia tri dung bang 0" - vd color_index = 0 (TEAL) la lua chon hop le.
static const uint8_t NVS_UNSET = 0xFF;

static Preferences prefs;

void save_settings_to_flash(uint8_t colorIdx, uint8_t clockFontIdx, uint8_t clockSize, uint8_t languageIdx, uint8_t clockStyleIdx)
{
    if (!prefs.begin(NVS_NAMESPACE, false)) // false = mo o che do ghi
        return;
    prefs.putUChar("color", colorIdx);
    prefs.putUChar("font", clockFontIdx);
    prefs.putUChar("size", clockSize);
    prefs.putUChar("lang", languageIdx);
    prefs.putUChar("style", clockStyleIdx);
    prefs.end();
}

// Doc cau hinh da luu (neu co) va ap dung vao color_index/UI_ACCENT + active_clock_font/
// active_clock_size/ui_language/active_clock_style; goi 1 lan trong setup(). Kich thuoc (size)
// chi kiem tra so bo o day (>0); gioi han thuc te toi da theo tung ho chu duoc kep lai an toan
// moi lan ve o PLG_screens.cpp (get_clock_char_metrics), phong truong hop gia tri cu/hong khong
// con hop le voi ho chu. Moi truong hop khong hop le deu giu nguyen gia tri mac dinh trong
// PLG_state.cpp thay vi ghi de bang du lieu rac.
void load_settings_from_flash()
{
    if (!prefs.begin(NVS_NAMESPACE, true)) // true = chi doc
        return;

    uint8_t colorIdx = prefs.getUChar("color", NVS_UNSET);
    uint8_t fontIdx = prefs.getUChar("font", NVS_UNSET);
    uint8_t size = prefs.getUChar("size", NVS_UNSET);
    uint8_t langIdx = prefs.getUChar("lang", NVS_UNSET);
    uint8_t styleIdx = prefs.getUChar("style", NVS_UNSET);
    prefs.end();

    if (colorIdx < UI_ACCENT_PRESET_COUNT)
    {
        color_index = colorIdx;
        UI_ACCENT = UI_ACCENT_PRESETS[color_index];
    }
    if (fontIdx < CLOCK_FONT_COUNT)
        active_clock_font = fontIdx;
    if (size > 0 && size != NVS_UNSET)
        active_clock_size = size;
    if (langIdx < UI_LANG_COUNT)
        ui_language = langIdx;
    if (styleIdx < CLOCK_STYLE_COUNT)
        active_clock_style = styleIdx;
}
