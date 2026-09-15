#include "button.h"
#include "common.h"
#include "btn_modes.h"
#include <stddef.h>

#define DEBOUNCE_MS      40
#define HOLD_MS          500
#define DOUBLE_CLICK_MS  250
#define REPEAT_PERIOD_MS 100

const button_pin_t button_pins[BTN_COUNT] = {
  { GPIOB, 14 }, // Кнопка 1
  { GPIOA, 8  }, // Кнопка 2
  { GPIOA, 11 }, // Кнопка 3
  { GPIOA, 12 }, // Кнопка 4
  { GPIOB, 1  }, // Кнопка 5 (клик энкодера)
};

typedef enum {
  STATE_IDLE,
  STATE_DEBOUNCE_PRESS,
  STATE_PRESSED,
  STATE_DEBOUNCE_RELEASE,
  STATE_WAIT_DOUBLE
} btn_fsm_state_t;

typedef struct {
  btn_fsm_state_t state;
  uint32_t timer;
  uint32_t repeat_timer;
  uint8_t hold_triggered;
} btn_state_t;

static btn_state_t btn_states[BTN_COUNT] = {0};

static uint8_t is_btn_pressed(uint8_t idx) {
  return (button_pins[idx].port->IDR & (1U << button_pins[idx].pin)) == 0;
}

void button_process(void) {
  uint32_t now = ttms;

  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    btn_state_t* s = &btn_states[i];
    uint8_t pressed = is_btn_pressed(i);

    switch (s->state) {
      case STATE_IDLE:
        if (pressed) {
          s->timer = now;
          s->state = STATE_DEBOUNCE_PRESS;
        }
        break;

      case STATE_DEBOUNCE_PRESS:
        if (!pressed) {
          s->state = STATE_IDLE;
        } else if ((now - s->timer) >= DEBOUNCE_MS) {
          s->state = STATE_PRESSED;
          s->timer = now;
          s->hold_triggered = 0;
        }
        break;

      case STATE_PRESSED:
        if (!pressed) {
          s->timer = now;
          s->state = STATE_DEBOUNCE_RELEASE;
        } else if (!s->hold_triggered) {
          if ((now - s->timer) >= HOLD_MS) {
            s->hold_triggered = 1;
            s->repeat_timer = now;
            btn_mode_dispatch(BTN_EVENT_HOLD, i);
          }
        } else {
          if ((now - s->repeat_timer) >= REPEAT_PERIOD_MS) {
            s->repeat_timer = now;
            btn_mode_dispatch(BTN_EVENT_HOLD, i);
          }
        }
        break;

      case STATE_DEBOUNCE_RELEASE:
        if (pressed) {
          s->state = STATE_PRESSED;
        } else if ((now - s->timer) >= DEBOUNCE_MS) {
          if (s->hold_triggered) {
            s->state = STATE_IDLE;
          } else {
            s->timer = now;
            s->state = STATE_WAIT_DOUBLE;
          }
        }
        break;

      case STATE_WAIT_DOUBLE:
        if (pressed) {
          s->timer = now;
          s->state = STATE_DEBOUNCE_PRESS;
          btn_mode_dispatch(BTN_EVENT_DOUBLE_CLICK, i);
          s->state = STATE_IDLE;
        } else if ((now - s->timer) >= DOUBLE_CLICK_MS) {
          btn_mode_dispatch(BTN_EVENT_CLICK, i);
          s->state = STATE_IDLE;
        }
        break;
    }
  }
}

void button_wait_any_press(void) {
  uint8_t pressed_idx = 0xFF;
  while (pressed_idx == 0xFF) {
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
      if (is_btn_pressed(i)) {
        uint32_t start = ttms;
        while ((ttms - start) < DEBOUNCE_MS) { if (!is_btn_pressed(i)) goto skip_press; }
        pressed_idx = i; break;
      }
      skip_press:;
    }
  }
  while (1) {
    if (!is_btn_pressed(pressed_idx)) {
      uint32_t start = ttms;
      while ((ttms - start) < DEBOUNCE_MS) { if (is_btn_pressed(pressed_idx)) goto skip_release; }
      break;
    }
    skip_release:;
  }
  for (uint8_t i = 0; i < BTN_COUNT; i++) btn_states[i].state = STATE_IDLE;
}
