#include "btn_modes.h"
#include "btn_actions.h"
#include <stddef.h>

static btn_mode_t current_mode = BTN_MODE_MAIN_DISPLAY;

static const btn_callback_t main_clicks[BTN_COUNT] = {
  action_main_click_1, action_main_click_2, action_main_click_3, action_main_click_4, NULL
};
static const btn_callback_t main_holds[BTN_COUNT]  = {
  action_main_hold_1,  action_main_hold_2,  NULL,                NULL,                NULL
};
static const btn_callback_t main_doubles[BTN_COUNT] = {
  NULL,                NULL,                NULL,                NULL,                action_main_double_5
};

static const btn_callback_t settings_clicks[BTN_COUNT] = {
  NULL, NULL, action_settings_click_3, NULL, NULL
};
static const btn_callback_t settings_holds[BTN_COUNT]  = { NULL, NULL, NULL, NULL, NULL };
static const btn_callback_t settings_doubles[BTN_COUNT] = { NULL, NULL, NULL, NULL, NULL };

static const btn_handlers_t mode_matrix[BTN_MODE_COUNT] = {
  [BTN_MODE_MAIN_DISPLAY] = {
    .on_click = main_clicks,
    .on_hold = main_holds,
    .on_double_click = main_doubles,
    .on_rotate = action_main_rotate
  },
  [BTN_MODE_SETTINGS] = {
    .on_click = settings_clicks,
    .on_hold = settings_holds,
    .on_double_click = settings_doubles,
    .on_rotate = action_settings_rotate
  }
};

void btn_modes_init(void) {
  current_mode = BTN_MODE_MAIN_DISPLAY;
}

void btn_mode_set(btn_mode_t mode) {
  if (mode < BTN_MODE_COUNT) current_mode = mode;
}

btn_mode_t btn_mode_get(void) {
  return current_mode;
}

void btn_mode_dispatch(btn_event_t event, uint8_t btn_idx) {
  if (btn_idx >= BTN_COUNT) return;
  const btn_handlers_t* h = &mode_matrix[current_mode];
  btn_callback_t handler = NULL;

  switch (event) {
    case BTN_EVENT_CLICK:        if (h->on_click)        handler = h->on_click[btn_idx];        break;
    case BTN_EVENT_HOLD:         if (h->on_hold)         handler = h->on_hold[btn_idx];         break;
    case BTN_EVENT_DOUBLE_CLICK: if (h->on_double_click) handler = h->on_double_click[btn_idx]; break;
  }
  if (handler) handler();
}

void enc_mode_dispatch(int8_t direction) {
  const btn_handlers_t* h = &mode_matrix[current_mode];
  if (h->on_rotate) h->on_rotate(direction);
}
