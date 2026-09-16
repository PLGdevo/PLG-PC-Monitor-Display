#pragma once
#include <stdint.h>
// Transport USB Serial: nhan du lieu Task Manager tu pc_monitor/monitor.py qua cong USB CDC.
// Viec phan tich du lieu nam o PLG_protocol.* (dung chung voi BLE/WiFi).

// Doc het byte dang co trong buffer Serial (khong block) va dua vao PLG_protocol.
void read_taskmanager_serial();

// true neu nhan duoc BAT KY dong hop le nao (du lieu CPU:... hoac bat tay PLG_ID?) trong 3s gan
// day. Dung lam nguon cho CONNECT_STATUS khi PLG_transport dang o TRANSPORT_USB (xem
// PLG_transport.cpp). USB CDC khong bao bao tin hieu "PC da ngat ket noi" mot cach dang tin cay
// nen phai suy ra tu viec con nhan duoc du lieu hay khong; PC gui moi ~0.8s (mac dinh cua
// monitor.py) nen 3s la du du de khong nhap nhay "mat ket noi" gia khi chi tre 1-2 chu ky gui.
bool serial_link_is_connected();
