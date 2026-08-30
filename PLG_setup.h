#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "st7789/ST7789_TFT.hpp"
#include "PLG_logo.hpp"

#define CLK 23      // EN_CLK
#define DT 20       // EN_DT
#define button 21   // EN_SW
#define button_1 26 // nut upload file

#define SPI_PORT spi0
uint8_t SDIN_TFT = 7; // Pin_MO-SI || SPI TX
uint8_t SCLK_TFT = 6; // SCL
uint8_t DC_TFT = 4;   // DC
uint8_t CS_TFT = 3;   // CS
uint8_t RST_TFT = 5;  // RST
ST7789_TFT myTFT;
#define PIN_LIGHT_BOARD 17
void setup(void);
void loop(void);

int32_t timer1 = 0, timer2 = 0, timer3 = 0;
bool flat1 = false;

int8_t battery1 = 0; // battery remoter (pin laptop, nhan tu monitor.py qua serial, truong BAT)
int8_t battery2 = 0; // battery may
int8_t last_battery1 = 0, last_battery2 = 0;

int8_t funtion_mode=0;
int8_t last_funtion_mode=-1;



int8_t player = 2;
int8_t last_player = 0;

bool desktop = 0;
bool last_desktop = 1;

bool status = 0;
bool last_status = 1;

bool now_button = 0;
bool last_button = 0;
bool button_long_fired = false; // true khi lan nhan hien tai da xu ly bang "giu lau" (chuyen tab)

bool now_button_1 = 0;
bool last_button_1 = 0;

bool CONNECT_STATUS = false;
bool LAST_CONNECT_STATUS = false;

bool last_clk;
int EN_CLK = 0, EN_DT = 0, EN_BT = 0;

volatile int encoder_value = 0;
volatile absolute_time_t last_time;

volatile int value = 0;
volatile uint8_t last_state = 0;

void encoder_isr(uint gpio, uint32_t events);
void read_isr(uint gpio, uint32_t events);
int8_t last_en = 0;
int8_t now_en = 0;
int8_t step = 1;

enum MONITOR_DESTOP
{
    DESKTOP_HOME,
    DESKTOP_SETING,
    DESKTOP_TASKMANAGER,
    DESKTOP_SETING_TIME,
    DESKTOP_SETING_ID,
    DESKTOP_SETING_PEOPLE
};
int8_t display_number=0;
int8_t last_display_number=0;
MONITOR_DESTOP desktop_state = DESKTOP_HOME;

// muc trong menu SETTING (funtion_mode)
#define FUNTION_MODE_PLAYER 0
#define FUNTION_MODE_FUNTION 1
#define FUNTION_MODE_MODE 2
#define FUNTION_MODE_ID 3
#define FUNTION_MODE_COLOR 4 // chon de vao man hinh chinh mau giao dien (UI_ACCENT)
#define FUNTION_MODE_TASK 5  // chon de mo man hinh Task Manager (CPU/RAM/GPU/WIFI)
#define FUNTION_MODE_COUNT 6

/*------------------- Dark UI theme (RGB565) -------------------*/
inline uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}
const uint16_t UI_BG = RGB565(8, 10, 14);        // nen chinh, den hoi ngat xanh
const uint16_t UI_PANEL = RGB565(22, 26, 32);     // nen khung/the
const uint16_t UI_PANEL_HI = RGB565(34, 40, 48);  // khung dang chon
const uint16_t UI_BORDER = RGB565(52, 60, 68);    // vien
const uint16_t UI_TEXT = RGB565(232, 236, 232);   // chu chinh
const uint16_t UI_TEXT_DIM = RGB565(128, 138, 132); // chu phu
uint16_t UI_ACCENT = RGB565(0, 194, 168);         // mau nhan dien (teal) - chinh duoc trong SETTING > COLOR
const uint16_t UI_DANGER = RGB565(235, 90, 90);   // canh bao / mat ket noi

const uint16_t UI_CPU = RGB565(70, 220, 130);   // xanh la
const uint16_t UI_RAM = RGB565(240, 185, 60);   // vang cam
const uint16_t UI_GPU = RGB565(70, 200, 235);   // xanh cyan
const uint16_t UI_WIFI = RGB565(110, 150, 255); // xanh duong

/*------------------- Bang mau nhan dien co the chon trong SETTING > COLOR -------------------*/
const uint16_t UI_ACCENT_PRESETS[] = {
    RGB565(0, 194, 168),  // Teal (mac dinh)
    RGB565(235, 90, 90),  // Red
    RGB565(240, 160, 40), // Orange
    RGB565(230, 210, 60), // Yellow
    RGB565(90, 210, 90),  // Green
    RGB565(70, 200, 235), // Cyan
    RGB565(90, 140, 240), // Blue
    RGB565(170, 110, 235),// Purple
    RGB565(232, 236, 232),// White
};
const char *UI_ACCENT_NAMES[] = {
    "TEAL", "RED", "ORANGE", "YELLOW", "GREEN", "CYAN", "BLUE", "PURPLE", "WHITE"};
#define UI_ACCENT_PRESET_COUNT (sizeof(UI_ACCENT_PRESETS) / sizeof(UI_ACCENT_PRESETS[0]))
int8_t color_index = 0;
int8_t last_color_index = -1;
bool show_color = false;      // true khi dang xem man hinh chinh mau (thay cho muc COLOR trong menu SETTING)
bool last_show_color = false;

