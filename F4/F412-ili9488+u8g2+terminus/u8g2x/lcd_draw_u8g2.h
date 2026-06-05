#ifndef LCD_DRAW_U8G2_H
#define LCD_DRAW_U8G2_H

#include <stdint.h>

// Объявление шрифтов (все с кириллицей, кроме fub20_tf)
extern const uint8_t u8g2_font_10x20_t_cyrillic[];
extern const uint8_t u8g2_font_fub20_tf[];          // БЕЗ кириллицы!
extern const uint8_t u8g2_font_inr24_t_cyrillic[];
extern const uint8_t u8g2_font_inr27_t_cyrillic[];
extern const uint8_t u8g2_font_unifont_t_cyrillic[];
extern const uint8_t u8g2_font_terminus_28b_cyr[];
void lcd_u8g2_set_font(const uint8_t *font);
void lcd_draw_u8g2_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);
uint8_t lcd_u8g2_get_font_height(void);
uint8_t lcd_u8g2_get_font_ascent(void);
uint8_t lcd_u8g2_get_font_descent(void);

#endif