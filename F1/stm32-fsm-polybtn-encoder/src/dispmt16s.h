#ifndef DISPMT16S_H
#define DISPMT16S_H

#include "stm32f103xb.h"
#include <stdint.h>

// Команды контроллера An6866
#define LCD_CMD_CLEAR        0x01   // Очистить память DDRAM дисплея
#define LCD_CMD_MODE_4BIT    0x2A   // 4-битная шина, 2 строки, альтернативный шрифт МЭЛТ
#define LCD_CMD_DISP_ON      0x0C   // Экран включен, аппаратный курсор скрыт
#define LCD_CMD_ENTRY_INC    0x06   // Автоинкремент адреса (курсор сдвигается вправо)
#define LCD_CMD_SET_DDRAM    0x80   // Маска установки адреса памяти строк

// Макросы управления пинами
#define RS1 GPIOA->BSRR |= GPIO_BSRR_BS15
#define RS0 GPIOA->BSRR |= GPIO_BSRR_BR15
#define EN1 GPIOB->BSRR |= GPIO_BSRR_BS3
#define EN0 GPIOB->BSRR |= GPIO_BSRR_BR3

#define D41 GPIOB->BSRR |= GPIO_BSRR_BS4
#define D40 GPIOB->BSRR |= GPIO_BSRR_BR4
#define D51 GPIOB->BSRR |= GPIO_BSRR_BS5
#define D50 GPIOB->BSRR |= GPIO_BSRR_BR5
#define D61 GPIOB->BSRR |= GPIO_BSRR_BS8
#define D60 GPIOB->BSRR |= GPIO_BSRR_BR8
#define D71 GPIOB->BSRR |= GPIO_BSRR_BS9
#define D70 GPIOB->BSRR |= GPIO_BSRR_BR9

typedef enum {
  LCD_CURSOR_OFF = 0,
  LCD_CURSOR_NORMAL,
  LCD_CURSOR_BLINK
} lcd_cursor_t;

// Все функции строчными буквами, комментарии строго //
void lcd_init(void);
void lcd_command(uint8_t cmd);
void lcd_char(char chr);
void lcd_clear(void);
void lcd_clear_via_chars(void);
void lcd_set_cursor(uint8_t col, uint8_t line, lcd_cursor_t mode);

void lcd_print(const char *s, uint8_t col, uint8_t line);
void lcd_string16(const char *s, uint8_t line);
void lcd_print_num2(int32_t num);

#endif // DISPMT16S_H
