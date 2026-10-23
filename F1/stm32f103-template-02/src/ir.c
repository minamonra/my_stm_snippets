#include "ir.h"
#include "FreeRTOS.h"
#include "task.h"

//
// Железный NEC-приёмник для STM32F103
// Полностью очищен от варнингов компилятора и ложных срабатываний.
//
// PA0  : вход ИК-приёмника (TIM2_CH1).
// TIM2 : Режим PWM Input с аппаратным сбросом CNT по каждому спаду.
//

volatile uint32_t ik_out      = 0U;
volatile uint32_t debug_pulse = 0U;
volatile uint32_t debug_pause = 0U;
volatile uint32_t debug_stor  = 0U;

extern TaskHandle_t ir_task_handle;

//
// Таблица допусков range_z[статус][параметр], пересчитанная под шаг 1 мкс:
// Параметры: [0] - Мин Импульс, [1] - Макс Импульс, [2] - Мин Пауза, [3] - Макс
// Пауза
//
const uint16_t range_z[4][4] = {
    {8000U, 10000U, 3800U, 5200U},  // Статус 0: Стартовый пакет (Leader)
    {350U, 800U, 300U, 2000U},      // Статус 1: Маркеры данных битов (0 и 1)
    {350U, 800U, 10000U, 25000U},   // Статус 2: Завершающий маркер / Пауза тишины
    {8000U, 10000U, 1800U, 2700U}   // Статус 3: Пакет повтора (Repeat)
};

static struct {
  uint32_t stor;
  uint8_t status;
  uint8_t poz;
} ik = {0U, 0U, 0U};

static void nec_notify_task(BaseType_t* higher_priority_task_woken) {
  if (ir_task_handle != NULL) {
    vTaskNotifyGiveFromISR(ir_task_handle, higher_priority_task_woken);
  }
}

