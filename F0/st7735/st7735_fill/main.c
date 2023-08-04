#include <stdio.h>
#include <stdlib.h>
#include <stm32f0xx.h>
#include "st7735defs.h"

volatile uint32_t ttms   = 0; // системный тикер
volatile uint32_t ddms   = 0; //
volatile uint32_t pa2ms  = 0;

// PB3 - SPI1_SCK (clock)
// PB4 - CS (chip select)
// PB5 - SPI1_MOSI (data)

// Chip select PB4
#define CS_UP  GPIOB->BSRR |= GPIO_BSRR_BS_4
#define CS_DN  GPIOB->BSRR |= GPIO_BSRR_BR_4

// DC (RS) PB1
#define DC_UP  GPIOB->BSRR |= GPIO_BSRR_BS_1
#define DC_DN  GPIOB->BSRR |= GPIO_BSRR_BR_1

// Reset PB0
#define RST_UP GPIOB->BSRR |= GPIO_BSRR_BS_0
#define RST_DN GPIOB->BSRR |= GPIO_BSRR_BR_0

#define LCD_X   160
#define LCD_Y   128
#define LCD_C  0x00
#define LCD_D  0x01

void delay_ms(uint32_t ms)
{
  ddms = ms;
  while(ddms) {};
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
  // DC (RS) PB1
  GPIOB->MODER |= GPIO_MODER_MODER1_0;
  GPIOB->OSPEEDR  |=  ( GPIO_OSPEEDER_OSPEEDR1_0);
  // Reset PB0
  GPIOB->MODER |= GPIO_MODER_MODER0_0;
  GPIOB->OSPEEDR  |=  ( GPIO_OSPEEDER_OSPEEDR0_0);
}

void spi_init(void)
{
  // SCK  PB3
  GPIOB->MODER |= GPIO_MODER_MODER3_1;  // alternate function
  // MOSI  PB5
  GPIOB->MODER |= GPIO_MODER_MODER5_1;  // alternate function
  // soft nCS   PB4
  GPIOB->MODER |= GPIO_MODER_MODER4_0; // PB4 as output
  GPIOB->OSPEEDR  |=  ( GPIO_OSPEEDER_OSPEEDR4_0);
  CS_UP; // GPIOB->ODR = GPIO_ODR_4; // deselect
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0;
  SPI1->CR2 |= SPI_CR2_FRXTH; // бит наличия данных в очереди на четверть длины - 8 бит (SPI_CR2_FRXTH)
  SPI1->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; //8-bit
  SPI1->CR1 |= SPI_CR1_SPE; // Go
}

void blink_(uint16_t freq)
{
  if (pa2ms > ttms || ttms - pa2ms > freq)
  {
    GPIOA->ODR ^= (1<<2);
    pa2ms = ttms;
  }
}

void st7735_send(uint8_t dc, uint8_t data)
{
  if (dc == LCD_D) DC_UP; else DC_DN;
  
  while (!(SPI1->SR & SPI_SR_TXE));
  *(uint8_t *)&SPI1->DR = data;

  while(SPI1->SR & SPI_SR_BSY);

  // while (!(SPI1->SR & SPI_SR_RXNE));
  // data = *(uint8_t *)&SPI1->DR;
}

void st7735_fill(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint16_t color)
{
  CS_DN;
  st7735_send(LCD_C,ST77XX_CASET);
  st7735_send(LCD_D,0);
  st7735_send(LCD_D,x0);
  st7735_send(LCD_D,0);
  st7735_send(LCD_D,x1);
  st7735_send(LCD_C,ST77XX_RASET);
  st7735_send(LCD_D,0);
  st7735_send(LCD_D,y0);
  st7735_send(LCD_D,0);
  st7735_send(LCD_D,y1);
  st7735_send(LCD_C,ST77XX_RAMWR);
  uint16_t len=(uint16_t)((uint16_t)(x1-x0+1)*(uint16_t)(y1-y0+1));
  DC_UP;
  SPI1->CR2 &= ~SPI_CR2_FRXTH;
  SPI1->CR2 |= SPI_CR2_DS_3; // переключаемся на 16 бит
  for (uint16_t i=0; i < len; i++)
  {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = color;
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY));
  CS_UP;
  SPI1->CR2 |= SPI_CR2_FRXTH;
  SPI1->CR2 &= ~SPI_CR2_DS_3; // обратно на 8 бит
}

void fill_all_colours(uint16_t t)
{
  st7735_fill(0,127,0,159,ST77XX_BLACK);   delay_ms(t);
  st7735_fill(0,127,0,159,ST77XX_WHITE);   delay_ms(t);
  st7735_fill(0,127,0,159,ST77XX_RED);     delay_ms(t);
  st7735_fill(0,127,0,159,ST77XX_GREEN);   delay_ms(t);
  st7735_fill(0,127,0,159,ST7735_BLUE);    delay_ms(t);
  st7735_fill(0,127,0,159,ST7735_CYAN);    delay_ms(t);
  st7735_fill(0,127,0,159,ST7735_MAGENTA); delay_ms(t);
  st7735_fill(0,127,0,159,ST7735_YELLOW);  delay_ms(t);
  st7735_fill(0,127,0,159,ST7735_ORANGE);  delay_ms(t);
}

