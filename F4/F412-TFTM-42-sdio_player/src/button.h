#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

// ============================================================================
// === КНОПКИ =================================================================
// ============================================================================
// Все кнопки замыкаются на GND -> используем внутренний pull-up, активный уровень = 0

typedef struct {
    GPIO_TypeDef* port;
    uint8_t       pin;          // номер бита 0..15
    uint8_t       repeatEnabled; // 1 = кнопка авто-повторяет клик при удержании (для +/- и т.п.)
} Button_Pin_t;

#define BTN_COUNT 5   // <-- при добавлении кнопки увеличить это число

static const Button_Pin_t buttonPins[BTN_COUNT] = {
    { GPIOB, 7, 1 },  // Кнопка 1
    { GPIOB, 6, 1 },  // Кнопка 2
    { GPIOB, 5, 0 },  // Кнопка 3
    { GPIOB, 4, 0 },  // Кнопка 4 (JTRST по умолчанию)
    { GPIOB, 3, 0 },  // Кнопка 5 (JTDO по умолчанию)
    // Добавление 6-й кнопки:
    // { GPIOB, 2, 0 },  // Кнопка 6
    // и увеличить BTN_COUNT до 6
};


void Buttons_Init(void);                 // Настройка GPIO под кнопки, вызвать один раз при старте
void Button_Process(uint32_t interval_ms); // Вызывать в главном цикле
void Button_WaitAnyPress(void);          // Блокирующее ожидание любой кнопки

// Обработчики кликов — реализовать в другом файле (main.c / app.c)
void Button1_Click(void);
void Button2_Click(void);
void Button3_Click(void);
void Button4_Click(void);
void Button5_Click(void);
// При добавлении кнопки: объявить ButtonN_Click() тут
// и добавить её в таблицу clickHandlers[] в button.c

#endif