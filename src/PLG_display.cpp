#include "PLG_display.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "PLG_pins.h"
#include "PLG_input.h"
#include "PLG_state.h"

ST7789_TFT myTFT;

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
    gpio_set_dir(DT, GPIO_IN);
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
    {
        // Spi freq in KiloHertz (10000 = 10Mhz). 100000 = 100Mhz: vuot qua muc RP2040 dat duoc
        // (toi da ~62.5Mhz = clk_peri/2 o xung he thong mac dinh 125Mhz), nen pico-sdk se tu
        // dong gioi han (clamp) xuong muc phan cung cho phep -> day da la toc do SPI toi da co the dat.
        uint32_t TFT_SCLK_FREQ = 125000; // yeu cau cao hon xung he thong de dam bao luon cham tran gioi han phan cung
        myTFT.TFTInitSPIType(TFT_SCLK_FREQ, spi0);
    }
    else
    {
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
