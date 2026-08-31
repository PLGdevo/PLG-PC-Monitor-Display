#pragma once
#include <stdint.h>
// Luu/doc mau giao dien da chon (color_index) vao flash, giu lai sau khi mat nguon.

void save_color_to_flash(uint8_t idx);
void load_color_from_flash();
