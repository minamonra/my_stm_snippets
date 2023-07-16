#pragma once
#ifndef _COMMON_H_
#define _COMMON_H_
#include <stm32f0xx.h>
#include "stdbool.h"

#define pin_toggle(gpioport, gpios)  do{  \
        register uint32_t __port = gpioport->ODR;  \
        gpioport->BSRR = ((__port & (gpios)) << 16) | (~__port & (gpios));}while(0)
#define pin_set(gpioport, gpios)  do{gpioport->BSRR = gpios;}while(0)
#define pin_clear(gpioport, gpios) do{gpioport->BSRR = ((gpios) << 16);}while(0)
#define pin_read(gpioport, gpios) (gpioport->IDR & (gpios) ? 1 : 0)
#define pin_write(gpioport, gpios)  do{gpioport->ODR = gpios;}while(0)

void gpio_init1(void);
void rcc_sysclockinit2(void);
void rcc_sysclockinit1(void);

#endif // _COMMON_H_