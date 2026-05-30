#ifndef ST7789V_H
#define ST7789V_H

#include <stdint.h>

// ==================== РАЗМЕРЫ ДИСПЛЕЯ ====================
#define ST7789_WIDTH    320
#define ST7789_HEIGHT   240

// ==================== ЦВЕТА (RGB565) ====================
#define ST7789_BLACK    0x0000
#define ST7789_WHITE    0xFFFF
#define ST7789_RED      0xF800
#define ST7789_GREEN    0x07E0
#define ST7789_BLUE     0x001F
#define ST7789_YELLOW   0xFFE0
#define ST7789_CYAN     0x07FF
#define ST7789_MAGENTA  0xF81F
#define ST7789_DARKGRAY 0x4208
#define ST7789_GRAY     0x7BEF
#define ST7789_ORANGE      0xFD20  // Яркий оранжевый (для названий станций)
#define ST7789_DARKGREEN   0x03E0  // Глубокий зеленый (для статуса "ГОТОВ")
#define ST7789_NAVY        0x000F  // Темно-синий благородный (подложка или бары)
#define ST7789_LIGHTBLUE   0x7D7C  // Светло-голубой (вспомогательный текст)
#define ST7789_GOLD        0xFEA0  // Золотой / Теплый желтый
#define ST7789_PURPLE      0x780F  // Фиолетовый


// Цвета RGB565
#define ST7789_BLACK       0x0000
#define ST7789_WHITE       0xFFFF
#define ST7789_RED         0xF800
#define ST7789_GREEN       0x07E0
#define ST7789_YELLOW      0xFFE0
#define ST7789_CYAN        0x07FF
#define ST7789_DARKGRAY    0x4208
#define ST7789_GRAY        0x7BEF
#define ST7789_ORANGE      0xFD20  
#define ST7789_DARKGREEN   0x03E0  
// =========================================================================
// МАКРОСЫ УПРАВЛЕНИЯ ПИНАМИ ПОРТОВ ST7789 (СТРОГО ПО НОВОЙ СХЕМЕ)
// =========================================================================
#define ST7789_CS_LOW()   GPIOA->BSRR = GPIO_BSRR_BR2
#define ST7789_CS_HIGH()  GPIOA->BSRR = GPIO_BSRR_BS2
#define ST7789_DC_LOW()   GPIOA->BSRR = GPIO_BSRR_BR3
#define ST7789_DC_HIGH()  GPIOA->BSRR = GPIO_BSRR_BS3

#define ST7789_RST_LOW()  GPIOA->BSRR = GPIO_BSRR_BR1
#define ST7789_RST_HIGH() GPIOA->BSRR = GPIO_BSRR_BS1
#define ST7789_BL_ON()    GPIOC->BSRR = GPIO_BSRR_BS15
#define ST7789_BL_OFF()   GPIOC->BSRR = GPIO_BSRR_BR15

// ==================== ИНИЦИАЛИЗАЦИЯ ====================
void ST7789_DMA_Init(void);
void ST7789_Init(void);
void ST7789_Reset(void);
void ST7789_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// ==================== БАЗОВЫЕ ПРИМИТИВЫ ====================
void ST7789_FillScreen_DMA(uint16_t color);
void ST7789_FillRect_CPU(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_FillRect_DMA(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
void ST7789_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
void ST7789_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

// ==================== ШРИФТЫ ====================
void ST7789_DrawChar_12x24(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg_color);
void ST7789_DrawString_12x24(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color);
void ST7789_DrawChar_16x32(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg_color);
void ST7789_DrawString_16x32(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color);
void ST7789_ClearScreen_DMA(uint16_t color);
void ST7789_ClearScreen_NoDMA(uint16_t color);

#endif // ST7789V_H