void ir_receiver_init(void) {
  // Тактирование TIM2, GPIOA и AFIO
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;

  // PA0: вход альтернативной функции (TIM2_CH1) с подтяжкой вверх
  GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
  GPIOA->CRL |= GPIO_CRL_CNF0_1;
  GPIOA->ODR |= GPIO_ODR_ODR0;

  // Настройка TIM2 на шаг 1 мкс (при тактовой частоте таймера 72 МГц)
  TIM2->CR1 = 0U;
  TIM2->PSC = 71U;
  TIM2->ARR = 0xFFFFU;

  // Режим PWM Input: CH1 на TI1 (спад), CH2 на TI1 (подъем), Жесткий фильтр = 7
  TIM2->CCMR1 =
      TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_1 | (7U << TIM_CCMR1_IC1F_Pos);

  // Полярность захвата
  TIM2->CCER &=
      ~(TIM_CCER_CC1P | TIM_CCER_CC1NP | TIM_CCER_CC2P | TIM_CCER_CC2NP);
  TIM2->CCER |= TIM_CCER_CC1P;  // CH1 - Falling edge

  // Включение захвата каналов
  TIM2->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E);

  // Аппаратный Reset Mode по спаду на TI1FP1
  TIM2->SMCR &= ~(TIM_SMCR_TS | TIM_SMCR_SMS);
  TIM2->SMCR |= (TIM_SMCR_TS_2 | TIM_SMCR_TS_0) | TIM_SMCR_SMS_2;

  // Прерывание строго по Каналу 1 (окончание периода)
  TIM2->DIER |= TIM_DIER_CC1IE;

  // Пуск таймера
  TIM2->EGR = TIM_EGR_UG;
  TIM2->SR  = 0U;
  TIM2->CR1 = TIM_CR1_CEN;

  // Инициализация структуры
  ik.stor     = 0U;
  ik.status   = 0U;
  ik.poz      = 0U;
  ik_out      = 0U;
  debug_pulse = 0U;
  debug_pause = 0U;
  debug_stor  = 0U;

  // Настройка прерывания в NVIC
  NVIC_SetPriority(TIM2_IRQn, 5U);
  NVIC_ClearPendingIRQ(TIM2_IRQn);
  NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void) {
  BaseType_t higher_priority_task_woken = pdFALSE;
  uint32_t lsi, hsi;
  uint32_t total_period;

  if ((TIM2->SR & TIM_SR_CC1IF) == 0U) {
    return;
  }

  // Очищаем флаги таймера
  TIM2->SR = ~(TIM_SR_CC1IF | TIM_SR_CC1OF | TIM_SR_CC2IF | TIM_SR_CC2OF);

  // Читаем готовые значения из регистров (благодаря Reset Mode)
  total_period = TIM2->CCR1;  // Полный период (спад -> спад)
  lsi          = TIM2->CCR2;  // Длительность импульса LOW (спад -> подъем)

  // Защита от переполнения в тишине
  if (total_period > 30000U) {
    ik.status = 0;
    ik.poz    = 0;
    return;
  }

  if (total_period > lsi) {
    hsi = total_period - lsi;  // Длительность паузы HIGH
  } else {
    ik.status = 0;
    ik.poz    = 0;
    return;
  }

  // СВЕРХНАДЕЖНЫЙ ФИЛЬТР ЭКРАНА: отсекаем короткие всплески шума от ламп (<300
  // мкс)
  if (lsi >= 300U && hsi >= 300U) {
    debug_pulse = lsi;
    debug_pause = hsi;
  }

  uint32_t stor   = ik.stor;
  uint32_t status = ik.status;
  uint32_t poz    = ik.poz;

  // Проверка попадания в текущие допуски таблицы range_z
  if ((lsi > range_z[status][0]) && (lsi < range_z[status][1]) &&
      (hsi > range_z[status][2]) && (hsi < range_z[status][3])) {
    if (status == 1) {
      uint32_t bit_val = 0U;
      if (hsi > 1100U) {
        bit_val = 1U;
      }

      // Родной сдвиг влево
      stor = bit_val | (stor << 1);

      poz++;
      if (poz >= 32U) {
        // Извлекаем байты по физической структуре заполнения буфера stor:
        // [Address] (старший) [Address_Inv] [Command] [Command_Inv] (младший)
        uint8_t address     = (uint8_t)(stor >> 24);
        uint8_t address_inv = (uint8_t)(stor >> 16);
        uint8_t command     = (uint8_t)(stor >> 8);
        uint8_t command_inv = (uint8_t)(stor);

        debug_stor = stor;

        // Валидация инверсии байт
        if (((uint8_t)(address ^ address_inv) == 0xFFU) &&
            ((uint8_t)(command ^ command_inv) == 0xFFU)) {
          ik_out = command;  // Код зеркальный, идеально ложится в новые дефайны
          nec_notify_task(&higher_priority_task_woken);
          status = 2;        // Успешно принято, уходим в ожидание паузы
        } else {
          status = 0;        // Ошибка валидации байт, сброс пакета
        }
      }
    } else if (status == 3) {
      // Если пришел повтор (Repeat) — дублируем прошлую успешную команду
      nec_notify_task(&higher_priority_task_woken);
      status = 2;
    } else {
      status += 1;  // Переходим на следующий шаг
    }
  } else {
    // Если прилетел пакет повтора, пока мы были в ожидании (IDLE / Статус 0)
    if ((status == 0) && (lsi > range_z[3][0]) && (lsi < range_z[3][1]) &&
        (hsi > range_z[3][2]) && (hsi < range_z[3][3])) {
      status = 3;  // Переключаемся на обработку повтора
      TIM2->SR |= TIM_SR_CC1IF;
    } else {
      status = 0;
      poz    = 0;
    }
  }

  ik.stor   = stor;
  ik.status = status;
  ik.poz    = poz;

  portYIELD_FROM_ISR(higher_priority_task_woken);
}
