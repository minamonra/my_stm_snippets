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

void blink_led(uint16_t freq) {
    static uint32_t last_time = 0;                                              // Хранит снимок системного времени ttms (статическая локальная)

    // Проверка на переполнение счетчика или истечение интервала freq
    if (last_time > ttms || (ttms - last_time) >= freq) {
        LED_SYSTEM_TOGGLE;                                                      // Переключаем светодиод через наш макрос
        last_time = ttms;                                                       // Делаем новый снимок глобального времени ttms
    }
}

static void system_clock_config_100MHz(void) {
    RCC->CR |= RCC_CR_HSEON;                                                    // Включаем внешний кварцевый резонатор HSE (25 МГц)
    while (!(RCC->CR & RCC_CR_HSERDY));                                         // Ожидаем стабилизации тактового сигнала от HSE

    // Для частоты 100 МГц при питании 3.3В по даташиту требуется 3 цикла ожидания Flash (3WS)
    FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN; // Настройка задержки Flash, включение префетча и кэша инструкций/данных

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);            // Очищаем старые делители системных шин AHB, APB1 и APB2
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;                                           // Делитель APB1 равен 2 (Частота шины периферии APB1 = 50 МГц, максимум 50)
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;                                           // Делитель APB2 равен 1 (Частота шины периферии APB2 = 100 МГц, максимум 100)

    // Конфигурация умножителя PLL: Вход 25 МГц / M(25) * N(200) / P(2) = 100 МГц системной частоты
    RCC->PLLCFGR = (25 << RCC_PLLCFGR_PLLM_Pos) |                               // Делитель входной частоты M=25 (Приводим вход к стабильной частоте 1 МГц)
                   (200 << RCC_PLLCFGR_PLLN_Pos) |                              // Множитель частоты генератора VCO N=200 (Получаем внутренние 200 МГц)
                   (0 << RCC_PLLCFGR_PLLP_Pos)  |                               // Делитель системного ядра P=2 (Значение битов 00 делит VCO на 2 -> 100 МГц)
                   (4 << RCC_PLLCFGR_PLLQ_Pos)  |                               // Делитель частоты для USB/SDIO Q=4 (Дает ровно необходимые 48 МГц)
                   RCC_PLLCFGR_PLLSRC_HSE;                                      // Источником опорной тактовой частоты для PLL выбираем HSE

    RCC->CR |= RCC_CR_PLLON;                                                    // Включаем основной блок умножителя частоты PLL
    while (!(RCC->CR & RCC_CR_PLLRDY));                                         // Ожидаем стабилизации и захвата частоты генератором PLL

    RCC->CFGR |= RCC_CFGR_SW_PLL;                                               // Переключаем тактирование процессора (SYSCLK) на выходной сигнал PLL
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);                     // Ожидаем от аппаратного контроллера подтверждения перехода на PLL

    // Ручной запуск системного таймера SysTick на прерывания каждые 1 мс
    SysTick->LOAD = (F_CPU / 1000) - 1;                                         // Задаем период перезагрузки счетчика на основе частоты из Makefile
    SysTick->VAL  = 0;                                                          // Принудительно обнуляем текущее значение счетчика SysTick
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |                                // Задаем источник тактов — полная частота процессора без делителей
                    SysTick_CTRL_TICKINT_Msk   |                                // Разрешаем генерацию прерывания SysTick_Handler при обнулении
                    SysTick_CTRL_ENABLE_Msk;                                    // Включаем системный таймер SysTick в работу
}

static void gpio_init(void) {
    // 1. Включаем тактование необходимых портов на шине AHB1
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
    __asm("nop");                                                               // Пауза для стабилизации тактов

    // Настройка системного светодиода PB2 на вывод
    GPIOB->MODER   &= ~GPIO_MODER_MODER2;    // Очищаем оба бита настройки пина PB2 (в 00)
    GPIOB->MODER   |=  GPIO_MODER_MODER2_0;   // Выставляем режим вывода (General purpose output: 01)
    GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR2; // Сбрасываем скорость в Low speed (00)

    // 3. Настройка пина реле PC13 на вывод
    GPIOC->MODER   &= ~GPIO_MODER_MODER13;                                      // Очищаем оба бита настройки пина PC13 (в 00)
    GPIOC->MODER   |=  GPIO_MODER_MODER13_0;                                    // Выставляем режим вывода (General purpose output: 01)
    GPIOC->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR13;                                 // Сбрасываем скорость в Low speed (00)
}

// Главная публичная функция инициализации
void System_Init(void) {
    system_clock_config_100MHz();                                               // Настраиваем клок и SysTick внутри модуля
    gpio_init();                                                                // Настраиваем порты внутри модуля
}