#include "lcd_draw_u8g2.h"
#include "ili9488x.h"
#include "pindefs.h"
#include "lcd_io.h"
#include "common.h"
#include "u8g2.h"

#define LCD_WRITE_RGB888(r, g, b) do { \
    while (!(SPI1->SR & SPI_SR_TXE)) { } \
    *(volatile uint8_t *)&SPI1->DR = (r); \
    while (!(SPI1->SR & SPI_SR_TXE)) { } \
    *(volatile uint8_t *)&SPI1->DR = (g); \
    while (!(SPI1->SR & SPI_SR_TXE)) { } \
    *(volatile uint8_t *)&SPI1->DR = (b); \
} while(0)

const uint8_t *u8g2_font_get_glyph_data(u8g2_t *u8g2, uint16_t encoding);
int16_t u8g2_font_decode_glyph(u8g2_t *u8g2, const uint8_t *glyph_data);

u8g2_t u8g2_display;
static const uint8_t *current_font = NULL;
static uint8_t r_txt, g_txt, b_txt;
static uint8_t r_bg, g_bg, b_bg;

// Кэшированные параметры шрифта
static uint8_t cached_font_height = 20;
static uint8_t cached_font_ascent = 16;
static uint8_t cached_font_descent = 4;

// ====================================================================
// ОБНОВЛЕНИЕ КЭША ПАРАМЕТРОВ ШРИФТА
// ====================================================================
static void update_font_cache(void) {
    if (!current_font) return;
    
    // Смещения в заголовке шрифта U8g2 (23 байта)
    cached_font_height = current_font[10];   // max_char_height
    cached_font_ascent = current_font[13];   // ascent_A
    cached_font_descent = current_font[14];  // descent_g
    
    if (cached_font_height == 0) cached_font_height = 20;
    if (cached_font_ascent == 0) cached_font_ascent = 16;
    if (cached_font_descent == 0) cached_font_descent = 4;
}

// ====================================================================
// ФУНКЦИИ ПОЛУЧЕНИЯ ПАРАМЕТРОВ ШРИФТА
// ====================================================================
uint8_t lcd_u8g2_get_font_height(void) {
    return cached_font_height;
}

uint8_t lcd_u8g2_get_font_ascent(void) {
    return cached_font_ascent;
}

uint8_t lcd_u8g2_get_font_descent(void) {
    return cached_font_descent;
}

// ====================================================================
// ФУНКЦИЯ ОТРИСОВКИ ЛИНИЙ
// ====================================================================
void u8g2_DrawHVLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir) {
    if (len == 0) return;

    // Оставляем логику, которая у вас работала: 0 — это пиксель буквы
    if (u8g2_GetDrawColor(u8g2) != 0) {
        return; // Пропускаем инверсные линии фонового заполнения
    }

    uint16_t w_win = (dir == 0) ? len : 1;
    uint16_t h_win = (dir == 1) ? len : 1;
    
    if (x >= 480 || y >= 320) return;
    if (x + w_win > 480) w_win = 480 - x;
    if (y + h_win > 320) h_win = 320 - y;

    ili9488_SetDisplayWindow(x, y, w_win, h_win);
    lcd_send_cmd(0x2C);
    ILI9486_DC_SET;
    ILI9486_CS_CLR;

    // Передаем только цвет текста, ведь фон уже подготовлен
    for (uint16_t i = 0; i < len; i++) {
        LCD_WRITE_RGB888(r_txt, g_txt, b_txt);
    }

    while (SPI1->SR & SPI_SR_BSY) { }
    ILI9486_CS_SET;
}

// ====================================================================
// ЗАГЛУШКИ ДЛЯ ЛИНКЕРА
// ====================================================================
uint8_t u8g2_IsIntersection(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t x1, u8g2_uint_t y1) {
    return 1; 
}

void u8x8_utf8_init(u8x8_t *u8x8) { (void)u8x8; }
uint16_t u8x8_utf8_next(u8x8_t *u8x8, uint8_t b) { return b; }
uint16_t u8x8_ascii_next(u8x8_t *u8x8, uint8_t b) { return b; }
uint8_t u8g2_GetKerning(u8g2_t *u8g2, u8g2_kerning_t *kerning, uint16_t e1, uint16_t e2) { return 0; }
uint8_t u8g2_GetKerningByTable(u8g2_t *u8g2, const uint16_t *kt, uint16_t e1, uint16_t e2) { return 0; }

// ====================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ====================================================================
void lcd_u8g2_set_font(const uint8_t *font) {
    if (font) {
        current_font = font;
        u8g2_SetFont(&u8g2_display, current_font);
        update_font_cache();
    }
}

void lcd_draw_u8g2_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg) {
    if (!str || !current_font) return;

    r_txt = (uint8_t)((color & 0xF800) >> 8);
    g_txt = (uint8_t)((color & 0x07E0) >> 3);
    b_txt = (uint8_t)((color & 0x001F) << 3);
    r_bg = (uint8_t)((bg & 0xF800) >> 8);
    g_bg = (uint8_t)((bg & 0x07E0) >> 3);
    b_bg = (uint8_t)((bg & 0x001F) << 3);

    u8g2_SetFont(&u8g2_display, current_font);

    uint16_t cursor_x = x;
    const char *ptr = str;
    uint16_t code;

    while (*ptr) {
        code = decode_utf8(&ptr); // Используем новый честный decode_utf8
        if (code == 0) break;

        // ЗАЩИТА: Игнорируем управляющие символы Юникода (0x7F - 0x9F), 
        // которые вызывают появление артефактов вроде "PAD", "DCS" и т.д.
        if (code >= 0x007F && code <= 0x009F) {
            continue; 
        }

        const uint8_t *glyph_data = u8g2_font_get_glyph_data(&u8g2_display, code);
        
        if (!glyph_data) {
            // Если символа (например, редкого знака) нет в шрифте — просто рисуем пробел
            uint8_t space_width = cached_font_height / 4;
            if (space_width == 0) space_width = 6;
            ili9488_FillRect(cursor_x, y - cached_font_ascent, space_width, cached_font_height, bg);
            cursor_x += space_width;
            continue;
        }

        // Получаем реальную ширину символа для заливки фона
        int8_t glyph_width = u8g2_GetGlyphWidth(&u8g2_display, code);
        if (glyph_width <= 0) glyph_width = 10;

        // Чистим фон ровно под символ
        ili9488_FillRect(cursor_x, y - cached_font_ascent, glyph_width, cached_font_height, bg);

        u8g2_display.font_decode.target_x = cursor_x;
        u8g2_display.font_decode.target_y = y;

        // Рисуем сам глиф
        int16_t dx = u8g2_font_decode_glyph(&u8g2_display, glyph_data);
        
        cursor_x += (dx > 0) ? dx : glyph_width;
        if (cursor_x >= 480) break;
    }
}
