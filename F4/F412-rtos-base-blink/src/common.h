#ifndef __COMMON_H__
#define __COMMON_H__

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef PROGMEM
#define PROGMEM  // Заглушка для совместимости с AVR-кодом
#endif

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t

// Регистры IWDG
#define IWDG_REFRESH 0xAAAA  // Ключ для перезагрузки IWDG

#define LED_SYSTEM_PIN    GPIO_BSRR_BS2                 // Задаем маску для PB2 из вашего файла (равно 1U << 2)
#define LED_SYSTEM_ON     GPIOB->BSRR = GPIO_BSRR_BR2   // Включить системный светодиод (низкий уровень на выходе)
#define LED_SYSTEM_OFF    GPIOB->BSRR = LED_SYSTEM_PIN  // Выключить системный светодиод (высокий уровень на выходе)
#define LED_SYSTEM_TOGGLE GPIOB->ODR ^= LED_SYSTEM_PIN  // Переключить состояние светодиода


//extern volatile u32 ttms;  // Глобальный счётчик миллисекунд (SysTick)
u32 get_tick_ms(void);

void system_clock_config_96MHz(void);
void hw_init(void);
void delay_ms(u32 ms);
void delay_nop(u32 ticks);
void blink_led(u16 freq);                // freq — период переключения в мс
void uint16_to_hex(u16 val, char* out);  // Конвертация uint16_t в 4-символьную HEX-строку
u16  get_random(u16 max);
void random_seed(u32 seed);
u16  decode_utf8(const char** ptr);
void format_time(u32 sec, char* buf);
void uint32_to_str(u32 num, char* buf);
int  strcasecmp(const char* s1, const char* s2);


#endif  // __COMMON_H__