#include "lcd_font.h"
#include "ili9488x.h" 
#include "common.h"
#include "lcd_io.h"   // ДОБАВЛЕНО: для lcd_send_cmd
#include "pindefs.h"  // ДОБАВЛЕНО: для макросов ILI9486_CS_SET и т.д.

// Безопасный макрос без предупреждений компилятора про индентацию
// Быстрая отправка 3 байт (RGB888) напрямую в регистр данных SPI1
#define LCD_WRITE_RGB888(r, g, b) do { \
    while (!(SPI1->SR & SPI_SR_TXE)) { } \
    *(volatile uint8_t *)&SPI1->DR = (r); \
    while (!(SPI1->SR & SPI_SR_TXE)) { } \
    *(volatile uint8_t *)&SPI1->DR = (g); \
    while (!(SPI1->SR & SPI_SR_TXE)) { } \
    *(volatile uint8_t *)&SPI1->DR = (b); \
} while(0)


static const GFXfont *current_font = 0;

void lcd_set_font(const GFXfont *f) {
    current_font = f;
}

// Отрисовка символа с расширенным затиром фона и границами экрана (0..480)
void lcd_draw_char_old(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg) {
    if (!current_font) return;
    if (c < current_font->first || c > current_font->last) return;

    GFXglyph *glyph = &(current_font->glyph[c - current_font->first]);
    uint8_t *bitmap = current_font->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width;
    uint8_t  h  = glyph->height;

    if (w == 0 || h == 0) return; 

    int16_t char_x = x + glyph->xOffset;
    int16_t char_y = y + glyph->yOffset;

    // Ширина затирания больше xAdvance на 12 пикселей для наклонных хвостов
    uint16_t clear_w = glyph->xAdvance + 12;

    if (x < 480) {
        uint16_t fill_w = ((x + clear_w) > 480) ? (480 - x) : clear_w;
        ili9488_FillRect(x, char_y, fill_w, current_font->yAdvance, bg);
    }

    uint8_t  bits = 0;
    uint8_t  bit_count = 0;

    for (uint16_t yy = 0; yy < h; yy++) {
        uint16_t line_length = 0;
        int16_t  line_start_x = -1;
        
        for (uint16_t xx = 0; xx < w; xx++) {
            if (bit_count == 0) {
                bits = bitmap[bo++];
                bit_count = 8;
            }
            
            int16_t px = char_x + xx;

            if (bits & 0x80) {
                if (px >= 0 && px < 480) {
                    if (line_length == 0) {
                        line_start_x = px;
                    }
                    line_length++;
                } else {
                    if (line_length > 0) {
                        ili9488_FillRect(line_start_x, char_y + yy, line_length, 1, color);
                        line_length = 0;
                    }
                }
            } else {
                if (line_length > 0) {
                    ili9488_FillRect(line_start_x, char_y + yy, line_length, 1, color);
                    line_length = 0;
                }
            }
            bits <<= 1;
            bit_count--;
        }
        
        if (line_length > 0) {
            ili9488_FillRect(line_start_x, char_y + yy, line_length, 1, color);
        }
    }
}

// Вывод строки текста через системный decode_utf8 из common.c
void lcd_draw_string_old(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg) {
    if (!current_font) return;

    uint16_t cursor_x = x;
    uint8_t c;

    // Передаем адрес указателя строки в системный декодер common.c
    while ((c = decode_utf8(&str))) {
        // Если символ не поддерживается шрифтом — безопасно шагаем вперед
        if (c < current_font->first || c > current_font->last) {
            cursor_x += current_font->glyph[' ' - current_font->first].xAdvance;
            continue;
        }

        GFXglyph *glyph = &(current_font->glyph[c - current_font->first]);

        // Полный игнор физической прорисовки пробела (код 32) для защиты от кавычек
        if (c == 32 || c == ' ') {
            uint16_t clear_w = glyph->xAdvance + 12;
            int16_t char_y = y + glyph->yOffset;
            if (cursor_x < 480) {
                uint16_t fill_w = ((cursor_x + clear_w) > 480) ? (480 - cursor_x) : clear_w;
                ili9488_FillRect(cursor_x, char_y, fill_w, current_font->yAdvance, bg);
            }
        } else {
            lcd_draw_char(cursor_x, y, c, color, bg);
        }
        
        cursor_x += glyph->xAdvance;
    }
}

// Расчет физической ширины строки через системный decode_utf8 из common.c
uint16_t lcd_get_string_width_old(const char *str) {
    if (!current_font) return 0;
    
    uint16_t width = 0;
    uint8_t c;
    
    while ((c = decode_utf8(&str))) {
        if (c >= current_font->first && c <= current_font->last) {
            width += current_font->glyph[c - current_font->first].xAdvance;
        } else {
            width += current_font->glyph[' ' - current_font->first].xAdvance;
        }
    }
    return width;
}






