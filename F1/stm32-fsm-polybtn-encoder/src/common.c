#include "common.h"

volatile uint32_t ttms   = 0;
volatile uint32_t ddms   = 0;  // Переменная счетчика миллисекунд
volatile uint32_t pc13ms = 0;
volatile uint32_t pc14ms = 0;

// Прерывание системного тикера (вызывается аппаратно каждую 1 мс)
void encoder_poll(void);

void SysTick_Handler(void) {
  // Глобальный счетчик миллисекунд системы
  ++ttms;

  // программные таймеры задержек
  if (ddms) ddms--;
  if (pc13ms) pc13ms--;
  if (pc14ms) pc14ms--;
}

// задержка
void delay_ms(uint16_t ms) {
  ddms = ms;
  while (ddms) {
    // Ждем, пока прерывание SysTick_Handler опустит ddms до 0
    __NOP();
  }
}

void delay_nop(uint32_t count) {
  for (volatile uint32_t i = 0; i < count; i++) {
    __NOP();
  }
}

void StartHSE(void) {
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
    while ((RCC->CR & RCC_CR_PLLRDY) == 0 && ++StartUpCounter < 1000) {
    }
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;
    StartUpCounter = 0;
    while (((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) && ++StartUpCounter < 1000) {
    }
  }
}

void hardware_init(void) {
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
                  RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;    // Включаем тактирование GPIOB
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;  // Включаем тактирование USART1
  RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;    // Включаем тактирование I2C1

  // Отключить JTAG, оставить SWD
  AFIO->MAPR &= ~AFIO_MAPR_SWJ_CFG;
  AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

  if (SysTick_Config(72000)) {
    while (1);
  }  // конфигурация SysTick = 1ms при тактовой 72MHz

  // LED PC13
  GPIOC->CRH &= ~GPIO_CRH_CNF13;
  GPIOC->CRH |= CRH(13, CNF_PPOUTPUT | MODE_NORMAL);
  // LED PC14
  GPIOC->CRH &= ~GPIO_CRH_CNF14;
  GPIOC->CRH |= CRH(14, CNF_PPOUTPUT | MODE_NORMAL);
  // PC15-BUZZ
  GPIOC->CRH &= ~GPIO_CRH_CNF15;
  GPIOC->CRH |= CRH(15, CNF_PPOUTPUT | MODE_NORMAL);

  // Диспелей
  //  PA15 (RS)
  GPIOA->CRH &= ~GPIO_CRH_CNF15;
  GPIOA->CRH |= CRH(15, CNF_PPOUTPUT | MODE_FAST);
  // PB3 (EN)
  GPIOB->CRL &= ~GPIO_CRL_CNF3;  // Сбрасываем биты CNF
  GPIOB->CRL |= CRL(3, CNF_PPOUTPUT | MODE_FAST);
  // PB4 (D4)
  GPIOB->CRL &= ~GPIO_CRL_CNF4;
  GPIOB->CRL |= CRL(4, CNF_PPOUTPUT | MODE_FAST);
  // PB5 (D5)
  GPIOB->CRL &= ~GPIO_CRL_CNF5;
  GPIOB->CRL |= CRL(5, CNF_PPOUTPUT | MODE_FAST);
  // PB8 (D6)
  GPIOB->CRH &= ~GPIO_CRH_CNF8;
  GPIOB->CRH |= CRH(8, CNF_PPOUTPUT | MODE_FAST);
  // PB9 (D7)
  GPIOB->CRH &= ~GPIO_CRH_CNF9;
  GPIOB->CRH |= CRH(9, CNF_PPOUTPUT | MODE_FAST);

  // Настройка пинов PB6 (SCL) и PB7 (SDA)
  // PB6 (SCL) - AF Open-Drain, 50MHz
  GPIOB->CRL &= ~GPIO_CRL_CNF6;
  GPIOB->CRL |= CRL(6, CNF_AFOD | MODE_FAST);
  // PB7 (SDA) - AF Open-Drain, 50MHz
  GPIOB->CRL &= ~GPIO_CRL_CNF7;
  GPIOB->CRL |= CRL(7, CNF_AFOD | MODE_FAST);

  // Настройка пинов PB6 (SCL) и PB7 (SDA)
  // PB6 (SCL) - AF Open-Drain, 50MHz
  GPIOB->CRL &= ~GPIO_CRL_CNF6;
  GPIOB->CRL |= CRL(6, CNF_AFOD | MODE_FAST);
  // PB7 (SDA) - AF Open-Drain, 50MHz
  GPIOB->CRL &= ~GPIO_CRL_CNF7;
  GPIOB->CRL |= CRL(7, CNF_AFOD | MODE_FAST);

  // Настроим кнопки PB14, PA8, PA11, PA12, PB1
  // PB14 — кнопка 1
  GPIOB->CRH &= ~(GPIO_CRH_MODE14 | GPIO_CRH_CNF14);
  GPIOB->CRH |= CRH(14, CNF_PUDINPUT | MODE_INPUT);
  GPIOB->ODR |= GPIO_ODR_ODR14;  // подтяжка вверх

  // PA8 — кнопка 2
  GPIOA->CRH &= ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8);
  GPIOA->CRH |= CRH(8, CNF_PUDINPUT | MODE_INPUT);
  GPIOA->ODR |= GPIO_ODR_ODR8;  // подтяжка вверх

  // PA11 — кнопка 3
  GPIOA->CRH &= ~(GPIO_CRH_MODE11 | GPIO_CRH_CNF11);
  GPIOA->CRH |= CRH(11, CNF_PUDINPUT | MODE_INPUT);
  GPIOA->ODR |= GPIO_ODR_ODR11;  // подтяжка вверх

  // PA12 — кнопка 4
  GPIOA->CRH &= ~(GPIO_CRH_MODE12 | GPIO_CRH_CNF12);
  GPIOA->CRH |= CRH(12, CNF_PUDINPUT | MODE_INPUT);
  GPIOA->ODR |= GPIO_ODR_ODR12;  // подтяжка вверх

  // PB1 — кнопка энкодера
  GPIOB->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1);
  GPIOB->CRL |= CRL(1, CNF_PUDINPUT | MODE_INPUT);
  GPIOB->ODR |= GPIO_ODR_ODR1;  // подтяжка вверх

  // PB15 DE: General purpose output push-pull 50MHz
  GPIOB->CRH &= ~GPIO_CRH_CNF15;
  GPIOB->CRH |= CRH(15, CNF_PPOUTPUT | MODE_NORMAL);

  // Настройка PA0 (Включение питания панели) - Выход Push-Pull, 10MHz
  GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
  GPIOA->CRL |= CRL(0, CNF_PPOUTPUT | MODE_NORMAL);
  PANEL_POWER_OFF;  // По умолчанию питание выключено
}

