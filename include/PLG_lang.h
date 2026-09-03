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

// nhan 1 ky tu ("C"/"R") hien thi cung hang thanh pin tren man hinh dong ho - khe rat hep, phai
// ket thuc truoc x=255 (gioi han uint8_t cua TFTdrawText), xem draw_clock_cpu_ram trong PLG_screens.cpp
const char *lang_label_cpu();
const char *lang_label_ram();
