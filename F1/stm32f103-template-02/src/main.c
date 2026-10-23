
#include "main.h"
#include "common.h"

// Дескриптор задачи обработки ИК-приёмника
TaskHandle_t ir_task_handle = NULL;

// Переменные, определённые в ir.c
extern volatile uint32_t debug_pulse;
extern volatile uint32_t debug_pause;
extern volatile uint32_t debug_stor;

//
// Вывод 32-битного значения в HEX:
// ровно 8 символов.
//
static void lcd_print_hex32(uint32_t value) {
  static const char hex_chars[] = "0123456789ABCDEF";

  for (int i = 7; i >= 0; i--) {
    lcd_char(hex_chars[(value >> (i * 4)) & 0x0F]);
  }
}

//
// Вывод десятичного числа ровно 5-ю цифрами.
// Например: 9000 -> "09000".
// Значения больше 99999 ограничиваются 99999.
//
static void lcd_print_dec5(uint32_t value) {
  char digits[5];

  if (value > 99999U)
    value = 99999U;

  for (int i = 4; i >= 0; i--) {
    digits[i] = (char)('0' + (value % 10U));
    value /= 10U;
  }

  for (int i = 0; i < 5; i++) {
    lcd_char(digits[i]);
  }
}

// Задача обработки ИК-команд и обновления дисплея
static void vIrProcessingTask(void* pvParameters) {
  (void)pvParameters;

  //
  // Инициализируем приёмник после запуска планировщика.
  // Сам дисплей и тактирование уже настроены в main().
  //
  ir_receiver_init();

  for (;;) {
    //
    // Ждём уведомления от EXTI0_IRQHandler()
    // или обновляем диагностические данные через 150 мс.
    //
    (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(150));

    // Строка 1: длительности последних интервалов, мкс
    lcd_set_cursor(0, 0, LCD_CURSOR_OFF);

    lcd_char('P');
    lcd_char(':');
    lcd_print_dec5(debug_pulse);

    lcd_char(' ');
    lcd_char('W');
    lcd_char(':');
    lcd_print_dec5(debug_pause);

    // Строка 2: принятый пакет и байт команды
    lcd_set_cursor(0, 1, LCD_CURSOR_OFF);

    // Строка 2: полный пакет и код команды
    lcd_set_cursor(0, 1, LCD_CURSOR_OFF);

    lcd_print_hex32(debug_stor);  // Столбцы 0–7
    lcd_print(" C:", 8, 1);       // Столбцы 8–10

    uint32_t command        = ik_out;
    static const char hex[] = "0123456789ABCDEF";

    lcd_char(hex[(command >> 4) & 0x0F]);  // Столбец 11
    lcd_char(hex[command & 0x0F]);         // Столбец 12

    lcd_print("   ", 13, 1);               // Очистка столбцов 13–15

    // Стираем возможные остатки предыдущего текста
    lcd_print("  ", 14, 1);
  }
}

// Задача мигания светодиодом PC13
static void vLedBlinkTask(void* pvParameters) {
  (void)pvParameters;

  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

  GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
  GPIOC->CRH |= GPIO_CRH_MODE13_1;

  for (;;) {
    GPIOC->ODR ^= GPIO_ODR_ODR13;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

int main(void) {
  // Настройка системной частоты
  clock_init();

  // Инициализация дисплея
  lcd_pins_init();
  lcd_init();
  lcd_clear();

  // Настройка группировки приоритетов NVIC
  NVIC_SetPriorityGrouping(3);

  // Создание задач
  xTaskCreate(vIrProcessingTask, "IR_Proc", configMINIMAL_STACK_SIZE + 48, NULL, 3, &ir_task_handle);

  xTaskCreate(vLedBlinkTask, "LED_Blink", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  // Запуск FreeRTOS
  vTaskStartScheduler();

  // Сюда выполнение не должно попасть
  for (;;) {
  }
}
