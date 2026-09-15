#ifndef __COMMON_H__
#define __COMMON_H__

#include "stm32f103xb.h"
#include <string.h>

// Макросы для переключения светодиодов
#define LED1TOGGLE GPIOC->ODR ^= (1<<13)
#define LED2TOGGLE GPIOC->ODR ^= (1<<14)

// Макросы для конфигурации портов ввода-вывода (CRL / CRH)
#define CRL(pin, config)       ((config) << ((pin) * 4))
#define CRH(pin, config)       ((config) << (((pin) - 8) * 4))

#define MODE_INPUT             0x00U
#define MODE_NORMAL            0x01U
#define MODE_SLOW              0x02U
#define MODE_FAST              0x03U

#define CNF_ANALOG             0x00U
#define CNF_FLOATING           0x04U
#define CNF_PUDINPUT           0x08U

#define CNF_PPOUTPUT           0x00U
#define CNF_ODOUTPUT           0x04U
#define CNF_AFPP               0x08U
#define CNF_AFOD               0x0CU

// Управление включением питания блинкерной панели (PA0)
#define PANEL_POWER_ON   GPIOA->BSRR = (1U << 0)   // Выдать 3.3В на PA0
#define PANEL_POWER_OFF  GPIOA->BRR  = (1U << 0)   // Опустить PA0 в 0В

// Внешние объявления переменных, физически лежащих в common.c
extern volatile uint32_t ttms;
extern volatile uint32_t ddms;
extern volatile uint32_t pc13ms;
extern volatile uint32_t pc14ms;

// Прототипы функций
void StartHSE(void);
void hardware_init(void);
void delay_ms(uint16_t ms);
void delay_nop(uint32_t count);
int replace_char_at(char *str, size_t position, char character, uint8_t edtstrlen);
void pad_string_with_spaces(char *str, size_t current_len, size_t total_len);
void trim_and_clean_string(char *str, size_t max_len);
uint16_t simple_rand(void);
void safe_strncpy(char *dest, const char *src, size_t n);

#endif // __COMMON_H__
