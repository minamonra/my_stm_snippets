#include "btn_actions.h"
#include "btn_modes.h"
#include "common.h"
#include <stddef.h>

const char* volatile disp_current_string = "ОЖИДАНИЕ КНОПКИ";
volatile uint8_t disp_need_update = 1;
volatile int32_t disp_counter_1 = 0;
volatile int32_t disp_counter_2 = 0;

static const char* disp_last_base_string = "ОЖИДАНИЕ КНОПКИ";
static uint32_t disp_timeout_ms = 0;

void action_main_click_1(void) { disp_last_base_string = "БЕТЕЛГЕЙЗЕ"; disp_current_string = disp_last_base_string; disp_timeout_ms = 0; disp_need_update = 1; }
void action_main_click_2(void) { disp_last_base_string = "ПИПИДАСТР";  disp_current_string = disp_last_base_string; disp_timeout_ms = 0; disp_need_update = 1; }
void action_main_click_3(void) { disp_last_base_string = "ВОЛГОГРАД";  disp_current_string = disp_last_base_string; disp_timeout_ms = 0; disp_need_update = 1; }
void action_main_click_4(void) { disp_last_base_string = "ГДЕ КРОТ";   disp_current_string = disp_last_base_string; disp_timeout_ms = 0; disp_need_update = 1; }

void action_main_hold_1(void) {
  disp_counter_1--;
  if (disp_counter_1 < 0) disp_counter_1 = 99;
  disp_current_string = "ИЗМЕНЕНИЕ СЧ1";
  disp_timeout_ms = ttms + 500; // Статус удержания держится 500 мс после отпускания
  disp_need_update = 1;
}

void action_main_hold_2(void) {
  disp_counter_1++;
  if (disp_counter_1 > 99) disp_counter_1 = 0;
  disp_current_string = "ИЗМЕНЕНИЕ СЧ1";
  disp_timeout_ms = ttms + 500;
  disp_need_update = 1;
}

void action_main_double_5(void) {
  disp_counter_1 = 0;
  disp_counter_2 = 0;
  disp_last_base_string = "СБРОС СЧЕТЧИКОВ";
  disp_current_string = disp_last_base_string;
  disp_timeout_ms = ttms + 1000; // Статус сброса держим 1 секунду
  disp_need_update = 1;
}

void action_main_rotate(int8_t direction) {
  disp_counter_2 += direction;
  if (disp_counter_2 > 99) disp_counter_2 = 0;
  if (disp_counter_2 < 0)  disp_counter_2 = 99;

  // Визуальный отклик направления по знаку импульса
  disp_current_string = (direction > 0) ? "ЭНК ВПРАВО" : "ЭНК ВЛЕВО";

  disp_timeout_ms = ttms + 300; // Вспышка направления на 300 мс
  disp_need_update = 1;
}

void action_settings_rotate(int8_t direction) {
  disp_current_string = (direction > 0) ? "НАСТРОЙКА +" : "НАСТРОЙКА -";
  disp_need_update = 1;
}

void action_settings_click_3(void) {
  btn_mode_set(BTN_MODE_MAIN_DISPLAY);
  disp_last_base_string = "ГЛАВНЫЙ ЭКРАН";
  disp_current_string = disp_last_base_string;
  disp_timeout_ms = 0;
  disp_need_update = 1;
}

void check_text_timeout(void) {
  if (disp_timeout_ms != 0 && ttms >= disp_timeout_ms) {
    disp_timeout_ms = 0;
    disp_current_string = disp_last_base_string;
    disp_need_update = 1;
  }
}
