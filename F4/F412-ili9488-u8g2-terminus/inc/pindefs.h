#ifndef PINDEFS_H
#define PINDEFS_H

#include "stm32f4xx.h"

// Системный светодиод платы WeAct (Пин PB2)
// Системный светодиод платы WeAct (Пин PB2)
#define LED_SYSTEM_ON       GPIOB->BSRR = GPIO_BSRR_BR_2
#define LED_SYSTEM_OFF      GPIOB->BSRR = GPIO_BSRR_BS_2
#define LED_SYSTEM_TOGGLE   GPIOB->ODR  ^= GPIO_ODR_OD2
// -----------------------------------------------------------------------------
// Дисплей ILI9488 (Интерфейс SPI1, PORTA)
// -----------------------------------------------------------------------------
#define ILI9486_CS_SET      GPIOA->BSRR = GPIO_BSRR_BS_4                        // CS = 1
#define ILI9486_CS_CLR      GPIOA->BSRR = GPIO_BSRR_BR_4                        // CS = 0

#define ILI9486_DC_SET      GPIOA->BSRR = GPIO_BSRR_BS_3                        // DC = 1 (Данные)
#define ILI9486_DC_CLR      GPIOA->BSRR = GPIO_BSRR_BR_3                        // DC = 0 (Команда)

#define ILI9486_RST_SET     GPIOA->BSRR = GPIO_BSRR_BS_2                        // RST = 1
#define ILI9486_RST_CLR     GPIOA->BSRR = GPIO_BSRR_BR_2                        // RST = 0

#define ILI9486_LED_ON      GPIOA->BSRR = GPIO_BSRR_BS_1                        // LED = 1
#define ILI9486_LED_OFF     GPIOA->BSRR = GPIO_BSRR_BR_1                        // LED = 0

#endif // PINDEFS_H