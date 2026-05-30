#ifndef COMMON_H
#define COMMON_H

#include "stm32f4xx.h"

extern volatile uint32_t ttms;

// Системные функции задержки
void delay_ms(uint32_t ms);
void delay_nop(uint32_t ticks);

// Функции инициализации
void system_clock_config_100MHz(void);
void gpio_init(void);

#endif // COMMON_H