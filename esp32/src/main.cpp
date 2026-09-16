// Entry point: setup()/loop() cua firmware PLG TFT LCD Task Manager - ban ESP32-S3.
// Sprint 2: tach lop Transport (USB/BLE/WiFi, xem PLG_transport.h) + man hinh SETTING >
// CONNECTION de chon giao thuc - BLE/WiFi con la stub, hien thuc that o Sprint 3-5 (xem
// README_ESP32_MIGRATION.md). Logic chi tiet nam trong include/ va src/.

#include <Arduino.h>
#include <string.h>

#include "PLG_pins.h"
#include "PLG_state.h"
#include "PLG_display.h"
#include "PLG_flash_settings.h"
#include "PLG_input.h"
#include "PLG_transport.h"
#include "PLG_charts.h"
#include "PLG_screens.h"
#include "PLG_wifi_ui.h"

void setup()
{
    Serial.begin(115200); // dung chung lam kenh debug (Serial.print...) cho ca 3 transport

    // Tat han den bao (khong chi bo phan nhap nhay): neu chi bo doan nhap nhay trong loop() ma
    // khong ghi muc o day thi den co the nam lai o trang thai sang lì.
    pinMode(PIN_LIGHT_BOARD, OUTPUT);
    digitalWrite(PIN_LIGHT_BOARD, LOW);

    setup_display();
    setup_input();
    load_settings_from_flash(); // khoi phuc mau + kieu chu dong ho da chon lan truoc (neu co)
    transport_begin(transport_load_mode()); // khoi dong lai dung giao thuc da chon lan truoc (mac dinh USB)

    // -1 = "chua co du lieu that" (xem giai thich o PLG_state.h canh khai bao chart_cpu...)
    memset(chart_cpu, -1, CHART_SAMPLES);
    memset(chart_ram, -1, CHART_SAMPLES);
    memset(chart_gpu, -1, CHART_SAMPLES);
    memset(chart_gpumem, -1, CHART_SAMPLES);
    memset(chart_wifi, -1, CHART_SAMPLES);
    memset(chart_temp, -1, CHART_SAMPLES);

    MONITOR_BEGIN();
    Serial.println("PLG_>>>> setup xong");
}

void loop()
{
    process_input();
    transport_poll();
    DISPLAY_ROLL();

    switch (desktop_state)
    {
    case DESKTOP_HOME:
        // nhan ngan 1 lan tren man hinh chinh: chuyen doi tab Task Manager <-> Clock (xem PLG_input.cpp)
        if (show_clock)
            MONITOR_CLOCK();
        else
            MONITOR_TASKMANAGER();
        break;
    case DESKTOP_SETING:
        if (show_clock)
            MONITOR_CLOCK();
        else if (show_color)
            MONITOR_COLOR();
        else if (show_font)
            MONITOR_FONT();
        else if (show_font_size)
            MONITOR_FONT_SIZE();
        else if (show_language)
            MONITOR_LANGUAGE();
        else if (show_clock_style)
            MONITOR_CLOCK_STYLE();
        else if (show_wifi_ui)
            wifi_ui_render(); // wizard nhieu buoc, tu ve theo buoc dang o (xem PLG_wifi_ui.h)
        else if (show_ble_status)
            MONITOR_BLE_STATUS();
        else if (show_connection)
            MONITOR_CONNECTION();
        else
            MONITOR_FUNTION();
        break;
    default:
        break;
    }

#if PLG_HEARTBEAT_LED
    // den bao nhap nhay 1Hz: dau hieu nhin thay ngay la firmware con chay (khong treo)
    if ((int32_t)millis() - timer1 > 500)
    {
        flat1 = !flat1;
        digitalWrite(PIN_LIGHT_BOARD, flat1);
        timer1 = (int32_t)millis();
    }
#endif
}
