#ifndef ILI9488X_H
#define ILI9488X_H

#include <stdint.h>

// Основные физические параметры дисплея
#define ILI9488_WIDTH   480
#define ILI9488_HEIGHT  320

// Базовые (0x0000 - 0xFFFF)
#define ILI9488_BLACK     0x0000
#define ILI9488_WHITE     0xFFFF
#define ILI9488_RED       0xF800
#define ILI9488_GREEN     0x07E0
#define ILI9488_BLUE      0x001F
#define ILI9488_YELLOW    0xFFE0
#define ILI9488_CYAN      0x07FF
#define ILI9488_MAGENTA   0xF81F

// Дополнительные (0x7800 - 0xFD20)
#define ILI9488_ORANGE    0xFD20
#define ILI9488_PURPLE    0x780F
#define ILI9488_MAROON    0x7800
#define ILI9488_NAVY      0x000F
#define ILI9488_DARKGREEN 0x03E0
#define ILI9488_OLIVE     0x7BE0
#define ILI9488_LIGHTGREY 0xC618
#define ILI9488_DARKGREY  0x7BEF

// Новые добавленные цвета
#define ILI9488_TEAL      0x0410  // Темно-бирюзовый (цвет морской волны)
#define ILI9488_GRAY      0x8410  // Стандартный серый нейтральный
#define ILI9488_GOLD      0xFEA0  // Золотой
#define ILI9488_PINK      0xF81F  // Розовый
#define ILI9488_LIME      0x07E0  // Ярко-лаймовый зеленый
#define ILI9488_BROWN     0x9940  // Коричневый

// Для конвертации 24-битного цвета в 16-бит используется макрос
#define ILI9488_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))


// --- ВЫСОКОУРОВНЕВЫЕ СИСТЕМНЫЕ ФУНКЦИИ ---
void ili9488_Init(void);
void ili9488_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize);

// --- ГРАФИЧЕСКИЕ ПРИМИТИВЫ ---
void ili9488_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode);
void ili9488_Clear(uint16_t RGBCode);
void ili9488_DrawPixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode);
// Быстрая горизонтальная линия через FillRect (использует DMA под капотом)
void ili9488_DrawFastHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length, uint16_t RGBCode);
// Быстрая вертикальная линия через FillRect (использует DMA под капотом)
void ili9488_DrawFastVLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length, uint16_t RGBCode);
// Контур прямоугольника (состоит из 4 быстрых линий)
void ili9488_DrawRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode);
// Универсальная линия произвольного направления (Алгоритм Брезенхема)
void ili9488_DrawLine(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t RGBCode);
// Контур окружности (Алгоритм Брезенхема/Мидпоинта)
void ili9488_DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint16_t RGBCode);
// Заполненная окружность (Оптимизирована: вместо точек рисует быстрые горизонтальные линии)
void ili9488_FillCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint16_t RGBCode);

#endif // ILI9488X_H