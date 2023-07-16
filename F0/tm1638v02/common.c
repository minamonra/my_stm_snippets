#include "common.h"
// https://cxem.net/mc/mc434.php
// https://github.com/carloscn/c04-bc35-ds1302-tm1638-stm32-ac6/tree/master
// https://github.com/carloscn/c04-bc35-ds1302-tm1638-stm32-ac6/blob/master/inc/tm1638.h
// https://github.com/carloscn/c04-bc35-ds1302-tm1638-stm32-ac6/blob/master/src/tm1638.c
//extern volatile uint32_t ttms;
//extern volatile uint32_t ddms;
// CLK_LOW
// CLK_HIGH
// DIO_LOW
// DIO_HIGH
// STB_LOW
// STB_HIGH

// #define SPI3_DELAY 10
// #define SPI3_DIO_SET pin_set(GPIOB, 1<<5)
// #define SPI3_DIO_CLR pin_clear(GPIOB, 1<<5)
// #define SPI3_CLK_SET pin_set(GPIOB, 1<<3)
// #define SPI3_CLK_CLR pin_clear(GPIOB, 1<<3)
// #define SPI3_CS_SET pin_set(GPIOB, 1<<4)
// #define SPI3_CS_CLR pin_clear(GPIOB, 1<<4)
// #define SPI3_CHECK_DIO pin_read(GPIOB, 1<<5)
volatile uint32_t ddms = 0;
void systick_delay(uint32_t ms)
{
  ddms = ms;
  while (ddms) {};
}
void gpio_init1(void)
{
  RCC->AHBENR  |= RCC_AHBENR_GPIOAEN  | RCC_AHBENR_GPIOBEN;

  //GPIOA->MODER    &= ~ GPIO_MODER_MODER2;
  GPIOA->MODER    |=   GPIO_MODER_MODER2_0;
  //GPIOA->OTYPER   &= ~ GPIO_OTYPER_OT_2;
  GPIOA->OSPEEDR  |=   GPIO_OSPEEDER_OSPEEDR2_0;
  // https://cxem.net/mc/mc434.php
  //GPIOB->MODER    &= ~( GPIO_MODER_MODER3       | GPIO_MODER_MODER4       | GPIO_MODER_MODER5 );
  GPIOB->MODER    |=  ( GPIO_MODER_MODER3_0     | GPIO_MODER_MODER4_0     | GPIO_MODER_MODER5_0 ) ;
  //GPIOB->OTYPER   &= ~( GPIO_OTYPER_OT_3        | GPIO_OTYPER_OT_4        | GPIO_OTYPER_OT_5 ) ;
  GPIOB->OSPEEDR  |=  ( GPIO_OSPEEDER_OSPEEDR3_0|GPIO_OSPEEDER_OSPEEDR4_0 | GPIO_OSPEEDER_OSPEEDR5_0 );
}

void rcc_sysclockinit2(void)
{
  __IO uint32_t StartUpCounter = 0, HSEStatus = 0;
  RCC->CR |= RCC_CR_HSEON;
  do
  {
    HSEStatus = RCC->CR & RCC_CR_HSERDY;
    StartUpCounter++;
  } while((HSEStatus == 0) && (StartUpCounter != 5000)); // Wait till HSE is ready and if Time out is reached exit

  if ((RCC->CR & RCC_CR_HSERDY) != RESET) { HSEStatus = 0x01; } else { HSEStatus = 0x00; }

  if (HSEStatus == 0x01)
  {
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY; // Enable Prefetch Buffer and set Flash Latency FLASH->ACR = _VAL2FLD(FLASH_ACR_LATENCY, 1) | FLASH_ACR_PRFTBE; // FLASH->ACR |= FLASH_ACR_LATENCY|FLASH_ACR_PRFTBE; // Активирование предвыборки
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1; // HCLK = SYSCLK
    RCC->CFGR |= RCC_CFGR_PPRE_DIV1; // PCLK = HCLK
    RCC->CFGR2 = RCC_CFGR2_PREDIV_DIV2;
    RCC->CFGR  = RCC_CFGR_PLLMUL4 | // 
               RCC_CFGR_PLLSRC_HSE_PREDIV;
    RCC->CR |= RCC_CR_PLLON;
    while(! (RCC->CR & RCC_CR_PLLRDY));
    RCC->CFGR |= RCC_CFGR_SWS_PLL;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
  } else { 
  // Err
  }
#ifdef MCOTEST /// MCO start
  RCC->CFGR |= RCC_CFGR_MCO_SYSCLK;
  GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER8)) | (GPIO_MODER_MODER8_1); //
  GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8; //
#endif // MCO end
  
}
void rcc_sysclockinit1(void)
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