#include "PLG_flash_settings.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "PLG_theme.h"
#include "PLG_state.h"

// dung sector flash cuoi cung, ngoai vung code, de luu 1 byte color_index (co magic de kiem tra hop le)
#define SETTINGS_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_MAGIC 0xA5

void save_color_to_flash(uint8_t idx)
{
    uint8_t buf[FLASH_PAGE_SIZE];
    memset(buf, 0xFF, sizeof(buf));
    buf[0] = SETTINGS_MAGIC;
    buf[1] = idx;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(SETTINGS_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_FLASH_OFFSET, buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

// doc mau da luu tu flash (neu co) va ap dung vao color_index/UI_ACCENT; goi 1 lan trong setup()
void load_color_from_flash()
{
    const uint8_t *flash_data = (const uint8_t *)(XIP_BASE + SETTINGS_FLASH_OFFSET);
    if (flash_data[0] == SETTINGS_MAGIC && flash_data[1] < UI_ACCENT_PRESET_COUNT)
    {
        color_index = flash_data[1];
        UI_ACCENT = UI_ACCENT_PRESETS[color_index];
    }
}
