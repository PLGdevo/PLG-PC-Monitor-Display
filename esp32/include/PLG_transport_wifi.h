#pragma once
// STUB Sprint 2: hien thuc WiFi that (scan/connect + TCP server + luu IP tinh) se lam o
// Sprint 4-5, xem README_ESP32_MIGRATION.md. Cac ham nay ton tai de PLG_transport.cpp goi duoc
// ngay bay gio, cho phep nguoi dung chon WIFI trong SETTING > CONNECTION ma khong lam vo build.

void transport_wifi_begin();
void transport_wifi_poll();
bool transport_wifi_is_connected();
