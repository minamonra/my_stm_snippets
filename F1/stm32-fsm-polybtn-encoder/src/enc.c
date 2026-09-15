#include "enc.h"
#include "btn_modes.h"

void encoder_init(void) {
  // Тактирование порта GPIOB уже включено в hardware_init, но продублируем по стандарту
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

  // Настройка PB12 и PB13 как входов с подтяжкой вверх (CNF=10, MODE=00 -> 0x8)
  GPIOB->CRH &= ~((0xFU << ((12 - 8) * 4)) | (0xFU << ((13 - 8) * 4)));
  GPIOB->CRH |=  ((0x8U << ((12 - 8) * 4)) | (0x8U << ((13 - 8) * 4)));

  // Включаем внутреннюю подтяжку к VCC
  GPIOB->ODR |= (1U << 12) | (1U << 13);
}

// Ультрастабильный рантайм опрос на базе изменения фронта фазы А
void encoder_process(void) {
  static uint8_t enc_a_last = 1;

  uint8_t enc_a = ENC_A_READ();
  uint8_t enc_b = ENC_B_READ();

  // Ловим момент перехода фазы А из 1 в 0 (спад сигнала)
  if (enc_a_last == 1 && enc_a == 0) {
    int8_t direction = 0;

    // Если на фазе Б единица — крутим вправо, если ноль — влево
    if (enc_b == 1) {
      direction = 1;   // Шаг вправо
    } else {
      direction = -1;  // Шаг влево
    }

    // Отправляем одиночный чистый импульс в распределитель
    enc_mode_dispatch(direction);
  }

  enc_a_last = enc_a;
}
