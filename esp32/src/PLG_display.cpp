#include "PLG_display.h"
#include <Arduino.h>
#include <SPI.h>
#include "PLG_pins.h"
#include "PLG_state.h"

ST7789_TFT myTFT;

static bool text_has_lower(const char *s)
{
    for (; *s; s++)
        if (*s >= 'a' && *s <= 'z')
            return true;
    return false;
}

void ui_drawText(int16_t x, int16_t y, const char *text, uint16_t color, uint16_t bg, uint8_t size)
{
    int8_t style = active_clock_font;
    if (style == 2) // Seven_Seg: chi ve dung so/dau, khong dung cho van ban chu binh thuong
        style = 0;
    else if ((style == 1 || style == 3) && text_has_lower(text)) // Thick/Wide: khong co chu thuong
        style = 0;

    switch (style)
    {
    case 1:
        myTFT.TFTFontNum(myTFT.TFTFont_Thick);
        break;
    case 3:
        myTFT.TFTFontNum(myTFT.TFTFont_Wide);
        break;
    case 4:
        myTFT.TFTFontNum(myTFT.TFTFont_HomeSpun);
        break;
    default:
        myTFT.TFTFontNum(myTFT.TFTFont_Default);
        break;
    }
    myTFT.TFTdrawText((uint16_t)x, (uint16_t)y, (char *)text, color, bg, size);
    myTFT.TFTFontNum(myTFT.TFTFont_Default);
}

void setup_display()
{
    // Den nen man hinh: giai doan nay chi bat cung HIGH. Khi lam tinh nang chinh do sang
    // se doi sang bam PWM (LEDC) tren chinh chan nay.
    pinMode(PIN_TFT_BLK, OUTPUT);
    digitalWrite(PIN_TFT_BLK, HIGH);

    // SPI phan cung. Khac ban Pico: o do truyen 125000 kHz de "cham tran" phan cung RP2040 va
    // duoc pico-sdk tu kep xuong; o day KHONG lam vay duoc vi _speedSPIKHz la uint16_t (toi da
    // 65535) - truyen lon hon se bi tran va cho ra toc do sai. 40 MHz la muc chay on dinh voi
    // day noi thong thuong; co the nang dan toi 80 MHz neu day ngan/chat luong tot.
    const uint32_t TFT_SCLK_FREQ_KHZ = 40000;
    myTFT.TFTInitSPIType(TFT_SCLK_FREQ_KHZ, &SPI);

    myTFT.TFTSetupGPIO(RST_TFT, DC_TFT, CS_TFT, SCLK_TFT, SDIN_TFT);

    uint16_t OFFSET_COL = 0;   // chinh lai neu man hinh cu the bi lech vien
    uint16_t OFFSET_ROW = 0;
    uint16_t TFT_WIDTH = 240;  // Screen width in pixels
    uint16_t TFT_HEIGHT = 320; // Screen height in pixels
    myTFT.TFTInitScreenSize(OFFSET_COL, OFFSET_ROW, TFT_WIDTH, TFT_HEIGHT);

    myTFT.TFTST7789Initialize();
    // Ban Pico xoay man hinh trong setup() cua main; o day gop luon vao day de moi thu lien
    // quan den cau hinh man hinh nam chung mot cho.
    myTFT.TFTsetRotation(myTFT.TFT_Degrees_270);
    myTFT.TFTfillScreen(ST7789_BLACK);
    myTFT.TFTFontNum(myTFT.TFTFont_Default);
    // Thu vien mac dinh BAT wrap chu (_wrap=true): chu ve vuot bien se tu nhay phan du sang DAU
    // DONG MOI thay vi bi cat - day chinh la nguyen nhan cac ky tu cuoi (vd don vi "C" cua TEMP)
    // dot ngot xuat hien o dau mot hang khac. Tat wrap: chu vuot bien se don gian bi cat o bien,
    // an toan hon nhieu so voi nhay lung tung sang vi tri khac tren man hinh.
    myTFT.TFTsetTextWrap(false);
}
