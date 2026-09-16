#pragma once
#include <stdint.h>
// Luu/doc cac tuy chinh giao dien da chon (mau nhan dien + ho chu/size dong ho + ngon ngu +
// kieu dong ho), giu lai sau khi mat nguon.
//
// Khac ban Pico: khong con ghi tay vao sector flash cuoi cung nua ma dung NVS (Preferences)
// cua ESP32 - co wear-levelling va khong the vo tinh de len vung code.

void save_settings_to_flash(uint8_t colorIdx, uint8_t clockFontIdx, uint8_t clockSize, uint8_t languageIdx, uint8_t clockStyleIdx);
void load_settings_from_flash();
