#include "demo.h"
#include "ili9488x.h"
#include "lcd_draw_u8g2.h"

void demo_show_terminus_maxout(void) {
    // Переменные для динамического переключения метрик
    uint8_t h, a;
    
    // 1. Полная очистка экрана в глубокий синий цвет
    ili9488_Clear(ILI9488_NAVY); 
    
    // 2. Рисуем двойную бирюзовую рамку по краям матрицы 480x320
    ili9488_DrawRect(2, 2, 476, 316, ILI9488_CYAN);
    ili9488_DrawRect(4, 4, 472, 312, ILI9488_CYAN);

    // Начальные координаты (отступ слева)
    uint16_t x = 14; 
    uint16_t y;

    // ========================================================================
    // ШАПКА ЭКРАНА (Шрифт 10х20)
    // ========================================================================
    lcd_u8g2_set_font(u8g2_font_10x20_t_cyrillic);
    a = lcd_u8g2_get_font_ascent();   
    
    y = 9 + a; // Первая строка шапки
    lcd_draw_u8g2_string(x, y, "     U8G2 ДЕКОДЕР ДЛЯ ILI9488 3.5\" ДИСПЛЕЯ", ILI9488_YELLOW, ILI9488_NAVY);

    
    y += 6; // Смещение под линию
    ili9488_DrawFastHLine(12, y, 456, ILI9488_CYAN);
    
    // ========================================================================
    // ОСНОВНОЙ КОНТЕНТ (Крупный кастомный Terminus 28b)
    // ========================================================================
    lcd_u8g2_set_font(u8g2_font_terminus_28b_cyr);
    h = lcd_u8g2_get_font_height(); 
    a = lcd_u8g2_get_font_ascent(); 
    
    // Координата Y для первой строки контента
    y = y + 9 + a; 
    
    // ЛОБ В ЛОБ: Сравнение заглавных букв встык (Строго по 32 символа)
    lcd_draw_u8g2_string(x, y, "Р:АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬ", ILI9488_WHITE, ILI9488_NAVY);
    y += h;
    lcd_draw_u8g2_string(x, y, "L:ABCDEFGHIJKLMNOPQRSTUVWXYZ+-[]", ILI9488_GREEN, ILI9488_NAVY);
    y += h;
    
    // Остальные алфавиты и регистры (Добиты ровно до 32 символов)
    lcd_draw_u8g2_string(x, y, "Р:ЦЧШЩЪЫЬЭЮЯ абвгдеёжзийклмнопрс", ILI9488_LIGHTGREY, ILI9488_NAVY);
    y += h;
    lcd_draw_u8g2_string(x, y, "Р:льмнопрстуфхцчшщъыьэюя|АБВГДЕЁ", ILI9488_LIGHTGREY, ILI9488_NAVY);
    y += h;

    lcd_draw_u8g2_string(x, y, "l:abcdefghijklmnopqrstuvwxyz|012", ILI9488_GREEN, ILI9488_NAVY);
    y += h;
    
    // Цифры + Спецсимволы (Строго 32 символа, правый край встанет идеально)
    lcd_draw_u8g2_string(x, y, "N:01234567890123456789!@#$%^&*()", ILI9488_CYAN, ILI9488_NAVY);
    
    // Разделительная оранжевая линия перед логами
    y += 4;
    ili9488_DrawFastHLine(12, y, 456, ILI9488_ORANGE);
    
    // Переход к нижнему текстовому блоку
    y += h; 
    
    // ============= ПРИКОЛЬНЫЕ ФРАЗЫ (Выровнены по ширине до 32 символов) =============
    lcd_draw_u8g2_string(x, y, "Зачем мы здесь? Чтобы течь! 1337", ILI9488_YELLOW, ILI9488_NAVY);
    y += h;
    lcd_draw_u8g2_string(x, y, "[ 42 - ОТВЕТ НА ГЛАВНЫЙ ВОПРОС ]", ILI9488_LIME, ILI9488_NAVY);
    y += h;
    lcd_draw_u8g2_string(x, y, "[   Работает? Не трогай! (c)   ]", ILI9488_GOLD, ILI9488_NAVY);
    
    // ========================================================================
    // СТРОКА СТАТУСА (Внизу, шрифт 10х20)
    // ========================================================================
    lcd_u8g2_set_font(u8g2_font_10x20_t_cyrillic);
    
    // Фиксированная координата золотой линии над подвалом
    uint16_t status_line_y = 286;
    ili9488_DrawFastHLine(12, status_line_y, 456, ILI9488_GOLD);
    
    // Вывод текста статуса
    lcd_draw_u8g2_string(x, 306, "  STM32F412RGT6 |CLK:100MHz | SPI:50MHz| DMA", ILI9488_GOLD, ILI9488_NAVY);
}
