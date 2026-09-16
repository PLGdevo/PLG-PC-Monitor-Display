/*!
	@file     ST7789_TFT_Print.hpp
	@brief    Cau noi toi lop Print cua Arduino core.

	@note Ban goc cho Pico tu dinh nghia lai mot lop `Print` (chinh tac gia ghi "Port of arduino
	      built-in print class") vi pico-sdk khong co san. Tren ESP32 thi Arduino core DA co
	      `Print`, nen giu ban sao se gay loi "redefinition of class Print".

	      Bo ban sao va dung thang `Print` cua core la lua chon dung o day: `ST7789_TFT_graphics`
	      von da hien thuc `virtual size_t write(uint8_t)` - dung thu duy nhat ma `Print` cua
	      Arduino yeu cau - nen no ke thua duoc ngay, lai co them print(String)/printf() cua core.

	      Khac biet duy nhat: ban Pico co `print(const std::string&)`, `Print` cua Arduino thi
	      khong. Khong noi nao trong thu vien hay firmware goi ban do nen bo di la an toan.
*/

#pragma once

#include <Arduino.h> // keo theo Print.h cua core, va ca DEC/HEX/OCT/BIN
