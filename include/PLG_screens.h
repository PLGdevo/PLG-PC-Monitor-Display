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

// menu SETTING dang danh sach (PLAYER/FUNTION/MODE/CLOCK/COLOR/FONT/TASK)
void MONITOR_FUNTION();

// man hinh dong ho: gio (12h + AM/PM) + ngay nhan tu monitor.py, ve bang ho chu + co chu dang
// chon (active_clock_font/active_clock_size)
void MONITOR_CLOCK();

// man hinh chon mau nhan dien (UI_ACCENT)
void MONITOR_COLOR();

// buoc 1/2 chon chu so dong ho: chon HO CHU (active_clock_font) trong 5 kieu co san.
// Nhan nut de chuyen sang buoc 2 (MONITOR_FONT_SIZE) chon co chu cho ho vua chon.
void MONITOR_FONT();

// buoc 2/2 chon chu so dong ho: chon CO CHU (active_clock_size) cho ho chu da chon o buoc 1.
// Nhan nut de ap dung ca hai (luu vao flash) va quay lai menu SETTING.
void MONITOR_FONT_SIZE();

// xoa cache noi bo cua dong ho Task Manager goc tren-phai, buoc ve lai lan sau (vd khi roi HOME)
void reset_taskmanager_clock_cache();

// tra ve gia tri size thuc te ung voi 1 index (0-based) trong danh sach co chu hop le cua ho
// chu style; dung khi ap dung lua chon o buoc 2 (MONITOR_FONT_SIZE), xem PLG_input.cpp
int8_t get_clock_size_value(int8_t style, int8_t index);

// tra ve index (0-based) ung voi 1 gia tri size cho truoc (kep an toan trong khoang hop le cua
// ho do); dung de khoi tao lai vi tri duyet trong buoc 2 tu active_clock_size hien tai khi vua
// chon xong ho chu o buoc 1, xem PLG_input.cpp
int8_t clock_size_value_to_index(int8_t style, int8_t sizeValue);
