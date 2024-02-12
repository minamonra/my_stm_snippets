#include <stdio.h>
#include <stdlib.h>
#include "stm32f0xx.h"
#include "lcd7735.h"

#define  LEDTOGGLE GPIOA->ODR ^= (1<<2)

// https://vg.ucoz.ru/load/stm32_ickhodnye_teksty_programm_na_si/stm32_biblioteka_podkljuchenija_displeja_na_kontrollere_st7735_dlja_stm32f4/16-1-0-41
// https://forum.easyelectronics.ru/viewtopic.php?f=35&t=13281&start=50

// MAIN
volatile uint32_t ttms   = 0; // системный тикер
volatile uint32_t ddms   = 0; //
volatile uint32_t pa2ms  = 0;

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
  CS_UP;
  RCC->APB2ENR   |= RCC_APB2ENR_SPI1EN;
  SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0;
  SPI1->CR2 |= SPI_CR2_FRXTH;
  
  SPI1->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; // 8-bit

  // SPI1->CR1 &= ~(SPI_CR1_RXONLY);	
  // SPI1->CR1 |= SPI_CR2_FRF;
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

void spiTransmit(uint8_t data)
{
  CS_DN;// RES;	
  SPI1->DR = data;
  while((SPI1->SR & SPI_SR_BSY)) {};
  CS_UP;// SET;
}

volatile unsigned int counter1000;      // счетчик интервалов (такт 10 мкс)
const unsigned char str1[]="VG.UCOZ.RU";
const unsigned char str2[]="STM32F030";
const unsigned char str3[]="ST7735 DRIVER";
const unsigned char str4[]="PRINT TIME: ";


int main(void) {
  rcc_sysclockinit();
  SysTick_Config(48000); // 1ms if HSI
  gpio_init();
  spi_init();
  lcd7735_init();
  
  unsigned char x,y;   // координаты красного поля
  signed char sx,sy; // направление движения красного поля
  x=70; y=10;
  sx=-1; sy=1;

  unsigned char y1=10;   // координаты надписи
  signed char sy1=1;    // направление движения

  unsigned char y2=5;   // координаты надписи
  signed char sy2=1;    // направление движения

  unsigned char ss=0;  // счетчики для обеспечения разных скоростей движения надписей
  unsigned char ss1=0;

  unsigned int counterStart=0; // переменная для расчета времени исполнения одного хода цикла

  lcd7735_fillrect(0, 0, 128, 160, 0xF800); // заполним весь экран красным  цветом

  unsigned char startChar=0;    // переменные для организации печати знакогенератора
  unsigned char startCharPos=8;
  unsigned char chrs;

  do {
    counterStart=counter1000;

    lcd7735_rect(x-1, y-1, x+35, y+111, 0xF800); // рисуем прямоугольную рамку

    lcd7735_fillrect(x, y, x+34, y+110, 0x001F); // внутри синий прямоугольник

    lcd7735_putstr(x+20,  y+y2, str1, 0xFFFF, 0x001F);  // печатаем строку1
    lcd7735_putstr(x+10,  y+y1, str2, 0xF7E0, 0x001F);  // печатаем строку2
    //lcd7735_putstr(x,     y+ 3, str3, 0x07E0, 0x001F);  // печатаем строку3
    lcd7735_putstr(x,     y+ 3, str3, 0x07E0, 0x001F);  // печатаем строку3

    // печать знакогенератора
    unsigned char prnChar;
    for(prnChar=0;prnChar<18;prnChar++)
    {

      chrs=prnChar+startChar;
      if (chrs<32) chrs=chrs+32;
      lcd7735_putchar(18, 4+startCharPos-1+prnChar*8, chrs, 0x0000, 0x07E0);

      chrs=~(startChar+prnChar);
      if (chrs<32) chrs=chrs+32;
      lcd7735_putchar(34, 148-(startCharPos-1+prnChar*8), chrs, 0x0000, 0x07E0);

      if (prnChar==0)
      {
    	lcd7735_putchar(18, 4, ' ', 0xFFFF, 0x07E0);
    	lcd7735_putchar(34, 4+18*8, ' ', 0xFFFF, 0x07E0);
      }
    }
    lcd7735_putchar(18, 4+18*8, ' ', 0xFFFF, 0x07E0);
    lcd7735_putchar(34, 4, ' ', 0xFFFF, 0x07E0);

    lcd7735_putstr(2,   12, str4, 0xFFFF, 0x0000);      // печатаем "FPS:"

    unsigned int counterLen=counter1000-counterStart;

    LCD7735_dec(counterLen/10, 3, 2, 108, 0xFFFF, 0x0000);	// печать значения времени цикла
    lcd7735_putchar(2, 128, '.', 0xFFFF, 0x0000);
    LCD7735_dec(counterLen, 1, 2, 136, 0xFFFF, 0x0000);	// печать значения времени цикла

    if (counterLen<334) delay_ms((335-counterLen)*2); // автоматический расчет задержки для устранения частого мерцания при непрерывном обновлении экрана
    else delay_ms(335*2);
    // расчеты для движения
    startCharPos--;
    if (startCharPos==0)
    {
      startCharPos=9;
      startChar++;
    }

    ss++;
    if (ss==2)
    {
      x=x+sx;
      y=y+sy;
      if (x==50) sx=1;
      if (x==92) sx=-1;

      if (y==2) sy=1;
      if (y==48) sy=-1;
      ss=0;
    }

    ss1++;
    if (ss1==3)
    {
      ss1=0;
      y1=y1+sy1;
      if (y1==1) sy1=1;
      if (y1==40) sy1=-1;
    }

    y2=y2+sy2;
    if (y2==1) sy2=1;
    if (y2==28) sy2=-1;
    }
    //blink_(300);
    
  while (1);
  //LCD_CS1;
}

void SysTick_Handler(void) 
{
  ++ttms;
  if (ddms) ddms--;
}

// End of file