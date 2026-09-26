#include "common.h"
#include "FreeRTOS.h"
#include "projdefs.h" // Добавляем этот заголовок для макросов состояний планировщика
#include "task.h"

// #define DEBUG  // Закомментировать эту строчку, если отладка осциллографом на PA8 больше не нужна

volatile uint32_t ttms = 0;  // Глобальный счётчик миллисекунд, инкрементируется в SysTick_Handler
// Быстрый генератор случайных чисел (линейный конгруэнтный метод)
static u32 next_random = 12345;  // Начальное значение (seed)
void random_seed(u32 seed) {
  next_random = seed;
}

uint16_t get_random(u16 max) {
  next_random = next_random * 1103515245 + 12345;  // LCG-формула
  return (u16)((next_random / 65536) % max);
}



// ============================================================================
// === ФУНКЦИИ ЗАДЕРЖЕК ======================================================
// ============================================================================

// Удаляем старый: volatile u32 ttms;
// Удаляем старый: void SysTick_Handler(void) { ... }

// Функция задержки, адаптированная под RTOS
void delay_ms(u32 ms) {
    // Проверяем, запущен ли планировщик задач ОС
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        // Если ОС работает, отдаем управление другим задачам на время задержки
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        // Если ОС еще не запущена (например, во время hw_init), используем глухой цикл
        // На частоте 96 МГц один цикл nop длится примерно 4-5 тактов в зависимости от оптимизации
        // 96 000 000 / 4 = 24 000 тактов на 1 мс без оптимизации, для -Os возьмем около 12000
        delay_nop(ms * 12000);
    }
}

// Новая функция для получения текущего времени в миллисекундах
u32 get_tick_ms(void) {
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        return xTaskGetTickCount();
    }
    return 0;
}

void delay_nop(u32 ticks) {
  while (ticks--) {
    __NOP();  // Пустой цикл для коротких задержек
  }
}




// ============================================================================
// === УПРАВЛЕНИЕ СВЕТОДИОДОМ ================================================
// ============================================================================

void blink_led(u16 freq) {
  static u32 last_time = 0;  // Хранит снимок системного времени ttms (статическая локальная)

  // Проверка на переполнение счетчика или истечение интервала freq
  if (last_time > ttms || (ttms - last_time) >= freq) {
    LED_SYSTEM_TOGGLE;  // Переключаем светодиод через наш макрос
    last_time = ttms;   // Делаем новый снимок глобального времени ttms
  }
}




// ============================================================================
// === КОНФИГУРАЦИЯ ТАКТИРОВАНИЯ (96 МГц) ====================================
// ============================================================================

void system_clock_config_96MHz(void) {
  // 1. Включаем HSI для безопасной коммутации
  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY));

  // Переводим систему временно на безопасный HSI
  RCC->CFGR &= ~RCC_CFGR_SW;
  RCC->CFGR |= RCC_CFGR_SW_HSI;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

  // 2. Включаем HSE (8 МГц)
  RCC->CR |= RCC_CR_HSEON;
  u32 startup_counter = 0;
  while (!(RCC->CR & RCC_CR_HSERDY)) {
    startup_counter++;
    if (startup_counter > 0x10000) {
      // Аварийный переход на HSI: настройка PLL от HSI (16 МГц) -> 96 МГц
      RCC->PLLCFGR = (16U << RCC_PLLCFGR_PLLM_Pos) |   // M = 16 (16 МГц / 16 = 1 МГц на вход VCO)
                     (192U << RCC_PLLCFGR_PLLN_Pos) |  // N = 192 (1 МГц * 192 = 192 МГц VCO)
                     (0U << RCC_PLLCFGR_PLLP_Pos) |    // P = 2 (192 МГц / 2 = 96 МГц SYSCLK)
                     (4U << RCC_PLLCFGR_PLLQ_Pos) |    // Q = 4 (192 МГц / 4 = 48 МГц для SDIO)
                     0U;                               // PLLSRC = HSI
      goto pll_start;
    }
  }

  // Настройка основного PLL от HSE (8 МГц) – ровно как в CubeMX
  RCC->PLLCFGR = (8U << RCC_PLLCFGR_PLLM_Pos) |    // M = 8 (8 МГц / 8 = 1 МГц на вход VCO)
                 (192U << RCC_PLLCFGR_PLLN_Pos) |  // N = 192 (1 МГц * 192 = 192 МГц VCO)
                 (0U << RCC_PLLCFGR_PLLP_Pos) |    // P = 2 (192 МГц / 2 = 96 МГц SYSCLK)
                 (4U << RCC_PLLCFGR_PLLQ_Pos) |    // Q = 4 (192 МГц / 4 = 48 МГц для SDIO)
                 RCC_PLLCFGR_PLLSRC_HSE;           // Источник — внешний кварц HSE

pll_start:
  // 3. Настройка задержек Flash памяти под 96 МГц (3 цикла ожидания)
  FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN |
               FLASH_ACR_ICEN | FLASH_ACR_DCEN;

  // 4. Включаем PLL
  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY));  // Ждём стабилизации частоты PLL

  // 5. Настройка делителей системных шин
  RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1;   // AHB = SYSCLK / 1 = 96 МГц
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;  // APB1 = HCLK / 2 = 48 МГц (максимум шины 50 МГц)
  RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;  // APB2 = HCLK / 1 = 96 МГц (максимум шины 100 МГц)

  // 6. Переключаем SYSCLK на PLL
  RCC->CFGR &= ~RCC_CFGR_SW;
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);  // Ждём подтверждения перехода

