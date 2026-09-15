#ifndef BTN_MODES_H
#define BTN_MODES_H

#include <stdint.h>
#include "button.h"

typedef enum {
  BTN_MODE_MAIN_DISPLAY = 0,
  BTN_MODE_SETTINGS,
  BTN_MODE_COUNT
} btn_mode_t;

typedef void (*btn_callback_t)(void);
typedef void (*enc_callback_t)(int8_t direction);

typedef struct {
  const btn_callback_t* on_click;
  const btn_callback_t* on_hold;
  const btn_callback_t* on_double_click;
  enc_callback_t on_rotate;
} btn_handlers_t;

void btn_modes_init(void);
void btn_mode_set(btn_mode_t mode);
btn_mode_t btn_mode_get(void);
void btn_mode_dispatch(btn_event_t event, uint8_t btn_idx);
void enc_mode_dispatch(int8_t direction);

#endif // BTN_MODES_H
