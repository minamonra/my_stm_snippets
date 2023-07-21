#include <stdio.h>
#include <stdlib.h>
#include "stm32f0xx.h"

// Segger -> Menu -> Project -> Options -> Preprocessor -> User include direcories
// add this:
// ./CMSIS/Device
// ./CMSIS/Core

volatile uint32_t ttms  = 0; // системный тикер
volatile uint32_t pa2ms = 0;

static void gpio_setup(void)
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
}

// EOF
