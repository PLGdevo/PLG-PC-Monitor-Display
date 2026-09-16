#include "PLG_input.h"
#include <Arduino.h>
#include "PLG_pins.h"
#include "PLG_state.h"
#include "PLG_theme.h"
#include "PLG_display.h"
#include "PLG_flash_settings.h"
#include "PLG_screens.h"
#include "PLG_transport.h"
#include "PLG_wifi_ui.h"

/*==================== Tang THO: ISR + doc su kien ====================*/

// Bo dem nac encoder, cong/tru trong ISR nen phai volatile.
static volatile int32_t encoder_delta = 0;

// ISR phai nam trong IRAM: neu de o flash, mot ngat xay ra dung luc flash dang ban
// (ghi NVS, doc code chua duoc cache) se lam treo chip.
static void IRAM_ATTR encoder_isr()
{
    // KHONG duoc goi Serial.print() hay bat cu ham cham nao trong ISR: se lam tre xu ly
    // canh xung tiep theo va mat nac khi quay nhanh.
    uint32_t now = micros();
    // debounce ~5ms de loc rung tiep diem co khi khi quay nhanh (giu nguyen nguong cua ban Pico)
    if (now - last_time_us < 5000)
        return;
    last_time_us = now;

    // Ngat bat o canh XUONG cua DT: luc do muc cua CLK cho biet chieu quay.
    if (digitalRead(CLK) == LOW)
        encoder_delta++;
    else
        encoder_delta--;
}

void setup_input()
{
    pinMode(CLK, INPUT_PULLUP);
    pinMode(DT, INPUT_PULLUP);
    pinMode(button, INPUT_PULLUP);

    last_time_us = micros();
    attachInterrupt(digitalPinToInterrupt(DT), encoder_isr, FALLING);

    last_button = digitalRead(button);
}

int32_t input_take_encoder_delta()
{
    // Chan ngat trong luc doc-va-xoa: neu ISR chay xen giua, nac do se bi mat.
    noInterrupts();
    int32_t d = encoder_delta;
    encoder_delta = 0;
    interrupts();
    return d;
}

// nhan giu nut > 2s => su kien LONG (chuyen tab); nha nut som hon => su kien SHORT (chon/xac nhan).
// Co button_long_fired dam bao MOI LAN NHAN chi sinh ra DUNG 1 su kien - neu khong, mot lan giu
// nut co the vua doi tab vua chon/xac nhan, khien man hinh ve chong len nhau khi chuyen tab.
ButtonEvent input_take_button_event()
{
    now_button = digitalRead(button);
    ButtonEvent event = BUTTON_NONE;

    if (last_button == 1 && now_button == 0)
    {
        // canh nhan xuong: bat dau dem thoi gian giu nut
        timer2 = (int32_t)millis();
        button_long_fired = false;
    }

    if (now_button == 0 && !button_long_fired && (int32_t)millis() - timer2 > 2000)
    {
        // giu du 2s: ban su kien LONG ngay, danh dau da xu ly de khi tha nut khong sinh them SHORT
        event = BUTTON_LONG_PRESS;
        button_long_fired = true;
    }

    if (last_button == 0 && now_button == 1 && !button_long_fired)
    {
        // canh tha nut ma chua bi xu ly bang giu lau -> day la 1 lan nhan ngan
        event = BUTTON_SHORT_PRESS;
    }

    last_button = now_button;
    return event;
}

/*==================== Tang DIEU PHOI: ap dung vao state UI ====================*/
// Port truc tiep tu key_value_tang/key_value_giam/read_button cua ban Pico (src/PLG_input.cpp),
// chi khac o cho: ban Pico goi 2 ham tang/giam ngay trong ISR (theo tung canh xung DT); o day
// ISR chi dem nac vao encoder_delta, roi process_input() (goi tu loop(), khong phai ISR) ap
// dung tang/giam - cho phep ISR gon toi muc toi thieu.

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
        else if (show_font)
            clock_font_index++;
        else if (show_font_size)
            clock_size_index++;
        else if (show_language)
            language_index++;
        else if (show_clock_style)
            clock_style_index++;
        else if (show_connection)
            connection_index++;
        else if (show_ble_status)
            ; // man hinh chi hien trang thai, khong co gi de chinh - nuot nac quay de khong am
              // tham doi muc menu SETTING dang nam duoi
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
        else if (show_font)
            clock_font_index--;
        else if (show_font_size)
            clock_size_index--;
        else if (show_language)
            language_index--;
        else if (show_clock_style)
            clock_style_index--;
        else if (show_connection)
            connection_index--;
        else if (show_ble_status)
            ; // xem giai thich o key_value_tang()
        else
            funtion_mode--;
        break;
    default:
        break;
    }
}

static void apply_encoder_delta()
{
    int32_t delta = input_take_encoder_delta();
    // Ap dung tung nac mot (giong het viec ISR ban Pico goi tang/giam moi canh xung), khong
    // cong don thang vao gia tri dich: cac bien nhu funtion_mode/color_index khong tu boc vong
    // trong ham nay (viec boc vong/kep trong khoang hop le do MONITOR_* ve man hinh dam nhiem
    // moi lan ve), nen phai giu dung so lan goi ham tang/giam nhu tren phan cung that.
    while (delta > 0)
    {
        key_value_tang();
        delta--;
    }
    while (delta < 0)
    {
        key_value_giam();
        delta++;
    }
}

