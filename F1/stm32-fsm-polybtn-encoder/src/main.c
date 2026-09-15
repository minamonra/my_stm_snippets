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

void check_text_timeout(void);

int main(void) {
  StartHSE();       // Разгон ядра до 72 МГц
  hardware_init();  // Инициализация портов, SysTick и AFIO

  lcd_init();        // Тройной запуск An6866
  encoder_init();    // Инициализация пинов PB12/PB13
  btn_modes_init();  // Инициализация движка режимов

  pc13ms = 500;
  pc14ms = 1000;

  uint32_t last_lcd_time = 0;

  while(1) {
    // Подкапотные неблокирующие рантайм механизмы опроса железа
    button_process();  // Клавиатура
    encoder_process(); // Энкодер Грея

    check_text_timeout(); // Автосброс временных надписей

    // Отрисовка дисплея по таймеру (40 мс) или мгновенно по disp_need_update
    if ((ttms - last_lcd_time) >= 40 || disp_need_update) {
      last_lcd_time = ttms;
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

    if (pc13ms == 0) { LED1TOGGLE; pc13ms = 500; }
    if (pc14ms == 0) { LED2TOGGLE; pc14ms = 1000; }
  }
}
