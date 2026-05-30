#ifndef COMMON_H
#define COMMON_H

#include "stm32f4xx.h"

#ifndef NULL
#define NULL ((void *)0)                                                        // Классическое Си-определение нулевого указателя
#endif

extern volatile uint32_t ttms;

// Системные функции задержки и времени
void delay_ms(uint32_t ms);
void delay_nop(uint32_t ticks);
void blink_led(uint16_t freq);
void morse_send_nb(const char* str);

// Единственная точка входа для инициализации всего железа
void System_Init(void);

#endif // COMMON_H