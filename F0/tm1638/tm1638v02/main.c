#include <stm32f0xx.h>
#include "tm1638_softspi.h"
// Для проверки работы китайского модуля на TM1638
// Софтовый SPI
// Обмен с TM`кой каждые 10 мс
// https://youtu.be/MRXQHX5MCf8
volatile uint32_t ttms   = 0; // системный тикер
volatile uint32_t ddms   = 0; // для "взвода" задержки
volatile uint32_t pa2    = 0; // для мигания
volatile uint32_t pr10ms = 0; // счетчик для 10 мс
volatile uint16_t key;        // кнопка tm1638
// по биту на каждую цифру, далее, когда зажгли все (0b11111111)- очистили
unsigned char  keypressed = 0b00000000;

void gpio_init(void)
{
  RCC->AHBENR  |= RCC_AHBENR_GPIOAEN  | RCC_AHBENR_GPIOBEN;
  GPIOA->MODER    |=   GPIO_MODER_MODER2_0;
  GPIOA->OSPEEDR  |=   GPIO_OSPEEDER_OSPEEDR2_0;
  GPIOB->MODER    |=  ( GPIO_MODER_MODER3_0     | GPIO_MODER_MODER4_0     | GPIO_MODER_MODER5_0 ) ;
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
    GPIOA->ODR ^= (1<<2);
    pa2 = ttms;
  }
}

void systick_delayms(uint16_t ms)
{
  ddms = ms;
  do {} while (ddms);
}

void tm1638_clear(void)
{
  for (uint8_t i = 0; i <= 7 ; i++) 
    tm1638_tube_dip(i, 16); // 16=0x00 (empty, clear)
}

void processtm1638(void)
{
  key = tm1638_read_key();
  if (key < 8) {
    while (tm1638_read_key() == key);
    if (key == 0) {
      tm1638_tube_dip(key, key);
      keypressed |= (1<<0);
    } else if (key == 1) {
      tm1638_tube_dip(key, key);
      keypressed |= (1<<1);
    } else if (key == 2) {
      tm1638_tube_dip(key, key);
      keypressed |= (1<<2);
    } else if (key == 3) {
      tm1638_tube_dip(key, key);
      keypressed |= (1<<3);
    } else if (key == 4) {
      tm1638_tube_dip(key, key);
      keypressed |= (1<<4);
    } else if (key == 5) {
      tm1638_tube_dip(key, key);
      keypressed |= (1<<5);
    } else if (key == 6) {
      tm1638_tube_dip(key, key);
      keypressed |= (1<<6);
    } else if (key == 7) {
      tm1638_tube_dip(key, key);
      keypressed |= (1<<7);
    }
  }
  if (keypressed == 0b11111111)
  { 
    systick_delayms(1000);
    tm1638_clear();
    keypressed   =  0b00000000;
  }

}

void process_10ms(void)
{
  if (pr10ms > ttms || ttms - pr10ms > 10)
  {
    processtm1638();
    pr10ms = ttms;
  }
}

int main() 
{
  rcc_sysclockinit();
  SysTick_Config(48000);
  gpio_init();
  tm1638_init();

  do 
  {

    blink_(499);
    process_10ms();

  } while (1);
}

void SysTick_Handler(void) 
{
  ++ttms;
  if (ddms) ddms--;
}

// EOF
