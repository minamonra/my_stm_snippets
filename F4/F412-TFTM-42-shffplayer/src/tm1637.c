// ============================================
// ИСПРАВЛЕННЫЙ TM1637.C
// ============================================

#include "tm1637.h"

// Четкие маски для BSRR
#define CLK_SET_MASK (1U << 9)   // PB9 в HIGH
#define CLK_RST_MASK (1U << 25)  // PB9 в LOW
#define DIO_SET_MASK (1U << 8)   // PB8 в HIGH
#define DIO_RST_MASK (1U << 24)  // PB8 в LOW

#define SEG_MINUS 0x40

static const uint8_t digitMap[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

static uint8_t displayBuffer[4]  = {0x00, 0x00, 0x00, 0x00};
static uint8_t currentBrightness = 0x04;

extern uint32_t SystemCoreClock;

static void delay_10us(uint8_t count) {
  uint16_t start = TIM6->CNT;
  while ((uint16_t)(TIM6->CNT - start) < count) {
    __NOP();
  }
}

static void tm1637_delay(void) {
  delay_10us(2);
}

#define CLK_HIGH() GPIOB->BSRR = CLK_SET_MASK
#define CLK_LOW()  GPIOB->BSRR = CLK_RST_MASK
#define DIO_HIGH() GPIOB->BSRR = DIO_SET_MASK
#define DIO_LOW()  GPIOB->BSRR = DIO_RST_MASK

// Старт-сигнал TM1637
static void tm1637_start(void) {
  CLK_HIGH();
  DIO_HIGH();
  tm1637_delay();
  DIO_LOW();
  tm1637_delay();
  CLK_LOW();
  tm1637_delay();
}

// Стоп-сигнал TM1637
static void tm1637_stop(void) {
  CLK_LOW();
  DIO_LOW();
  tm1637_delay();
  CLK_HIGH();
  tm1637_delay();
  DIO_HIGH();
  tm1637_delay();
}

// Запись байта
static void tm1637_write_byte(uint8_t byte) {
  for (uint8_t i = 0; i < 8; i++) {
    CLK_LOW();
    tm1637_delay();

    if (byte & 0x01)
      DIO_HIGH();
    else
      DIO_LOW();

    byte >>= 1;
    tm1637_delay();

    CLK_HIGH();
    tm1637_delay();
  }

  // Импульс ACK
  CLK_LOW();
  DIO_HIGH();
  tm1637_delay();

  CLK_HIGH();
  tm1637_delay();

  CLK_LOW();
  tm1637_delay();
}

// Отправка данных на дисплей
static void tm1637_send_data(const uint8_t* data, uint8_t length) {
  tm1637_start();
  tm1637_write_byte(0x40);  // Команда данных: автоинкремент
  tm1637_stop();

  tm1637_start();
  tm1637_write_byte(0xC0);  // Адрес начала
  for (uint8_t i = 0; i < length; i++) {
    tm1637_write_byte(data[i]);
  }
  tm1637_stop();

  tm1637_start();
  tm1637_write_byte(0x88 | (currentBrightness & 0x07));  // Яркость
  tm1637_stop();
}

static void tm1637_update(void) {
  tm1637_send_data(displayBuffer, 4);
}

// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ УПРАВЛЕНИЯ
// ============================================================================

static uint8_t tm1637_encode_char(char c) {
  if (c >= '0' && c <= '9') return digitMap[c - '0'];
  if (c >= 'a' && c <= 'z') c -= 32;
  switch (c) {
    case 'A':
      return 0x77;
    case 'B':
      return 0x7C;
    case 'C':
      return 0x39;
    case 'D':
      return 0x5E;
    case 'E':
      return 0x79;
    case 'F':
      return 0x71;
    case 'G':
      return 0x3D;
    case 'H':
      return 0x76;
    case 'I':
      return 0x06;
    case 'J':
      return 0x1E;
    case 'L':
      return 0x38;
    case 'N':
      return 0x37;
    case 'O':
      return 0x3F;
    case 'P':
      return 0x73;
    case 'Q':
      return 0x67;
    case 'R':
      return 0x50;
    case 'S':
      return 0x6D;
    case 'T':
      return 0x78;
    case 'U':
      return 0x3E;
    case 'Y':
      return 0x6E;
    case 'Z':
      return 0x5B;
    case '-':
      return SEG_MINUS;
    case '_':
      return 0x08;
    case ' ':
    default:
      return 0x00;
  }
}

void tm1637_init(void) {
  // Включаем порты
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

  GPIOB->MODER &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
  GPIOB->MODER |= ((1U << (8 * 2)) | (1U << (9 * 2)));

  GPIOB->OTYPER |= (GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);

  GPIOB->OSPEEDR |= ((3U << (8 * 2)) | (3U << (9 * 2)));

  GPIOB->PUPDR &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
  GPIOB->PUPDR |= ((1U << (8 * 2)) | (1U << (9 * 2)));

  CLK_HIGH();
  DIO_HIGH();

  // Настройка таймера для задержек
  RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
  __DSB();

  uint32_t freq = SystemCoreClock;
  if (freq < 1000000UL) freq = 100000000UL;

  TIM6->PSC = (freq / 100000UL) - 1;  // 1 тик = 10 мкс
  TIM6->ARR = 0xFFFF;
  TIM6->EGR |= TIM_EGR_UG;
  TIM6->CR1 |= TIM_CR1_CEN;

  for (volatile uint32_t i = 0; i < 500000; i++);
  tm1637_clear();
}

void tm1637_set_brightness(uint8_t brightness) {
  currentBrightness = (brightness > 7) ? 7 : brightness;
  tm1637_update();
}

void tm1637_clear(void) {
  for (uint8_t i = 0; i < 4; i++) displayBuffer[i] = 0x00;
  tm1637_update();
}

void tm1637_display_dec(uint16_t number, uint8_t showColon) {
  displayBuffer[0] = digitMap[(number / 1000) % 10];
  displayBuffer[1] = digitMap[(number / 100) % 10];
  displayBuffer[2] = digitMap[(number / 10) % 10];
  displayBuffer[3] = digitMap[number % 10];
  for (uint8_t i = 0; i < 4; i++) {
    if (showColon & (1 << i)) displayBuffer[i] |= 0x80;
  }
  tm1637_update();
}

void tm1637_display_int(int16_t number, uint8_t showColon) {
  if (number > 9999) number = 9999;
  if (number < -999) number = -999;
  if (number >= 0) {
    tm1637_display_dec((uint16_t)number, showColon);
  } else {
    uint16_t abs_num = (uint16_t)(-number);
    displayBuffer[0] = SEG_MINUS;
    displayBuffer[1] = digitMap[(abs_num / 100) % 10];
    displayBuffer[2] = digitMap[(abs_num / 10) % 10];
    displayBuffer[3] = digitMap[abs_num % 10];
    for (uint8_t i = 0; i < 4; i++) {
      if (showColon & (1 << i)) displayBuffer[i] |= 0x80;
    }
    tm1637_update();
  }
}

void tm1637_display_str(const char* str) {
  for (uint8_t i = 0; i < 4; i++) {
    if (str[i] == '\0' || str[i] == 0) {
      while (i < 4) displayBuffer[i++] = 0x00;
      break;
    }
    displayBuffer[i] = tm1637_encode_char(str[i]);
  }
  tm1637_update();
}

void tm1637_display_seg(const uint8_t* segments) {
  for (uint8_t i = 0; i < 4; i++) displayBuffer[i] = segments[i];
  tm1637_update();
}

// ============================================================================
// НОВЫЕ ФУНКЦИИ ДЛЯ УПРАВЛЕНИЯ ОТДЕЛЬНЫМИ СЕГМЕНТАМИ
// ============================================================================

void tm1637_set_segment(uint8_t position, uint8_t value) {
  if (position > 3) return;
  displayBuffer[position] = value;
  tm1637_update();  // Используем существующую функцию обновления
}

uint8_t tm1637_get_segment(uint8_t position) {
  if (position > 3) return 0;
  return displayBuffer[position];
}

void tm1637_set_colon(uint8_t position, uint8_t enabled) {
  if (position > 3) return;
  if (enabled) {
    displayBuffer[position] |= 0x80;  // Включить точку
  } else {
    displayBuffer[position] &= ~0x80;  // Выключить точку
  }
  tm1637_update();
}

void tm1637_set_dot(uint8_t position, uint8_t enabled) {
  tm1637_set_colon(position, enabled);
}

// Функция для показа числа с плавающей точкой (1 знак)
void tm1637_display_float(float number, uint8_t decimal_places) {
  if (decimal_places > 3) decimal_places = 3;

  // Ограничиваем диапазон
  if (number > 9999) number = 9999;
  if (number < -999) number = -999;

  // Масштабируем
  int32_t scaled = (int32_t)(number * 10);

  // Отображаем с точкой
  if (scaled >= 0) {
    displayBuffer[0] = digitMap[(scaled / 1000) % 10];
    displayBuffer[1] = digitMap[(scaled / 100) % 10];
    displayBuffer[2] = digitMap[(scaled / 10) % 10];
    displayBuffer[3] = digitMap[scaled % 10];

    // Включаем точку на нужной позиции
    if (decimal_places >= 0 && decimal_places <= 3) {
      displayBuffer[3 - decimal_places] |= 0x80;
    }
  } else {
    int32_t abs_scaled = -scaled;
    displayBuffer[0]   = SEG_MINUS;
    displayBuffer[1]   = digitMap[(abs_scaled / 100) % 10];
    displayBuffer[2]   = digitMap[(abs_scaled / 10) % 10];
    displayBuffer[3]   = digitMap[abs_scaled % 10];

    if (decimal_places >= 0 && decimal_places <= 3) {
      displayBuffer[3 - decimal_places] |= 0x80;
    }
  }

  tm1637_update();
}

void tm1637_demo(void) {
  static uint32_t          last = 0;
  static uint8_t           step = 0;
  extern volatile uint32_t ttms;

  if ((ttms - last) >= 700) {
    last = ttms;
    step = (step + 1) % 6;

    switch (step) {
      case 0:
        tm1637_display_str("1234");
        break;
      case 1:
        tm1637_display_str("5678");
        break;
      case 2:
        tm1637_display_str("9ABC");
        break;
      case 3:
        tm1637_display_str("DEF_");
        break;
      case 4:
        tm1637_clear();
        break;
      case 5:
        tm1637_display_dec(8888, 0);
        break;
    }
  }
}
