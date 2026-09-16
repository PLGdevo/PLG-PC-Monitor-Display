#pragma once
#include <stdint.h>
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
// Ban Pico dung absolute_time_t; tren ESP32 dung thang micros() (uint32_t) cho gon.
// Phep tru uint32_t van dung ke ca khi bo dem tran (~70 phut 1 lan) nen khong can xu ly rieng.
extern volatile uint32_t last_time_us;

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
#define FUNTION_MODE_CLOCK_STYLE 8 // chon de mo man hinh chon kieu hien thi dong ho: SO (digital) / KIM (analog)
#define FUNTION_MODE_CONNECTION 9  // chon de mo man hinh chon giao thuc ket noi PC: USB/BLUETOOTH/WIFI (xem PLG_transport.h)
#define FUNTION_MODE_COUNT 10

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

/*------------------- Kieu hien thi dong ho: SO (digital, ho/co chu o tren) / KIM (analog) -------------------*/
// co 3 bien the KIM (analog) khac nhau ve mat dong ho (CLASSIC/MINIMAL/BOLD) - dung
// IS_CLOCK_STYLE_ANALOG(s) thay vi so sanh == CLOCK_STYLE_ANALOG_CLASSIC de kiem tra "co phai kim khong"
// (bao gom ca 3 bien the), tranh phai sua nhieu noi moi khi them/bot bien the.
#define CLOCK_STYLE_DIGITAL 0
#define CLOCK_STYLE_ANALOG_CLASSIC 1 // mat hien tai: vien tron + so 12/3/6/9 + vach chia
#define CLOCK_STYLE_ANALOG_MINIMAL 2 // toi gian: khong so, chi vach gio to + kim manh
#define CLOCK_STYLE_ANALOG_BOLD 3    // dam: cham tron danh dau gio thay so, kim day, vien mau nhan dien
#define CLOCK_STYLE_COUNT 4
#define IS_CLOCK_STYLE_ANALOG(s) ((s) >= CLOCK_STYLE_ANALOG_CLASSIC && (s) <= CLOCK_STYLE_ANALOG_BOLD)
extern int8_t clock_style_index;      // muc dang duyet trong man hinh chon kieu dong ho (nhu color_index)
extern int8_t last_clock_style_index; // -1 khi vua vao/can ve lai toan bo
extern int8_t active_clock_style;     // kieu dang duoc ap dung cho dong ho, luu vao flash
extern bool show_clock_style;         // true khi dang xem man hinh chon kieu dong ho
extern bool last_show_clock_style;

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

/*------------------- Giao thuc ket noi PC: USB/BLUETOOTH/WIFI (xem PLG_transport.h) -------------------*/
// Kieu int8_t (khong phai enum TransportMode) de PLG_state.h khong phai include PLG_transport.h -
// giu dung quy uoc cua file nay: chi luu du lieu tho, khong phu thuoc cac module khac.
extern int8_t active_connection_mode; // giao thuc dang dung, luu vao NVS rieng (transport_save_mode)
extern int8_t connection_index;       // muc dang duyet trong man hinh chon giao thuc (nhu color_index)
extern int8_t last_connection_index;  // -1 khi vua vao/can ve lai toan bo
extern bool show_connection;          // true khi dang xem man hinh chon giao thuc ket noi
extern bool last_show_connection;

// Man hinh trang thai BLE, hien ngay sau khi chon BLUETOOTH (thay vi quay thang ve menu):
// nguoi dung can biet ten thiet bi de ghep noi tu PC va thay duoc da ket noi hay chua.
extern bool show_ble_status;
extern bool last_show_ble_status;

// Wizard cau hinh WiFi (quet mang -> nhap mat khau -> hien IP). Chi 1 co duy nhat o day; buoc
// dang o trong wizard la trang thai NOI BO cua PLG_wifi_ui.cpp, khong bay ra state toan cuc.
extern bool show_wifi_ui;

/*------------------- PC Task Manager chart (nhan tu pc_monitor/monitor.py) -------------------*/
// dinh dang du lieu nhan qua USB serial: "CPU:<int>;RAM:<int>;GPU:<int>;GPUMEM:<int>;WIFI:<int>;TEMP:<int>;TIME:..;DATE:..;BAT:<int>\n"
#define CHART_SAMPLES 100 // so mau hien thi tren moi bieu do (= chieu rong bieu do, px)
// gia tri -1 = "chua co du lieu that", khac voi 0 = "co du lieu that va la 0%".
extern int8_t chart_cpu[CHART_SAMPLES];
extern int8_t chart_ram[CHART_SAMPLES];
extern int8_t chart_gpu[CHART_SAMPLES];    // GPU 3D (core usage %)
extern int8_t chart_gpumem[CHART_SAMPLES]; // GPU memory (VRAM %)
extern int8_t chart_wifi[CHART_SAMPLES];
extern int8_t chart_temp[CHART_SAMPLES]; // nhiet do CPU (do C, gia tri 0-100 vua khop thang do % co san)
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
