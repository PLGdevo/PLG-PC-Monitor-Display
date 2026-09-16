#pragma once
#include <stdint.h>
// Bang chuoi text hien thi theo ngon ngu dang chon (ui_language, xem PLG_state.h).
// Tat ca cac ham deu tra ve chuoi ung voi ui_language HIEN TAI, khong can truyen tham so ngon ngu.

// nhan cac muc trong menu SETTING (PLAYER/FUNTION/MODE/CLOCK/COLOR/FONT/TASK/LANGUAGE),
// mang co FUNTION_MODE_COUNT phan tu, dung chung cho draw_menu_row
const char *const *lang_menu_labels();

// dong huong dan "Nhan nut de chon co chu" (man hinh chon HO CHU dong ho, buoc 1)
const char *lang_hint_choose_size();

// dong huong dan "Nhan nut de ap dung" (man hinh chon CO CHU dong ho, chon mau, chon ngon ngu)
const char *lang_hint_apply();

// ten hien thi cua 1 ngon ngu theo index (0=VI, 1=EN), dung cho man hinh chon ngon ngu
const char *lang_name(int8_t index);

// ten hien thi cua 1 kieu dong ho theo index (0=DIGITAL, 1=ANALOG), dung cho man hinh chon kieu dong ho
const char *lang_clock_style_name(int8_t index);

// nhan 1 ky tu ("C"/"R") hien thi cung hang thanh pin tren man hinh dong ho - khe rat hep, phai
// ket thuc truoc x=255 (gioi han uint8_t cua TFTdrawText), xem draw_clock_cpu_ram trong PLG_screens.cpp
const char *lang_label_cpu();
const char *lang_label_ram();

/*------------------- Man hinh trang thai ket noi (BLE/WiFi) -------------------*/
// tieu de "THIET BI BLUETOOTH" / "BLUETOOTH DEVICE"
const char *lang_ble_title();

// trang thai "Dang cho ket noi..." / "Waiting for connection..."
const char *lang_status_waiting();

// trang thai "Da ket noi" / "Connected"
const char *lang_status_connected();

// dong huong dan "Nhan nut de quay lai" (thoat man hinh trang thai ve menu SETTING)
const char *lang_hint_back();

/*------------------- Wizard WiFi (PLG_wifi_ui.cpp) -------------------*/
const char *lang_wifi_scanning();     // "Dang quet mang..."
const char *lang_wifi_no_network();   // "Khong tim thay mang nao"
const char *lang_wifi_pick_network(); // "Chon mang WiFi"
const char *lang_wifi_password();     // "Nhap mat khau"
const char *lang_wifi_connecting();   // "Dang ket noi..."
const char *lang_wifi_failed();       // "Ket noi that bai"
const char *lang_wifi_connected();    // "Da ket noi WiFi"

// huong dan trong buoc nhap mat khau: giu nut de xoa lui (quy uoc rieng cua wizard)
const char *lang_wifi_hint_password();

// nhan cua 2 muc dac biet o cuoi bang ky tu wheel-picker
const char *lang_wifi_done();   // "[XONG]"
const char *lang_wifi_cancel(); // "[HUY]"

// huong dan o man hinh hien IP: giu nut de luu thanh IP tinh (khi chua luu)
const char *lang_wifi_hint_save();

// huong dan o man hinh hien IP: giu nut de quen mang (khi da luu)
const char *lang_wifi_hint_forget();

// bao da luu cau hinh thanh cong
const char *lang_wifi_saved();