static void handle_button_event(ButtonEvent event)
{
    if (event == BUTTON_LONG_PRESS)
    {
        // giu du 2s: chuyen tab (HOME <-> SETTING)
        display_number++;
        return;
    }
    if (event != BUTTON_SHORT_PRESS)
        return;

    // nha nut ma chua bi xu ly bang giu lau -> nhan ngan: chon/xac nhan
    if (desktop_state == DESKTOP_HOME && show_clock)
    {
        Serial.println("PLG_>>>> HOME: exit CLOCK, back to TASK MANAGER");
        show_clock = false;
        last_show_clock = false; // dong bo lai co, de lan sau bat CLOCK duoc xoa man hinh dung cach
        reset_clock_cpu_ram_cache(); // xoa vung CPU/RAM cua CLOCK, tranh de sot chu cu sang TASK MANAGER
        // buoc MONITOR_TASKMANAGER ve lai toan bo (xoa sach noi dung CLOCK cu, ve lai khung chart)
        last_desktop = !desktop;
    }
    else if (desktop_state == DESKTOP_HOME)
    {
        Serial.println("PLG_>>>> HOME: enter CLOCK");
        show_clock = true;
    }
    else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_TASK)
    {
        Serial.println("PLG_>>>> back to TASK MANAGER (home)");
        display_number = 0; // man hinh chinh (HOME) chinh la Task Manager
    }
    else if (desktop_state == DESKTOP_SETING && show_clock)
    {
        Serial.println("PLG_>>>> exit CLOCK, back to SETTING menu");
        show_clock = false;
        last_show_clock = false; // dong bo lai co, de lan sau vao CLOCK duoc xoa man hinh dung cach
        reset_clock_cpu_ram_cache(); // xoa vung CPU/RAM cua CLOCK, tranh de sot chu cu sang menu SETTING
        menu_needs_full_draw = true;
    }
    else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_ID)
    {
        Serial.println("PLG_>>>> enter CLOCK");
        show_clock = true;
    }
    else if (desktop_state == DESKTOP_SETING && show_color)
    {
        Serial.printf("PLG_>>>> apply COLOR #%d, back to SETTING menu\n", color_index);
        UI_ACCENT = UI_ACCENT_PRESETS[color_index];
        save_settings_to_flash((uint8_t)color_index, (uint8_t)active_clock_font, (uint8_t)active_clock_size, (uint8_t)ui_language, (uint8_t)active_clock_style); // luu lai, mat nguon van giu mau da chon
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
        Serial.println("PLG_>>>> enter COLOR");
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
    else if (desktop_state == DESKTOP_SETING && show_font_size)
    {
        Serial.printf("PLG_>>>> apply CLOCK FONT #%d size #%d, back to SETTING menu\n", active_clock_font, clock_size_index);
        active_clock_size = get_clock_size_value(active_clock_font, clock_size_index);
        save_settings_to_flash((uint8_t)color_index, (uint8_t)active_clock_font, (uint8_t)active_clock_size, (uint8_t)ui_language, (uint8_t)active_clock_style); // luu lai, mat nguon van giu ho/co chu da chon
        show_font_size = false;
        last_show_font_size = false; // dong bo lai co, de lan sau vao man hinh chon size duoc xoa man hinh dung cach
        menu_needs_full_draw = true;
    }
    else if (desktop_state == DESKTOP_SETING && show_font)
    {
        Serial.printf("PLG_>>>> pick CLOCK FONT FAMILY #%d, choose size next\n", clock_font_index);
        active_clock_font = clock_font_index;
        // khoi tao vi tri duyet buoc 2 tu size dang dung, kep an toan trong khoang hop le cua ho chu vua chon
        clock_size_index = clock_size_value_to_index(active_clock_font, active_clock_size);
        show_font = false;
        last_show_font = false; // dong bo lai co, de lan sau vao man hinh chon ho chu duoc xoa man hinh dung cach
        show_font_size = true;  // chuyen sang buoc 2: chon co chu
    }
    else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_FONT)
    {
        Serial.println("PLG_>>>> enter FONT (step 1: choose family)");
        clock_font_index = active_clock_font; // bat dau duyet tu ho chu dang dung
        show_font = true;
    }
    else if (desktop_state == DESKTOP_SETING && show_language)
    {
        Serial.printf("PLG_>>>> apply LANGUAGE #%d, back to SETTING menu\n", language_index);
        ui_language = language_index;
        save_settings_to_flash((uint8_t)color_index, (uint8_t)active_clock_font, (uint8_t)active_clock_size, (uint8_t)ui_language, (uint8_t)active_clock_style); // luu lai, mat nguon van giu ngon ngu da chon
        show_language = false;
        last_show_language = false; // dong bo lai co, de lan sau vao LANGUAGE duoc xoa man hinh dung cach
        // buoc ve lai toan bo menu SETTING de cap nhat nhan theo ngon ngu moi
        menu_needs_full_draw = true;
    }
    else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_LANGUAGE)
    {
        Serial.println("PLG_>>>> enter LANGUAGE");
        language_index = ui_language; // bat dau duyet tu ngon ngu dang dung
        show_language = true;
    }
    else if (desktop_state == DESKTOP_SETING && show_clock_style)
    {
        Serial.printf("PLG_>>>> apply CLOCK STYLE #%d, back to SETTING menu\n", clock_style_index);
        active_clock_style = clock_style_index;
        save_settings_to_flash((uint8_t)color_index, (uint8_t)active_clock_font, (uint8_t)active_clock_size, (uint8_t)ui_language, (uint8_t)active_clock_style); // luu lai, mat nguon van giu kieu dong ho da chon
        show_clock_style = false;
        last_show_clock_style = false; // dong bo lai co, de lan sau vao CLOCK STYLE duoc xoa man hinh dung cach
        menu_needs_full_draw = true;
    }
    else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_CLOCK_STYLE)
    {
        Serial.println("PLG_>>>> enter CLOCK STYLE");
        clock_style_index = active_clock_style; // bat dau duyet tu kieu dang dung
        show_clock_style = true;
    }
    else if (desktop_state == DESKTOP_SETING && show_ble_status)
    {
        Serial.println("PLG_>>>> exit BLE STATUS, back to SETTING menu");
        show_ble_status = false;
        last_show_ble_status = false; // dong bo lai co, de lan sau vao lai duoc xoa man hinh dung cach
        menu_needs_full_draw = true;
    }
    else if (desktop_state == DESKTOP_SETING && show_connection)
    {
        Serial.printf("PLG_>>>> apply CONNECTION #%d (%s)\n", connection_index, TRANSPORT_MODE_NAMES[connection_index]);
        transport_begin((TransportMode)connection_index); // ap dung ngay (chuyen sang mode moi), cap nhat active_connection_mode
        transport_save_mode((TransportMode)connection_index); // luu vao NVS rieng, song sot qua mat nguon
        show_connection = false;
        last_show_connection = false; // dong bo lai co, de lan sau vao CONNECTION duoc xoa man hinh dung cach

        if (connection_index == TRANSPORT_BLE)
        {
            // BLE can hien ten thiet bi de nguoi dung biet ghep noi voi cai nao tu PC, nen vao
            // thang man hinh trang thai thay vi quay ve menu nhu USB.
            show_ble_status = true;
            last_show_ble_status = false; // buoc MONITOR_BLE_STATUS xoa man hinh + ve lai tu dau
        }
        else if (connection_index == TRANSPORT_WIFI)
        {
            // WiFi khong the dung duoc ngay sau khi chon: phai chon mang + nhap mat khau da.
            show_wifi_ui = true;
            wifi_ui_enter();
        }
        else
        {
            menu_needs_full_draw = true;
        }
    }
    else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_CONNECTION)
    {
        Serial.println("PLG_>>>> enter CONNECTION");
        connection_index = active_connection_mode; // bat dau duyet tu giao thuc dang dung
        show_connection = true;
    }
}

