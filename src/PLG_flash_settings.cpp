#include "PLG_flash_settings.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "PLG_theme.h"
#include "PLG_state.h"

// dung sector flash cuoi cung, ngoai vung code, de luu vai byte cau hinh (co magic de kiem tra hop le)
#define SETTINGS_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_MAGIC 0xA5

void save_settings_to_flash(uint8_t colorIdx, uint8_t clockFontIdx, uint8_t clockSize, uint8_t languageIdx)
{
    uint8_t buf[FLASH_PAGE_SIZE];
    memset(buf, 0xFF, sizeof(buf));
    buf[0] = SETTINGS_MAGIC;
    buf[1] = colorIdx;
    buf[2] = clockFontIdx;
    buf[3] = clockSize;
    buf[4] = languageIdx;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(SETTINGS_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_FLASH_OFFSET, buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

// doc cau hinh da luu tu flash (neu co) va ap dung vao color_index/UI_ACCENT + active_clock_font/
// active_clock_size; goi 1 lan trong setup(). Kich thuoc (size) chi kiem tra so bo o day (>0);
// gioi han thuc te toi da theo tung ho chu duoc kep lai an toan moi lan ve o PLG_screens.cpp
// (get_clock_char_metrics), phong truong hop gia tri flash cu/hong khong con hop le voi ho chu.
void load_settings_from_flash()
{
    const uint8_t *flash_data = (const uint8_t *)(XIP_BASE + SETTINGS_FLASH_OFFSET);
    if (flash_data[0] != SETTINGS_MAGIC)
        return;

    if (flash_data[1] < UI_ACCENT_PRESET_COUNT)
    {
        color_index = flash_data[1];
        UI_ACCENT = UI_ACCENT_PRESETS[color_index];
    }
    if (flash_data[2] < CLOCK_FONT_COUNT)
    {
        active_clock_font = flash_data[2];
    }
    if (flash_data[3] > 0 && flash_data[3] != 0xFF)
    {
        active_clock_size = flash_data[3];
    }
    if (flash_data[4] < UI_LANG_COUNT)
    {
        ui_language = flash_data[4];
    }
}
