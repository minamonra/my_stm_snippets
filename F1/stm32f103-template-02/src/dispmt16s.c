#include "dispmt16s.h"
#include "common.h"
#include <stddef.h>

// Подключаем заголовочные файлы FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

//
// Тайминги дисплея (для частоты ядра 72 МГц):
// SDELAY = 36 итераций нопов дает ~500 нс, что удовлетворяет требованию 
// минимальной ширины импульса EN (t_w = 220-500 нс для дисплеев типа HD44780/An6866).
// LDELAY = 720 итераций дает ~10 мкс — время фиксации данных контроллером дисплея.
//
#define SDELAY 36    // ~500ns (ширина строба EN)
#define LDELAY 720   // ~10µs (пауза после передачи ниббла)
#define IDELAY 30000 // задержка при старте
// 
// Если планировщик RTOS запущен, задача блокируется (уступает процессор).
// Если вызвано до старта RTOS (в main), выполняется активное ожидание (NOP).
// @param ms_delay Время задержки в миллисекундах.
// @param nop_count Альтернативное количество NOP для работы до старта RTOS.
//
static void lcd_smart_delay(TickType_t ms_delay, uint32_t nop_count) {
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    // Планировщик активен, переводим задачу в состояние Blocked
    vTaskDelay(pdMS_TO_TICKS(ms_delay));
  } else {
    // Планировщик еще не запущен, используем пустой цикл
    delay_nop(nop_count);
  }
}

void lcd_pins_init(void) {
  // Включение тактирования портов GPIOA, GPIOB и альтернативных функций (AFIO)
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

  // Освобождаем пины JTAG (PA15, PB3, PB4) для использования в качестве обычных GPIO
  uint32_t temp = AFIO->MAPR;
  temp &= ~AFIO_MAPR_SWJ_CFG;
  temp |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
  AFIO->MAPR = temp;

  // Настройка PA15 (RS) на выход Push-Pull, 50 МГц
  GPIOA->CRH &= ~(GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
  GPIOA->CRH |= GPIO_CRH_MODE15_1 | GPIO_CRH_MODE15_0;

  // Настройка PB3 (EN), PB4 (D4), PB5 (D5) на выход Push-Pull, 50 МГц
  GPIOB->CRL &= ~(GPIO_CRL_MODE3 | GPIO_CRL_CNF3 | GPIO_CRL_MODE4 |
                  GPIO_CRL_CNF4 | GPIO_CRL_MODE5 | GPIO_CRL_CNF5);
  GPIOB->CRL |= (GPIO_CRL_MODE3_1 | GPIO_CRL_MODE3_0) |
                (GPIO_CRL_MODE4_1 | GPIO_CRL_MODE4_0) |
                (GPIO_CRL_MODE5_1 | GPIO_CRL_MODE5_0);

  // Настройка PB8 (D6), PB9 (D7) на выход Push-Pull, 50 МГц
  GPIOB->CRH &=
      ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8 | GPIO_CRH_MODE9 | GPIO_CRH_CNF9);
  GPIOB->CRH |= (GPIO_CRH_MODE8_1 | GPIO_CRH_MODE8_0) |
                (GPIO_CRH_MODE9_1 | GPIO_CRH_MODE9_0);
}

static void lcd_nibble(uint8_t nibble) {
  // Запрещаем переключение задач прерываниями на время стробирования пина EN.
  // Это гарантирует стабильность импульса t_w и исключает "расщепление" данных.
  taskENTER_CRITICAL();

  // Выставляем 4 бита данных на линии дисплея D4-D7
  if (nibble & 0x08)
    D71;
  else
    D70;
  if (nibble & 0x04)
    D61;
  else
    D60;
  if (nibble & 0x02)
    D51;
  else
    D50;
  if (nibble & 0x01)
    D41;
  else
    D40;

  // Формируем строб EN (минимальное время удержания высокого уровня)
  EN1;
  delay_nop(SDELAY);
  EN0;
  delay_nop(SDELAY);

  // Разрешаем планировщику работу в штатном режиме
  taskEXIT_CRITICAL();
}

static void lcd_send(uint8_t is_command, uint8_t data) {
  // Установка линии RS: 0 - команда, 1 - данные
  if (is_command)
    RS0;
  else
    RS1;

  // Передача байта по 4-битной шине: сначала старший полубайт, затем младший
  lcd_nibble(data >> 4);
  lcd_nibble(data & 0x0F);

  // Команде очистки дисплея требуется значительно больше времени на выполнение
  if (is_command && data == LCD_CMD_CLEAR) {
    // Очистка экрана занимает ~1.64 мс. Блокируем задачу на 2 мс через умную задержку
    lcd_smart_delay(2, LDELAY * 4);
  } else {
    // Обычная операция занимает ~40 мкс. Быстрее подождать в NOP, чем переключать контекст RTOS
    delay_nop(LDELAY);
  }
}