void process_input()
{
    if (show_wifi_ui)
    {
        // Wizard WiFi chiem toan quyen dieu khien khi dang mo, ke ca nhan GIU - trong wizard
        // thao tac do la "xoa lui 1 ky tu" chu khong phai chuyen tab HOME/SETTING.
        // Xem giai thich quy uoc rieng o dau PLG_wifi_ui.h.
        wifi_ui_on_rotate(input_take_encoder_delta());
        switch (input_take_button_event())
        {
        case BUTTON_SHORT_PRESS:
            wifi_ui_on_short_press();
            break;
        case BUTTON_LONG_PRESS:
            wifi_ui_on_long_press();
            break;
        default:
            break;
        }
        return;
    }

    apply_encoder_delta();
    handle_button_event(input_take_button_event());
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
            show_font = false;           // luon vao menu truoc, khong vao thang man hinh chon ho chu
            show_font_size = false;      // luon vao menu truoc, khong vao thang man hinh chon co chu
            show_language = false;       // luon vao menu truoc, khong vao thang man hinh chon ngon ngu
            show_clock_style = false;    // luon vao menu truoc, khong vao thang man hinh chon kieu dong ho
            show_connection = false;     // luon vao menu truoc, khong vao thang man hinh chon giao thuc ket noi
            show_ble_status = false;     // luon vao menu truoc, khong vao thang man hinh trang thai BLE
            show_wifi_ui = false;        // luon vao menu truoc, khong vao thang wizard WiFi

            // xoa gio Task Manager o goc tren-phai khi roi HOME, tranh no bi dinh lai
            // (khong duoc xoa) tren cac man hinh khac nhu SETTING/CLOCK
            myTFT.TFTfillRect(188, 4, 65, 17, UI_BG);
            reset_taskmanager_clock_cache();
        }
    }
    if (display_number > 1)
    {
        display_number = 0;
    }
}
