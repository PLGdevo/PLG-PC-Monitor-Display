#pragma once
#include <stdint.h>
// Khai bao chan GPIO va cau hinh TFT/encoder cua board ESP32-S3-DevKitC-1.
//
// Toan bo GPIO duoc chon lai so voi ban Pico, tranh cac chan "strapping" cua ESP32-S3
// (GPIO0/3/45/46) va cap chan USB native (GPIO19/20) de khong xung dot luc boot/flash.
// Bang chan day du: xem muc "So do chan (pinout)" trong README_ESP32_MIGRATION.md.

#define CLK 4    // Encoder EN_CLK
#define DT 5     // Encoder EN_DT
#define button 6 // Encoder EN_SW (nut nhan tren encoder)

// Khac ban Pico: khong con chan "button_1" de vao che do nap firmware. DevKitC-1 da co san
// nut BOOT + RESET tren board nen khong can nut roi (GPIO26 cu duoc bo).

#define PIN_LIGHT_BOARD 2 // den bao trang thai tren board
#define PIN_TFT_BLK 7     // den nen man hinh; sau nay co the bam PWM de chinh do sang

// Chan SPI toi man hinh TFT (khoi tao gia tri trong PLG_pins.cpp)
extern uint8_t SDIN_TFT; // MOSI || SPI TX
extern uint8_t SCLK_TFT; // SCL
extern uint8_t DC_TFT;
extern uint8_t CS_TFT;
extern uint8_t RST_TFT;
