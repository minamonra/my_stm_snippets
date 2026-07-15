#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include "stm32f4xx.h"

// ============================================================================
// === КНОПКИ =================================================================
// ============================================================================
// Все кнопки замыкаются на GND -> используем внутренний pull-up, активный уровень = 0

typedef struct {
    GPIO_TypeDef* port;
    uint8_t       pin;          // номер бита 0..15
    uint8_t       repeatEnabled; // 1 = кнопка авто-повторяет клик при удержании
} Button_Pin_t;

#define BTN_COUNT 5

static const Button_Pin_t buttonPins[BTN_COUNT] = {
    { GPIOB, 7, 1 },  // Кнопка 1 - ПРЕДЫДУЩИЙ ТРЕК
    { GPIOB, 6, 1 },  // Кнопка 2 - ВОСПР/ПАУЗА
    { GPIOB, 5, 0 },  // Кнопка 3 - СЛЕДУЮЩИЙ ТРЕК
    { GPIOB, 4, 0 },  // Кнопка 4 (зарезервирована)
    { GPIOB, 3, 0 },  // Кнопка 5 (зарезервирована)
};

void buttons_init(void);
void button_process(uint32_t interval_ms);
void button_waitanypress(void);

// Обработчики кликов
void button1_click(void);
void button2_click(void);
void button3_click(void);
void button4_click(void);
void button5_click(void);

#endif