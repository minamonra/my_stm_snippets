#ifndef PINDEFS_H
#define PINDEFS_H

#include "stm32f4xx.h"

// -----------------------------------------------------------------------------
// Системный светодиод платы WeAct (Пин PB2)
// -----------------------------------------------------------------------------
// На платах WeAct светодиод горит от логического нуля, поэтому:
// BS_2 (Bit Set) — гасит диод, а BR_2 (Bit Reset) — зажигает его.
#define LED_SYSTEM_ON       GPIOB->BSRR = GPIO_BSRR_BR_2                        // Сброс PB2 в 0 (зажечь)
#define LED_SYSTEM_OFF      GPIOB->BSRR = GPIO_BSRR_BS_2                        // Установка PB2 в 1 (погасить)
#define LED_SYSTEM_TOGGLE   GPIOB->ODR  ^= GPIO_ODR_OD2                         // Инверсия состояния PB2

// -----------------------------------------------------------------------------
// Внешнее реле (Пин PC13)
// -----------------------------------------------------------------------------
#define RELAY_ON            GPIOC->BSRR = GPIO_BSRR_BS_13                       // Установка PC13 в 1 (включить)
#define RELAY_OFF           GPIOC->BSRR = GPIO_BSRR_BR_13                       // Сброс PC13 в 0 (выключить)

#endif // PINDEFS_H