#include <stm32f0xx.h>
#include "st7735.h"
#include "cp866.h"

#define  LEDTOGGLE GPIOA->ODR ^= (1<<2)

volatile uint32_t ttms   = 0; // системный тикер
volatile uint32_t ddms   = 0; //
volatile uint32_t pa2ms  = 0;

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
}


void blink_(uint16_t freq)
{
  if (pa2ms > ttms || ttms - pa2ms > freq)
  {
    LEDTOGGLE;
    pa2ms = ttms;
  }
}

void fill_all_colours(uint16_t t)
{
  st7735_fill(0, 127, 0, 159, ST77XX_BLACK);
  delay_ms(t);
  st7735_fill(0, 127, 0, 159, ST77XX_WHITE);
  delay_ms(t);
  st7735_fill(0, 127, 0, 159, ST77XX_RED);
  delay_ms(t);
  st7735_fill(0, 127, 0, 159, ST77XX_GREEN);
  delay_ms(t);
  st7735_fill(0, 127, 0, 159, ST7735_BLUE);
  delay_ms(t);
  st7735_fill(0, 127, 0, 159, ST7735_CYAN);
  delay_ms(t);
  st7735_fill(0, 127, 0, 159, ST7735_MAGENTA);
  delay_ms(t);
  st7735_fill(0, 127, 0, 159, ST7735_YELLOW);
  delay_ms(t);
  st7735_fill(0, 127, 0, 159, ST7735_ORANGE);
  delay_ms(t);
}

void st7735_set_madctl(uint8_t value)
{
  CS_DN;
  st7735_send(LCD_C,ST77XX_MADCTL);
  st7735_send(LCD_D,value);
  CS_UP;
}

// MAIN
int main(void)
{

  uint32_t count=0;
  rcc_sysclockinit();
  SysTick_Config(48000); // 1ms if HSI
  gpio_init();
  spi_init();
  st7735_init();
  st7735_fill(0,127,0,159,ST77XX_BLACK);
  st7735_set_madctl(0xA8);
        
  do
  {
    
    for (uint8_t i=0; i<16;i++)
    {
      cp866_print_str("\xe2\xa5\xaa\xe3\xe9\xa8\xa9\x20\xe1\xe7\xa5\xe2\x3a\x20", 10, (i*8), ST7735_WHITE, ST7735_BLACK);
      cp866_print_num(count++, 120, (i*8), ST7735_GREEN, ST7735_BLACK);
    }
    delay_ms(1000);
    LEDTOGGLE;
  } while (1);
}

void SysTick_Handler(void) 
{
  ++ttms;
  if (ddms) ddms--;
}

// EOF