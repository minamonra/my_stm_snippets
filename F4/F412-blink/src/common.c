#include "common.h"
#include "pindefs.h"

volatile uint32_t ttms = 0;

void SysTick_Handler(void) {
    ttms++;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ttms;
    while ((ttms - start) < ms) {
        __WFI();
    }
}

void delay_nop(uint32_t ticks) {
    while (ticks--) {
        __NOP();
    }
}

void system_clock_config_100MHz(void) {
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

    RCC->PLLCFGR = (25 << RCC_PLLCFGR_PLLM_Pos) |
                   (200 << RCC_PLLCFGR_PLLN_Pos) |
                   (0 << RCC_PLLCFGR_PLLP_Pos)  |
                   (4 << RCC_PLLCFGR_PLLQ_Pos)  |
                   RCC_PLLCFGR_PLLSRC_HSE;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // Ручной запуск таймера SysTick на 1 мс
    //SysTick->LOAD = (100000000 / 1000) - 1;
    SysTick->LOAD = (F_CPU / 1000) - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void gpio_init(void) {
    // Включаем тактование необходимых портов на шине AHB1
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
    __asm("nop");

    // Настройка системного светодиода PB2 на вывод
    GPIOB->MODER &= ~(3U << (2 * 2));
    GPIOB->MODER |=  (1U << (2 * 2));
    GPIOB->OSPEEDR &= ~(3U << (2 * 2));

    // Настройка пина реле PC13 на вывод
    GPIOC->MODER &= ~(3U << (13 * 2));
    GPIOC->MODER |=  (1U << (13 * 2));
    GPIOC->OSPEEDR &= ~(3U << (13 * 2));
}