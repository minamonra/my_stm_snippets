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

#ifdef DEBUG
  RCC->CFGR &= ~(RCC_CFGR_MCO1 | RCC_CFGR_MCO1PRE); // Сброс битов MCO1 и делителя
  RCC->CFGR |= (3U << RCC_CFGR_MCO1_Pos) | (6U << RCC_CFGR_MCO1PRE_Pos); // MCO1 = PLL / 4
#endif

}

void hw_init(void) {
  // Включаем FPU
  SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2)); __DSB(); __ISB();

  // Включаем аппаратный кэш ядра Cortex-M4 (инструкции, данные, prefetch)
  FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;

  // Включаем тактирование портов A, B, C
  RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
  delay_nop(10);

  // === НАСТРОЙКА LED (PB2) ===
  GPIOB->MODER   = (GPIOB->MODER   & ~GPIO_PIN_MASK2(2)) | GPIO_PIN_BIT2(2, MODE_OUTPUT);
  GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~GPIO_PIN_MASK2(2)) | GPIO_PIN_BIT2(2, SPEED_HIGH);
  GPIOB->OTYPER &= ~(1U << 2); // Сброс в Push-Pull (TYPE_PP)
  LED_SYSTEM_OFF;

  // === НАСТРОЙКА КНОПКИ USER (PC13) ===
  GPIOC->MODER   = (GPIOC->MODER   & ~GPIO_PIN_MASK2(13)) | GPIO_PIN_BIT2(13, MODE_INPUT);
  GPIOC->PUPDR   = (GPIOC->PUPDR   & ~GPIO_PIN_MASK2(13)) | GPIO_PIN_BIT2(13, PULL_DOWN);

  // === ОТКЛЮЧЕНИЕ JTAG (ОСВОБОЖДАЕМ PA15, PB3, PB4, СОХРАНЯЯ SWD) ===
  GPIOA->MODER &= ~(GPIO_PIN_MASK2(15)); // PA15 = Вход (Очистка альтернативной функции)
  GPIOB->MODER &= ~(GPIO_PIN_MASK2(3) | GPIO_PIN_MASK2(4)); // PB3, PB4 = Вход

  // Для отладки MCO1 на PA8
#ifdef DEBUG
  // PA8 в режим альтернативной функции
  GPIOA->MODER = (GPIOA->MODER & ~GPIO_PIN_MASK2(8)) | GPIO_PIN_BIT2(8, MODE_AF);

  // Привязываем PA8 к AF0 (MCO1) через регистр высокой части AFR[1]
  GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL8);
  GPIOA->AFR[1] |=  (0U << GPIO_AFRH_AFSEL8_Pos);
#endif
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
