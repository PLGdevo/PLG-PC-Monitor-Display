#pragma once
#include <stdint.h>
// Luu/doc cac tuy chinh giao dien da chon (mau nhan dien + ho chu/size dong ho + ngon ngu) vao
// flash, giu lai sau khi mat nguon.

void save_settings_to_flash(uint8_t colorIdx, uint8_t clockFontIdx, uint8_t clockSize, uint8_t languageIdx);
void load_settings_from_flash();
