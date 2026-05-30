#ifndef PINDEFS_H
#define PINDEFS_H

#include "stm32f4xx.h"

// Конфигурация системного светодиода платы WeAct (PB2)
#define LED_SYSTEM_PIN      (1U << 2)
#define LED_SYSTEM_ON       GPIOB->BSRR = (LED_SYSTEM_PIN << 16)                // Сброс в 0 (зажечь)
#define LED_SYSTEM_OFF      GPIOB->BSRR = LED_SYSTEM_PIN                        // Установка в 1 (погасить)
#define LED_SYSTEM_TOGGLE   GPIOB->ODR ^= LED_SYSTEM_PIN                        // Инверсия

// Заготовка для внешнего реле (например, на PC13)
#define RELAY_PIN           (1U << 13)
#define RELAY_ON            GPIOC->BSRR = RELAY_PIN                             // Установка в 1 (включить)
#define RELAY_OFF           GPIOC->BSRR = (RELAY_PIN << 16)                     // Сброс в 0 (выключить)

#endif // PINDEFS_H