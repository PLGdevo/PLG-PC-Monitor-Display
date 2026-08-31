# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# version code

include(pico_sdk_import.cmake)
set(PICO_BOARD pico CACHE STRING "Board type")
project(PLG_TFT_LCD C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
#build file c
add_executable(PLG_TFT_LCD
    PLG_TFT_LCD.cpp

    src/PLG_pins.cpp
    src/PLG_theme.cpp
    src/PLG_state.cpp
    src/PLG_display.cpp
    src/PLG_flash_settings.cpp
    src/PLG_serial_link.cpp
    src/PLG_charts.cpp
    src/PLG_screens.cpp
    src/PLG_input.cpp

    PLG_lib/ST7789_TFT_PICO-main/src/st7789/ST7789_TFT.cpp
    PLG_lib/ST7789_TFT_PICO-main/src/st7789/ST7789_TFT_graphics.cpp
    PLG_lib/ST7789_TFT_PICO-main/src/st7789/ST7789_TFT_Print.cpp
    PLG_lib/ST7789_TFT_PICO-main/src/st7789/ST7789_TFT_Font.cpp
)

target_include_directories(PLG_TFT_LCD PRIVATE
    .
    include
    PLG_lib/ST7789_TFT_PICO-main/include
)
# link lib computer
target_link_libraries(PLG_TFT_LCD
    pico_stdlib
    pico_multicore
    hardware_spi
    hardware_gpio
   
    
)
# debug terminal
pico_enable_stdio_usb(PLG_TFT_LCD 1)
pico_enable_stdio_uart(PLG_TFT_LCD 0)

pico_add_extra_outputs(PLG_TFT_LCD)
