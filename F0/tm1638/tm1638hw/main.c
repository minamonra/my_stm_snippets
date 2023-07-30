#include <stdio.h>
#include <stdlib.h>
#include <stm32f0xx.h>
#include "tm1638.h"

volatile uint32_t ttms   = 0; // системный тикер
volatile uint32_t pa2ms  = 0;
volatile uint16_t number = 0;

void gpio_init(void)
{
  RCC->AHBENR   |= RCC_AHBENR_GPIOAEN;   // GPIOA clock enable
  GPIOA->MODER  |= GPIO_MODER_MODER2_0;  // PA2 output
}

void blink_(uint16_t freq)
{
  if (pa2ms > ttms || ttms - pa2ms > freq)
  {
    GPIOA->ODR ^= (1<<2);
    pa2ms = ttms;
    
 tm1638_disp_4dec1(number);
    if (command == 1) number++;
    if (command == 2) number--;

    if (number > 9999) number = 0;
    
    tm1638_disp_4dec2(tm1638_get_key());
    switch (tm1638_get_key())
    {
      case (1<<0): command = 0; break; // стоп
      case (1<<1): command = 1; break; // счёт вверх
      case (1<<2): command = 2; break; // счёт вниз
      case (1<<3): break;
      case (1<<4): break;
      case (1<<5): break;
      case (1<<6): break;
      case (1<<7): break;
      default:;
    }
  }
}

// MAIN
int main(void)
{
  gpio_init();
  SysTick_Config(8000); // 1ms if HSI
  spi_init();
  tm1638_initc(3);
  do
  {
    blink_(99);
  } while (1);
}

// прерывание системного тикера
void SysTick_Handler(void) 
{
  ++ttms;
}

// EOF
