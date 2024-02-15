#ifndef __MAIN_H__
#define __MAIN_H__
#include "stm32f0xx.h"

#define  LEDTOGGLE GPIOA->ODR ^= (1<<2)

volatile uint32_t ttms   = 0; // системный тикер
volatile uint32_t ddms   = 0; //
volatile uint32_t pa2ms  = 0;
volatile uint32_t count  = 0;

void SysTick_Handler(void) 
{
  ++ttms;
  if (ddms) ddms--;
}

void rcc_sysclockinit(void)
{
  RCC->CR = RCC_CR_HSEON;
  while(! (RCC->CR & RCC_CR_HSERDY));
  RCC->CFGR2 = RCC_CFGR2_PREDIV_DIV2;
  RCC->CFGR  = RCC_CFGR_PLLMUL4 | // 
               RCC_CFGR_PLLSRC_HSE_PREDIV;
  RCC->CR |= RCC_CR_PLLON;
  FLASH->ACR = _VAL2FLD(FLASH_ACR_LATENCY, 1) | FLASH_ACR_PRFTBE;
  while(! (RCC->CR & RCC_CR_PLLRDY));
  RCC->CFGR |= RCC_CFGR_SW_PLL;
}

void gpio_init(void)
{
  RCC->AHBENR   |= RCC_AHBENR_GPIOAEN;
  RCC->AHBENR   |= RCC_AHBENR_GPIOBEN;
  // LED PA2
  GPIOA->MODER  |= GPIO_MODER_MODER2_0;
}

void blink_(uint16_t freq)
{
  if (pa2ms > ttms || ttms - pa2ms > freq)
  {
    LEDTOGGLE;
    pa2ms = ttms;
  }
}

void delay_ms(uint32_t ms)
{
  ddms = ms;
  while(ddms) {};
}

void spi_init(void)
{
  // SCK PB3
  GPIOB->MODER   |= GPIO_MODER_MODER3_1; // alternate function
  // MOSI PB5
  GPIOB->MODER   |= GPIO_MODER_MODER5_1; // alternate function
  // soft nCS PB4
  GPIOB->MODER   |= GPIO_MODER_MODER4_0; // PB4 as output
  GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR4_0);
  // DC (RS) PB1
  GPIOB->MODER   |= GPIO_MODER_MODER1_0;
  GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR1_0);
  // Reset PB0
  GPIOB->MODER   |= GPIO_MODER_MODER0_0;
  GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR0_0);
  // GPIOB->BSRR |= GPIO_BSRR_BS_4 // CS_UP;
  RCC->APB2ENR   |= RCC_APB2ENR_SPI1EN;
  SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0;
  SPI1->CR2 |= SPI_CR2_FRXTH;
  
  SPI1->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; // 8-bit

  SPI1->CR1 |= SPI_CR1_SPE; // Go
}

void recive_byte(uint8_t data)
{
  while (!(SPI1->SR & SPI_SR_TXE)); // Test Tx empty
  // Chip select
  *(__IO uint8_t *) (&SPI1->DR) = 0xFF; //SPI1-›DR = data; untranslatable pun!!!!!
  while(!(SPI1->SR & SPI_SR_RXNE)); // ready for buffer empty
  data = (SPI1->DR)<<8;

  while (!(SPI1->SR & SPI_SR_TXE)); /* Test Tx empty */
  *(__IO uint8_t *) (&SPI1->DR) = 0xFF; //SPI1-›DR = data; untranslatable pun!!!!!
  while(!(SPI1->SR & SPI_SR_RXNE)); // ready for buffer empty
  // while((SPI1-›SR & SPI_SR_RXNE) == SPI_SR_RXNE); // ready for buffer empty
  data |= (uint8_t)SPI1->DR;
  while(SPI1->SR & SPI_SR_BSY); // ready for buffer empty
}

void spi_send8(uint8_t data)
{
  //CS_DN;// RES;	
  SPI1->DR = data;
  while((SPI1->SR & SPI_SR_BSY)) {};
  //CS_UP;// SET;
}

#endif // __MAIN_H__