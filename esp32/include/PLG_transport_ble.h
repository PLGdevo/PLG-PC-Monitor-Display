#pragma once
// Transport Bluetooth Low Energy: nhan du lieu Task Manager tu PC qua BLE thay vi cap USB.
//
// Dung Nordic UART Service (NUS) - "cong COM ao tren BLE": 1 characteristic WRITE de PC gui
// du lieu xuong, 1 characteristic NOTIFY de board tra loi (bat tay PLG_ID?). Chon NUS vi day
// la chuan de-facto duoc moi thu vien BLE phia PC (ke ca `bleak` cua Python) ho tro san, khong
// phai tu dinh nghia giao thuc rieng.

// Ten thiet bi hien khi PC do tim BLE. Cung hien tren man hinh MONITOR_BLE_STATUS de nguoi dung
// biet can ghep noi voi thiet bi nao.
#define BLE_DEVICE_NAME "PLG_TFT_LCD"

// Khoi tao BLE stack + bat quang ba (advertising). Goi khi chuyen sang TRANSPORT_BLE.
void transport_ble_begin();

// Lay du lieu PC da gui (duoc callback BLE xep vao hang doi) ra xu ly. Goi moi vong loop().
void transport_ble_poll();

// true khi PC dang ghep noi. Khac USB (phai suy ra tu "con nhan duoc du lieu khong"), BLE bao
// su kien connect/disconnect ro rang nen day la trang thai that, khong phai suy doan.
bool transport_ble_is_connected();
