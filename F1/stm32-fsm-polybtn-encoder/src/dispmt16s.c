#include "dispmt16s.h"
#include "common.h"
#include <stddef.h>

// Дисплей MT16S-2D-2YLG
// на основе примеров отсюда:
// https://www.melt.com.ru/shop/mt-16s2d-2ylg-2.html


// Константы таймингов для delay_nop (1 в 1 из оригинального серийного проекта)
#define SDELAY 36     // ~500ns (строб EN)
#define LDELAY 720    // ~10μs (пауза после ниббла)

static void lcd_nibble(uint8_t nibble) {
  if (nibble & 0x08) D71; else D70;
  if (nibble & 0x04) D61; else D60;
  if (nibble & 0x02) D51; else D50;
  if (nibble & 0x01) D41; else D40;

  EN1;
  delay_nop(SDELAY);
  EN0;
  delay_nop(SDELAY);
}

static void lcd_send(uint8_t is_command, uint8_t data) {
  if (is_command) RS0; else RS1;

  lcd_nibble(data >> 4);      // Старший ниббл
  lcd_nibble(data & 0x0F);    // Младший ниббл

  if (is_command && data == LCD_CMD_CLEAR) {
    delay_nop(LDELAY * 4);    // Увеличенная задержка после очистки экрана
  } else {
    delay_nop(LDELAY);
  }
}

void lcd_command(uint8_t cmd) {
  lcd_send(1, cmd);
}

void lcd_char(char chr) {
  lcd_send(0, chr);
}

void lcd_clear(void) {
  lcd_command(LCD_CMD_CLEAR);
}

void lcd_set_cursor(uint8_t col, uint8_t line, lcd_cursor_t mode) {
  uint8_t addr = (line == 0) ? 0x00 : 0x40;
  addr += col;
  lcd_command(LCD_CMD_SET_DDRAM | addr);

  if (mode == LCD_CURSOR_OFF) {
    lcd_command(0x0C);
  } else if (mode == LCD_CURSOR_NORMAL) {
    lcd_command(0x0E);
  } else if (mode == LCD_CURSOR_BLINK) {
    lcd_command(0x0F);
  }
}

void lcd_print(const char *s, uint8_t col, uint8_t line) {
  lcd_set_cursor(col, line, LCD_CURSOR_OFF);
  uint32_t i = 0;
  while (s[i] != '\0') {
    unsigned char c = (unsigned char)s[i];
    if (c < 0x80) {
      lcd_char(c);
      i += 1;
    } else {
      unsigned char c2 = (unsigned char)s[i + 1];
      if (c2 == '\0') break;
      if (c == 0xD0) {
        if (c2 >= 0x90 && c2 <= 0xBF) lcd_char(c2 + 0x30); // А–п
        else if (c2 == 0x81) lcd_char(0xA8);              // Ё
      } else if (c == 0xD1) {
        if (c2 >= 0x80 && c2 <= 0x8F) lcd_char(c2 + 0x70); // р–я
        else if (c2 == 0x91) lcd_char(0xB8);              // ё
      }
      i += 2;
    }
  }
}

void lcd_string16(const char *s, uint8_t line) {
  lcd_set_cursor(0, line, LCD_CURSOR_OFF);
  uint32_t i = 0;
  uint8_t count = 0;

  while (s[i] != '\0' && count < 16) {
    unsigned char c = (unsigned char)s[i];
    if (c < 0x80) {
      lcd_char(c);
      i += 1;
    } else {
      unsigned char c2 = (unsigned char)s[i + 1];
      if (c2 == '\0') break;
      if (c == 0xD0) {
        if (c2 >= 0x90 && c2 <= 0xBF) lcd_char(c2 + 0x30);
        else if (c2 == 0x81) lcd_char(0xA8);
      } else if (c == 0xD1) {
        if (c2 >= 0x80 && c2 <= 0x8F) lcd_char(c2 + 0x70);
        else if (c2 == 0x91) lcd_char(0xB8);
      }
      i += 2;
    }
    count++;
  }
  while (count < 16) {
    lcd_char(' ');
    count++;
  }
}

void lcd_print_num2(int32_t num) {
  if (num < 0)  num = 0;
  if (num > 99) num = 99;
  lcd_char((num / 10) + '0');
  lcd_char((num % 10) + '0');
}

void lcd_clear_via_chars(void) {
  lcd_string16("                ", 0);
  lcd_string16("                ", 1);
}

static void lcd_init_single(void) {
  RS0;
  for (int i = 0; i < 3; i++) {
    EN1;
    delay_nop(SDELAY);
    EN0;
    delay_nop(LDELAY);
  }
  D70; D60; D51; D40;
  EN1;
  delay_nop(SDELAY);
  EN0;
  delay_nop(LDELAY);

  lcd_command(LCD_CMD_MODE_4BIT);
  lcd_command(LCD_CMD_DISP_ON);
  lcd_clear();
  lcd_command(LCD_CMD_ENTRY_INC);
}

void lcd_init(void) {
  delay_nop(20000);   // Ожидание стабилизации питания
  lcd_init_single();
  delay_nop(30000);   // Тройной запуск по спецификации An6866
  lcd_init_single();
  delay_nop(30000);
  lcd_init_single();
}
