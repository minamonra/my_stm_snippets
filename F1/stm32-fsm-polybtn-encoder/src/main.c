#include <stddef.h>
#include <stdint.h>
#include "common.h"
#include "button.h"
#include "enc.h"
#include "btn_modes.h"
#include "dispmt16s.h"

extern volatile uint32_t pc13ms;
extern volatile uint32_t pc14ms;

extern const char* volatile disp_current_string;
extern volatile uint8_t disp_need_update;
extern volatile int32_t disp_counter_1;
extern volatile int32_t disp_counter_2;

volatile uint32_t last_lcd_time = 0;
void check_text_timeout(void);

// Отрисовка дисплея по таймеру (40 мс) или мгновенно по disp_need_update
void display_process() {
  if ((ttms - last_lcd_time) >= 40 || disp_need_update) {
    last_lcd_time    = ttms;
    disp_need_update = 0;

    // 1-я строка: Текущее слово или статус направления
    lcd_string16(disp_current_string, 0);

    // 2-я строка: Выводим СЧ1 и СЧ2
    lcd_set_cursor(0, 1, LCD_CURSOR_OFF);
    lcd_print("СЧ1:", 0, 1);
    lcd_print_num2(disp_counter_1);

    lcd_print("  СЧ2:", 6, 1);
    lcd_print_num2(disp_counter_2);
  }
}

int main(void) {
  StartHSE();        // 72 МГц
  hardware_init();   // Инициализация портов, SysTick и AFIO
  lcd_init();        // Инициализация дисплея
  encoder_init();    // Инициализация пинов PB12/PB13
  btn_modes_init();  // Инициализация режимов кнопок

  while (1) {
    button_process();      // опрос кнопок
    encoder_process();     // опрос энкодера
    display_process();     // вывод на дисплей
    check_text_timeout();  // автосброс временных надписей

    blink13led(300);
    blink14led(700);
  }
}