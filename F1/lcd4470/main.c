#include <stdio.h>
#include <stdlib.h>
#include "stm32f10x.h"
#include "stdbool.h"
volatile uint32_t ttms   = 0;
volatile uint32_t ddms   = 0;
volatile uint32_t pc13ms = 0;

#define  LEDTOGGLE GPIOC->ODR ^= (1<<13)

// MODE:
#define MODE_INPUT      0
#define MODE_NORMAL     1  // 10MHz
#define MODE_SLOW       2  // 2MHz
#define MODE_FAST       3  // 50MHz
// CNF:
#define CNF_ANALOG      (0<<2)
#define CNF_PPOUTPUT    (0<<2)
#define CNF_FLINPUT     (1<<2)
#define CNF_ODOUTPUT    (1<<2)
#define CNF_PUDINPUT    (2<<2)
#define CNF_AFPP        (2<<2)
#define CNF_AFOD        (3<<2)

#define CRL(pin, cnfmode)  ((cnfmode) << (pin*4))
#define CRH(pin, cnfmode)  ((cnfmode) << ((pin-8)*4))

static void StartHSE()
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
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;// HCLK = SYSCLK
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;// PCLK2 = HCLK
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;// PCLK1 = HCLK
    //  PLL configuration: PLLCLK = HSE * 9 = 72 MHz
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);   //
    RCC->CR |= RCC_CR_PLLON;// Enable PLL
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

static void hardware_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // PORT_A enable
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // PORT_B enable
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; // PORT_C enable

  if (SysTick_Config(72000)) { while(1); } // конфигурация SysTick = 1ms при тактовой 72MHz
  SysTick->LOAD |= SysTick_CTRL_ENABLE;    // включить SysTick
  // LED PC13
  GPIOC->CRH &= ~GPIO_CRH_CNF13;
  //GPIOC->CRH |= GPIO_CRH_MODE13_0;
  //GPIOC->CRH |= CRH(13, CNF_PPOUTPUT | MODE_FAST);
  GPIOC->CRH |= CRH(13, CNF_PPOUTPUT | MODE_NORMAL);

  // PB0
  //GPIOB->CRL &= ~GPIO_CRL_CNF0;
  GPIOB->CRL &= ~GPIO_CRL_CNF0;
  //GPIOB->CRL |=  GPIO_CRL_MODE0_0;
  GPIOB->CRL |= CRL(0, CNF_PPOUTPUT | MODE_NORMAL);
  // PB3
  //GPIOB->CRL &= ~GPIO_CRL_CNF3;
  
  GPIOB->CRL &= ~GPIO_CRL_CNF3;	  // Сбрасываем биты CNF для бита 5. Режим 00 - Push-Pull 
  GPIOB->CRL |= CRL(3, CNF_PPOUTPUT | MODE_NORMAL);
  //GPIOB->CRL |=  GPIO_CRL_MODE3_0; // Выставляем бит MODE0 для пятого пина. Режим MODE01 = Max Speed 10MHz
  // PB4
  GPIOB->CRL &= ~GPIO_CRL_CNF4;
  GPIOB->CRL |= CRL(4, CNF_PPOUTPUT | MODE_NORMAL);
  // PB5
  GPIOB->CRL &= ~GPIO_CRL_CNF5;
  GPIOB->CRL |= CRL(5, CNF_PPOUTPUT | MODE_NORMAL);
  // PB6
  GPIOB->CRL &= ~GPIO_CRL_CNF6;
  GPIOB->CRL |= CRL(6, CNF_PPOUTPUT | MODE_NORMAL);
  // PB7
  GPIOB->CRL &= ~GPIO_CRL_CNF7;
  GPIOB->CRL |= CRL(7, CNF_PPOUTPUT | MODE_NORMAL);
}

// прерывание системного тикера
void SysTick_Handler(void) 
{
  ++ttms;
  if (ddms) ddms--;
}
void blink_pc13led(uint16_t freq)
{
  if (pc13ms > ttms || ttms - pc13ms > freq)
  {
    LEDTOGGLE;
    pc13ms = ttms;
  }
}

void delay_ms(uint16_t ms)
{
  ddms = ms;
  do {} while (ddms);
}

// RS = PB0
#define RS1 GPIOB->BSRR |= GPIO_BSRR_BS0 // set (1)
#define RS0 GPIOB->BSRR |= GPIO_BSRR_BR0 // reset (0)
// E (en) = PB3
#define EN1 GPIOB->BSRR |= GPIO_BSRR_BS3
#define EN0 GPIOB->BSRR |= GPIO_BSRR_BR3
// D4 = PB4
#define D41 GPIOB->BSRR |= GPIO_BSRR_BS4
#define D40 GPIOB->BSRR |= GPIO_BSRR_BR4
// D5 = PB5
#define D51 GPIOB->BSRR |= GPIO_BSRR_BS5
#define D50 GPIOB->BSRR |= GPIO_BSRR_BR5
// D6 = PB6
#define D61 GPIOB->BSRR |= GPIO_BSRR_BS6
#define D60 GPIOB->BSRR |= GPIO_BSRR_BR6
// D7 = PB7
#define D71 GPIOB->BSRR |= GPIO_BSRR_BS7
#define D70 GPIOB->BSRR |= GPIO_BSRR_BR7

void lcdSend(uint8_t isCommand, uint8_t data) {
  //isCommand ? (RS0) : (RS1);
  if (isCommand) RS0; else RS1;
  delay_ms(5);
  if ((data >> 7) & 1) D71; else D70;
  if ((data >> 6) & 1) D61; else D60;
  if ((data >> 5) & 1) D51; else D50;
  if ((data >> 4) & 1) D41; else D40;
  EN1;
  delay_ms(5);
  EN0;
  if ((data >> 3) & 1) D71; else D70;
  if ((data >> 2) & 1) D61; else D60;
  if ((data >> 1) & 1) D51; else D50;
  if ((data >> 0) & 1) D41; else D40;
  EN1;
  delay_ms(5);
  EN0;
}

void lcdCommand(uint8_t cmd) {
  lcdSend(true, cmd);
}

void lcdChar(const char chr) {
  lcdSend(false, (uint8_t)chr);
}

void lcdString(const char* str) {
  while(*str != '\0') {
    lcdChar(*str);
    str++;
  }
}

// ============================================================Main== //
int main(void) {
  StartHSE();
  hardware_init();

  // 4-bit mode, 2 lines, 5x7 format
  lcdCommand(0b00110000);
  // display & cursor home (keep this!)
  lcdCommand(0b00000010);
  // display on, right shift, underline off, blink off
  lcdCommand(0b00001100);
  // clear display (optional here)
  lcdCommand(0b00000001);

  lcdCommand(0b10000000); // set address to 0x00
  lcdString("Using HD44780");
  lcdCommand(0b11000000); // set address to 0x40
  lcdString("LCD directly! :)");
  
  do {
  //blink_pc13led(199);
  delay_ms(399);
  //delay_us(88000); 
  LEDTOGGLE;
  } while (1);
}
// ============================================================EoF=== //