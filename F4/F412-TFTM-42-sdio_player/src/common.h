#ifndef __COMMON_H__
#define __COMMON_H__

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

// Регистры IWDG
#define IWDG_REFRESH 0xAAAA                                // Ключ для перезагрузки IWDG

// Макросы управления системным светодиодом (PB2)
#define LED_SYSTEM_PIN    (1U << 2)
#define LED_SYSTEM_ON     GPIOB->BRR = LED_SYSTEM_PIN      // Включить (низкий уровень)
#define LED_SYSTEM_OFF    GPIOB->BSRR = LED_SYSTEM_PIN     // Выключить (высокий уровень)
#define LED_SYSTEM_TOGGLE GPIOB->ODR ^= LED_SYSTEM_PIN     // Переключить

extern volatile uint32_t ttms;                             // Глобальный счётчик миллисекунд (SysTick)

void system_clock_config_96MHz(void);
void hw_init(void);
void gpio_init(void);
void delay_ms(uint32_t ms);
void delay_nop(uint32_t ticks);
void blink_led(uint16_t freq);                             // freq — период переключения в мс
void uint16_to_hex(uint16_t val, char* out);               // Конвертация uint16_t в 4-символьную HEX-строку
uint16_t get_random(uint16_t max);
void random_seed(uint32_t seed);
uint16_t decode_utf8(const char** ptr);
void format_time(uint32_t sec, char* buf);
void uint32_to_str(uint32_t num, char* buf);
int strcasecmp(const char* s1, const char* s2);

#ifndef PROGMEM
#define PROGMEM                                             // Заглушка для совместимости с AVR-кодом
#endif

#endif  // __COMMON_H__