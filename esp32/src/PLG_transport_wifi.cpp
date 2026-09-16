#include "PLG_transport_wifi.h"
#include <Arduino.h>

// In log CANH BAO dung 1 lan moi lan begin(), khong phai moi vong loop() (se ngap Serial monitor).
static bool warned = false;

void transport_wifi_begin()
{
    warned = false;
}

void transport_wifi_poll()
{
    if (!warned)
    {
        Serial.println("PLG_>>>> WIFI: transport chua trien khai (Sprint 4-5), khong nhan duoc du lieu");
        warned = true;
    }
}

bool transport_wifi_is_connected()
{
    return false;
}
