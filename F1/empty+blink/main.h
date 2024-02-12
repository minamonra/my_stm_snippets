#pragma once
#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include "stm32f10x.h"
#include "stdbool.h"

// GPIO
//
// CNF1: 0 - general output or input; 1 - alternate output or pullup/down input
// CNF0: 0 - push/pull, analog or pullup/down input
// MODE: 00 - input, 01 - 10MHz, 10 - 2MHz, 11 - 50MHz
// Pullup/down: ODR = 0 - pulldown, 1 - pullup
// GPIO_BSRR and BRR also works
// IDR - input, ODR - output (or pullups management), 
//
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

void hardware_init(void)
{
  // GPIOA->CRL = CRL(6, CNF_PPOUTPUT|MODE_SLOW); // test pin (MISO)
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // PORT_A enable
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // PORT_B enable
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; // PORT_C enable

  if (SysTick_Config(72000)) { while(1); } // конфигурация SysTick = 1ms при тактовой 72MHz
  SysTick->LOAD |= SysTick_CTRL_ENABLE; // включить SysTick
  // LED PC13
  GPIOC->CRH &= ~GPIO_CRH_CNF13;
  GPIOC->CRH |= CRH(13, CNF_PPOUTPUT | MODE_SLOW);
}

#endif // __MAIN_H__