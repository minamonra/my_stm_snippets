#pragma once
#ifndef __COMMON_H__
#define __COMMON_H__
#include "stm32f10x.h"
#include <stddef.h> 

volatile uint32_t ttms   = 0;
volatile uint32_t ddms   = 0;
volatile uint32_t pc13ms = 0;


#define LED1TOGGLE GPIOC->ODR ^= (1<<13)
#define LED2TOGGLE GPIOC->ODR ^= (1<<14)





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



static void hardware_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // PORT_A enable
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // PORT_B enable
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; // PORT_C enable
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;

  // Отключить JTAG, оставить SWD
  AFIO->MAPR &= ~AFIO_MAPR_SWJ_CFG;
  AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
  if (SysTick_Config(72000)) { while(1); } // конфигурация SysTick = 1ms при тактовой 72MHz
  SysTick->LOAD |= SysTick_CTRL_ENABLE;    // включить SysTick

  // LED PC13
  GPIOC->CRH &= ~GPIO_CRH_CNF13;
  GPIOC->CRH |= CRH(13, CNF_PPOUTPUT | MODE_NORMAL);
  // PA15 (RS)
  GPIOA->CRH &= ~GPIO_CRH_CNF15;
  GPIOA->CRH |= CRH(15, CNF_PPOUTPUT | MODE_NORMAL);
  // PB3 (EN)
  GPIOB->CRL &= ~GPIO_CRL_CNF3;	  // Сбрасываем биты CNF
  GPIOB->CRL |= CRL(3, CNF_PPOUTPUT | MODE_NORMAL);
  // PB4 (D4)
  GPIOB->CRL &= ~GPIO_CRL_CNF4;
  GPIOB->CRL |= CRL(4, CNF_PPOUTPUT | MODE_NORMAL);
  // PB5 (D5)
  GPIOB->CRL &= ~GPIO_CRL_CNF5;
  GPIOB->CRL |= CRL(5, CNF_PPOUTPUT | MODE_NORMAL);
  // PB8 (D6)
  GPIOB->CRH &= ~GPIO_CRH_CNF8;
  GPIOB->CRH |= CRH(8, CNF_PPOUTPUT | MODE_NORMAL);
  // PB9 (D7)
  GPIOB->CRH &= ~GPIO_CRH_CNF9;
  GPIOB->CRH |= CRH(9, CNF_PPOUTPUT | MODE_NORMAL);
}

static void StartHSE(void)
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
    LED1TOGGLE;
    pc13ms = ttms;
  }
}

void delay_ms(uint16_t ms)
{
  ddms = ms;
  do {} while (ddms);
}


size_t utf8_to_cp1251(const char *src, char *dst, size_t dst_size) {
    size_t i = 0, j = 0;
    
    while (src[i] && j + 1 < dst_size) {
        unsigned char c = (unsigned char)src[i];
        
        if (c < 128) {
            // ASCII
            dst[j++] = c;
            i++;
        }
        else if (c == 0xD0) {
            // Первый байт кириллического символа
            unsigned char c2 = (unsigned char)src[i+1];
            if (c2 >= 0x90 && c2 <= 0xBF) {
                // А-п
                dst[j++] = c2 + 0x30;
            }
            else if (c2 == 0x81) {
                // Ё
                dst[j++] = 0xA8;
            }
            else if (c2 == 0x84) {
                // Є
                dst[j++] = 0xA2;
            }
            else if (c2 == 0x86) {
                // І
                dst[j++] = 0xA3;
            }
            else if (c2 == 0x87) {
                // Ї
                dst[j++] = 0xA1;
            }
            else {
                dst[j++] = '?';
            }
            i += 2;
        }
        else if (c == 0xD1) {
            // Первый байт для р-я и ё
            unsigned char c2 = (unsigned char)src[i+1];
            if (c2 >= 0x80 && c2 <= 0x8F) {
                // р-я
                dst[j++] = c2 + 0x70;
            }
            else if (c2 == 0x91) {
                // ё
                dst[j++] = 0xB8;
            }
            else if (c2 == 0x94) {
                // є
                dst[j++] = 0xB3;
            }
            else if (c2 == 0x96) {
                // і
                dst[j++] = 0xB4;
            }
            else if (c2 == 0x97) {
                // ї
                dst[j++] = 0xAF;
            }
            else {
                dst[j++] = '?';
            }
            i += 2;
        }
        else {
            dst[j++] = '?';
            i++;
        }
    }
    
    dst[j] = '\0';
    return j;
}
#endif // __COMMON_H__