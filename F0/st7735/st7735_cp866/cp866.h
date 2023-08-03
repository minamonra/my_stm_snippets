#ifndef __CP866_H__
#define __CP866_H__
#include "st7735.h"

void cp866_print_num(uint32_t num, uint8_t x, uint8_t y, uint16_t fg, uint16_t bg);
void cp866_print_str(char *str, uint8_t x, uint8_t y, uint16_t fg_color, uint16_t bg_color);
void cp866_char_to_buf(uint8_t x_pos, uint8_t ch, uint16_t fg_color, uint16_t bg_color);

#endif // __CP866_H__