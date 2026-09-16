#pragma once
#include <stdint.h>
// Khai bao chan GPIO va cau hinh TFT/encoder cua board ESP32 DevKit (ESP32-D0WD-V3).
//
// Cac chan PHAI tranh tren ESP32 co dien:
//   - GPIO6..11: noi thang vao chip flash noi bo. Dung lam gi khac cung lam board khong boot
//     duoc. Day la khac biet lon nhat so voi ESP32-S3 (o S3 cac chan nay dung tu do duoc) -
//     ban thiet ke ban dau nham toi S3 nen dat TFT vao dung vung nay, phai chon lai het.
//   - GPIO34..39: chi vao (input-only), khong co dien tro keo len noi bo -> khong dung cho
//     encoder (can INPUT_PULLUP) va khong dung lam ngo ra duoc.
//   - GPIO0, 12, 15: chan "strapping" quyet dinh che do boot. Rieng GPIO12 neu bi keo len luc
//     khoi dong se dat sai dien ap flash va lam board khong boot.
//   - GPIO1, 3: UART0 (TX/RX) dang dung cho nap firmware + log Serial qua chip CH340.

/*------------------- Chon kieu dieu khien -------------------*/
// 1 = 3 nut bam roi (dang dung, vi chua co encoder trong tay)
// 0 = encoder xoay (code van con nguyen trong PLG_input.cpp, doi so nay la chay lai duoc)
#define PLG_INPUT_USE_BUTTONS 1

/*--- Kieu 1: 3 nut bam roi ---*/
// GPIO34/36/39 la chan CHI VAO (input-only) va KHONG CO dien tro keo noi bo - pinMode(...,
// INPUT_PULLUP/INPUT_PULLDOWN) tren may chan nay khong co tac dung gi. Bat buoc phai co dien
// tro keo ben ngoai, neu khong chan se tha noi va doc ra gia tri ngau nhien -> man hinh tu
// nhay lung tung ngay ca khi khong bam.
//
// Dang dung nut TICH CUC MUC CAO (xem BTN_ACTIVE_LEVEL) nen dien tro phai keo XUONG:
// 10k tu moi chan xuong GND, nut bam noi chan len 3.3V.
#define BTN_UP 36     // tuong duong xoay encoder THUAN chieu kim dong ho (+1 nac)
#define BTN_DOWN 39   // tuong duong xoay encoder NGUOC chieu kim dong ho (-1 nac)
#define BTN_SELECT 34 // tuong duong nut nhan tren encoder (nhan ngan / giu 2s)

// Muc dien ap khi nut DANG BI NHAN.
//   HIGH = nut noi chan len 3.3V, dien tro 10k keo xuong GND (dang dung).
//   LOW  = nut noi chan xuong GND, dien tro 10k keo len 3.3V (cach dau con lai).
#define BTN_ACTIVE_LEVEL HIGH

/*--- Kieu 0: encoder xoay (hien khong dung, giu lai de quay ve) ---*/
#define CLK 32    // Encoder EN_CLK
#define DT 33     // Encoder EN_DT
#define button 25 // Encoder EN_SW (nut nhan tren encoder)

// Khac ban Pico: khong con chan "button_1" de vao che do nap firmware. DevKit da co san nut
// BOOT + EN(RESET) tren board nen khong can nut roi (GPIO26 cu duoc bo).

#define PIN_LIGHT_BOARD 2 // den bao mau xanh co san tren hau het board ESP32 DevKit

// Nhap nhay den bao 1Hz de biet firmware con chay (khong treo). Tat di cho do choi mat; doi
// lai 1 la co ngay dau hieu "con song" khi di tim loi treo may.
#define PLG_HEARTBEAT_LED 0

// Den nen man hinh (BLK) KHONG dau vao GPIO nao: tren cach dau day dang dung, no noi thang
// 3.3V nen luon sang. -1 = khong cau hinh chan nao (xem setup_display trong PLG_display.cpp).
// Muon chinh do sang bang PWM sau nay thi doi sang 1 chan con trong (vd 26) va noi lai day BLK
// - KHONG dung lai GPIO4, chan do dang la RST cua man hinh.
#define PIN_TFT_BLK (-1)

// Chan SPI toi man hinh TFT (khoi tao gia tri trong PLG_pins.cpp).
// Man hinh chi nhan lenh, khong tra du lieu ve, nen chan MISO (GPIO19 cua nhom VSPI) bo trong.
extern uint8_t SDIN_TFT; // MOSI || SPI TX
extern uint8_t SCLK_TFT; // SCL
extern uint8_t DC_TFT;
extern uint8_t CS_TFT;
extern uint8_t RST_TFT;
