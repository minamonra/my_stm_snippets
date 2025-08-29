#include "dispmt16s.h"
#define SDELAY 100
#define LDELAY 2500

// === Упрощенная задержка для HD44780 ===
void delay_nop(uint16_t count) {
    for(volatile int i = 0; i < count; i++); // ~1-2 мкс при 72 МГц
}

// === Исправленная функция отправки ниблов ===
void lcdNibble(uint8_t nibble) {
  // Устанавливаем данные
  if (nibble & 0x08) D71; else D70;
  if (nibble & 0x04) D61; else D60;
  if (nibble & 0x02) D51; else D50;
  if (nibble & 0x01) D41; else D40;
  // Импульс E
  EN1;
  delay_nop(SDELAY); // Очень короткая задержка ~1 мкс
  EN0;
  delay_nop(SDELAY); // Задержка между операциями
}

// === Исправленная функция отправки данных ===
void lcdSend(uint8_t isCommand, uint8_t data) {
  if (isCommand) RS0; else RS1;
  lcdNibble(data >> 4);   // старшие 4 бита
  lcdNibble(data & 0x0F); // младшие 4 бита
  delay_nop(1500);
  //delay_ms(1); // Задержка между командами
}

void lcdCommand(uint8_t cmd) { lcdSend(1, cmd); }
void lcdChar(char chr)       { lcdSend(0, chr); }
void lcdString(const char *s){ while(*s) lcdChar(*s++); }

// === Исправленная инициализация ===
void lcd_init(void) {
  delay_nop(20000);// Ждем после подачи питания
  // ВАЖНО: Инициализация должна быть в 8-битном режиме!
  RS0; // RS = 0 для команд
  EN1; delay_nop(SDELAY); EN0; delay_nop(LDELAY);
  EN1; delay_nop(SDELAY); EN0; delay_nop(LDELAY);
  EN1; delay_nop(SDELAY); EN0; delay_nop(LDELAY);
  //// Переход в 4-битный режим: посылаем 0x02
  //if (0x02 & 0x08) D71; else D70;
  //if (0x02 & 0x04) D61; else D60;
  //if (0x02 & 0x02) D51; else D50;
  //if (0x02 & 0x01) D41; else D40;
  D70;D60;D51;D40;
  EN1; delay_nop(SDELAY); EN0; delay_nop(LDELAY);
  lcdCommand(0x2A);		//Настройка правильного режима ЖКИ
  lcdCommand(0x0C);		//Включение индикатора, курсор выключен
  lcdCommand(0x01);		//Очистка индикатора
  lcdCommand(0x06);		//Установка режима ввода данных: сдвигать курсор вправо
  delay_nop(LDELAY);
}
