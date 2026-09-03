#include "PLG_theme.h"

const uint16_t UI_BG = RGB565(8, 10, 14);
const uint16_t UI_PANEL = RGB565(22, 26, 32);
const uint16_t UI_PANEL_HI = RGB565(34, 40, 48);
const uint16_t UI_BORDER = RGB565(52, 60, 68);
const uint16_t UI_TEXT = RGB565(232, 236, 232);
const uint16_t UI_TEXT_DIM = RGB565(128, 138, 132);
const uint16_t UI_TEXT_FAINT = RGB565(58, 64, 62);
uint16_t UI_ACCENT = RGB565(0, 194, 168); // teal, mac dinh
const uint16_t UI_DANGER = RGB565(235, 90, 90);

const uint16_t UI_CPU = RGB565(70, 220, 130);
const uint16_t UI_RAM = RGB565(240, 185, 60);
const uint16_t UI_GPU = RGB565(70, 200, 235);
const uint16_t UI_GPUMEM = RGB565(200, 120, 235);
const uint16_t UI_WIFI = RGB565(110, 150, 255);

const uint16_t UI_ACCENT_PRESETS[] = {
    RGB565(0, 194, 168),   // Teal (mac dinh)
    RGB565(235, 90, 90),   // Red
    RGB565(240, 160, 40),  // Orange
    RGB565(230, 210, 60),  // Yellow
    RGB565(90, 210, 90),   // Green
    RGB565(70, 200, 235),  // Cyan
    RGB565(90, 140, 240),  // Blue
    RGB565(170, 110, 235), // Purple
    RGB565(232, 236, 232), // White
};
static_assert(sizeof(UI_ACCENT_PRESETS) / sizeof(UI_ACCENT_PRESETS[0]) == UI_ACCENT_PRESET_COUNT,
              "UI_ACCENT_PRESET_COUNT phai khop so phan tu UI_ACCENT_PRESETS");

const char *UI_ACCENT_NAMES[] = {
    "TEAL", "RED", "ORANGE", "YELLOW", "GREEN", "CYAN", "BLUE", "PURPLE", "WHITE"};
