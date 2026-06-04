#ifndef LCD_FONT_H
#define LCD_FONT_H

#include <stdint.h>

#ifndef PROGMEM
#define PROGMEM
#endif

// 1. СТРУКТУРЫ ФОРМАТА ADAFRUIT GFX (перенесены наверх)
typedef struct {
    uint16_t bitmapOffset;     // Смещение символа в общем массиве бит
    uint8_t  width, height;    // Размеры видимой части символа в пикселях
    uint8_t  xAdvance;         // Шаг курсора по горизонтали после печати
    int8_t   xOffset, yOffset; // Сдвиги рисования относительно курсора
} GFXglyph;

typedef struct {
    uint8_t  *bitmap;          // Указатель на массив сырых бит
    GFXglyph *glyph;           // Указатель на таблицу символов
    uint8_t   first, last;     // Минимальный и максимальный ASCII-код в шрифте
    uint8_t   yAdvance;        // Высота всей строки (шаг по вертикали)
} GFXfont;

// --- СИСТЕМНЫЙ ПРОТОТИП (нужен для работы макроса) ---
void lcd_set_font(const GFXfont *f);

// 2. МАКРОС ГЕНЕРАЦИИ (теперь компилятор знает, что такое GFXfont и lcd_set_font)
#define DECLARE_FONT(font_name) \
    extern const GFXfont font_name; \
    static inline void lcd_set_font_##font_name(void) { \
        lcd_set_font(&font_name); \
    }

// --- АВТОМАТИЧЕСКОЕ ОБЪЯВЛЕНИЕ ВАШИХ ШРИФТОВ ---
DECLARE_FONT(FreeSans14pt8b)
DECLARE_FONT(FreeMonoBoldOblique16pt8b)
DECLARE_FONT(FreeMono14pt8b)
DECLARE_FONT(FreeSansBold14pt8b)

// --- ИНТЕРФЕЙС ТЕКСТОВОГО СЛОЯ ---
void lcd_draw_char(uint16_t x, uint16_t y, uint16_t c, uint16_t color, uint16_t bg);
void lcd_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);

// Функция для динамического расчета длины строки в пикселях (нужна для выравнивания по центру)
uint16_t lcd_get_string_width(const char *str);

uint8_t decode_utf8(const char **str);
#endif // LCD_FONT_H