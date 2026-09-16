#pragma once
#include <stdint.h>
// Doc du lieu Task Manager (CPU/RAM/GPU/WIFI/gio/pin) tu pc_monitor/monitor.py qua USB Serial.

// Day 1 gia tri moi vao cuoi bieu do (dich mang sang trai), kep trong [0,cap].
// cap mac dinh 100 (dung cho cac chi so phan tram); WIFI (toc do mang Mbps) dung
// cap 127 (gioi han cua int8_t) vi toc do mang co the vuot qua 100.
void chart_push(int8_t *buf, int value, int cap = 100);

// Doc tung ky tu tu USB stdio (khong block), gom thanh dong, parse khi gap '\n'.
// Cung xu ly bat tay nhan dien thiet bi "PLG_ID?" -> "I AM PLG_TFT_LCD_TASKMANAGER".
void read_taskmanager_serial();

// true neu nhan duoc BAT KY dong hop le nao (du lieu CPU:... hoac bat tay PLG_ID?) trong 3s gan
// day. Dung lam nguon cho CONNECT_STATUS khi PLG_transport dang o TRANSPORT_USB (xem
// PLG_transport.cpp) - PC gui du lieu moi ~0.8s (mac dinh cua monitor.py) nen 3s la du du de
// khong nhap nhay "mat ket noi" gia khi chi bi tre 1-2 chu ky gui.
bool serial_link_is_connected();

// Dat lai moc thoi gian "nhan du lieu lan cuoi" ve "chua bao gio nhan". Goi khi vua chuyen sang
// TRANSPORT_USB, tranh hien "connected" gia tu du lieu nhan duoc TRUOC khi doi mode (vd nguoi
// dung dang o BLE, cam lai cap USB de debug, roi chuyen ve USB - khong nen "connected" ngay lap
// tuc truoc khi PC thuc su gui gi qua cong nay).
void serial_link_reset_connection();
