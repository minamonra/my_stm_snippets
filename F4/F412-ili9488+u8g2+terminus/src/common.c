#include "common.h"
#include "pindefs.h"

//#define DEBUG  // Закомментировать эту строчку, если отладка осциллографом на PA8 больше не нужна

volatile uint32_t ttms = 0;

// Быстрый генератор случайных чисел
static uint32_t next_random = 12345;
uint16_t get_random(uint16_t max) {
    next_random = next_random * 1103515245 + 12345;
    return (uint16_t)((next_random / 65536) % max);
}

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

void spi_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    volatile uint32_t delay = RCC->APB2ENR; (void)delay;

    SPI1->CR1 = 0; 
    SPI1->CR2 = 0; 
    __asm("nop");

    // Запускаем SPI1 строго в 8-битном Master-режиме (DFF=0 по дефолту), Mode 3, скорость /2 (50 МГц)
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | 
                SPI_CR1_CPOL | SPI_CR1_CPHA | (0U << SPI_CR1_BR_Pos);
                
    SPI1->CR1 |= SPI_CR1_SPE;
}

static void system_clock_config_100MHz(void) {
    // 1. Включаем внутренний генератор HSI для безопасной коммутации шин
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    // Переводим систему временно на безопасный HSI
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

    // 2. Включаем внешний кварцевый резонатор HSE (8 МГц)
    RCC->CR |= RCC_CR_HSEON;
    uint32_t startup_counter = 0;
    while (!(RCC->CR & RCC_CR_HSERDY)) {
        startup_counter++;
        if (startup_counter > 0x10000) {
            // Аварийный выход на HSI, если кварц не завелся
            RCC->PLLCFGR = (16U << RCC_PLLCFGR_PLLM_Pos) | (200U << RCC_PLLCFGR_PLLN_Pos) | (0U << RCC_PLLCFGR_PLLP_Pos) | (4U << RCC_PLLCFGR_PLLQ_Pos) | 0U;
            goto pll_start;
        }
    }

    // ЭТАЛОННАЯ КОНФИГУРАЦИЯ ПОД КВАРЦ 8 МГц ДЛЯ ЧЕСТНЫХ 100 МГц ЯДРА:
    // Вход 8 МГц / M(8) = 1 МГц.
    // 1 МГц * N(200) = 200 МГц — внутренняя частота генератора VCO.
    // 200 МГц / P(2) = 100 МГц — номинальная скорость процессора SYSCLK!
    RCC->PLLCFGR = (8U << RCC_PLLCFGR_PLLM_Pos)    |                            // Делитель M = 8 (Входной сигнал приводим к 1 МГц)
                   (200U << RCC_PLLCFGR_PLLN_Pos)  |                            // Множитель VCO N = 200
                   (0U << RCC_PLLCFGR_PLLP_Pos)    |                            // Делитель ядра P = 2 (биты 00 дают деление на 2 -> 100 МГц)
                   (4U << RCC_PLLCFGR_PLLQ_Pos)    |                            // Делитель Q = 4
                   RCC_PLLCFGR_PLLSRC_HSE;                                      // Источник — внешний кварц HSE

pll_start:
    // 3. Настройка задержек Flash памяти под финальные 100 МГц (3 цикла ожидания WS)
    FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    // 4. Запускаем умножитель частоты PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));                                         // Жестко ждем стабилизации частоты PLL

    // 5. Настройка делителей системных шин периферии
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);            
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;                                           // APB1 = 50 МГц (Максимум шины)
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;                                           // APB2 = 100 МГц (Максимум шины, тут наш SPI1!)

    // 6. Переключаем тактирование процессора (SYSCLK) на выходной сигнал PLL
    RCC->CFGR &= ~RCC_CFGR_SW;                                                  
    RCC->CFGR |= RCC_CFGR_SW_PLL;                                               
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);                     // Ждем от аппаратного переключателя подтверждения перехода

