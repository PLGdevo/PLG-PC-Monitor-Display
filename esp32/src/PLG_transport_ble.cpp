#include "PLG_transport_ble.h"
#include <Arduino.h>

// In log CANH BAO dung 1 lan moi lan begin(), khong phai moi vong loop() (se ngap Serial monitor).
static bool warned = false;

void transport_ble_begin()
{
    warned = false;
}

void transport_ble_poll()
{
    if (!warned)
    {
        Serial.println("PLG_>>>> BLE: transport chua trien khai (Sprint 3), khong nhan duoc du lieu");
        warned = true;
    }
}

bool transport_ble_is_connected()
{
    return false;
}
