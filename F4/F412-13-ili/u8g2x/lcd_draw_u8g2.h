#ifndef LCD_DRAW_U8G2_H
#define LCD_DRAW_U8G2_H

#include <stdint.h>

// Сообщаем линкеру лаконичное имя массива шрифта
extern const uint8_t u8g2_font_10x20[];

void lcd_draw_u8g2_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);

#endif // LCD_DRAW_U8G2_H