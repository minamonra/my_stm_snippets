#include "rife.h"
#include "stm32f10x.h"

volatile uint32_t pc13ms = 0;

void StartHSE(void)
{
  __IO uint32_t StartUpCounter = 0;
  // SYSCLK, HCLK, PCLK2 and PCLK1 configuration
  RCC->CR |= ((uint32_t)RCC_CR_HSEON); // Enable HSE
  // Wait till HSE is ready and if Time out is reached exit
  do
  {
    ++StartUpCounter;
  } while(!(RCC->CR & RCC_CR_HSERDY) && (StartUpCounter < 10000));
  if (RCC->CR & RCC_CR_HSERDY) // HSE started
  {
    FLASH->ACR |= FLASH_ACR_PRFTBE; // Enable Prefetch Buffer
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;    // Flash 2 wait state
    RCC->CFGR  |= (uint32_t)RCC_CFGR_HPRE_DIV1;// HCLK = SYSCLK
    RCC->CFGR  |= (uint32_t)RCC_CFGR_PPRE2_DIV1;// PCLK2 = HCLK
    RCC->CFGR  |= (uint32_t)RCC_CFGR_PPRE1_DIV2;// PCLK1 = HCLK
    //  PLL configuration: PLLCLK = HSE * 9 = 72 MHz
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);   //
    RCC->CR   |= RCC_CR_PLLON;// Enable PLL
    // Wait till PLL is ready
    StartUpCounter = 0;
    while((RCC->CR & RCC_CR_PLLRDY) == 0 && ++StartUpCounter < 1000){}
    // Select PLL as system clock source
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    
    // Wait till PLL is used as system clock source
    StartUpCounter = 0;
    while(((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) && ++StartUpCounter < 1000){}
  }
  else // HSE fails to start-up
  { 
        ; // add some code here (use HSI)
  }  
}

// прерывание системного тикера
extern void SysTick_Handler(void) 
{
  ++ttms;
  if (ddms) ddms--;
}

void delay_ms(uint16_t ms)
{
  ddms = ms;
  do {} while (ddms);
}

void blink_pc13led(uint16_t freq)
{
  if (pc13ms > ttms || ttms - pc13ms > freq)
  {
    LEDTOGGLE;
    pc13ms = ttms;
  }
}

void HW_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // PORT_A enable
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // PORT_B enable
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; // PORT_C enable

  if (SysTick_Config(72000)) { while(1); } // конфигурация SysTick = 1ms при тактовой 72MHz
  SysTick->LOAD |= SysTick_CTRL_ENABLE;    // включить SysTick

  //PC13 - LED13 on Blue Pill
  GPIOC->CRH &= ~GPIO_CRH_CNF13;
  GPIOC->CRH |= GPIO_CRH_MODE13_0;
  //PB1=nOE
  GPIOB->CRL &= ~GPIO_CRL_CNF1;    // Сбрасываем биты CNF для бита 1. Режим 00 - Push-Pull 
  GPIOB->CRL |= GPIO_CRL_MODE1_0;  // Выставляем бит MODE0 для пина 1. Режим MODE01 = Max Speed
  //PA1=PinA
  GPIOA->CRL &= ~GPIO_CRL_CNF1;
  GPIOA->CRL |= GPIO_CRL_MODE1_0;
  //PA4=PinB
  GPIOA->CRL &= ~GPIO_CRL_CNF4;
  GPIOA->CRL |= GPIO_CRL_MODE4_0;
  //PA15=SCLK
  GPIOA->CRH &= ~GPIO_CRH_CNF15;
  GPIOA->CRH |= GPIO_CRH_MODE15_0;
}

void SPI1_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN; // тактирование портов ввода вывода
  AFIO->MAPR &= ~AFIO_MAPR_SPI1_REMAP; // ремап
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;  // тактирование алтернативных функций
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;  // тактирование SPI1
  // CLK = PA5
  GPIOA->CRL |=GPIO_CRL_MODE5; GPIOA->CRL &=~ GPIO_CRL_CNF5; GPIOA->CRL |= GPIO_CRL_CNF5_1;
  // MOSI = PA7
  GPIOA->CRL |=GPIO_CRL_MODE7; GPIOA->CRL &=~ GPIO_CRL_CNF7; GPIOA->CRL |= GPIO_CRL_CNF7_1;
  // скорость передачи (делители)
  SPI1->CR1 |= SPI_CR1_BR_0| SPI_CR1_BR_2;
  // тип работы
  //CHOL = 0
  SPI1->CR1 &= ~SPI_CR1_CPOL;
  //CPHA=0
  SPI1->CR1 &= ~SPI_CR1_CPHA;
  // размер слова для передачи
  SPI1->CR1 &= ~SPI_CR1_DFF;
  // направление сдвига
  SPI1->CR1 &= ~SPI_CR1_LSBFIRST;
  SPI1->CR1 |= SPI_CR1_MSTR; // устройство в мастер
  SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
  SPI1->CR1 |= SPI_CR1_SPE; // запуск SPI
} 