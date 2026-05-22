#include "hardware.h"

volatile uint32_t ttms = 0;                                                     // Системный таймер миллисекунд

void SystemInit(void) {
    // Оставляем пустой, так как всю настройку тактирования делаем в SystemClock_Config_84MHz
}


/*
void Periph_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN; // Включаем тактирование портов A, B, C
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN  | RCC_AHB1ENR_DMA2EN;                   // Включаем тактирование DMA1 и DMA2
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN  | RCC_APB2ENR_SYSCFGEN;                  // Включаем SPI1 экрана и системную конфигурацию
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN  | RCC_APB1ENR_SPI3EN;                   // Включаем I2S2 для звука и SPI3 для флешки

    DBGMCU->CR &= ~DBGMCU_CR_TRACE_IOEN;                                        // Отключаем трассировку для освобождения пинов

    // =========================================================================
    // 1. ПОРТ А: АППАРАТНЫЙ ЭКРАН (SPI1) + СВОБОДНЫЕ ЛИНИИ КАРТЫ
    // =========================================================================
    GPIOA->MODER   &= ~(GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3 | 
                        GPIO_MODER_MODER4 | GPIO_MODER_MODER8 | GPIO_MODER_MODER9); // Сброс режимов управляющих пинов
    GPIOA->MODER   |=  (GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | 
                        GPIO_MODER_MODER4_0 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0); // Настройка управляющих пинов на выход

    GPIOA->OSPEEDR |=  (GPIO_OSPEEDR_OSPEED1 | GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3 | 
                        GPIO_OSPEEDR_OSPEED4 | GPIO_OSPEEDR_OSPEED8 | GPIO_OSPEEDR_OSPEED9); // Максимальная скорость для всех выходов

    GPIOA->BSRR     =  GPIO_BSRR_BS1 | GPIO_BSRR_BS2 | GPIO_BSRR_BS3 | GPIO_BSRR_BS4 | 
                       GPIO_BSRR_BS8 | GPIO_BSRR_BS9;                           // Установка высокого уровня на пинах CS/ресет

    GPIOA->MODER   &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER7);                 // Сброс режимов для пинов SPI1 (SCK, MOSI)
    GPIOA->MODER   |=  (GPIO_MODER_MODER5_1 | GPIO_MODER_MODER7_1);               // Перевод пинов PA5 и PA7 в режим альтернативной функции
    GPIOA->OSPEEDR |=  (GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED7);           // Максимальная скорость для линий SPI1
    
    GPIOA->AFR[0]  &= ~((0x0F << GPIO_AFRL_AFSEL5_Pos) | (0x0F << GPIO_AFRL_AFSEL7_Pos)); // Очистка альтернативных функций PA5, PA7
    GPIOA->AFR[0]  |=  ((5 << GPIO_AFRL_AFSEL5_Pos) | (5 << GPIO_AFRL_AFSEL7_Pos)); // Назначение AF5 (SPI1) на пины PA5 и PA7

    GPIOA->MODER   &= ~(GPIO_MODER_MODER6);                                     // PA6 (MISO) переводим в режим входа
    GPIOA->PUPDR   &= ~(GPIO_PUPDR_PUPDR6);                                     // Очистка подтяжки для PA6
    GPIOA->PUPDR   |=  (GPIO_PUPDR_PUPDR6_0);                                   // Включение подтяжки вверх (Pull-up) на MISO флешки


        // --- НАСТРОЙКА КНОПКИ PA0 С ПОДТЯЖКОЙ PULL-UP ---
    
    GPIOA->MODER &= ~(3 << (0 * 2)); // Режим: Вход (Input)
    GPIOA->PUPDR &= ~(3 << (0 * 2)); // Сброс старых настроек
    GPIOA->PUPDR |=  (1 << (0 * 2)); // Включаем Pull-up (в покое 1, нажата 0)
    // =========================================================================
    // 2. ПОРТ B: ЗВУК I2S2 (12, 13, 15) + АППАРАТНЫЙ SPI3 ДЛЯ КАРТЫ (3, 4, 5)
    // =========================================================================
    // Сбрасываем маски режимов для нижних пинов (PB3, PB4, PB5) и верхних пинов звука (PB12, PB13, PB15)
    GPIOB->MODER   &= ~((3 << (3 * 2))  | (3 << (4 * 2))  | (3 << (5 * 2)) |
                        (3 << (12 * 2)) | (3 << (13 * 2)) | (3 << (15 * 2)));   
    // Включаем режим Альтернативной Функции (10 в бинарнике, то есть сдвиг бита _1)
    GPIOB->MODER   |=  ((2 << (3 * 2))  | (2 << (4 * 2))  | (2 << (5 * 2)) |
                        (2 << (12 * 2)) | (2 << (13 * 2)) | (2 << (15 * 2)));   
    // Выставляем максимальную скорость тактования для всех линий Порта B
    GPIOB->OSPEEDR |=  ((3 << (3 * 2))  | (3 << (4 * 2))  | (3 << (5 * 2)) |
                        (3 << (12 * 2)) | (3 << (13 * 2)) | (3 << (15 * 2)));   
    
    // Назначаем AF6 (SPI3) на нижние пины PB3 (CLK), PB4 (MISO), PB5 (MOSI). Используем AFR[0]!
    GPIOB->AFR[0]  &= ~((0xF << (3 * 4)) | (0xF << (4 * 4)) | (0xF << (5 * 4)));
    GPIOB->AFR[0]  |=  ((6   << (3 * 4)) | (6   << (4 * 4)) | (6   << (5 * 4)));

    // Назначаем AF5 (I2S2) на верхние пины PB12, PB13, PB15. Используем AFR[1]!
    GPIOB->AFR[1]  &= ~((0xF << ((12 - 8) * 4)) | (0xF << ((13 - 8) * 4)) | (0xF << ((15 - 8) * 4))); 
    GPIOB->AFR[1]  |=  ((5   << ((12 - 8) * 4)) | (5   << ((13 - 8) * 4)) | (5   << ((15 - 8) * 4))); 

    // Включаем подтяжку вверх на линию ввода данных карты PB4 (SPI3_MISO)
    GPIOB->PUPDR   &= ~(3 << (4 * 2));
    GPIOB->PUPDR   |=  (1 << (4 * 2));

    // =========================================================================
    // 3. ПОРТ C: ПОДСВЕТКА И СВЕТОДИОД ПЛАТЫ
    // =========================================================================
    GPIOC->MODER   &= ~(GPIO_MODER_MODER13 | GPIO_MODER_MODER15);               // Сброс режимов PC13 и PC15
    GPIOC->MODER   |=  (GPIO_MODER_MODER13_0 | GPIO_MODER_MODER15_0);           // Настройка PC13 и PC15 на выход
    GPIOC->BSRR     =  GPIO_BSRR_BS13 | GPIO_BSRR_BS15;                         // Отключение светодиода и подсветки при старте

   // =========================================================================
    // 4. НАСТРОЙКА И СТАРТ СИСТЕМНЫХ ИНТЕРФЕЙСОВ
    // =========================================================================
    // Исправлено: BR=0 → максимальная скорость 42 МГц
    SPI1->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR | (0 << SPI_CR1_BR_Pos);
    SPI1->CR1 |= SPI_CR1_SPE;

    // SPI3 для SD-карты: стартовая скорость 2.6 МГц (BR=3), потом разгонится в SD_Init
    SPI3->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR | (3 << SPI_CR1_BR_Pos);
    SPI3->CR1 |= SPI_CR1_SPE;

    SysTick->LOAD = (84000000 / 1000) - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}
*/
void Periph_Init(void) {
    // 1. Включение тактирования портов и системных блоков
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN  | RCC_AHB1ENR_DMA2EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN  | RCC_APB2ENR_SYSCFGEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN  | RCC_APB1ENR_SPI3EN;

    // Отключение трассировки для освобождения пинов (если нужно)
    DBGMCU->CR &= ~DBGMCU_CR_TRACE_IOEN;

    // 2. НАСТРОЙКА КНОПКИ USER (PA0) С ПОДТЯЖКОЙ PULL-UP
    GPIOA->MODER   &= ~GPIO_MODER_MODER0;    // Режим: Вход (00)
    GPIOA->PUPDR   &= ~GPIO_PUPDR_PUPDR0;    // Сброс подтяжки
    GPIOA->PUPDR   |=  GPIO_PUPDR_PUPDR0_0;  // Включение Pull-up (01)

    // 3. Выходы дисплея ST7789 (PA1..PA4, PA8, PA9)
    GPIOA->MODER   &= ~(GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3 | 
                        GPIO_MODER_MODER4 | GPIO_MODER_MODER8 | GPIO_MODER_MODER9);
    GPIOA->MODER   |=  (GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | 
                        GPIO_MODER_MODER4_0 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0);

    // Максимальная скорость для пинов дисплея (чтобы DMA качало без затыков)
    GPIOA->OSPEEDR |=  (GPIO_OSPEEDR_OSPEED1 | GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3 | 
                        GPIO_OSPEEDR_OSPEED4 | GPIO_OSPEEDR_OSPEED8 | GPIO_OSPEEDR_OSPEED9);

    // Установка начального HIGH уровня на линиях управления экраном
    GPIOA->BSRR     =  GPIO_BSRR_BS1 | GPIO_BSRR_BS2 | GPIO_BSRR_BS3 | GPIO_BSRR_BS4 | 
                       GPIO_BSRR_BS8 | GPIO_BSRR_BS9;

    // 4. SPI1 Пины дисплея (PA5 = CLK, PA7 = MOSI) в режим Alternate Function
    GPIOA->MODER   &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER7);
    GPIOA->MODER   |=  (GPIO_MODER_MODER5_1 | GPIO_MODER_MODER7_1); 
    GPIOA->OSPEEDR |=  (GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED7);
    
    GPIOA->AFR[0]  &= ~((0x0F << GPIO_AFRL_AFSEL5_Pos) | (0x0F << GPIO_AFRL_AFSEL7_Pos));
    GPIOA->AFR[0]  |=  ((5 << GPIO_AFRL_AFSEL5_Pos) | (5 << GPIO_AFRL_AFSEL7_Pos)); // AF5 = SPI1

    // 5. PA6 (MISO для SPI1, если используется) с подтяжкой
    GPIOA->MODER   &= ~(GPIO_MODER_MODER6);
    GPIOA->PUPDR   &= ~(GPIO_PUPDR_PUPDR6);
    GPIOA->PUPDR   |=  GPIO_PUPDR_PUPDR6_0;

    // 6. Настройка шины звука I2S и SD-карты на GPIOB (PB3, PB4, PB5, PB12, PB13, PB15)
    GPIOB->MODER   &= ~((3 << (3 * 2))  | (3 << (4 * 2))  | (3 << (5 * 2)) |
                        (3 << (12 * 2)) | (3 << (13 * 2)) | (3 << (15 * 2)));   
    GPIOB->MODER   |=  ((2 << (3 * 2))  | (2 << (4 * 2))  | (2 << (5 * 2)) |
                        (2 << (12 * 2)) | (2 << (13 * 2)) | (2 << (15 * 2))); // Режим AF (10)  

    GPIOB->OSPEEDR |=  ((3 << (3 * 2))  | (3 << (4 * 2))  | (3 << (5 * 2)) |
                        (3 << (12 * 2)) | (3 << (13 * 2)) | (3 << (15 * 2))); // Max Speed
    
    // Назначение альтернативных функций для нижних пинов GPIOB
    GPIOB->AFR[0]  &= ~((0xF << (3 * 4)) | (0xF << (4 * 4)) | (0xF << (5 * 4)));
    GPIOB->AFR[0]  |=  ((6   << (3 * 4)) | (6   << (4 * 4)) | (6   << (5 * 4))); // AF6

    // Назначение альтернативных функций для верхних пинов GPIOB
    GPIOB->AFR[1]  &= ~((0xF << ((12 - 8) * 4)) | (0xF << ((13 - 8) * 4)) | (0xF << ((15 - 8) * 4))); 
    GPIOB->AFR[1]  |=  ((5   << ((12 - 8) * 4)) | (5   << ((13 - 8) * 4)) | (5   << ((15 - 8) * 4))); // AF5 

    // Подтяжка на линию чтения данных SD-карты (PB4)
    GPIOB->PUPDR   &= ~(3 << (4 * 2));
    GPIOB->PUPDR   |=  (1 << (4 * 2));

    // 7. Светодиоды платы на GPIOC (PC13 и PC15) в режим обычного Выхода
    GPIOC->MODER   &= ~(GPIO_MODER_MODER13 | GPIO_MODER_MODER15);
    GPIOC->MODER   |=  (GPIO_MODER_MODER13_0 | GPIO_MODER_MODER15_0);
    GPIOC->BSRR     =  GPIO_BSRR_BS13 | GPIO_BSRR_BS15; // Потушены по умолчанию
 
    // 8. Запуск аппаратного SPI1 (Дисплей)
    SPI1->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR | (0 << SPI_CR1_BR_Pos);
    SPI1->CR1 |= SPI_CR1_SPE;

    // 9. Запуск аппаратного SPI3 (SD-карта) с делителем частоты DIV16 (3) для разгона шины
    SPI3->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR | (3 << SPI_CR1_BR_Pos);
    SPI3->CR1 |= SPI_CR1_SPE;

    // 10. Настройка системного таймера SysTick на интервал 1 мс (ttms)
    SysTick->LOAD = (84000000 / 1000) - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}


