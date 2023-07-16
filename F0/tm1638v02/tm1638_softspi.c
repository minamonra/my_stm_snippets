#include "common.h"
#include "tm1638_softspi.h"

unsigned char code_tab[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71};

void tm1638_write_byte(unsigned char data) {
  unsigned char i;
  for (i = 0; i < 8; i++) {
    SPI3_CLK_CLR; 
    if (data & 0x01)
      SPI3_DIO_SET;
    else
      SPI3_DIO_CLR; 
    data >>= 1;
    SPI3_CLK_SET; 
  }
}

unsigned char tm1638_read_byte(void) {
  unsigned char i;
  unsigned char temp = 0;
  SPI3_DIO_SET;
  for (i = 0; i < 8; i++) {
    temp >>= 1;
    SPI3_CLK_CLR;
    if (SPI3_CHECK_DIO)
      temp |= 0x80;
    SPI3_CLK_SET;
  }
  return temp;
}

void tm1638_write_com(unsigned char cmd) {
  SPI3_CS_CLR;
  tm1638_write_byte(cmd);
  SPI3_CS_SET;
}

unsigned char tm1638_read_key(void) {
  unsigned char c[4], i, key_value = 0;
  SPI3_CS_CLR;
  tm1638_write_byte(0x42);
  for (i = 0; i < 4; i++)
    c[i] = tm1638_read_byte();
  SPI3_CS_SET;
  for (i = 0; i < 4; i++)
    key_value |= c[i] << i;
  for (i = 0; i < 8; i++)
    if ((0x01 << i) == key_value)
      break;
  return i;
}

void tm1638_write_data(unsigned char add, unsigned char data) {
  tm1638_write_com(0x44);
  SPI3_CS_CLR;
  tm1638_write_byte(0xc0 | add);
  tm1638_write_byte(data);
  SPI3_CS_SET;
}

void tm1638_write_led(unsigned char led_flag) {
  unsigned char i;
  for (i = 0; i < 8; i++) {
    if (led_flag & (1 << i))
      tm1638_write_data(2 * i + 1, 1);
    else
      tm1638_write_data(2 * i + 1, 0);
  }
}

void tm1638_init(void) {
  unsigned char i;
  tm1638_write_com(0x8b);
  tm1638_write_com(0x40);
  SPI3_CS_CLR;
  tm1638_write_byte(0xc0);
  for (i = 0; i < 16; i++)
    tm1638_write_byte(0x00);
  SPI3_CS_SET;
}
extern unsigned char num[8];
void tm1638_tube_dip(uint16_t pos, uint16_t data) {
  tm1638_write_data(pos * 2, code_tab[data]);
  tm1638_write_led(1 << data);
}