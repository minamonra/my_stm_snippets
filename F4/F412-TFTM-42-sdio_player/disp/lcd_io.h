#ifndef LCD_IO_H
#define LCD_IO_H

/////////////////////////
// ST7796 Display driver
/////////////////////////

#include <stdint.h>
#include "common.h"

// Оригинальные алиасы типов из SPL
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

// ============================================================================
// НАСТРОЙКА ОРИЕНТАЦИИ И РАЗМЕРОВ ЭКРАНА (ST7796S)
// ============================================================================
// Выберите режим работы экрана от 0 до 3:
// 0 - Книжная/Портретная (Провода модуля сверху, шлейф матрицы внизу)
// 1 - Альбомная/Горизонтальная стандартная (Провода слева)
// 2 - Книжная/Портретная перевернутая на 180° (Провода внизу)
// 3 - Альбомная/Горизонтальная перевернутая на 180° (Провода справа) -> ВАШ РЕЖИМ
#define USE_HORIZONTAL  3

#define LCD_W 320
#define LCD_H 480

// Цветовая палитра из оригинального lcd.h
#define WHITE         0xFFFF
#define BLACK         0x0000
#define BLUE          0x001F
#define BRED          0XF81F
#define GRED          0XFFE0
#define GBLUE         0X07FF
#define RED           0xF800
#define MAGENTA       0xF81F
#define GREEN         0x07E0
#define CYAN          0x7FFF
#define YELLOW        0xFFE0
#define BROWN         0XBC40
#define BRRED         0XFC07
#define GRAY          0X8430

// Оригинальная структура из вашего lcd.h
typedef struct
{
    u16 width;
    u16 height;
    u16 id;
    u8  dir;
    u16 wramcmd;
    u16 rramcmd;
    u16 setxcmd;
    u16 setycmd;
} _lcd_dev;

extern _lcd_dev lcddev;
extern u16 POINT_COLOR;
extern u16 BACK_COLOR;

// === МАКРОСЫ ДЛЯ ВАШЕЙ ПРОВЕРЕННОЙ СХЕМЫ (PB0, PB1, PB2, PA1) ===
#define ST7796_CS_PIN     (1U << 0)   // PB0
#define ST7796_CS_SET     (GPIOB->BSRR = ST7796_CS_PIN)
#define ST7796_CS_CLR     (GPIOB->BSRR = (u32)ST7796_CS_PIN << 16U)

#define ST7796_RST_PIN    (1U << 1)   // PB1
#define ST7796_RST_SET    (GPIOB->BSRR = ST7796_RST_PIN)
#define ST7796_RST_CLR    (GPIOB->BSRR = (u32)ST7796_RST_PIN << 16U)

#define ST7796_DC_PIN     (1U << 2)   // PB2
#define ST7796_DC_SET     (GPIOB->BSRR = ST7796_DC_PIN)
#define ST7796_DC_CLR     (GPIOB->BSRR = (u32)ST7796_DC_PIN << 16U)

#define ST7796_LED_PIN     (1U << 1)  // PA1
#define ST7796_LED_ON      (GPIOA->BSRR = ST7796_LED_PIN)
#define ST7796_LED_OFF     (GPIOA->BSRR = (u32)ST7796_LED_PIN << 16U)
#define ST7796_LED_TOGGLE  (GPIOA->ODR ^= ST7796_LED_PIN)

// Прототипы функций официальной библиотеки Alientek
void lcd_init(void);
void lcd_clear(u16 Color);
void lcd_setcursor(u16 Xpos, u16 Ypos);
void lcd_drawpoint(u16 x, u16 y);
void lcd_drawrectangle(u16 x1, u16 y1, u16 x2, u16 y2);
void lcd_setwindows(u16 xStar, u16 yStar, u16 xEnd, u16 yEnd);
void lcd_direction(u8 direction);
u16  lcd_read_id(void);
void lcd_fillrect(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);

// НОВОЕ: пакетный блит буфера произвольных 16-битных пикселей одним DMA-проходом
// (в отличие от lcd_fillrect, здесь адрес источника ИНКРЕМЕНТИРУЕТСЯ по буферу buf,
// а не остаётся зафиксированным на одном цвете). Используется, например, для
// быстрой отрисовки глифов шрифта, собранных заранее в RAM-буфер.
void lcd_blit_buffer(u16 x1, u16 y1, u16 w, u16 h, const u16 *buf);

// Новые переименованные функции отправки данных
void LCD_WR_REG(u8 data); // Была нужна для регистров
void LCD_WR_DATA8(u8 data);
void LCD_WR_DATA16(u16 Data);

// Аппаратная настройка DMA (Добавлено, чтобы убрать ошибку!)
void lcd_dma_init(void);

// Системный интерфейс обмена
// ВАЖНО: lcd_spi_transfer() — ПОЛНОДУПЛЕКСНАЯ функция (ждёт RXNE и читает DR).
// Используйте её ТОЛЬКО там, где реально нужен байт, принятый по MISO
// (например, lcd_read_id()). Для обычной отправки команд/данных используйте
// spi_raw_write() — она не тратит время на ожидание RXNE, т.к. ответ не нужен.
u8 lcd_spi_transfer(u8 byte);
void spi_raw_write(uint8_t byte);

#endif