void lcd_command(uint8_t cmd) { lcd_send(1, cmd); }

void lcd_char(char chr) { lcd_send(0, chr); }

void lcd_clear(void) { lcd_command(LCD_CMD_CLEAR); }

void lcd_set_cursor(uint8_t col, uint8_t line, lcd_cursor_t mode) {
  // Вычисление адреса DDRAM (0x00 для 1-й строки, 0x40 для 2-й строки)
  uint8_t addr = (line == 0) ? 0x00 : 0x40;
  addr += col;
  lcd_command(LCD_CMD_SET_DDRAM | addr);

  // Управление отображением курсора
  if (mode == LCD_CURSOR_OFF) {
    lcd_command(0x0C);  // Экран включен, курсор выключен
  } else if (mode == LCD_CURSOR_NORMAL) {
    lcd_command(0x0E);  // Курсор в виде подчеркивания
  } else if (mode == LCD_CURSOR_BLINK) {
    lcd_command(0x0F);  // Мигающий курсор-блок
  }
}

void lcd_print(const char* s, uint8_t col, uint8_t line) {
  lcd_set_cursor(col, line, LCD_CURSOR_OFF);
  uint32_t i = 0;
  // Парсинг UTF-8 строки и конвертация кириллицы под таблицу символов МЭЛТ/An6866
  while (s[i] != '\0') {
    unsigned char c = (unsigned char)s[i];
    if (c < 0x80) {
      lcd_char(c);  // Стандартные ASCII символы
      i += 1;
    } else {
      unsigned char c2 = (unsigned char)s[i + 1];
      if (c2 == '\0')
        break;
      if (c == 0xD0) {
        if (c2 >= 0x90 && c2 <= 0xBF)
          lcd_char(c2 + 0x30);  // А–п
        else if (c2 == 0x81)
          lcd_char(0xA8);       // Буква Ё
      } else if (c == 0xD1) {
        if (c2 >= 0x80 && c2 <= 0x8F)
          lcd_char(c2 + 0x70);  // р–я
        else if (c2 == 0x91)
          lcd_char(0xB8);       // Буква ё
      }
      i += 2;
    }
  }
}

void lcd_string16(const char* s, uint8_t line) {
  lcd_set_cursor(0, line, LCD_CURSOR_OFF);
  uint32_t i    = 0;
  uint8_t count = 0;

  // Вывод строки с жестким ограничением в 16 символов
  while (s[i] != '\0' && count < 16) {
    unsigned char c = (unsigned char)s[i];
    if (c < 0x80) {
      lcd_char(c);
      i += 1;
    } else {
      unsigned char c2 = (unsigned char)s[i + 1];
      if (c2 == '\0')
        break;
      if (c == 0xD0) {
        if (c2 >= 0x90 && c2 <= 0xBF)
          lcd_char(c2 + 0x30);
        else if (c2 == 0x81)
          lcd_char(0xA8);
      } else if (c == 0xD1) {
        if (c2 >= 0x80 && c2 <= 0x8F)
          lcd_char(c2 + 0x70);
        else if (c2 == 0x91)
          lcd_char(0xB8);
      }
      i += 2;
    }
    count++;
  }
  // Добиваем оставшуюся часть строки пробелами для очистки старых данных
  while (count < 16) {
    lcd_char(' ');
    count++;
  }
}

void lcd_print_num2(int32_t num) {
  if (num < 0) num = 0;
  if (num > 99) num = 99;
  lcd_char((num / 10) + '0');  // Десятки
  lcd_char((num % 10) + '0');  // Единицы
}

void lcd_clear_via_chars(void) {
  lcd_string16("                ", 0);
  lcd_string16("                ", 1);
}

static void lcd_init_single(void) {
  RS0;
  // Программный сброс интерфейса: посылаем трижды команду инициализации
  for (int i = 0; i < 3; i++) {
    EN1;
    delay_nop(SDELAY);
    EN0;
    delay_nop(LDELAY);
  }
  // Перевод контроллера в 4-битный режим (выставляем D5)
  D70;
  D60;
  D51;
  D40;
  EN1;
  delay_nop(SDELAY);
  EN0;
  delay_nop(LDELAY);

  // Конфигурация функций дисплея
  lcd_command(LCD_CMD_MODE_4BIT);
  lcd_command(LCD_CMD_DISP_ON);
  lcd_clear();
  lcd_command(LCD_CMD_ENTRY_INC);
}

void lcd_init(void) {
  // Поскольку инициализация вызывается в main() ДО vTaskStartScheduler(),
  // здесь используются "глухие" задержки. Это безопасно, так как ОС еще не запущена.
  delay_nop(IDELAY);  // Ожидание стабилизации питания дисплея при включении (>15мс)
  lcd_init_single();

  delay_nop(IDELAY);  // Пауза по спецификации
  lcd_init_single();

  delay_nop(IDELAY);
  lcd_init_single();
}
