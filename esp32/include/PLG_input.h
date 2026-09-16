#pragma once
#include <stdint.h>
// Xu ly nguoi dung: encoder xoay (qua ngat), nut nhan (ngan/giu lau), chuyen man hinh.
//
// Kien truc chia 2 tang, khac ban Pico (ISR goi thang vao logic UI):
//  - Tang THO (setup_input/input_take_encoder_delta/input_take_button_event): chi sinh su
//    kien, ISR cuc ngan (bat buoc tren ESP32, xem PLG_input.cpp).
//  - Tang DIEU PHOI (process_input): doc su kien tho va ap dung vao state UI - tuong duong
//    key_value_tang/giam + read_button() cua ban Pico gop lai. Goi 1 lan moi vong loop().

// Cau hinh chan encoder + gan ngat. Goi 1 lan trong setup().
void setup_input();

// So nac encoder da quay ke tu lan goi truoc (duong = thuan chieu kim dong ho, am = nguoc).
// Doc xong thi bo dem duoc tra ve 0, nen moi nac chi duoc xu ly dung 1 lan.
int32_t input_take_encoder_delta();

// Cac loai su kien nut nhan, lay ra bang input_take_button_event().
enum ButtonEvent : uint8_t
{
    BUTTON_NONE = 0,
    BUTTON_SHORT_PRESS, // nha nut truoc 2s: chon/xac nhan
    BUTTON_LONG_PRESS   // giu du 2s: chuyen tab (ban ngay khi du 2s, khong doi nha nut)
};

// Cap nhat trang thai nut va tra ve su kien phat sinh (neu co). Goi moi vong loop().
// Moi lan nhan chi sinh ra DUNG 1 su kien - ngan/giu lau khong bao gio cung fire.
ButtonEvent input_take_button_event();

// Doc encoder + nut, ap dung vao toan bo state UI (tang gia tri dang chon, chuyen man hinh,
// ap dung + luu NVS khi xac nhan...). Tuong duong key_value_tang/giam + read_button() cua
// ban Pico gop lai o 1 cho. Goi 1 lan moi vong loop(), truoc khi ve man hinh.
void process_input();

// cap nhat desktop_state theo display_number, xoa man hinh khi chuyen tab
void DISPLAY_ROLL();
