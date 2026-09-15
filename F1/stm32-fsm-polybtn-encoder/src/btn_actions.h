#ifndef BTN_ACTIONS_H
#define BTN_ACTIONS_H

#include <stdint.h>

// Глобальные серийные переменные связи с главным циклом и дисплеем
extern const char* volatile disp_current_string;
extern volatile uint8_t disp_need_update;
extern volatile int32_t disp_counter_1;
extern volatile int32_t disp_counter_2;

void action_main_click_1(void);
void action_main_click_2(void);
void action_main_click_3(void);
void action_main_click_4(void);
void action_main_hold_1(void);
void action_main_hold_2(void);
void action_main_double_5(void);
void action_main_rotate(int8_t direction);
void action_settings_rotate(int8_t direction);
void action_settings_click_3(void);
void check_text_timeout(void); // Контроллер таймаутов строк

#endif // BTN_ACTIONS_H
