#include "PLG_serial_link.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include "PLG_state.h"

// "Heartbeat" ket noi: moc thoi gian (millis) cua dong hop le gan nhat + co "da tung nhan".
// Co rieng vi millis()==0 luc vua boot la gia tri hop le, khong the dung no lam dau hieu
// "chua bao gio nhan du lieu".
static uint32_t last_rx_ms = 0;
static bool has_rx_ever = false;
static const uint32_t CONNECTION_TIMEOUT_MS = 3000;

bool serial_link_is_connected()
{
    return has_rx_ever && (millis() - last_rx_ms < CONNECTION_TIMEOUT_MS);
}

void serial_link_reset_connection()
{
    has_rx_ever = false;
}

void chart_push(int8_t *buf, int value, int cap)
{
    if (value < 0)
        value = 0;
    if (value > cap)
        value = cap;
    memmove(buf, buf + 1, (CHART_SAMPLES - 1) * sizeof(int8_t));
    buf[CHART_SAMPLES - 1] = (int8_t)value;
}

void read_taskmanager_serial()
{
    // Serial.available()/read() la ban tuong duong khong-block cua getchar_timeout_us(0) ben
    // pico-sdk: doc het nhung byte da san co trong buffer, khong doi them neu chua co gi.
    while (Serial.available() > 0)
    {
        int c = Serial.read();
        if (c == '\n' || c == '\r')
        {
            if (serial_line_len > 0)
            {
                serial_line_buf[serial_line_len] = '\0';
                // Bat ky dong nao doc duoc trot lot (ke ca bat tay PLG_ID?) deu tinh la "co tin
                // hieu song" tu PC - khong doi den khi parse ra CPU/RAM... moi coi la connected.
                last_rx_ms = millis();
                has_rx_ever = true;
                // Bat tay nhan dien thiet bi: monitor.py gui "PLG_ID?" khi do cong tu dong,
                // board tra loi cau co dinh nay de PC xac nhan dung la board PLG (khong
                // phai thiet bi USB Serial khac) ma khong can nguoi dung tu chon cong.
                if (strcmp(serial_line_buf, "PLG_ID?") == 0)
                {
                    Serial.print("I AM PLG_TFT_LCD_TASKMANAGER\n");
                }
                else
                {
                    int cpu = -1, ram = -1, gpu = -1, gpumem = -1, wifi = -1, temp = -1, bat = -1;
                    char time_buf[9] = "", date_buf[11] = "";
                    int n = sscanf(serial_line_buf, "CPU:%d;RAM:%d;GPU:%d;GPUMEM:%d;WIFI:%d;TEMP:%d;TIME:%8[^;];DATE:%10[^;];BAT:%d",
                                   &cpu, &ram, &gpu, &gpumem, &wifi, &temp, time_buf, date_buf, &bat);
                    if (n >= 5)
                    {
                        chart_push(chart_cpu, cpu);
                        chart_push(chart_ram, ram);
                        chart_push(chart_gpu, gpu);
                        chart_push(chart_gpumem, gpumem);
                        chart_push(chart_wifi, wifi, 127); // WIFI: toc do mang Mbps, khong phai %, nen cap 127 (max int8_t)
                        taskmanager_dirty = true;
                    }
                    // TEMP la truong moi them sau (monitor.py cu chua gui): chi push khi co mat,
                    // khong thi giu nguyen bieu do nhiet do o trang thai "chua co du lieu" (-1)
                    if (n >= 6)
                    {
                        chart_push(chart_temp, temp);
                    }
                    if (n >= 7 && strcmp(current_time_str, time_buf) != 0)
                    {
                        strncpy(current_time_str, time_buf, sizeof(current_time_str) - 1);
                        current_time_str[sizeof(current_time_str) - 1] = '\0';
                        clock_dirty = true;
                    }
                    if (n >= 8 && strcmp(current_date_str, date_buf) != 0)
                    {
                        strncpy(current_date_str, date_buf, sizeof(current_date_str) - 1);
                        current_date_str[sizeof(current_date_str) - 1] = '\0';
                        clock_dirty = true;
                    }
                    // pin ben trai = pin laptop nhan tu monitor.py; -1 nghia la may khong co pin (PC ban) -> giu nguyen gia tri cu
                    if (n == 9 && bat >= 0)
                    {
                        battery1 = (int8_t)bat;
                    }
                }
                serial_line_len = 0;
            }
        }
        else if (serial_line_len < sizeof(serial_line_buf) - 1)
        {
            serial_line_buf[serial_line_len++] = (char)c;
        }
        else
        {
            serial_line_len = 0; // dong qua dai, bo qua
        }
    }
}
