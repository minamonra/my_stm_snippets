#include "cp866.h"
#include "cp866_8x8.h"

#define numlen 10
extern uint16_t buf[];

void cp866_print_num(uint32_t num, uint8_t x, uint8_t y, uint16_t fg, uint16_t bg)
{
  uint8_t n[numlen];
  uint8_t *s = n + (numlen - 1);
  *s = 0; // EOL
  do 
  {
    *(--s) = ('0' + num % 10);
    num = num / 10;
  } while (num > 0);
  cp866_print_str((char *)s, x, y, fg, bg);
}

void cp866_print_str(char *str, uint8_t x, uint8_t y, uint16_t fg_color, uint16_t bg_color)
{
  uint8_t i = 0;
  while (*str) {
    st7735_send_char(x + i * 8, y, *str++, fg_color, bg_color);
    ++i;
  }
}

void cp866_char_to_buf(uint8_t x_pos, uint8_t ch, uint16_t fg_color, uint16_t bg_color) {

  uint16_t c = (((uint16_t)ch) * FONT_CP866_8_CHAR_WIDTH); // FONT_CP866_8_CHAR_WIDTH=8
  for (uint8_t i = 0; i < FONT_CP866_8_CHAR_WIDTH; i++) {
    uint8_t data = CP866_8x8[c + i];

    uint16_t index = (i * FONT_CP866_8_CHAR_WIDTH); // FONT_CP866_8_CHAR_WIDTH=8
    for (uint8_t j = 0; j < FONT_CP866_8_CHAR_WIDTH; j++) {

      buf[index++] = (data & 0x80) ? fg_color : bg_color;
      data = data << 1;
    }
  }
}