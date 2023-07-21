#include <stdio.h>
#include <stdlib.h>
#include <stm32f0xx.h>

// Мигаем на PA2 и выдаём на линию SPI 0xAA
// Смотрим анализатором
//
//	PB3 - SPI1_SCK (clock)
//	PB4 - CS (chip select)
//	PB5 - SPI1_MOSI (data)

#define CS_SET    GPIOB->BSRR |= GPIO_BSRR_BS_4 // GPIOB->ODR &= ~GPIO_ODR_4;
#define CS_CLR    GPIOB->BSRR |= GPIO_BSRR_BR_4 // GPIOB->ODR |= GPIO_ODR_4;
#define SPI1_DR_8bit  (*(__IO uint8_t *)((uint32_t)&(SPI1->DR)))

volatile uint32_t ttms  = 0; // системный тикер
volatile uint32_t pa2ms = 0;

static void gpio_init(void)
{
  RCC->AHBENR   |= RCC_AHBENR_GPIOAEN;   // GPIOA clock enable
  RCC->AHBENR   |= RCC_AHBENR_GPIOBEN;
  GPIOA->MODER  |= GPIO_MODER_MODER2_0;  // PA2 output

  // SCK
  GPIOB->MODER |= GPIO_MODER_MODER3_1;  // alternate function
  // GPIOB->OTYPER default push-pull
  // GPIOB->AFR[] default AF 0: SPI1_SCK,
  
  // MOSI
  GPIOB->MODER |= GPIO_MODER_MODER5_1;  // alternate function
  
  // soft nCS
  GPIOB->MODER |= GPIO_MODER_MODER4_0; // PB4 as output
  CS_SET; //GPIOB->ODR = GPIO_ODR_4; // deselect
}

static void spi_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  // SPI configuration
  SPI1->CR1 |= SPI_CR1_MSTR;  // бит мастера (SPI_CR1_MSTR)
  SPI1->CR1 |= SPI_CR1_BR;    // SPI_CR1_BR_2; // spi_sck = SystemCoreClock /
  SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;    // software slave CS management & internal slave select
  // SSM(Software slave management) — программное управление NSS, когда этот бит установлен вместо уровня 
  // на входе NSS контролируется состояние бита SSI, если сброшен — контролируется состояние вывода NSS
  // SPI1->CR2 |= SPI_CR2_DS_3 | SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; // 16 bit format
  SPI1->CR2 |= SPI_CR2_FRXTH; // бит наличия данных в очереди на четверть длины - 8 бит (SPI_CR2_FRXTH)
  SPI1->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; //8-bit
  SPI1->CR1 |= SPI_CR1_SPE;   // SPI вкл
}

uint8_t spi_send(uint8_t data)
{
  CS_SET; // chip select
  while (!(SPI1->SR & SPI_SR_TXE)); // TX buffer is empty
  SPI1_DR_8bit = data; // SPI1->DR = data; // //*(uint8_t *)&SPI1->DR=0xAA;
  while (!(SPI1->SR & SPI_SR_RXNE)); 
  CS_CLR; // deselet
  return (SPI1_DR_8bit);
}

uint8_t spi_read(void)
{
 return spi_send(0xFF); 
}

void blink_(uint16_t freq)
{
  if (pa2ms > ttms || ttms - pa2ms > freq)
  {
    GPIOA->ODR ^= (1<<2);
    pa2ms = ttms;
    spi_send(0xAA);
  }
}

int main(void)
{
  gpio_init();
  SysTick_Config(8000); // 1ms if HSI
  spi_init();
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
