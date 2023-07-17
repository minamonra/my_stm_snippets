#include <stdio.h>
#include <stdlib.h>
#include "stm32f031x6.h"

// Segger -> Menu -> Project -> Options -> Preprocessor -> User include direcories
// add this:
// ./CMSIS/Device
// ./CMSIS/Core

volatile uint32_t ttms  = 0;
volatile uint32_t pa2ms = 0; // системный тикер

static void gpio_setup(void)
{
  RCC->AHBENR   |= RCC_AHBENR_GPIOAEN;   // GPIOA clock enable
  RCC->AHBENR   |= RCC_AHBENR_GPIOBEN;   // GPIOA clock enable
  GPIOA->MODER  |= GPIO_MODER_MODER2_0;  // PA3 output
}

void blink_(uint16_t freq)
{
  if (pa2ms > ttms || ttms - pa2ms > freq)
  {
    GPIOA->ODR ^= 1<<2;
    pa2ms = ttms;
  }
}

int main(void)
{
  gpio_setup();
  SysTick_Config(8000); // 1ms if HSI
  do
  {
    blink_(999);
  } while (1);
}

// прерывание системного тикера
void SysTick_Handler(void) 
{
  ++ttms;
  // if (ddms) ddms--;
}

// EOF