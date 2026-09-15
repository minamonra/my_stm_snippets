#include "rs485.h"
#include "common.h"
#include <string.h>

// Карта замен для перевода русских букв А..Я в коды панели
const char utf8ToSingleByteMap[] = {
  128, // А
  129, // Б
  130, // В
  131, // Г
  132, // Д
  133, // Е
  240, // Ё
  135, // Ж
  136, // З
  137, // И
  138, // Й
  139, // К
  140, // Л
  141, // М
  142, // Н
  143, // О
  144, // П
  145, // Р
  146, // С
  147, // Т
  148, // У
  149, // Ф
  150, // Х
  151, // Ц
  152, // Ч
  153, // Ш
  154, // Щ
  155, // Ъ
  156, // Ы
  157, // Ь
  158, // Э
  159, // Ю
  160  // Я
};

void rs485_init_9n1(void) {
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN;

  GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9);
  GPIOA->CRH |= (0x3U << 4);  // MODE9 = 11 (50 MHz)
  GPIOA->CRH |= (0x2U << 6);  // CNF9  = 10 (AF push-pull)

  GPIOA->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
  GPIOA->CRH |= (0x1U << 10); // CNF10 = 01 (Floating input)

  GPIOB->CRH &= ~(GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
  GPIOB->CRH |= (0x3U << 28); // MODE15 = 11 (50 MHz)

  RS485DE0;

  USART1->CR1 = 0;
  USART1->CR1 |= USART_CR1_M;
  USART1->BRR = 15000;

  USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
  USART1->CR1 |= USART_CR1_UE;
}

void usart1_send9_buffer(const uint16_t *words, size_t count) {
  RS485DE1;
  for (size_t i = 0; i < count; ++i) {
    while (!(USART1->SR & USART_SR_TXE)) {}
    USART1->DR = (uint16_t)(words[i] & 0x01FF);
  }
  while (!(USART1->SR & USART_SR_TC)) {}
  RS485DE0;
}

void convert2char_arr(int number, char* result) {
    result[0] = '0' + (number / 10);
    result[1] = '0' + (number % 10);
}

// Конвертер UTF-8 строки (из редактора) в строку с кодами твоей панели
void utf8_to_panel_string(const char *utf8_in, char *panel_out) {
  size_t i = 0;
  int out_idx = 0;

  while (utf8_in[i] && out_idx < 10) {
    uint8_t b1 = (uint8_t)utf8_in[i];

    if (b1 < 0x80) {
      panel_out[out_idx++] = (char)b1;
      i++;
    }
    else if (b1 == 0xD0 || b1 == 0xD1) {
      uint8_t b2 = (uint8_t)utf8_in[i + 1];
      uint16_t utf_code = ((uint16_t)b1 << 8) | b2;
      i += 2;

      if (utf_code >= 0xD090 && utf_code <= 0xD0AF) {
        int map_idx = utf_code - 0xD090;
        panel_out[out_idx++] = utf8ToSingleByteMap[map_idx];
      }
      else if (utf_code == 0xD081) {
        panel_out[out_idx++] = 133; // Код буквы Ё, подменяем на Е для карты замен
      }
      else {
        panel_out[out_idx++] = '?';
      }
    }
    else {
      i++;
    }
  }
  panel_out[out_idx] = '\0';
}

// Функция автоматической сборки кадра со встроенной очисткой экрана пробелами
void send2blink_panel(const char *utf8_text) {
  uint16_t clear_buffer[15] = {466, 1, 10}; // Буфер для очищающего кадра
  uint16_t buffer[15] = {466, 1, 10};       // Буфер для основного слова
  char cp1251_text[11] = {0};                 // Локальный буфер под перекодированный текст
  char blinktextr[11];

  // --- 1. ФОРМИРУЕМ ОЧИЩАЮЩИЙ КАДР (ВСЕ 10 СИМВОЛОВ - ПРОБЕЛЫ) ---
  for (int i = 3; i < 13; i++) {
    clear_buffer[i] = 32; // Забиваем пробелами
  }
  // Считаем контрольную сумму для очищающего кадра
  calc_panel_crc(clear_buffer, 15, &clear_buffer[13], &clear_buffer[14]);

  // --- 2. ФОРМИРУЕМ ОСНОВНОЙ КАДР С ТЕКСТОМ ---
  utf8_to_panel_string(utf8_text, cp1251_text);
  int inputLen = strlen(cp1251_text);

  // Добиваем пробелами остаток строки
  for (int i = inputLen; i < 10; i++) {
    cp1251_text[i] = 32;
  }
  // Переворачиваем строку
  for (int i = 0; i < 10; i++) {
    blinktextr[i] = cp1251_text[10 - 1 - i];
  }
  // Копируем перевертыш в основной буфер
  for (int i = 3; i < 13; i++) {
    buffer[i] = (uint16_t)blinktextr[i - 3];
  }
  // Считаем контрольную сумму основного кадра
  calc_panel_crc(buffer, 15, &buffer[13], &buffer[14]);

  // --- 3. ВЫДАЧА В ЖЕЛЕЗО С ОЧИСТКОЙ ---
  PANEL_POWER_ON;                   // Включаем блок питания панели
  delay_ms(400);                    // Пауза на стабилизацию питания панели

  // Отправляем первый (очищающий) кадр, чтобы панель погасила старую подсветку
  usart1_send9_buffer(clear_buffer, 15);
  // Микропауза 150 мс, чтобы контроллер табло успел переварить очистку
  delay_ms(110);
  // Отправляем основной кадр с новым словом
  usart1_send9_buffer(buffer, 15);

  delay_ms(3000);                   // Ждем, пока блинкеры механически доверContinuous шуршать
  PANEL_POWER_OFF;                  // Полностью тушим питание панели
}



void reverse_str(const char* input, char* output, int length) {
  for (int i = 0; i < length; i++) {
    output[i] = input[length - 1 - i];
  }
}

void calc_panel_crc(const uint16_t buffer[], size_t length, uint16_t *byte13, uint16_t *byte14) {
    if (!buffer || length < 15 || !byte13 || !byte14) {
        if (byte13) *byte13 = 0;
        if (byte14) *byte14 = 0;
        return;
    }

    uint32_t sum = 0;
    for (size_t i = 1; i <= 12; ++i) {
        sum += (uint32_t)(buffer[i] & 0x1FFu);
    }

    uint32_t total = sum + 200u;

    *byte13 = (uint16_t)((total >> 8) & 0x1FFu);
    *byte14 = (uint16_t)(total & 0xFFu);
}
