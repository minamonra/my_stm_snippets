#include <stm32f0xx.h>
#include "tm1638_softspi.h"
// https://youtu.be/MRXQHX5MCf8
// PIN_STB // PB4 STB SPI1_MISO // SPI3_CS_SET  SPI3_CS_CLR
// PIN_CLK // PB3 CLK SPI1_SCK  // SPI3_CLK_SET SPI3_CLK_CLR
// PIN_DIO // PB5 DIO SPI1_MOSI // SPI3_DIO_SET SPI3_DIO_CLR
volatile uint32_t ttms   = 0; // системный тикер
volatile uint32_t pa2    = 0; // для мигания
volatile uint32_t pr10ms = 0; // счетчик для 10 мс
volatile uint16_t key;        // кнопка tm1638

#define pin_toggle(gpioport, gpios)  do{  \    // поноавились дефайны Eddy
        register uint32_t __port = gpioport->ODR;  \
        gpioport->BSRR = ((__port & (gpios)) << 16) | (~__port & (gpios));}while(0)

void gpio_init(void)
{
  RCC->AHBENR  |= RCC_AHBENR_GPIOAEN  | RCC_AHBENR_GPIOBEN;

  GPIOA->MODER    &= ~ GPIO_MODER_MODER2;
  GPIOA->MODER    |=   GPIO_MODER_MODER2_0;
  GPIOA->OTYPER   &= ~ GPIO_OTYPER_OT_2;
  GPIOA->OSPEEDR  |=   GPIO_OSPEEDER_OSPEEDR2_0;
  
  GPIOB->MODER    &= ~( GPIO_MODER_MODER3       | GPIO_MODER_MODER4       | GPIO_MODER_MODER5 );
  GPIOB->MODER    |=  ( GPIO_MODER_MODER3_0     | GPIO_MODER_MODER4_0     | GPIO_MODER_MODER5_0 ) ;
  GPIOB->OTYPER   &= ~( GPIO_OTYPER_OT_3        | GPIO_OTYPER_OT_4        | GPIO_OTYPER_OT_5 ) ;
  GPIOB->OSPEEDR  |=  ( GPIO_OSPEEDER_OSPEEDR3_0|GPIO_OSPEEDER_OSPEEDR4_0 | GPIO_OSPEEDER_OSPEEDR5_0 );
}

void rcc_sysclockinit(void) // 48MHz кварц 12MHz
{
  RCC->CR = RCC_CR_HSEON;
  while(! (RCC->CR & RCC_CR_HSERDY));
  RCC->CFGR2 = RCC_CFGR2_PREDIV_DIV2;
  RCC->CFGR  = RCC_CFGR_PLLMUL4 | 
               RCC_CFGR_PLLSRC_HSE_PREDIV;
  RCC->CR |= RCC_CR_PLLON;
  FLASH->ACR = _VAL2FLD(FLASH_ACR_LATENCY, 1) | FLASH_ACR_PRFTBE;
  while(! (RCC->CR & RCC_CR_PLLRDY));
  RCC->CFGR |= RCC_CFGR_SW_PLL;
}


void blink_(uint16_t freq)
{
  if (pa2 > ttms || ttms - pa2 > freq)
  {
    pin_toggle(GPIOA, 1<<2);
    pa2 = ttms;
  }
}

void processtm1638(void)
{
   if (key < 8) {
      while (tm1638_read_key() == key);
      if (key == 0) {
        // tm1638_write_data(7<<1,code_tab[key]);
        tm1638_tube_dip(key, key);
      } else if (key == 1) {
        // tm1638_write_data(7<<1,code_tab[key]);
        tm1638_tube_dip(key, key);
      } else if (key == 2) {
        // tm1638_write_data(7<<1,code_tab[key]);
        tm1638_tube_dip(key, key);
      } else if (key == 3) {
        // tm1638_write_data(7<<1,code_tab[key]);
        tm1638_tube_dip(key, key);
      } else if (key == 4) {
        // tm1638_write_data(7<<1,code_tab[key]);
        tm1638_tube_dip(key, key);
      } else if (key == 5) {
        // tm1638_write_data(7<<1,code_tab[key]);
        tm1638_tube_dip(key, key);
      } else if (key == 6) {
        // tm1638_write_data(7<<1,code_tab[key]);
        tm1638_tube_dip(key, key);
      } else if (key == 7) {
        // tm1638_write_data(7<<1,code_tab[key]);
        tm1638_tube_dip(key, key);
      }
    }
}

void process_10ms(void)
{
  if (pr10ms > ttms || ttms - pr10ms > 10)
  {
    key = tm1638_read_key();
    processtm1638();
  }
}


int main() {
  rcc_sysclockinit();
  SysTick_Config(48000);
  gpio_init();
  tm1638_init();

  do {

    blink_(199);
    process_10ms();
    
   } while (1);
}

void SysTick_Handler(void) 
{
  ++ttms;
  // if (ddms) ddms--;
}