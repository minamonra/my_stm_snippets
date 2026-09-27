#ifndef __COMMON_H__
#define __COMMON_H__

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t

// Универсальный макрос сдвига конфигурации (2 бита на пин, для любого пина 0-15)
#define PIN_CONF(pin, val)   ((val) << ((pin) * 2U))

// Режимы работы (MODER)
#define MODE_INPUT           0x00U // Вход
#define MODE_OUTPUT          0x01U // Выход общего назначения
#define MODE_AF              0x02U // Альтернативная функция
#define MODE_ANALOG          0x03U // Аналоговый режим

// Скорость портов (OSPEEDR)
#define SPEED_LOW            0x00U // Low speed
#define SPEED_MEDIUM         0x01U // Medium speed
#define SPEED_HIGH           0x02U // High speed
#define SPEED_VERY_HIGH      0x03U // Very high speed

// Подтяжка (PUPDR)
#define PULL_NONE            0x00U // Без подтяжки
#define PULL_UP              0x01U // Pull-up к VDD
#define PULL_DOWN            0x02U // Pull-down к GND


#define LED_SYSTEM_PIN    GPIO_BSRR_BS2                 // Задаем маску для PB2 из вашего файла (равно 1U << 2)
#define LED_SYSTEM_OFF    GPIOB->BSRR = GPIO_BSRR_BR2   // Включить системный светодиод (низкий уровень на выходе)
#define LED_SYSTEM_ON     GPIOB->BSRR = LED_SYSTEM_PIN  // Выключить системный светодиод (высокий уровень на выходе)
#define LED_SYSTEM_TOGGLE GPIOB->ODR ^= LED_SYSTEM_PIN  // Переключить состояние светодиода

void clock_init(void);
void hw_init(void);

#endif  // __COMMON_H__