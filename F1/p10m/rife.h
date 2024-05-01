#pragma once
#ifndef __RIFE_H__
#define __RIFE_H__
#include "stm32f10x.h"
extern volatile uint32_t ttms;
extern volatile uint32_t ddms;

#define  LEDTOGGLE GPIOC->ODR ^= (1<<13)

void StartHSE(void);
void delay_ms(uint16_t ms);
void blink_pc13led(uint16_t freq);
void HW_init(void);
void SPI1_init(void);

#endif // __RIFE_H__