#ifndef BUTTON_H
#define BUTTON_H

#include "stm32f103xb.h"
#include <stdint.h>

#define BTN_COUNT 5

// Твои оригинальные коды состояний переходов энкодера (измени, если в проекте другие)
#define ENCUPCNT  0x02  // Код шага вперед
#define ENCDNCNT  0x01  // Код шага назад

typedef enum {
  BTN_EVENT_CLICK,
  BTN_EVENT_HOLD,
  BTN_EVENT_DOUBLE_CLICK
} btn_event_t;

typedef struct {
  GPIO_TypeDef* port;
  uint8_t pin;
} button_pin_t;

extern const button_pin_t button_pins[BTN_COUNT];

void button_init(void);
void button_process(void);
void encoder_init(void);
void encoder_poll(void); // Вызывать строго из SysTick_Handler
void button_wait_any_press(void);

#endif // BUTTON_H
