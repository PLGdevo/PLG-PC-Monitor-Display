#include "PLG_serial_link.h"
#include <Arduino.h>
#include "PLG_protocol.h"

static const uint32_t CONNECTION_TIMEOUT_MS = 3000;

// Gui tra loi bat tay nguoc lai PC qua chinh cong USB Serial.
static void serial_reply(const char *text)
{
    Serial.print(text);
}

bool serial_link_is_connected()
{
    return protocol_ms_since_rx() < CONNECTION_TIMEOUT_MS;
}

void read_taskmanager_serial()
{
    // Serial.available()/read() la ban tuong duong khong-block cua getchar_timeout_us(0) ben
    // pico-sdk: doc het nhung byte da san co trong buffer, khong doi them neu chua co gi.
    while (Serial.available() > 0)
    {
        uint8_t c = (uint8_t)Serial.read();
        protocol_feed(&c, 1, serial_reply);
    }
}
