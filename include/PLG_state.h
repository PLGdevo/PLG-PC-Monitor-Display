#pragma once
#include <stdint.h>
#include "pico/stdlib.h"
// Trang thai toan cuc dung chung giua cac module (man hinh, input, serial...).
// Cac gia tri duoc dinh nghia trong PLG_state.cpp.

extern int32_t timer1, timer2, timer3;
extern bool flat1;

extern int8_t battery1; // pin remote (nhan tu monitor.py qua serial, truong BAT)
extern int8_t battery2; // pin may
extern int8_t last_battery1, last_battery2;

extern int8_t funtion_mode;
extern int8_t last_funtion_mode;

extern int8_t player;
extern int8_t last_player;

extern bool desktop;
extern bool last_desktop;

extern bool status;
extern bool last_status;

extern bool now_button;
extern bool last_button;
extern bool button_long_fired; // true khi lan nhan hien tai da xu ly bang "giu lau" (chuyen tab)

extern bool now_button_1;
extern bool last_button_1;

extern bool CONNECT_STATUS;
extern bool LAST_CONNECT_STATUS;

extern bool last_clk;
extern int EN_CLK, EN_DT, EN_BT;

extern volatile int encoder_value;
extern volatile absolute_time_t last_time;

extern volatile int value;
extern volatile uint8_t last_state;

extern int8_t last_en;
extern int8_t now_en;
extern int8_t step;

enum MONITOR_DESTOP
{
    DESKTOP_HOME,
    DESKTOP_SETING,
    DESKTOP_TASKMANAGER,
    DESKTOP_SETING_TIME,
    DESKTOP_SETING_ID,
    DESKTOP_SETING_PEOPLE
};
extern int8_t display_number;
extern int8_t last_display_number;
extern MONITOR_DESTOP desktop_state;

// muc trong menu SETTING (funtion_mode)
#define FUNTION_MODE_PLAYER 0
#define FUNTION_MODE_FUNTION 1
#define FUNTION_MODE_MODE 2
#define FUNTION_MODE_ID 3
#define FUNTION_MODE_COLOR 4 // chon de vao man hinh chinh mau giao dien (UI_ACCENT)
#define FUNTION_MODE_FONT 5  // chon de vao man hinh chon kieu chu so cho dong ho
#define FUNTION_MODE_TASK 6  // chon de mo man hinh Task Manager (CPU/RAM/GPU/WIFI)
#define FUNTION_MODE_LANGUAGE 7 // chon de mo man hinh doi ngon ngu giao dien (VI/EN)
#define FUNTION_MODE_COUNT 8

extern int8_t color_index;
extern int8_t last_color_index;
extern bool show_color;      // true khi dang xem man hinh chinh mau
extern bool last_show_color;

// Chon kieu chu so cho dong ho gom 2 buoc, giong 1 "wizard": buoc 1 chon HO CHU (5 kieu, xem
// CLOCK_FONT_FAMILIES trong PLG_screens.cpp), buoc 2 chon CO CHU (size) rieng cho ho chu vua
// chon (moi ho chu co so co lon nho toi da khac nhau, xem get_clock_size_count/get_clock_size_value).
#define CLOCK_FONT_COUNT 5
extern int8_t clock_font_index;      // muc dang duyet trong man hinh chon HO CHU (nhu color_index)
extern int8_t last_clock_font_index; // -1 khi vua vao/can ve lai toan bo
extern int8_t active_clock_font;     // ho chu dang duoc ap dung cho dong ho, luu vao flash
extern bool show_font;               // true khi dang xem man hinh chon HO CHU (buoc 1)
extern bool last_show_font;

extern int8_t clock_size_index;      // muc (index) dang duyet trong man hinh chon CO CHU (buoc 2)
extern int8_t last_clock_size_index; // -1 khi vua vao/can ve lai toan bo
extern int8_t active_clock_size;     // co chu (size) dang duoc ap dung cho dong ho, luu vao flash
extern bool show_font_size;          // true khi dang xem man hinh chon CO CHU (buoc 2, sau khi da chon HO CHU)
extern bool last_show_font_size;

extern bool menu_needs_full_draw; // true khi vua vao menu SETTING -> ve lai tat ca

/*------------------- Ngon ngu giao dien (VI/EN) -------------------*/
#define UI_LANG_VI 0
#define UI_LANG_EN 1
#define UI_LANG_COUNT 2
extern int8_t ui_language;         // ngon ngu dang ap dung cho toan bo giao dien, luu vao flash
extern int8_t language_index;      // muc dang duyet trong man hinh chon ngon ngu (nhu color_index)
extern int8_t last_language_index; // -1 khi vua vao/can ve lai toan bo
extern bool show_language;         // true khi dang xem man hinh chon ngon ngu
extern bool last_show_language;

/*------------------- PC Task Manager chart (nhan tu pc_monitor/monitor.py) -------------------*/
// dinh dang du lieu nhan qua USB serial: "CPU:<int>;RAM:<int>;GPU:<int>;GPUMEM:<int>;WIFI:<int>;TIME:..;DATE:..;BAT:<int>\n"
#define CHART_SAMPLES 100 // so mau hien thi tren moi bieu do (= chieu rong bieu do, px)
// gia tri -1 = "chua co du lieu that", khac voi 0 = "co du lieu that va la 0%".
extern int8_t chart_cpu[CHART_SAMPLES];
extern int8_t chart_ram[CHART_SAMPLES];
extern int8_t chart_gpu[CHART_SAMPLES];    // GPU 3D (core usage %)
extern int8_t chart_gpumem[CHART_SAMPLES]; // GPU memory (VRAM %)
extern int8_t chart_wifi[CHART_SAMPLES];
extern bool taskmanager_dirty; // true khi co mau moi can ve lai
extern char serial_line_buf[112];
extern uint8_t serial_line_len;

/*------------------- Dong ho (nhan tu pc_monitor/monitor.py, truong TIME:HH:MM:SS;DATE:DD/MM/YYYY) -------------------*/
extern char current_time_str[9];
extern char last_time_str[9];
extern char current_date_str[11];
extern char last_date_str[11];
extern bool clock_dirty;
extern bool show_clock;      // true khi dang xem man hinh dong ho
extern bool last_show_clock;
