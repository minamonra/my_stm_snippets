#include "common.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"

// Константы тактирования и задержек
#define CALIBRATION_NOP_PER_MS 12000U // Для delay_nop до старта ОС
#define HSE_STARTUP_TIMEOUT    0x10000UL // Таймаут запуска кварца

// Значения для регистра PLLCFGR (96 МГц)
#define PLL_M_HSI              16U
#define PLL_M_HSE              8U
#define PLL_N_96MHZ            192U
#define PLL_P_DIV2             0U
#define PLL_Q_DIV4             4U

void delay_nop(u32 nops) {
  while (nops--) { __NOP(); } // Пустой цикл
}

void clock_init(void) {
  RCC->CR |= RCC_CR_HSION; // Включаем HSI
  while (!(RCC->CR & RCC_CR_HSIRDY)); // Ждем HSI

  RCC->CFGR &= ~RCC_CFGR_SW; // Очистка SW
  RCC->CFGR |= RCC_CFGR_SW_HSI; // Переход на HSI
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI); // Ждем HSI

  RCC->CR |= RCC_CR_HSEON; // Включаем HSE
  u32 startup_counter = 0; // Счетчик таймаута
  while (!(RCC->CR & RCC_CR_HSERDY)) { // Ждем HSE
    startup_counter++;
    if (startup_counter > HSE_STARTUP_TIMEOUT) { // Если кварц не завёлся
      RCC->PLLCFGR = (PLL_M_HSI << RCC_PLLCFGR_PLLM_Pos) |
                     (PLL_N_96MHZ << RCC_PLLCFGR_PLLN_Pos) |
                     (PLL_P_DIV2 << RCC_PLLCFGR_PLLP_Pos) |
                     (PLL_Q_DIV4 << RCC_PLLCFGR_PLLQ_Pos) |
                     0U; // PLL от HSI
      goto pll_start; // На запуск PLL
    }
  }

  RCC->PLLCFGR = (PLL_M_HSE << RCC_PLLCFGR_PLLM_Pos) |
                 (PLL_N_96MHZ << RCC_PLLCFGR_PLLN_Pos) |
                 (PLL_P_DIV2 << RCC_PLLCFGR_PLLP_Pos) |
                 (PLL_Q_DIV4 << RCC_PLLCFGR_PLLQ_Pos) |
                 RCC_PLLCFGR_PLLSRC_HSE; // PLL от HSE

pll_start:
  FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN |
               FLASH_ACR_ICEN | FLASH_ACR_DCEN; // Настройка Flash

  RCC->CR |= RCC_CR_PLLON; // Включаем PLL
  while (!(RCC->CR & RCC_CR_PLLRDY)); // Ждем PLL

  RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2); // Сброс шин
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1;   // AHB = 96 МГц
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;  // APB1 = 48 МГц
  RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;  // APB2 = 96 МГц

  RCC->CFGR &= ~RCC_CFGR_SW; // Очистка SW
  RCC->CFGR |= RCC_CFGR_SW_PLL; // Переход на PLL
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); // Ждем PLL
}

void hw_init(void) {
  SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2)); // Включаем FPU
  __DSB(); // Синхронизация данных
  __ISB(); // Синхронизация конвейера

  RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN); // Тактирование GPIO
  delay_nop(10); // Ожидание шины

  GPIOB->MODER   &= ~(GPIO_MODER_MODER2); // Сброс PB2
  GPIOB->MODER   |=  (GPIO_MODER_MODER2_0); // PB2 = Выход
  GPIOB->OSPEEDR |=  (GPIO_OSPEEDR_OSPEED2); // PB2 = High speed
  LED_SYSTEM_OFF; // Потушить LED

  GPIOC->MODER   &= ~(GPIO_MODER_MODER13); // PC13 = Вход
  GPIOC->PUPDR   &= ~(GPIO_PUPDR_PUPDR13); // Сброс подтяжки
  GPIOC->PUPDR   |=  (GPIO_PUPDR_PUPDR13_1); // PC13 = Pull-down
}

void delay_ms(u32 ms) {
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        vTaskDelay(pdMS_TO_TICKS(ms)); // Задержка RTOS
    } else {
        delay_nop(ms * CALIBRATION_NOP_PER_MS); // Глухая задержка
    }
}

u32 get_tick_ms(void) {
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        return xTaskGetTickCount(); // Миллисекунды ОС
    }
    return 0; // ОС не запущена
}
