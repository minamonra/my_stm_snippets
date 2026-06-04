#ifndef PINDEFS_H
#define PINDEFS_H

#include "stm32f4xx.h"

// Системный светодиод платы WeAct (Пин PB2)
#define LED_SYSTEM_ON       GPIOB->BSRR = GPIO_BSRR_BR_2                        // Зажечь (0)
#define LED_SYSTEM_OFF      GPIOB->BSRR = GPIO_BSRR_BS_2                        // Погасить (1)
#define LED_SYSTEM_TOGGLE   GPIOB->ODR  ^= GPIO_ODR_OD2                         // Инверсия

// Пользовательская кнопка K1 (Пин PC13)
#define BUTTON_K1_PRESSED   GPIOC->IDR & GPIO_IDR_IDR_13                        // Вернет true, если кнопка зажата

#endif // PINDEFS_H