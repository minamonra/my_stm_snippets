#pragma once
#ifndef _TM1638_SOFTSPI_H_
#define _TM1638_SOFTSPI_H_
#include <stm32f0xx.h>

#define DATA_COMMAND  0X40
#define DISP_COMMAND  0x80
#define ADDR_COMMAND  0XC0

#define SPI3_DIO_SET   GPIOB->BSRR |= GPIO_BSRR_BS_5
#define SPI3_DIO_CLR   GPIOB->BSRR |= GPIO_BSRR_BR_5
#define SPI3_CLK_SET   GPIOB->BSRR |= GPIO_BSRR_BS_3
#define SPI3_CLK_CLR   GPIOB->BSRR |= GPIO_BSRR_BR_3
#define SPI3_CS_SET    GPIOB->BSRR |= GPIO_BSRR_BS_4
#define SPI3_CS_CLR    GPIOB->BSRR |= GPIO_BSRR_BR_4
#define SPI3_CHECK_DIO GPIOB->IDR & (1 << 5)

void tm1638_write_byte(unsigned char data);
unsigned char tm1638_read_byte(void);
void tm1638_write_com(unsigned char cmd);
unsigned char tm1638_read_key(void);
void tm1638_write_data(unsigned char add, unsigned char data);
void tm1638_write_led(unsigned char led_flag);
extern void tm1638_init(void);
extern void tm1638_tube_dip(uint16_t pos, uint16_t data);
extern unsigned char code_tab[];

#endif // _TM1638_SOFTSPI_H_
