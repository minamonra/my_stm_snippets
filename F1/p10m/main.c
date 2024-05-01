#include <stdio.h>
#include <stdlib.h>
#include "stdbool.h"
#include "stm32f10x.h"
#include "rife.h"

// nOE  = PB0
#define nOE1 GPIOB->BSRR  |= GPIO_BSRR_BS1 // set (1)
#define nOE0 GPIOB->BSRR  |= GPIO_BSRR_BR1 // reset (0)
// PinA = PA1
#define PinA1 GPIOA->BSRR |= GPIO_BSRR_BS1 // set (1)
#define PinA0 GPIOA->BSRR |= GPIO_BSRR_BR1 // reset (0)
// PinB = PA4
#define PinB1 GPIOA->BSRR |= GPIO_BSRR_BS4 // set (1)
#define PinB0 GPIOA->BSRR |= GPIO_BSRR_BR4 // reset (0)
// SCLK = PA15
#define SCLK1 GPIOA->BSRR |= GPIO_BSRR_BS15 // set (1)
#define SCLK0 GPIOA->BSRR |= GPIO_BSRR_BR15 // reset (0)

volatile uint32_t ttms = 0;
volatile uint32_t ddms = 0;

void SPI1TR(uint8_t data) {
  // ждем, пока не освободится буфер передатчика
  while(!(SPI1->SR & SPI_SR_TXE));
  *(volatile uint8_t *)&SPI1->DR = data;
  // *((__IO uint8_t *) &SPI1->DR) = (uint8_t)data;
}

uint8_t SPI1TRWR(const uint8_t data)
{
  while(!(SPI1 -> SR & SPI_SR_TXE));
  *(volatile uint8_t *)&SPI1->DR = data;
  while (!(SPI1 -> SR & SPI_SR_RXNE));
  while(SPI1->SR & SPI_SR_BSY){}
  return *(volatile uint8_t *)&SPI1-> DR;
}

int main(void) {

  StartHSE();
  HW_init();

  do {
    blink_pc13led(199); // LEDTOGGLE;
    uint8_t _data = 1;
    
    // SPI1TRWR(_data); // elP10mono06.jpg
    SPI1TR(_data); // elP10mono07.jpg
    delay_ms(5);
    
  } while (1);
 }