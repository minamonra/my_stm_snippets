#include "common.h"

void delay_nop(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        __NOP();
    }
}

void clock_init(void) {
    __IO uint32_t StartUpCounter = 0;
    RCC->CR |= ((uint32_t)RCC_CR_HSEON);

    do {
        ++StartUpCounter;
    } while (!(RCC->CR & RCC_CR_HSERDY) && (StartUpCounter < 10000));

    if (RCC->CR & RCC_CR_HSERDY) {
        FLASH->ACR |= FLASH_ACR_PRFTBE;
        FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
        FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;

        RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
        RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
        RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;

        RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));

        RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9);
        RCC->CR |= RCC_CR_PLLON;

        StartUpCounter = 0;
        while ((RCC->CR & RCC_CR_PLLRDY) == 0 && ++StartUpCounter < 1000) {}

        RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
        RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

        StartUpCounter = 0;
        while (((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) && ++StartUpCounter < 1000) {}
    } else {
        for (;;);
    }
    SystemCoreClockUpdate();
}
