#include "PLG_transport.h"
#include <Preferences.h>
#include "PLG_state.h"
#include "PLG_protocol.h"
#include "PLG_serial_link.h"
#include "PLG_transport_ble.h"
#include "PLG_transport_wifi.h"

const char *const TRANSPORT_MODE_NAMES[TRANSPORT_MODE_COUNT] = {"USB (COM)", "BLUETOOTH", "WIFI"};

static const char *NVS_NAMESPACE = "plg_net";

void transport_begin(TransportMode mode)
{
    active_connection_mode = (int8_t)mode;

    // Xoa dong dang do dang + quen moc "da tung nhan du lieu" cua transport truoc do: tranh
    // dong bi cat giua chung dinh vao dong dau tien cua transport moi, va tranh hien
    // "connected" gia tu du lieu nhan duoc TRUOC khi doi mode.
    protocol_reset();

    switch (mode)
    {
    case TRANSPORT_USB:
        // Serial.begin() da chay tu setup() (dung chung lam kenh debug cho ca 3 transport)
        // nen khong con gi phai khoi dong rieng.
        break;
    case TRANSPORT_BLE:
        transport_ble_begin();
        break;
    case TRANSPORT_WIFI:
        transport_wifi_begin();
        break;
    }
    CONNECT_STATUS = false;
}

void transport_poll()
{
    switch ((TransportMode)active_connection_mode)
    {
    case TRANSPORT_USB:
        read_taskmanager_serial();
        CONNECT_STATUS = serial_link_is_connected();
        break;
    case TRANSPORT_BLE:
        transport_ble_poll();
        CONNECT_STATUS = transport_ble_is_connected();
        break;
    case TRANSPORT_WIFI:
        transport_wifi_poll();
        CONNECT_STATUS = transport_wifi_is_connected();
        break;
    }
}

void transport_save_mode(TransportMode mode)
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) // false = mo o che do ghi
        return;
    prefs.putUChar("mode", (uint8_t)mode);
    prefs.end();
}

TransportMode transport_load_mode()
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) // true = chi doc
        return TRANSPORT_USB;
    uint8_t mode = prefs.getUChar("mode", (uint8_t)TRANSPORT_USB);
    prefs.end();

    if (mode >= TRANSPORT_MODE_COUNT)
        mode = TRANSPORT_USB; // gia tri hong/khong hop le tu NVS -> ve mac dinh an toan
    return (TransportMode)mode;
}
