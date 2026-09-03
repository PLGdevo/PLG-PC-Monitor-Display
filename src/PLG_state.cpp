#include "PLG_state.h"

int32_t timer1 = 0, timer2 = 0, timer3 = 0;
bool flat1 = false;

int8_t battery1 = 0;
int8_t battery2 = 0;
int8_t last_battery1 = 0, last_battery2 = 0;

int8_t funtion_mode = 0;
int8_t last_funtion_mode = -1;

int8_t player = 2;
int8_t last_player = 0;

bool desktop = 0;
bool last_desktop = 1;

bool status = 0;
bool last_status = 1;

bool now_button = 0;
bool last_button = 0;
bool button_long_fired = false;

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

int8_t last_en = 0;
int8_t now_en = 0;
int8_t step = 1;

int8_t display_number = 0;
int8_t last_display_number = 0;
MONITOR_DESTOP desktop_state = DESKTOP_HOME;

int8_t color_index = 0;
int8_t last_color_index = -1;
bool show_color = false;
bool last_show_color = false;

int8_t clock_font_index = 0;
int8_t last_clock_font_index = -1;
int8_t active_clock_font = 2; // 2 = SEVEN_SEG (kieu dong ho dien tu), mac dinh
bool show_font = false;
bool last_show_font = false;

int8_t clock_size_index = 0;
int8_t last_clock_size_index = -1;
int8_t active_clock_size = 6; // co chu lon nhat cua SEVEN_SEG, mac dinh
bool show_font_size = false;
bool last_show_font_size = false;

bool menu_needs_full_draw = true;

int8_t ui_language = UI_LANG_VI;
int8_t language_index = 0;
int8_t last_language_index = -1;
bool show_language = false;
bool last_show_language = false;

int8_t chart_cpu[CHART_SAMPLES];
int8_t chart_ram[CHART_SAMPLES];
int8_t chart_gpu[CHART_SAMPLES];
int8_t chart_gpumem[CHART_SAMPLES];
int8_t chart_wifi[CHART_SAMPLES];
bool taskmanager_dirty = false;
char serial_line_buf[112];
uint8_t serial_line_len = 0;

char current_time_str[9] = "--:--:--";
char last_time_str[9] = "";
char current_date_str[11] = "--/--/----";
char last_date_str[11] = "";
bool clock_dirty = false;
bool show_clock = false;
bool last_show_clock = false;
