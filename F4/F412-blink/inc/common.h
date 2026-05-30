#ifndef COMMON_H
#define COMMON_H

#include "stm32f4xx.h"

extern volatile uint32_t ttms;

// Системные функции задержки и времени
void delay_ms(uint32_t ms);
void delay_nop(uint32_t ticks);
void blink_led(uint16_t freq);

// Единственная точка входа для инициализации всего железа
void System_Init(void);

#endif // COMMON_H