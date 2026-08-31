#pragma once
#include "st7789/ST7789_TFT.hpp"
// Instance TFT dung chung cho toan bo firmware. Dinh nghia trong PLG_display.cpp.

extern ST7789_TFT myTFT;

// Cau hinh chan + tham so man hinh, khoi tao SPI/GPIO/kich thuoc man hinh cho myTFT.
void setup_pin();
