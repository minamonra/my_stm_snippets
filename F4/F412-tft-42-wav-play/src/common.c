#include "common.h"

#include "lcd_io.h"
#include "audio_i2s.h"
#include "sdcard.h"
#include "diskio.h"

#include "sdcard_dma.h"

// #define DEBUG  // Закомментировать эту строчку, если отладка осциллографом на PA8 больше не нужна

volatile uint32_t ttms = 0;  // Глобальный счётчик миллисекунд, инкрементируется в SysTick_Handler

// Быстрый генератор случайных чисел (линейный конгруэнтный метод)

static uint32_t next_random = 12345;  // Начальное значение (seed)

void random_seed(uint32_t seed) {
  next_random = seed;
}

uint16_t get_random(uint16_t max) {
  next_random = next_random * 1103515245 + 12345;  // LCG-формула
  return (uint16_t)((next_random / 65536) % max);
}

// ============================================================================
// === ОБРАБОТЧИКИ СИСТЕМНЫХ ПРЕРЫВАНИЙ ======================================
// ============================================================================

void SysTick_Handler(void) {
  ttms++;  // Инкремент каждую миллисекунду
}

// ============================================================================
// === ФУНКЦИИ ЗАДЕРЖЕК ======================================================
// ============================================================================

void delay_ms(uint32_t ms) {
  uint32_t start = ttms;
  while ((ttms - start) < ms) {
    __WFI();  // Ждём в режиме низкого энергопотребления
  }
}

void delay_nop(uint32_t ticks) {
  while (ticks--) {
    __NOP();  // Пустой цикл для коротких задержек
  }
}

// ============================================================================
// === УПРАВЛЕНИЕ СВЕТОДИОДОМ ================================================
// ============================================================================

void blink_led(uint16_t freq) {
  static uint32_t last_time = 0;  // Хранит снимок системного времени ttms (статическая локальная)

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
  uint32_t startup_counter = 0;
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

void gpio_init(void) {
  // Включаем тактирование портов GPIO и модуля DMA2 на шине AHB1
  RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_DMA2EN);

  delay_nop(10);  // Короткая задержка для стабилизации тактирования

  // === НАСТРОЙКА LED (PB2) ===
  GPIOB->MODER |= (GPIO_MODER_MODER2_0);     // PB2 = Output
  GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED2);  // High speed

  // ВАЖНО: Пины TM1637 настраиваются в tm1637_init(), а не здесь!
  // Убираем конфликтную настройку Open-Drain для PB8/PB9

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

void uint16_to_hex(uint16_t val, char* out) {
  const char hex_chars[] = "0123456789ABCDEF";
  out[0]                 = hex_chars[(val >> 12) & 0xF];  // Старший полубайт
  out[1]                 = hex_chars[(val >> 8) & 0xF];
  out[2]                 = hex_chars[(val >> 4) & 0xF];
  out[3]                 = hex_chars[val & 0xF];  // Младший полубайт
  out[4]                 = '\0';                  // Завершающий нуль
}

/**
 * ИСПРАВЛЕНО: Защищенный безопасный UTF-8 декодер.
 * Не позволяет указателю p вылететь за терминатор \0 при чтении битых или обрезанных строк.
 */
uint16_t decode_utf8(const char** ptr) {
  const uint8_t* p = (const uint8_t*)*ptr;
  if (p == 0 || *p == 0) return 0;

  uint16_t ch = *p++;

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

void format_time(uint32_t sec, char* buf) {
  uint32_t m = sec / 60, s = sec % 60;
  buf[0] = '0' + m / 10;
  buf[1] = '0' + m % 10;
  buf[2] = ':';
  buf[3] = '0' + s / 10;
  buf[4] = '0' + s % 10;
  buf[5] = '\0';
}

void uint32_to_str(uint32_t num, char* buf) {
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

void hw_init(void) {
  // Включаем аппаратный кэш ядра Cortex-M4 (инструкции, данные, prefetch)
  FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;
  system_clock_config_96MHz();
  sd_dma_init();
  gpio_init();
  lcd_init();
  ST7796_LED_ON;
  sdio_initpins();
  delay_ms(500);
  audio_init();
}