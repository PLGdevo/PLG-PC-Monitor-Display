#pragma once
#include <stdint.h>
// Doc du lieu Task Manager (CPU/RAM/GPU/WIFI/gio/pin) tu pc_monitor/monitor.py qua USB Serial.

// Day 1 gia tri moi vao cuoi bieu do (dich mang sang trai), kep trong [0,100].
void chart_push(int8_t *buf, int value);

// Doc tung ky tu tu USB stdio (khong block), gom thanh dong, parse khi gap '\n'.
// Cung xu ly bat tay nhan dien thiet bi "PLG_ID?" -> "I AM PLG_TFT_LCD_TASKMANAGER".
void read_taskmanager_serial();