void st7735_init(void);

int main(void) {
  rcc_sysclockinit();
  SysTick_Config(48000); // 1ms if HSI
  gpio_init();
  spi_init();
  st7735_init();
  st7735_fill(0,127,0,159,ST77XX_BLACK);

  do {
    // blink_(499);
    fill_all_colours(1000);
  } while (1);
}

void SysTick_Handler(void) 
{
  ++ttms;
  if (ddms) ddms--;
}

void st7735_init(void) 
{
  // hardware reset
  RST_DN;
  delay_ms(10);
  RST_UP;
  delay_ms(10);
  // init routine
  CS_DN;
  st7735_send(LCD_C, ST77XX_SWRESET);
  delay_ms(150);
  st7735_send(LCD_C, ST77XX_SLPOUT);
  delay_ms(150);

  st7735_send(LCD_C, ST7735_FRMCTR1);
  st7735_send(LCD_D, 0x01);
  st7735_send(LCD_D, 0x2C);
  st7735_send(LCD_D, 0x2D);

  st7735_send(LCD_C, ST7735_FRMCTR2);
  st7735_send(LCD_D, 0x01);
  st7735_send(LCD_D, 0x2C);
  st7735_send(LCD_D, 0x2D);

  st7735_send(LCD_C, ST7735_FRMCTR3);
  st7735_send(LCD_D, 0x01);
  st7735_send(LCD_D, 0x2C);
  st7735_send(LCD_D, 0x2D);
  st7735_send(LCD_D, 0x01);
  st7735_send(LCD_D, 0x2C);
  st7735_send(LCD_D, 0x2D);

  st7735_send(LCD_C, ST7735_INVCTR);
  st7735_send(LCD_D, 0x07);

  st7735_send(LCD_C, ST7735_PWCTR1);
  st7735_send(LCD_D, 0xA2);
  st7735_send(LCD_D, 0x02);
  st7735_send(LCD_D, 0x84);

  st7735_send(LCD_C, ST7735_PWCTR2);
  st7735_send(LCD_D, 0xC5);

  st7735_send(LCD_C, ST7735_PWCTR3);
  st7735_send(LCD_D, 0x0A);
  st7735_send(LCD_D, 0x00);

  st7735_send(LCD_C, ST7735_PWCTR4);
  st7735_send(LCD_D, 0x8A);
  st7735_send(LCD_D, 0x2A);

  st7735_send(LCD_C, ST7735_PWCTR5);
  st7735_send(LCD_D, 0x8A);
  st7735_send(LCD_D, 0xEE);

  st7735_send(LCD_C, ST7735_VMCTR1);
  st7735_send(LCD_D, 0x0E);

  st7735_send(LCD_C, ST77XX_INVOFF);

  st7735_send(LCD_C, ST77XX_MADCTL);
  st7735_send(LCD_D, 0xC0);

  st7735_send(LCD_C, ST77XX_COLMOD);
  st7735_send(LCD_D, 0x05);

  st7735_send(LCD_C, ST7735_GMCTRP1);
  st7735_send(LCD_D, 0x02);
  st7735_send(LCD_D, 0x1C);
  st7735_send(LCD_D, 0x07);
  st7735_send(LCD_D, 0x12);
  st7735_send(LCD_D, 0x37);
  st7735_send(LCD_D, 0x32);
  st7735_send(LCD_D, 0x29);
  st7735_send(LCD_D, 0x2D);
  st7735_send(LCD_D, 0x29);
  st7735_send(LCD_D, 0x25);
  st7735_send(LCD_D, 0x2B);
  st7735_send(LCD_D, 0x39);
  st7735_send(LCD_D, 0x00);
  st7735_send(LCD_D, 0x01);
  st7735_send(LCD_D, 0x03);
  st7735_send(LCD_D, 0x10);

  st7735_send(LCD_C, ST7735_GMCTRN1);
  st7735_send(LCD_D, 0x03);
  st7735_send(LCD_D, 0x1D);
  st7735_send(LCD_D, 0x07);
  st7735_send(LCD_D, 0x06);
  st7735_send(LCD_D, 0x2E);
  st7735_send(LCD_D, 0x2C);
  st7735_send(LCD_D, 0x29);
  st7735_send(LCD_D, 0x2D);
  st7735_send(LCD_D, 0x2E);
  st7735_send(LCD_D, 0x2E);
  st7735_send(LCD_D, 0x37);
  st7735_send(LCD_D, 0x3F);
  st7735_send(LCD_D, 0x00);
  st7735_send(LCD_D, 0x00);
  st7735_send(LCD_D, 0x02);
  st7735_send(LCD_D, 0x10);

  st7735_send(LCD_C, ST77XX_NORON);
  delay_ms(10);

  st7735_send(LCD_C, ST77XX_DISPON);
  delay_ms(10);
  CS_UP;
}

// EOF
