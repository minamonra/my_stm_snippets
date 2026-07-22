#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include "stm32f4xx.h"

#define BTN_COUNT 5

typedef struct {
    GPIO_TypeDef* port;
    uint8_t       pin;          // номер бита 0..15
    uint8_t       repeatEnabled; // 1 = кнопка авто-повторяет клик при удержании
} Button_Pin_t;

extern const Button_Pin_t buttonPins[BTN_COUNT];

void buttons_init(void);
void button_process(uint32_t interval_ms);
void button_waitanypress(void);

void button1_click(void);
void button1_hold(void);
void button2_click(void);
void button3_click(void);
void button3_hold(void);
void button4_click(void);
void button5_click(void);

#endif // BUTTON_H