void SysTick_Handler(void) {
    ttms++;                                                                     // Инкремент счетчика времени каждую миллисекунду
}

void delay_ms(uint32_t ms) {
    uint32_t start = ttms;                                                      // Запоминаем текущее время старта
    while ((ttms - start) < ms) {                                               // Крутимся в цикле, пока не пройдет нужное время
        __WFI();                                                                // Спим до прерывания для экономии энергии
    }
}

void delay_nop(uint32_t ticks) {
    while (ticks--) {
        __NOP();                                                                // Пустая процессорная инструкция задержки
    }
}

void SystemClock_Config_84MHz(void) {
    RCC->CR |= RCC_CR_HSEON;                                                    // Включаем внешний кварц HSE (25 МГц)
    while (!(RCC->CR & RCC_CR_HSERDY));                                         // Ожидаем стабилизации кварцевого резонатора

    FLASH->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN; // Настройка Flash на 2 задержки WS и кэш

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);            // Очистка делителей шин процессора AHB/APB
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;                                           // Делитель APB1 равен 2 (Частота шины 42 МГц)

    RCC->PLLCFGR = (25 << RCC_PLLCFGR_PLLM_Pos) |                               // Делитель входной частоты M=25 (Получаем 1 МГц)
                   (336 << RCC_PLLCFGR_PLLN_Pos) |                              // Множитель частоты VCO N=336 (Получаем 336 МГц)
                   (1 << RCC_PLLCFGR_PLLP_Pos) |                                // Делитель системного ядра P=4 (Получаем 84 МГц)
                   RCC_PLLCFGR_PLLSRC_HSE;                                      // Источником тактирования для PLL выбран HSE

    RCC->CR |= RCC_CR_PLLON;                                                    // Запускаем основной блок умножителя частоты PLL
    while (!(RCC->CR & RCC_CR_PLLRDY));                                         // Дожидаемся готовности основного генератора PLL

    RCC->CFGR |= RCC_CFGR_SW_PLL;                                               // Переключаем системный клок процессора на PLL
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);                       // Ждем подтверждения переключения процессора

    RCC->CR &= ~RCC_CR_PLLI2SON;                                                // Отключаем аудио генератор для его настройки
    while(RCC->CR & RCC_CR_PLLI2SRDY);                                          // Убеждаемся что модуль аудио тактирования затих
    
    RCC->PLLI2SCFGR &= ~(RCC_PLLI2SCFGR_PLLI2SN | RCC_PLLI2SCFGR_PLLI2SR);      // Очищаем старые конфигурационные биты N и R
    RCC->PLLI2SCFGR |= (192 << RCC_PLLI2SCFGR_PLLI2SN_Pos) | (2 << RCC_PLLI2SCFGR_PLLI2SR_Pos); // Выставляем N=192 и R=2 для звука

    RCC->CR |= RCC_CR_PLLI2SON;                                                 // Запускаем специализированный клок аудио PLLI2S
    while(!(RCC->CR & RCC_CR_PLLI2SRDY));                                       // Ожидаем полной стабилизации аудио частоты

    RCC->CFGR &= ~RCC_CFGR_I2SSRC;                                              // Направляем шину I2S на работу от блока PLLI2S
}

