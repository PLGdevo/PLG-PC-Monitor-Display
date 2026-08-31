#pragma once
#include <stdint.h>
// Khai bao chan GPIO va cau hinh TFT/encoder/nut nhan cua board.

#define CLK 23      // Encoder EN_CLK
#define DT 20       // Encoder EN_DT
#define button 21   // Encoder EN_SW (nut nhan tren encoder)
#define button_1 26 // Nut roi: vao che do BOOTSEL de nap firmware

#define SPI_PORT spi0
#define PIN_LIGHT_BOARD 17

// Chan SPI toi man hinh TFT (khoi tao gia tri trong PLG_pins.cpp)
extern uint8_t SDIN_TFT; // MOSI || SPI TX
extern uint8_t SCLK_TFT; // SCL
extern uint8_t DC_TFT;
extern uint8_t CS_TFT;
extern uint8_t RST_TFT;
