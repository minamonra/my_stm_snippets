#pragma once
#ifndef __ST7735_H__
#define __ST7735_H__
#include "st7735defs.h"
#include <stm32f0xx.h>

// PB3 - SPI1_SCK (clock)
// PB4 - CS (chip select)
// PB5 - SPI1_MOSI (data)

// Chip select PB4
#define CS_UP GPIOB->BSRR |= GPIO_BSRR_BS_4
#define CS_DN GPIOB->BSRR |= GPIO_BSRR_BR_4

// DC (RS) PB1
#define DC_UP GPIOB->BSRR |= GPIO_BSRR_BS_1
#define DC_DN GPIOB->BSRR |= GPIO_BSRR_BR_1

// Reset PB0
#define RST_UP GPIOB->BSRR |= GPIO_BSRR_BS_0
#define RST_DN GPIOB->BSRR |= GPIO_BSRR_BR_0

#define LCD_X 160
#define LCD_Y 128
#define LCD_C 0x00
#define LCD_D 0x01

#define st7735_clear(x) st7735_fill(0, 0x9f, 0, 0x7f, x)
#define st7735_rectangle(a, b, c, d, e) st7735_fill(a, a + b - 1, c, c + d - 1, e)
#define st7735_point(x, y, c) st7735_fill(x, x, y, y, c)
#define st7735_line_vertical(x, y, l, d, c) st7735_fill(x, x + d - 1, y, y + l - 1, c)
#define st7735_line_horizont(x, y, l, d, c) st7735_fill(x, x + l - 1, y, y + d - 1, c)

void spi_init(void);
void st7735_init(void);
void st7735_send(uint8_t dc, uint8_t data);
void st7735_fill(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint16_t color);
void st7735_send_char(uint8_t x, uint8_t y, uint8_t ch, uint16_t fg_color, uint16_t bg_color);

#endif // __ST7735_H__