void blink_pc13led(uint16_t freq) {
    static uint32_t last_blink = 0;                                             // Храним время последнего мигания
    if (ttms - last_blink >= freq) {                                            // Если интервал времени превысил заданный
        GPIOC->ODR ^= GPIO_ODR_OD13;                                            // Переключаем состояние светодиода PC13
        last_blink = ttms;                                                      // Обновляем метку времени
    }
}

void itoa32(uint32_t num, char* str) {
    char temp[11];                                                              // Временный буфер для разворота строки
    int i = 0;                                                                  // Индекс для записи в буфер
    int j = 0;                                                                  // Индекс для финальной строки
    
    if (num == 0) {                                                             // Отдельно обрабатываем нулевое значение
        str[0] = '0';                                                           // Записываем символ нуля
        str[1] = '\0';                                                          // Закрываем строку нуль-терминатором
        return;                                                                 // Выходим из функции
    }
    
    while (num > 0) {                                                           // Цикл разбора числа на цифры
        temp[i++] = '0' + (num % 10);                                           // Берем остаток от деления и превращаем в символ
        num /= 10;                                                              // Уменьшаем число в 10 раз
    }
    
    for (j = 0; j < i; j++) {                                                   // Цикл переворота строки в правильный порядок
        str[j] = temp[i - 1 - j];                                               // Копируем символы задом наперед
    }
    str[j] = '\0';                                                              // Обязательно добавляем нуль-терминатор конца строки
}

void format_time(uint32_t total_seconds, char* out_str) {
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;
    out_str[0] = (minutes / 10) + '0';
    out_str[1] = (minutes % 10) + '0';
    out_str[2] = ':';
    out_str[3] = (seconds / 10) + '0';
    out_str[4] = (seconds % 10) + '0';
    out_str[5] = '\0';
}
