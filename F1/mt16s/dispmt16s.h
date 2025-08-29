#ifndef __DISPMT16S_H__
#define __DISPMT16S_H__
#include "stm32f10x.h"

// PA15-RS PB3-E PB4-D4 PB5-D5 PB8-D6 PB9-D7
// === Макросы для управления пинами дисплея ===
// RS = PA15
#define RS1 GPIOA->BSRR |= GPIO_BSRR_BS15 // set (1)
#define RS0 GPIOA->BSRR |= GPIO_BSRR_BR15 // reset (0)
// E (en) = PB3
#define EN1 GPIOB->BSRR |= GPIO_BSRR_BS3
#define EN0 GPIOB->BSRR |= GPIO_BSRR_BR3
// D4 = PB4
#define D41 GPIOB->BSRR |= GPIO_BSRR_BS4
#define D40 GPIOB->BSRR |= GPIO_BSRR_BR4
// D5 = PB5
#define D51 GPIOB->BSRR |= GPIO_BSRR_BS5
#define D50 GPIOB->BSRR |= GPIO_BSRR_BR5
// D6 = PB8
#define D61 GPIOB->BSRR |= GPIO_BSRR_BS8
#define D60 GPIOB->BSRR |= GPIO_BSRR_BR8
// D7 = PB9
#define D71 GPIOB->BSRR |= GPIO_BSRR_BS9
#define D70 GPIOB->BSRR |= GPIO_BSRR_BR9

void lcd_init(void);
void lcdCommand(uint8_t cmd);
void lcdString(const char *s);

#endif // __DISPMT16S_H__