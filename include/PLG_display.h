#pragma once
#include "st7789/ST7789_TFT.hpp"
// Instance TFT dung chung cho toan bo firmware. Dinh nghia trong PLG_display.cpp.

extern ST7789_TFT myTFT;

// Cau hinh chan + tham so man hinh, khoi tao SPI/GPIO/kich thuoc man hinh cho myTFT.
void setup_pin();

// nhu myTFT.TFTdrawText, nhung tu dong chuyen sang HO CHU dang chon trong SETTING > FONT
// (active_clock_font, xem PLG_state.h) truoc khi ve, roi tra lai font mac dinh ngay sau do
// (TFTFontNum la trang thai dung chung toan thu vien). Tu dong roi ve font mac dinh (khong doi
// font) neu: kieu Seven_Seg (chi hop chu so) hoac chuoi co chu thuong ma kieu dang chon (Thick/
// Wide) khong co chu thuong -> tranh ve sai/trong ky tu khi ap dung font cho van ban chung
// (menu, nhan bieu do, gio nho...) chu khong chi rieng dong ho lon.
void ui_drawText(int16_t x, int16_t y, const char *text, uint16_t color, uint16_t bg, uint8_t size);