#ifdef DEBUG
  // GPIOA->MODER &= ~GPIO_MODER_MODER8;
  // GPIOA->MODER |= GPIO_MODER_MODER8_1;  // Alternate function
  //  --- КОНТРОЛЬНЫЙ ВЫВОД СИГНАЛА НА НОЖКУ MCO1 (PA8) ---
  //  MCO1: PLL / 4 = 24 МГц (для контроля осциллографом)
  RCC->CFGR &= ~(RCC_CFGR_MCO1 | RCC_CFGR_MCO1PRE);
  RCC->CFGR |= (3U << 21) | (6U << 24);  // MCO1SRC = PLL (0b11), MCO1PRE = /4 (0b110)
#endif

  // 7. Настройка SysTick на 1 мс (частота ядра 96 МГц)
  SysTick->LOAD = (96000000 / 1000) - 1;  // 96000 тиков на миллисекунду
  SysTick->VAL  = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  // Источник — AHB (96 МГц)
                  SysTick_CTRL_TICKINT_Msk |    // Разрешаем прерывание
                  SysTick_CTRL_ENABLE_Msk;      // Запускаем счётчик
}




// ============================================================================
// === ИНИЦИАЛИЗАЦИЯ GPIO =====================================================
// ============================================================================

void hw_init(void) {
  // Включаем аппаратный кэш ядра Cortex-M4 (инструкции, данные, prefetch)
  FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;

  // Включаем тактирование портов GPIO и модуля DMA2 на шине AHB1
  RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_DMA2EN);

  delay_nop(10);  // Короткая задержка для стабилизации тактирования

  // === НАСТРОЙКА LED (PB2) === SYS LED
  GPIOB->MODER |= (GPIO_MODER_MODER2_0);     // PB2 = Output
  GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED2);  // High speed


  // РЕЛЕ:   PB14 (в новой плате - PB1)
  GPIOB->MODER |= (GPIO_MODER_MODER14_0);
  GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED14_0);



  // Для отладки MCO1 на PA8
#ifdef DEBUG
  // Привязываем PA8 к альтернативной функции AF00 (MCO1)
  GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL8);
  GPIOA->AFR[1] |= (0U << GPIO_AFRH_AFSEL8_Pos);  // AF0 = MCO1
#endif
}




// ============================================================================
// === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===============================================
// ============================================================================

void uint16_to_hex(u16 val, char* out) {
  const char hex_chars[] = "0123456789ABCDEF";
  out[0] = hex_chars[(val >> 12) & 0xF];  // Старший полубайт
  out[1] = hex_chars[(val >> 8) & 0xF];
  out[2] = hex_chars[(val >> 4) & 0xF];
  out[3] = hex_chars[val & 0xF];  // Младший полубайт
  out[4] = '\0';                  // Завершающий нуль
}





// безопасный UTF-8 декодер.
// Не позволяет указателю p вылететь за терминатор \0 при чтении битых или обрезанных строк.
uint16_t decode_utf8(const char** ptr) {
  const u8* p = (const u8*)*ptr;
  if (p == 0 || *p == 0) return 0;

  u16 ch = *p++;

  // Если это многобайтовый символ Юникода (например, русская буква)
  if (ch >= 0x80) {
    if ((ch & 0xE0) == 0xC0) {  // 2 байта (Кириллица)
      if (*p != 0) {            // Защита от выхода за границы строки
        ch = ((ch & 0x1F) << 6) | (*p++ & 0x3F);
      }
    } else if ((ch & 0xF0) == 0xE0) {  // 3 байта
      if (*p != 0 && *(p + 1) != 0) {
        ch = ((ch & 0x0F) << 12);
        ch |= ((*p++ & 0x3F) << 6);
        ch |= (*p++ & 0x3F);
      }
    } else if ((ch & 0xF8) == 0xF0) {  // 4 байта
      ch = 0xFFFD;                     // Символ замены
      // Безопасный сдвиг указателя с проверкой на конец строки
      for (int k = 0; k < 3; k++) {
        if (*p != 0) p++;
      }
    }
  }

  *ptr = (const char*)p;  // Сдвигаем указатель строки вперед
  return ch;
}




void format_time(u32 sec, char* buf) {
  u32 m = sec / 60, s = sec % 60;
  buf[0] = '0' + m / 10;
  buf[1] = '0' + m % 10;
  buf[2] = ':';
  buf[3] = '0' + s / 10;
  buf[4] = '0' + s % 10;
  buf[5] = '\0';
}



void uint32_to_str(u32 num, char* buf) {
  if (num == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  char temp[12];
  int  i = 0;
  while (num > 0) {
    temp[i++] = '0' + (num % 10);
    num /= 10;
  }
  int j = 0;
  while (i > 0) {
    buf[j++] = temp[--i];
  }
  buf[j] = '\0';
}




int strcasecmp(const char* s1, const char* s2) {
  while (*s1 && *s2) {
    char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
    char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
    if (c1 != c2) return c1 - c2;
    s1++;
    s2++;
  }
  return (*s1) - (*s2);
}
