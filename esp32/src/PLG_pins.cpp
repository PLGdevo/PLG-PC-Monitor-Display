#include "PLG_pins.h"

// Nhom chan VSPI mac dinh cua ESP32 co dien (MOSI 23 / SCLK 18 / CS 5) theo dung cach dau day
// dang dung tren board; DC va RST lay o 2 chan tro trong. CS dat o GPIO5 la co chu y: GPIO5 la
// chan strapping phai o muc CAO luc khoi dong, ma chan CS cua SPI von da nghi o muc cao -
// khong xung dot.
uint8_t SDIN_TFT = 23;
uint8_t SCLK_TFT = 18;
uint8_t DC_TFT = 27;
uint8_t CS_TFT = 5;
uint8_t RST_TFT = 4;
