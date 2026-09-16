#pragma once
// STUB Sprint 2: hien thuc BLE that (NimBLE GATT server) se lam o Sprint 3, xem
// README_ESP32_MIGRATION.md. Cac ham nay ton tai de PLG_transport.cpp goi duoc ngay bay gio,
// cho phep nguoi dung chon BLUETOOTH trong SETTING > CONNECTION ma khong lam vo build.

void transport_ble_begin();
void transport_ble_poll();
bool transport_ble_is_connected();
