#include "enc.h"
#include "btn_modes.h"

void encoder_init(void) {
  // Тактирование порта GPIOB
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

  // Настройка PB12 и PB13 как входов с подтяжкой вверх (CNF=10, MODE=00 -> 0x8)
  GPIOB->CRH &= ~((0xFU << ((12 - 8) * 4)) | (0xFU << ((13 - 8) * 4)));
  GPIOB->CRH |=  ((0x8U << ((12 - 8) * 4)) | (0x8U << ((13 - 8) * 4)));

  // Включаем внутреннюю подтяжку к VCC
  GPIOB->ODR |= (1U << 12) | (1U << 13);
}

// Ультрастабильный рантайм опрос энкодера в главном цикле while(1)
void encoder_process(void) {
  // Флаг для первой инициализации состояния
  static uint8_t first_run = 1;
  static uint8_t enc_a_last = 1;

  // Читаем текущие физические уровни фаз
  uint8_t enc_a = ENC_A_READ();
  uint8_t enc_b = ENC_B_READ();

  // Защита от холодного старта: при первом проходе просто запоминаем реальный уровень ноги
  if (first_run) {
    enc_a_last = enc_a;
    first_run = 0;
    return;
  }

  // Ловим момент, когда фаза А только что упала в 0 (строгий спад сигнала из 1 в 0)
  if (enc_a_last == 1 && enc_a == 0) {
    int8_t direction = 0;

    // Если на фазе Б единица — это шаг вправо (+1), если ноль — влево (-1)
    if (enc_b == 1) {
      direction = 1;
    } else {
      direction = -1;
    }

    // Отправляем одиночный чистый импульс вращения в движок
    enc_mode_dispatch(direction);
  }

  // Сохраняем состояние для следующей итерации while(1)
  enc_a_last = enc_a;
}