// Сама функция отрисовки с исправленным оформлением циклов ожидания SPI
void lcd_draw_char(uint16_t x, uint16_t y, uint16_t c, uint16_t color, uint16_t bg) {
    if (!current_font) return;
    if (c < current_font->first || c > current_font->last) return;

    GFXglyph *glyph = &(current_font->glyph[c - current_font->first]);
    uint8_t *bitmap = current_font->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width;
    uint8_t  h  = glyph->height;

    if (w == 0 || h == 0) return; 

    int16_t char_x = x + glyph->xOffset;
    int16_t char_y = y + glyph->yOffset;

    // Защита от вылета за физические границы матрицы
    if (char_x >= 480 || char_y >= 320 || char_x + w < 0 || char_y + h < 0) return;

    // Шаг 1: Задаем ОДНО единое окно под физический габарит буквы
    ili9488_SetDisplayWindow(char_x, char_y, w, h);
    lcd_send_cmd(0x2C); // RAMWR
    ILI9486_DC_SET;
    ILI9486_CS_CLR; // Удерживаем CS зажатым на протяжении всей буквы!

    // Шаг 2: Предрасчет байт цветов RGB888 для ILI9488
    uint8_t r_txt = (uint8_t)((color & 0xF800) >> 8);
    uint8_t g_txt = (uint8_t)((color & 0x07E0) >> 3);
    uint8_t b_txt = (uint8_t)((color & 0x001F) << 3);

    uint8_t r_bg  = (uint8_t)((bg & 0xF800) >> 8);
    uint8_t g_bg  = (uint8_t)((bg & 0x07E0) >> 3);
    uint8_t b_bg  = (uint8_t)((bg & 0x001F) << 3);

    uint8_t  bits = 0;
    uint8_t  bit_count = 0;

    // Шаг 3: Монолитный сквозной рендеринг строго по сетке W x H без микропереносов координат
    for (uint16_t yy = 0; yy < h; yy++) {
        for (uint16_t xx = 0; xx < w; xx++) {
            if (bit_count == 0) {
                bits = bitmap[bo++];
                bit_count = 8;
            }
            
            // Выстреливаем пиксели непрерывным потоком
            if (bits & 0x80) {
                LCD_WRITE_RGB888(r_txt, g_txt, b_txt);
            } else {
                LCD_WRITE_RGB888(r_bg, g_bg, b_bg);
            }
            
            bits <<= 1;
            bit_count--;
        }
    }
    
    // Шаг 4: Только теперь завершаем транзакцию SPI
    while (SPI1->SR & SPI_SR_BSY) { }
    ILI9486_CS_SET;
}




void lcd_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg) {
    if (!current_font || !str) return;

    uint16_t cursor_x = x;
    uint8_t c;

    while ((c = decode_utf8(&str))) {
        if (c < current_font->first || c > current_font->last) {
            cursor_x += current_font->glyph[' ' - current_font->first].xAdvance;
            continue;
        }

        GFXglyph *glyph = &(current_font->glyph[c - current_font->first]);

        // Возвращаем ваш оригинальный затир фона строки
        int16_t char_y = y + glyph->yOffset;
        uint16_t clear_w = glyph->xAdvance + 12;

        if (cursor_x < 480) {
            uint16_t fill_w = ((cursor_x + clear_w) > 480) ? (480 - cursor_x) : clear_w;
            ili9488_FillRect(cursor_x, char_y, fill_w, current_font->yAdvance, bg);
        }

        // Выводим физическое тело буквы только если это не чистый пробел
        if (c != 32 && c != ' ') {
            lcd_draw_char(cursor_x, y, c, color, bg);
        }
        
        cursor_x += glyph->xAdvance;

        if (cursor_x >= 480) {
            break;
        }
    }
}

// Корректный расчет ширины строки с учетом таблицы Adafruit_Rus
uint16_t lcd_get_string_width(const char *str) {
    if (!current_font || !str) return 0;
    
    uint16_t width = 0;
    uint32_t c;
    
    while ((c = decode_utf8(&str))) {
        // Повторяем ту же конвертацию для точности расчета пропорциональных букв
        if (c >= 0x0410 && c <= 0x043F) {
            c = c - 0x0410 + 0x80;
        } else if (c >= 0x0440 && c <= 0x044F) {
            c = c - 0x0440 + 0xE0;
        } else if (c == 0x0401) {
            c = 0xA2;
        } else if (c == 0x0451) {
            c = 0xB2;
        }

        if (c >= current_font->first && c <= current_font->last) {
            width += current_font->glyph[c - current_font->first].xAdvance;
        } else {
            width += current_font->glyph[' ' - current_font->first].xAdvance;
        }
    }
    return width;
}
