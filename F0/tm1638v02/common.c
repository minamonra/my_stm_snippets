#include "common.h"

extern volatile uint32_t ddms = 0;

void systick_delay(uint32_t ms)
{
  ddms = ms;
  while (ddms) {};
}
void gpio_init1(void)
{
  RCC->AHBENR  |= RCC_AHBENR_GPIOAEN  | RCC_AHBENR_GPIOBEN;
  GPIOA->MODER    |=   GPIO_MODER_MODER2_0;
  GPIOA->OSPEEDR  |=   GPIO_OSPEEDER_OSPEEDR2_0;
  GPIOB->MODER    |=  ( GPIO_MODER_MODER3_0     | GPIO_MODER_MODER4_0     | GPIO_MODER_MODER5_0 ) ;
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
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY; //
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
