#pragma once
#include <stdint.h>
// Cac man hinh cua giao dien: splash khoi dong, status bar, Task Manager, menu SETTING, CLOCK, COLOR.

// ve 1 bitmap RGB565 day du (khong nen), dung cho logo splash
void drawLogoFull(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *img);

// splash khoi dong: logo + thanh loading, cho den khi co du lieu Task Manager that hoac nguoi dung bo qua
void MONITOR_BEGIN();

// thanh trang thai tren cung: pin remote/may + icon ket noi
void MONITOR_STATUS();

// man hinh chinh: 5 bieu do CPU/RAM/GPU/GPUMEM/WIFI + dong ho goc tren-phai
void MONITOR_TASKMANAGER();

// menu SETTING dang danh sach (PLAYER/FUNTION/MODE/CLOCK/COLOR/TASK)
void MONITOR_FUNTION();

// man hinh dong ho: gio + ngay nhan tu monitor.py
void MONITOR_CLOCK();

// man hinh chon mau nhan dien (UI_ACCENT)
void MONITOR_COLOR();

// xoa cache noi bo cua dong ho Task Manager goc tren-phai, buoc ve lai lan sau (vd khi roi HOME)
void reset_taskmanager_clock_cache();
