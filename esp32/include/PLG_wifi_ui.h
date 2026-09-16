#pragma once
#include <stdint.h>
// Wizard cau hinh WiFi: quet mang -> chon SSID -> nhap mat khau -> ket noi -> hien IP.
//
// Tach rieng khoi PLG_screens.cpp vi day khong phai mot "man hinh" don le ma la mot chuoi
// nhieu buoc co trang thai rieng (dang go toi ky tu thu may, dang quet hay quet xong...).
// Nhet vao PLG_screens.cpp se lam file do - von da 1100 dong - phinh them va tron 2 kieu
// logic khac han nhau.
//
// Quy uoc dieu khien trong wizard, KHAC voi phan con lai cua giao dien:
//   - nhan GIU 2s = XOA LUI 1 ky tu (khong phai chuyen tab HOME/SETTING nhu binh thuong),
//     vi trong luc go mat khau thi xoa lui la thao tac can nhieu hon nhieu.
//   - thoat wizard bang cach xoay toi muc [HUY] o cuoi bang ky tu roi nhan.

// Bat dau wizard tu dau (quet mang). Goi khi nguoi dung chon WIFI o SETTING > CONNECTION.
void wifi_ui_enter();

// Ket thuc wizard, giai phong ket qua quet. Goi khi nguoi dung huy hoac hoan tat.
void wifi_ui_exit();

// Ve man hinh ung voi buoc hien tai + tu tien trien (vd quet xong thi chuyen sang danh sach,
// ket noi thanh cong thi chuyen sang man hinh IP). Goi moi vong loop() khi show_wifi_ui.
void wifi_ui_render();

// Dua thao tac nguoi dung vao wizard. Goi tu PLG_input.cpp khi show_wifi_ui dang bat.
void wifi_ui_on_rotate(int32_t delta);
void wifi_ui_on_short_press();
void wifi_ui_on_long_press();