// Заменяет символ в строке по позиции
int replace_char_at(char* str, size_t position, char character, uint8_t edtstrlen) {
  if (!str) return -1;
  if (position >= edtstrlen) return 1;
  str[position] = character;
  return 0;
}

// Выравнивает строку пробелами до total_len (если нужно)
void pad_string_with_spaces(char* str, size_t current_len, size_t total_len) {
  if (!str) return;  // добавить проверку на NULL
  if (current_len >= total_len) return;
  // Предполагаем, что буфер достаточно большой (total_len + 1)
  for (size_t i = current_len; i < total_len; i++) {
    str[i] = 0x20;
  }
  str[total_len] = '\0';
}

// Удаляет пробелы с конца строки и обрезает по max_len
void trim_and_clean_string(char* str, size_t max_len) {
  if (!str) return;
  size_t len = strlen(str);
  if (len > max_len) {
    str[max_len] = '\0';
    len          = max_len;
  }
  while (len > 0 && str[len - 1] == 0x20) {
    str[len - 1] = '\0';
    len--;
  }
}

// Простой генератор случайных чисел
static uint32_t random_seed = 0;
void            simple_srand(uint32_t seed) { random_seed = seed; }
uint16_t        simple_rand(void) {
  random_seed = (random_seed * 1103515245 + 12345) & 0x7FFFFFFF;
  return (uint16_t)random_seed;
}

// Безопасная функция копирования строки
void safe_strncpy(char* dest, const char* src, size_t n) {
  if (dest == NULL || src == NULL || n == 0) return;
  char*       d = dest;
  const char* s = src;
  // Копируем символы пока есть место и не конец строки
  while (n > 1 && *s != '\0') {
    *d++ = *s++;
    n--;
  }
  *d = '\0';  // Всегда завершаем строку нулём
}