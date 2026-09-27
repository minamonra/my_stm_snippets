#ifndef __COMMON_H__
#define __COMMON_H__

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t

#define LED_SYSTEM_PIN    GPIO_BSRR_BS2                 // Задаем маску для PB2 из вашего файла (равно 1U << 2)
#define LED_SYSTEM_OFF    GPIOB->BSRR = GPIO_BSRR_BR2   // Включить системный светодиод (низкий уровень на выходе)
#define LED_SYSTEM_ON     GPIOB->BSRR = LED_SYSTEM_PIN  // Выключить системный светодиод (высокий уровень на выходе)
#define LED_SYSTEM_TOGGLE GPIOB->ODR ^= LED_SYSTEM_PIN  // Переключить состояние светодиода

void clock_init(void);
void hw_init(void);

#endif  // __COMMON_H__