#pragma once
#include "pico/stdlib.h"
// Xu ly nguoi dung: encoder xoay, nut nhan (ngan/giu lau), chuyen man hinh.

void encoder_isr(uint gpio, uint32_t events);

// nhan giu nut > 2s => chuyen tab (HOME <-> SETTING); nha nut som hon => nhan ngan (chon/xac nhan)
void read_button();

// cap nhat desktop_state theo display_number, xoa man hinh khi chuyen tab
void DISPLAY_ROLL();
