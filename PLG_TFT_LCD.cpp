// Entry point: setup()/loop() cua firmware PLG TFT LCD Task Manager.
// Logic chi tiet nam trong include/ va src/ (xem README.md - muc "Cau truc thu muc").
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "PLG_pins.h"
#include "PLG_state.h"
#include "PLG_display.h"
#include "PLG_flash_settings.h"
#include "PLG_input.h"
#include "PLG_serial_link.h"
#include "PLG_charts.h"
#include "PLG_screens.h"

void setup(void);
void loop(void);

// debug: bat nhap nhay den bao + in trang thai encoder/nut ra Serial, khong dung trong loop() binh thuong
void live_pico()
{
    if (to_ms_since_boot(get_absolute_time()) - timer1 > 1000)
    {
        flat1 = !flat1;
        gpio_put(PIN_LIGHT_BOARD, flat1);
        timer1 = to_ms_since_boot(get_absolute_time());

        EN_CLK = gpio_get(CLK);
        EN_DT = gpio_get(DT);
        EN_BT = gpio_get(button);
        printf("ENA:%d  ENB:%d  SW:%d\n", EN_CLK, EN_DT, EN_BT);
    }
}

void setup()
{
    setup_pin();
    load_settings_from_flash(); // khoi phuc mau + kieu chu dong ho da chon lan truoc (neu co)
    // -1 = "chua co du lieu that" (xem giai thich o PLG_state.h canh khai bao chart_cpu...)
    memset(chart_cpu, -1, CHART_SAMPLES);
    memset(chart_ram, -1, CHART_SAMPLES);
    memset(chart_gpu, -1, CHART_SAMPLES);
    memset(chart_gpumem, -1, CHART_SAMPLES);
    memset(chart_wifi, -1, CHART_SAMPLES);
    myTFT.TFTsetRotation(myTFT.TFT_Degrees_270);
    MONITOR_BEGIN();
    printf("PLG_end setup\n\r");
}

uint32_t lastTime = 0;
uint32_t loopCount = 0;
void loop()
{
    loopCount++;

    read_button();
    read_taskmanager_serial();
    DISPLAY_ROLL();

    switch (desktop_state)
    {
    case DESKTOP_HOME:
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
        else
            MONITOR_FUNTION();
        break;
    default:
        break;
    }

    if (to_us_since_boot(get_absolute_time()) - lastTime >= 1000)
    {
        printf("PLG_end loop  %d \n\r", to_us_since_boot(get_absolute_time()) - lastTime);
        loopCount = 0;
        lastTime = to_us_since_boot(get_absolute_time());
    }
}

int main()
{
    setup();
    while (true)
    {
        loop();
    }
}