#ifdef DEBUG
    // --- КОНТРОЛЬНЫЙ ВЫВОД СИГНАЛА НА НОЖКУ MCO1 (PA8) ---
    RCC->CFGR &= ~RCC_CFGR_MCO1;
    RCC->CFGR |= (3U << 21);                                                    // Выбираем источник MCO1 = PLL (0b11)
    
    RCC->CFGR &= ~RCC_CFGR_MCO1PRE;
    RCC->CFGR |= (6U << 24);                                                    // Аппаратный делитель MCO1 на 4 (100 / 4 = 25.00 МГц)
#endif

    // Возвращаем SysTick на честные 100 МГц
    SysTick->LOAD = (100000000 / 1000) - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void gpio_init(void) {
    // Включаем тактирование портов GPIO и модуля DMA2 на шине AHB1
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_DMA2EN);
    
    // Небольшая задержка после включения тактирования
    volatile uint32_t delay = RCC->AHB1ENR; 
    (void)delay;
    __asm("nop");

    // --- НАСТРОЙКА ПИНОВ НА ВЫХОД (MAX SPEED) ---
    // Настраиваем: PA1, PA2, PA3, PA4 и PB2 как General Purpose Output (Push-Pull, Max Speed)

    // Сброс режимов (Reset)
    GPIOA->MODER &= ~(GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3 | GPIO_MODER_MODER4);
    GPIOB->MODER &= ~(GPIO_MODER_MODER2);

    // Установка режима Выход (01)
    GPIOA->MODER |=  (GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0);
    GPIOB->MODER |=  (GPIO_MODER_MODER2_0);

    // Установка Push-Pull (0)
    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT1 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3 | GPIO_OTYPER_OT4);
    
    // Установка Максимальной скорости (11)
    GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED1 | GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3 | GPIO_OSPEEDR_OSPEED4);
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED2);

    // --- НАСТРОЙКА АЛЬТЕРНАТИВНЫХ ФУНКЦИЙ ---
    // Настраиваем PA5 и PA7 как Alternate Function (AF5), High Speed
    GPIOA->MODER   &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER7);
    GPIOA->MODER   |=  (GPIO_MODER_MODER5_1 | GPIO_MODER_MODER7_1); // Режим AF (10)
    GPIOA->OSPEEDR |=  (GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED7); // Max Speed (11)
    GPIOA->OTYPER  &= ~(GPIO_OTYPER_OT5 | GPIO_OTYPER_OT7);         // Push-Pull (0)
    
    // Привязка PA5 и PA7 к AF5 (например, SPI1)
    GPIOA->AFR[0]  &= ~(GPIO_AFRL_AFSEL5 | GPIO_AFRL_AFSEL7);
    GPIOA->AFR[0]  |=  ((5U << GPIO_AFRL_AFSEL5_Pos) | (5U << GPIO_AFRL_AFSEL7_Pos));

    // --- НАСТРОЙКА ВЫВОДА ТАКТОВ MCO1 (PA8) ---
    // Переводим пин PA8 в режим Альтернативной Функции (10), Максимальная скорость
    GPIOA->MODER   &= ~(GPIO_MODER_MODER8);
    GPIOA->MODER   |=  (GPIO_MODER_MODER8_1);
    GPIOA->OSPEEDR |=  (GPIO_OSPEEDR_OSPEED8);
    
#ifdef DEBUG
    // Привязываем PA8 к альтернативной функции AF00 (MCO1)
    GPIOA->AFR[1]  &= ~(GPIO_AFRH_AFSEL8);
    GPIOA->AFR[1]  |=  (0U << GPIO_AFRH_AFSEL8_Pos);
#endif
}

void dma_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    __asm("nop");

    RESET_DMA_STREAM5();
    DMA2_Stream5->CR = 0; 
    __asm("nop");

    // Конфигурируем Поток строго в 8-битный режим раз и навсегда
    DMA2_Stream5->CR |= (3U << DMA_SxCR_CHSEL_Pos)  | // Канал 3 (SPI1_TX)
                        (1U << DMA_SxCR_DIR_Pos)    | // Направление: память -> периферия
                        (0U << DMA_SxCR_MSIZE_Pos)  | // Память: 8 бит (00)
                        (0U << DMA_SxCR_PSIZE_Pos)  | // Периферия: 8 бит (00)
                        DMA_SxCR_MINC               | // Инкремент адреса памяти включен
                        (3U << DMA_SxCR_PL_Pos);      // Очень высокий приоритет шины

    DMA2_Stream5->PAR = (uint32_t)&(SPI1->DR);        // Адрес назначения — регистр данных SPI1
    SPI1->CR2 |= SPI_CR2_TXDMAEN;                     // Разрешаем SPI слать запросы к DMA
}

