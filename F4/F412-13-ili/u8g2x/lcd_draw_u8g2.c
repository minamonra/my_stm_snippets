#include "lcd_draw_u8g2.h"
#include "ili9488x.h"
#include "pindefs.h"
#include "lcd_io.h"
#include "common.h"
#include "u8g2.h"

// ЯВНЫЕ ПРОТОТИПЫ ИЗ ОФИЦИАЛЬНОГО ЯДРА U8G2
const uint8_t *u8g2_font_get_glyph_data(u8g2_t *u8g2, uint16_t encoding);
int16_t u8g2_font_decode_glyph(u8g2_t *u8g2, const uint8_t *glyph_data);

// Глобальная структура для официального ядра
u8g2_t u8g2_display;

// Быстрый макрос вывода цвета в SPI1
#define LCD_WRITE_RGB888(r, g, b) do { \
    while (!(SPI1->SR & SPI_SR_TXE)) { } \
    *(volatile uint8_t *)&SPI1->DR = (r); \
    while (!(SPI1->SR & SPI_SR_TXE)) { } \
    *(volatile uint8_t *)&SPI1->DR = (g); \
    while (!(SPI1->SR & SPI_SR_TXE)) { } \
    *(volatile uint8_t *)&SPI1->DR = (b); \
} while(0)

// Глобальные переменные для предрасчитанных байт цветов RGB888
static uint8_t r_txt, g_txt, b_txt;
static uint8_t r_bg,  g_bg,  b_bg;

// Функция отрисовки линий (РАБОЧАЯ - без инверсии)
void u8g2_DrawHVLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir) {
    if (len == 0) return;

    uint16_t w_win = (dir == 0) ? len : 1;
    uint16_t h_win = (dir == 1) ? len : 1;

    // Без инверсии - u8g2 сам дает правильные координаты
    uint16_t corrected_y = y;
    
    // Проверка границ
    if (x >= 480 || corrected_y >= 320) return;
    if (x + w_win > 480) w_win = 480 - x;
    if (corrected_y + h_win > 320) h_win = 320 - corrected_y;

    ili9488_SetDisplayWindow(x, corrected_y, w_win, h_win);
    lcd_send_cmd(0x2C);
    ILI9486_DC_SET;
    ILI9486_CS_CLR;

    uint8_t is_glyph_pixel = (u8g2_GetDrawColor(u8g2) == 0);

    for (uint16_t i = 0; i < len; i++) {
        if (is_glyph_pixel) {
            LCD_WRITE_RGB888(r_txt, g_txt, b_txt);
        } else {
            LCD_WRITE_RGB888(r_bg, g_bg, b_bg);
        }
    }

    while (SPI1->SR & SPI_SR_BSY) { }
    ILI9486_CS_SET;
}

// Заглушка для функции пересечений
uint8_t u8g2_IsIntersection(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t x1, u8g2_uint_t y1) {
    (void)u8g2; (void)x0; (void)y0; (void)x1; (void)y1;
    return 1; 
}

// СТАБЫ (ЗАГЛУШКИ) ДЛЯ ЛИНКЕРА
void u8x8_utf8_init(u8x8_t *u8x8) { (void)u8x8; }
uint16_t u8x8_utf8_next(u8x8_t *u8x8, uint8_t b) { (void)u8x8; (void)b; return 0; }
uint16_t u8x8_ascii_next(u8x8_t *u8x8, uint8_t b) { (void)u8x8; (void)b; return 0; }
uint8_t u8g2_GetKerning(u8g2_t *u8g2, u8g2_kerning_t *kerning, uint16_t e1, uint16_t e2) { 
    (void)u8g2; (void)kerning; (void)e1; (void)e2;
    return 0; 
}
uint8_t u8g2_GetKerningByTable(u8g2_t *u8g2, const uint16_t *kt, uint16_t e1, uint16_t e2) { 
    (void)u8g2; (void)kt; (void)e1; (void)e2;
    return 0; 
}

// Полноценная функция вывода строки
void lcd_draw_u8g2_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg) {
    if (!str) return;

    r_txt = (uint8_t)((color & 0xF800) >> 8);
    g_txt = (uint8_t)((color & 0x07E0) >> 3);
    b_txt = (uint8_t)((color & 0x001F) << 3);

    r_bg  = (uint8_t)((bg & 0xF800) >> 8);
    g_bg  = (uint8_t)((bg & 0x07E0) >> 3);
    b_bg  = (uint8_t)((bg & 0x001F) << 3);

    u8g2_SetFont(&u8g2_display, u8g2_font_10x20);

    uint16_t cursor_x = x;
    const char *ptr = str;
    uint8_t c;

    while ((c = decode_utf8(&ptr))) {
        uint16_t u8g2_code = c;

        // Конвертация кириллицы из CP1251 в Unicode
        if (u8g2_code >= 144 && u8g2_code <= 191) {
            u8g2_code = u8g2_code - 144 + 0x0410; // А-Я
        } else if (u8g2_code >= 128 && u8g2_code <= 143) {
            u8g2_code = u8g2_code - 128 + 0x0440; // а-п
        } else if (u8g2_code == 181) {
            u8g2_code = 0x0451; // ё
        }

        const uint8_t *glyph_data = u8g2_font_get_glyph_data(&u8g2_display, u8g2_code);
        
        if (!glyph_data) {
            ili9488_FillRect(cursor_x, y - 16, 10, 20, bg);
            cursor_x += 10;
            continue;
        }

        ili9488_FillRect(cursor_x, y - 16, 10, 20, bg);

        u8g2_display.font_decode.target_x = cursor_x;
        u8g2_display.font_decode.target_y = y;

        u8g2_font_decode_glyph(&u8g2_display, glyph_data);

        cursor_x += 10; 
        if (cursor_x >= 480) break;
    }
}