/*------------------- Luu mau da chon vao flash (giu lai sau khi mat nguon) -------------------*/
// dung sector flash cuoi cung, ngoai vung code, de luu 1 byte color_index (co magic de kiem tra hop le)
#define SETTINGS_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_MAGIC 0xA5

void save_color_to_flash(uint8_t idx)
{
    uint8_t buf[FLASH_PAGE_SIZE];
    memset(buf, 0xFF, sizeof(buf));
    buf[0] = SETTINGS_MAGIC;
    buf[1] = idx;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(SETTINGS_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_FLASH_OFFSET, buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

// doc mau da luu tu flash (neu co) va ap dung vao color_index/UI_ACCENT; goi 1 lan trong setup()
void load_color_from_flash()
{
    const uint8_t *flash_data = (const uint8_t *)(XIP_BASE + SETTINGS_FLASH_OFFSET);
    if (flash_data[0] == SETTINGS_MAGIC && flash_data[1] < UI_ACCENT_PRESET_COUNT)
    {
        color_index = flash_data[1];
        UI_ACCENT = UI_ACCENT_PRESETS[color_index];
    }
}

/*------------------- PC Task Manager chart (nhan tu pc_monitor/monitor.py) -------------------*/
// dinh dang du lieu nhan qua USB serial: "CPU:<int>;RAM:<int>;GPU:<int>;WIFI:<int>\n"
#define CHART_SAMPLES 100 // so mau hien thi tren moi bieu do (= chieu rong bieu do, px)
// gia tri -1 = "chua co du lieu that", khac voi 0 = "co du lieu that va la 0%".
// Neu khoi tao bang 0 thi luc moi bat may / man hinh moi ve lai, phan chua co du lieu (0%)
// va phan vua nhan duoc (vd 50%) tao thanh 1 bac dung dot ngot nhu 1 duong thang dung o
// cuoi bieu do; dat -1 va bo qua doan noi lien quan -1 khi ve se tranh duoc bac nay.
int8_t chart_cpu[CHART_SAMPLES];
int8_t chart_ram[CHART_SAMPLES];
int8_t chart_gpu[CHART_SAMPLES];
int8_t chart_wifi[CHART_SAMPLES];
bool taskmanager_dirty = false; // true khi co mau moi can ve lai
char serial_line_buf[96]; // du cho dong "CPU:..;RAM:..;GPU:..;WIFI:..;TIME:..;DATE:..;BAT:.." (~70 ky tu)
uint8_t serial_line_len = 0;

/*------------------- Dong ho thoi gian (nhan tu pc_monitor/monitor.py, truong TIME:HH:MM:SS;DATE:DD/MM/YYYY) ---*/
char current_time_str[9] = "--:--:--";
char last_time_str[9] = "";
char current_date_str[11] = "--/--/----";
char last_date_str[11] = "";
bool clock_dirty = false;
bool show_clock = false;      // true khi dang xem man hinh dong ho (thay cho muc ID trong menu SETTING)
bool last_show_clock = false;



void setup_pin()
{
    stdio_init_all();
    gpio_init(PIN_LIGHT_BOARD);
    gpio_set_dir(PIN_LIGHT_BOARD, GPIO_OUT);

    gpio_init(CLK);
    gpio_init(DT);
    gpio_init(button);
    gpio_init(button_1);
    gpio_set_dir(CLK, GPIO_IN);
    // gpio_pull_up(CLK);
    gpio_set_dir(DT, GPIO_IN);
    // gpio_pull_up(DT);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button);

    gpio_set_dir(button_1, GPIO_IN);
    gpio_pull_up(button_1);
    gpio_set_irq_enabled_with_callback(
        DT,
        GPIO_IRQ_EDGE_FALL,
        true,
        &encoder_isr);
    last_time = to_us_since_boot(get_absolute_time());

    //*************** USER OPTION 0 SPI_SPEED + TYPE ***********
    bool bhardwareSPI = true; // true for hardware spi, false for software

    if (bhardwareSPI == true)
    {                                    // hw spi
        uint32_t TFT_SCLK_FREQ = 100000; // Spi freq in KiloHertz , 10000 = 10Mhz
        myTFT.TFTInitSPIType(TFT_SCLK_FREQ, spi0);
    }
    else
    {                                // sw spi
        uint16_t SWSPICommDelay = 0; // optional SW SPI GPIO delay in uS
        myTFT.TFTInitSPIType(SWSPICommDelay);
    }

    myTFT.TFTSetupGPIO(RST_TFT, DC_TFT, CS_TFT, SCLK_TFT, SDIN_TFT);
    // ****** USER OPTION 2 Screen Setup ******
    uint16_t OFFSET_COL = 0;   // 2, These offsets can be adjusted for any issues->
    uint16_t OFFSET_ROW = 0;   // 3, with screen manufacture tolerance/defects
    uint16_t TFT_WIDTH = 240;  // Screen width in pixels
    uint16_t TFT_HEIGHT = 320; // Screen height in pixels
    myTFT.TFTInitScreenSize(OFFSET_COL, OFFSET_ROW, TFT_WIDTH, TFT_HEIGHT);
    // ******************************************
    myTFT.TFTST7789Initialize();
    myTFT.TFTfillScreen(ST7789_BLACK);
    myTFT.TFTFontNum(myTFT.TFTFont_Default);

    last_button = gpio_get(button);
    last_button_1 = gpio_get(button_1);
}
int main()
{
    setup();
    while (true)
    {
        loop();
    }
}
