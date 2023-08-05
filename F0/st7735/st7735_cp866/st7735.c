#include "st7735.h"
#include "cp866.h"

#define CHAR_LEN 64                // 64=(8 x 8 )  i.e. width * height
#define MAX_CHAR 1                 //
#define BUF_LEN MAX_CHAR *CHAR_LEN // =64

#define SPI2SIXTEEN SPI1->CR2 &= ~SPI_CR2_FRXTH; SPI1->CR2 |= SPI_CR2_DS_3; // переключаемся на 16 бит
#define SPI2EIGHT   SPI1->CR2 |= SPI_CR2_FRXTH; SPI1->CR2 &= ~SPI_CR2_DS_3; // обратно на 8 бит  
#define SPIDR8BIT (*(__IO uint8_t *)((uint32_t)&SPI1->DR))

volatile uint16_t buf[BUF_LEN];
extern void delay_ms(uint32_t ms);

void spi_init(void)
{
  // SCK PB3
  GPIOB->MODER |= GPIO_MODER_MODER3_1; // alternate function
  // MOSI PB5
  GPIOB->MODER |= GPIO_MODER_MODER5_1; // alternate function
  // soft nCS PB4
  GPIOB->MODER |= GPIO_MODER_MODER4_0; // PB4 as output
  GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR4_0);
  // DC (RS) PB1
  GPIOB->MODER |= GPIO_MODER_MODER1_0;
  GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR1_0);
  // Reset PB0
  GPIOB->MODER |= GPIO_MODER_MODER0_0;
  GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR0_0);
  CS_UP;
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0;
  SPI1->CR2 |= SPI_CR2_FRXTH;
  SPI1->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; // 8-bit
  SPI1->CR1 |= SPI_CR1_SPE; // Go
}

void st7735_send(uint8_t dc, uint8_t data)
{
  if (dc == LCD_D) DC_UP; else DC_DN;
  
  SPIDR8BIT = data;
  while(SPI1->SR & SPI_SR_BSY);
  
  // while (!(SPI1->SR & SPI_SR_TXE));
  // SPIDR8BIT = data;
  // while (!(SPI1->SR & SPI_SR_RXNE));
  // uint8_t temp = SPIDR8BIT;
}

void st7735_fill(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint16_t color)
{
  CS_DN;
  st7735_send(LCD_C, ST77XX_CASET);
  st7735_send(LCD_D, 0);
  st7735_send(LCD_D, x0);
  st7735_send(LCD_D, 0);
  st7735_send(LCD_D, x1);
  st7735_send(LCD_C, ST77XX_RASET);
  st7735_send(LCD_D, 0);
  st7735_send(LCD_D, y0);
  st7735_send(LCD_D, 0);
  st7735_send(LCD_D, y1);
  st7735_send(LCD_C, ST77XX_RAMWR);
  uint16_t len = (uint16_t)((uint16_t)(x1 - x0 + 1) * (uint16_t)(y1 - y0 + 1));
  DC_UP;
  SPI2SIXTEEN;
  for (uint16_t i = 0; i < len; i++)
  {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = color;
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY));
  CS_UP;
  SPI2EIGHT;
}

void st7735_send_char(uint8_t x, uint8_t y, uint8_t ch, uint16_t fg_color, uint16_t bg_color)
{
  cp866_char_to_buf(x, ch, fg_color, bg_color);
  CS_DN;
  // 8x8 area
  st7735_send(LCD_C, ST77XX_CASET);
  st7735_send(LCD_D, 0);
  st7735_send(LCD_D, x);
  st7735_send(LCD_D, 0);
  st7735_send(LCD_D, x + 7);
  st7735_send(LCD_C, ST77XX_RASET);
  st7735_send(LCD_D, 0);
  st7735_send(LCD_D, y);
  st7735_send(LCD_D, 0);
  st7735_send(LCD_D, y + 7);
  st7735_send(LCD_C, ST77XX_RAMWR);
  DC_UP;
  SPI2SIXTEEN;
  for (uint8_t i = 0; i < BUF_LEN; i++) {
    uint16_t pixel = buf[i];
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = pixel;
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY));
  CS_UP;
  SPI2EIGHT;
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
