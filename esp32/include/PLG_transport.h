#pragma once
#include <stdint.h>
// Dispatcher chon nguon du lieu Task Manager: USB Serial / Bluetooth (BLE) / WiFi.
//
// UI (PLG_screens.cpp) va logic Task Manager khong can biet du lieu den tu dau - chi goi
// transport_poll() moi vong loop() va doc CONNECT_STATUS (PLG_state.h) de ve icon ket noi.
// Viec phan tich du lieu nam o PLG_protocol.* (dung chung cho ca 3 duong truyen).
//
// Ca 3 duong truyen deu da hien thuc that: USB (Sprint 1), BLE (Sprint 3), WiFi (Sprint 4).

enum TransportMode : uint8_t
{
    TRANSPORT_USB = 0,
    TRANSPORT_BLE = 1,
    TRANSPORT_WIFI = 2,
    TRANSPORT_MODE_COUNT = 3
};

// Ten hien thi cho tung mode trong SETTING > CONNECTION. La ten rieng (USB/BLUETOOTH/WIFI) nen
// dung chung cho ca VI/EN, khong di qua PLG_lang.h nhu cac nhan menu khac.
extern const char *const TRANSPORT_MODE_NAMES[TRANSPORT_MODE_COUNT];

// Chon transport dang dung + khoi dong no. Cap nhat active_connection_mode (PLG_state.h) va dat
// lai CONNECT_STATUS ve false (tranh hien "connected" gia tu transport cu truoc do chuyen mode).
// Goi 1 lan trong setup() (voi transport_load_mode()) va moi khi nguoi dung doi mode trong
// SETTING > CONNECTION.
void transport_begin(TransportMode mode);

// Doc/xu ly du lieu dang den tu transport dang dung (active_connection_mode) + cap nhat
// CONNECT_STATUS. Goi 1 lan moi vong loop(), thay cho read_taskmanager_serial() truoc day.
void transport_poll();

// Luu/doc mode dang chon vao NVS RIENG voi mau/font/ngon ngu (namespace "plg_net" thay vi
// "plg_ui" cua PLG_flash_settings) - vi day la cau hinh "may dang dung gi de noi chuyen voi
// PC", khac ban chat voi tuy chinh giao dien, va Sprint 5 se can them cung namespace nay de
// luu SSID/mat khau/IP tinh.
void transport_save_mode(TransportMode mode);
TransportMode transport_load_mode();
