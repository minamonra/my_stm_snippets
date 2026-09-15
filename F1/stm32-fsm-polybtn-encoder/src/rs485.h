#ifndef __RS485_H__
#define __RS485_H__

#include "stm32f103xb.h"
#include <stddef.h>

// Управление пином направления DE/RE (PB15)
#define RS485DE1       GPIOB->BSRR = (1U << 15)  // DE = 1 (Передача)
#define RS485DE0       GPIOB->BRR  = (1U << 15)  // DE = 0 (Приём)

#define TRANSMITLENGHT 28

// Прототипы функций драйвера блинковой панели
void rs485_init_9n1(void);
void usart1_send9_buffer(const uint16_t *words, size_t count);
void send2blink_panel(const char *utf8_text); // Функция теперь принимает UTF-8 строку напрямую
void reverseString(const char* input, char* output, int length);
void calc_panel_crc(const uint16_t buffer[], size_t length, uint16_t *byte13, uint16_t *byte14);
void convertToCharArray(int number, char* result);
void utf8_to_panel_string(const char *utf8_in, char *panel_out);

#endif // __RS485_H__
