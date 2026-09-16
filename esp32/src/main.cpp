// Entry point: setup()/loop() cua firmware PLG TFT LCD Task Manager - ban ESP32-S3.
// Sprint 1: dat ngang tinh nang voi ban Pico, van chi dung USB Serial (xem
// README_ESP32_MIGRATION.md). Logic chi tiet nam trong include/ va src/.

#include <Arduino.h>
#include <string.h>

#include "PLG_pins.h"
#include "PLG_state.h"
#include "PLG_display.h"
#include "PLG_flash_settings.h"
#include "PLG_input.h"
#include "PLG_serial_link.h"
#include "PLG_charts.h"
#include "PLG_screens.h"

void setup()
{
    Serial.begin(115200);
    pinMode(PIN_LIGHT_BOARD, OUTPUT);

    setup_display();
    setup_input();
    load_settings_from_flash(); // khoi phuc mau + kieu chu dong ho da chon lan truoc (neu co)

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
    read_taskmanager_serial();
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
        else
            MONITOR_FUNTION();
        break;
    default:
        break;
    }

    // den bao nhap nhay 1Hz: dau hieu nhin thay ngay la firmware con chay (khong treo)
    if ((int32_t)millis() - timer1 > 500)
    {
        flat1 = !flat1;
        digitalWrite(PIN_LIGHT_BOARD, flat1);
        timer1 = (int32_t)millis();
    }
}
