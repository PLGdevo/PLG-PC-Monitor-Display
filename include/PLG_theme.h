#pragma once
#include <stdint.h>
// Bang mau giao dien (RGB565) + danh sach mau nhan dien co the chon trong SETTING > COLOR.

inline uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

extern const uint16_t UI_BG;         // nen chinh, den hoi ngat xanh
extern const uint16_t UI_PANEL;      // nen khung/the
extern const uint16_t UI_PANEL_HI;   // khung dang chon
extern const uint16_t UI_BORDER;     // vien
extern const uint16_t UI_TEXT;       // chu chinh
extern const uint16_t UI_TEXT_DIM;   // chu phu
extern const uint16_t UI_TEXT_FAINT; // chu rat mo (vd cac muc khong duoc chon trong carousel SETTING)
extern uint16_t UI_ACCENT;           // mau nhan dien, chinh duoc trong SETTING > COLOR
extern const uint16_t UI_DANGER;     // canh bao / mat ket noi

extern const uint16_t UI_CPU;
extern const uint16_t UI_RAM;
extern const uint16_t UI_GPU;    // GPU 3D (usage core)
extern const uint16_t UI_GPUMEM; // GPU memory (VRAM)
extern const uint16_t UI_WIFI;

extern const uint16_t UI_ACCENT_PRESETS[];
extern const char *UI_ACCENT_NAMES[];
#define UI_ACCENT_PRESET_COUNT (9)