void System_Init(void) {
    system_clock_config_100MHz();
    gpio_init();
    spi_init();
    dma_init();
    
}

// Внутренний UTF-8 декодер кириллицы преобразует 2-байтовый UTF-8 символ в стандартный индекс 192-255 (CP1251)
uint16_t decode_utf8(const char **str) {
    uint8_t c = (uint8_t)(**str);
    if (c == 0) return 0;
    
    (*str)++;
    
    // 1-байтовый символ (ASCII: 0x00 - 0x7F)
    if ((c & 0x80) == 0) {
        return c;
    }
    
    // 2-байтовый символ (Кириллица и др.: 0xC0 - 0xDF)
    if ((c & 0xE0) == 0xC0) {
        uint8_t next_c = (uint8_t)(**str);
        if (next_c == 0) return 0;
        (*str)++;
        
        // Склеиваем 2 байта в один честный Unicode код
        return ((c & 0x1F) << 6) | (next_c & 0x3F);
    }
    
    // 3-байтовый символ (Если вдруг прилетят сложные символы: 0xE0 - 0xEF)
    if ((c & 0xF0) == 0xE0) {
        uint8_t c2 = (uint8_t)(**str); if (c2 == 0) return 0; (*str)++;
        uint8_t c3 = (uint8_t)(**str); if (c3 == 0) return 0; (*str)++;
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }
    
    return c;
}




// void dma_init(void) {
//     RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
//     __asm("nop");

//     DMA2_Stream5->CR &= ~DMA_SxCR_EN;
//     while (DMA2_Stream5->CR & DMA_SxCR_EN);

//     DMA2->HIFCR = (DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5);
//     DMA2_Stream5->CR = 0; __asm("nop");

//     // // НАШ ЧЕСТНЫЙ И ИДЕАЛЬНО РАБОТАЮЩИЙ 16-БИТНЫЙ КОНВЕЙЕР
//     // DMA2_Stream5->CR |= (3U << DMA_SxCR_CHSEL_Pos)  | 
//     //                     (1U << DMA_SxCR_DIR_Pos)    | 
//     //                     (1U << DMA_SxCR_MSIZE_Pos)  |                           // ОЗУ: 16 бит (Half-Word: 01)
//     //                     (1U << DMA_SxCR_PSIZE_Pos)  |                           // SPI1: 16 бит (Half-Word: 01)
//     //                     DMA_SxCR_MINC               | 
//     //                     (3U << DMA_SxCR_PL_Pos);


//     // По умолчанию сбрасываем в 8-битный режим (00)
//     DMA2_Stream5->CR |= (3U << DMA_SxCR_CHSEL_Pos)  | 
//                         (1U << DMA_SxCR_DIR_Pos)    | 
//                         (0U << DMA_SxCR_MSIZE_Pos)  |                           // 8-бит память по дефолту
//                         (0U << DMA_SxCR_PSIZE_Pos)  |                           // 8-бит периферия по дефолту
//                         DMA_SxCR_MINC               | 
//                         (3U << DMA_SxCR_PL_Pos);


//     DMA2_Stream5->PAR = (uint32_t)&(SPI1->DR);
//     SPI1->CR2 |= SPI_CR2_TXDMAEN;
// }

// void spi_init(void) {
//     RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
//     volatile uint32_t delay = RCC->APB2ENR; (void)delay;

//     SPI1->CR1 = 0; SPI1->CR2 = 0; __asm("nop");

//     // Запускаем SPI1 в 8-битном Master-режиме, Mode 3, скорость /2 (50 МГц)
//     SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | 
//                 SPI_CR1_CPOL | SPI_CR1_CPHA | (0U << SPI_CR1_BR_Pos);
                
//     SPI1->CR1 |= SPI_CR1_SPE;
// }