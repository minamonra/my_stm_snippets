#pragma once
#ifndef _TM1638_H_
#define _TM1638_H_
#include <stm32f0xx.h>

void spi_init(void);
uint8_t spi_send_byte(uint8_t data);

uint8_t tm1638_get_key(void);
void tm1638_init(void);
// 0-all; 1-segments; 2-led
void tm1638_clear(uint8_t par);
void tm1638_seg(uint8_t seg_num, uint8_t data, uint8_t dot); // seg_num 0-7; ключить точку 1
void tm1638_initc(uint8_t bright);
void tm1638_tube_dip(uint16_t pos, uint16_t data);
void tm1638_disp_4dec1(uint16_t y);
void tm1638_disp_4dec2(uint16_t y);
#endif // _TM1638_H_