#include "PLG_input.h"
#include <stdio.h>
#include "pico/bootrom.h"
#include "PLG_pins.h"
#include "PLG_state.h"
#include "PLG_theme.h"
#include "PLG_display.h"
#include "PLG_flash_settings.h"

// tang/giam gia tri dang duoc dieu khien boi encoder, tuy man hinh hien tai
static void key_value_tang()
{
    switch (desktop_state)
    {
    case DESKTOP_HOME:
        // pin ben trai (battery1) gio lay tu pin laptop qua serial, encoder o HOME
        // chi con dung de chinh pin may (battery2) khi chua co cam bien that
        battery2++;
        break;
    case DESKTOP_SETING:
        if (show_color)
            color_index++;
        else
            funtion_mode++;
        break;
    default:
        break;
    }
}
static void key_value_giam()
{
    switch (desktop_state)
    {
    case DESKTOP_HOME:
        battery2--;
        break;
    case DESKTOP_SETING:
        if (show_color)
            color_index--;
        else
            funtion_mode--;
        break;
    default:
        break;
    }
}

void encoder_isr(uint gpio, uint32_t events)
{
    // KHONG duoc goi printf() trong ISR (in qua USB CDC rat cham), se lam tre xu ly canh xung tiep theo
    absolute_time_t now = get_absolute_time();
    // debounce ~5ms de loc rung tiep diem co khi khi quay nhanh
    if (absolute_time_diff_us(last_time, now) < 5000)
        return;
    last_time = now;

    if (gpio_get(CLK) == 0)
        key_value_tang();
    else
        key_value_giam();
}

// nhan giu nut > 2s => chuyen tab (HOME <-> SETTING); nha nut som hon => nhan ngan (chon/xac nhan).
// Truoc day 2 loai thao tac nay dung chung mot dieu kien va co the cung fire trong 1 lan giu nut
// (vua chon/xac nhan, vua doi tab), khien man hinh ve chong len nhau khi chuyen tab ("chong cheo").
// Gio dung 1 co button_long_fired de dam bao MOI LAN NHAN chi sinh ra DUNG 1 HANH DONG.
void read_button()
{
    /*---------------UPDATE FIRMWARE-----------------------*/
    now_button_1 = gpio_get(button_1);
    if (now_button_1 == 0)
    {
        if (last_button_1 != now_button_1)
        {
            reset_usb_boot(0, 0);
            last_button_1 = now_button_1;
        }
    }
    /*-----------------------------------------------------*/

    now_button = gpio_get(button);

    if (last_button == 1 && now_button == 0)
    {
        // canh nhan xuong: bat dau dem thoi gian giu nut
        timer2 = to_ms_since_boot(get_absolute_time());
        button_long_fired = false;
    }

    if (now_button == 0 && !button_long_fired &&
        to_ms_since_boot(get_absolute_time()) - timer2 > 2000)
    {
        // giu du 2s: chuyen tab, danh dau da xu ly de khi tha nut khong con fire hanh dong chon/xac nhan nua
        display_number++;
        button_long_fired = true;
    }

    if (last_button == 0 && now_button == 1)
    {
        // canh tha nut: neu chua bi xu ly bang giu lau thi day la 1 lan nhan ngan -> chon/xac nhan
        if (!button_long_fired)
        {
            if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_TASK)
            {
                printf("PLG_>>>> back to TASK MANAGER (home)\n\r");
                display_number = 0; // man hinh chinh (HOME) chinh la Task Manager
            }
            else if (desktop_state == DESKTOP_SETING && show_clock)
            {
                printf("PLG_>>>> exit CLOCK, back to SETTING menu\n\r");
                show_clock = false;
                last_show_clock = false; // dong bo lai co, de lan sau vao CLOCK duoc xoa man hinh dung cach
                menu_needs_full_draw = true;
            }
            else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_ID)
            {
                printf("PLG_>>>> enter CLOCK\n\r");
                show_clock = true;
            }
            else if (desktop_state == DESKTOP_SETING && show_color)
            {
                printf("PLG_>>>> apply COLOR #%d, back to SETTING menu\n\r", color_index);
                UI_ACCENT = UI_ACCENT_PRESETS[color_index];
                save_color_to_flash(color_index); // luu lai, mat nguon van giu mau da chon
                show_color = false;
                last_show_color = false; // dong bo lai co, de lan sau vao COLOR duoc xoa man hinh dung cach
                // buoc ve lai toan bo cac man hinh dung UI_ACCENT
                menu_needs_full_draw = true;
                last_desktop = !desktop;
                last_status = !status;
                taskmanager_dirty = true;
            }
            else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_COLOR)
            {
                printf("PLG_>>>> enter COLOR\n\r");
                for (uint8_t i = 0; i < UI_ACCENT_PRESET_COUNT; i++)
                {
                    if (UI_ACCENT_PRESETS[i] == UI_ACCENT)
                    {
                        color_index = i;
                        break;
                    }
                }
                show_color = true;
            }
        }
    }

    last_button = now_button;
}

void DISPLAY_ROLL()
{
    switch (display_number)
    {
    case 0:
        desktop_state = DESKTOP_HOME; // man hinh chinh = Task Manager
        break;
    case 1:
        desktop_state = DESKTOP_SETING;
        break;

    default:
        break;
    }
    if (last_display_number != display_number)
    {
        myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
        last_display_number = display_number;
        desktop = !last_desktop;

        if (display_number == 1)
        {
            funtion_mode = 0;
            menu_needs_full_draw = true; // buoc ve lai toan bo menu 1 lan khi vao SETTING
            show_clock = false;          // luon vao menu truoc, khong vao thang man hinh dong ho
            show_color = false;          // luon vao menu truoc, khong vao thang man hinh chon mau

            // xoa gio Task Manager o goc tren-phai khi roi HOME, tranh no bi dinh lai
            // (khong duoc xoa) tren cac man hinh khac nhu SETTING/CLOCK
            myTFT.TFTfillRect(188, 4, 65, 17, UI_BG);
            extern void reset_taskmanager_clock_cache(); // dinh nghia trong PLG_screens.cpp
            reset_taskmanager_clock_cache();
        }
    }
    if (display_number > 1)
    {
        display_number = 0;
    }
}
