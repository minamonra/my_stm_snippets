#include "ir.h"
#include "FreeRTOS.h"
#include "task.h"

/*
 * Диагностический NEC-приёмник для STM32F103.
 *
 * PA0  : выход демодулирующего ИК-приёмника (активный LOW).
 * TIM2 : свободный счётчик времени с шагом 1 мкс.
 * EXTI0: обработка обоих фронтов PA0.
 *
 * ВАЖНО: AFIO_MAPR.SWJ_CFG здесь намеренно не изменяется,
 * чтобы не нарушать настройку JTAG/SWD, сделанную остальным проектом.
 */

volatile uint32_t ik_out = 0U;
volatile uint32_t debug_pulse = 0U; /* длительность LOW, мкс */
volatile uint32_t debug_pause = 0U; /* длительность HIGH, мкс */
volatile uint32_t debug_stor = 0U;  /* последний принятый 32-битный кадр */

extern TaskHandle_t ir_task_handle;

/* Допуски длительностей NEC, микросекунды */
#define NEC_MARK_MIN      350U
#define NEC_MARK_MAX      800U
#define NEC_LEADER_MIN    8000U
#define NEC_LEADER_MAX    10000U
#define NEC_HEADER_MIN    4000U
#define NEC_HEADER_MAX    5000U
#define NEC_REPEAT_MIN    2000U
#define NEC_REPEAT_MAX    2800U
#define NEC_ZERO_SPACE    300U
#define NEC_ZERO_MAX      900U
#define NEC_ONE_MIN       1300U
#define NEC_ONE_MAX       2000U
#define NEC_FRAME_TIMEOUT 12000U

#define NEC_IDLE         0U
#define NEC_HEADER_SPACE 1U
#define NEC_BIT_MARK     2U
#define NEC_BIT_SPACE    3U
#define NEC_REPEAT_MARK  4U

static uint8_t nec_state = NEC_IDLE;
static uint8_t nec_bit_count = 0U;
static uint32_t nec_data = 0U;
static uint16_t nec_last_edge = 0U;
static uint8_t nec_have_edge = 0U;

static uint8_t nec_last_command = 0U;
static uint8_t nec_have_command = 0U;

static uint8_t nec_between(uint32_t value, uint32_t min, uint32_t max) {
    return (value >= min) && (value <= max);
}

static void nec_reset_frame(void) {
    nec_state = NEC_IDLE;
    nec_bit_count = 0U;
    nec_data = 0U;
}

static void nec_notify_task(BaseType_t* higher_priority_task_woken) {
    if (ir_task_handle != NULL) {
        vTaskNotifyGiveFromISR(ir_task_handle, higher_priority_task_woken);
    }
}

void ir_receiver_init(void) {
    /* Тактирование периферии */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;

    /* PA0: вход с подтяжкой вверх */
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
    GPIOA->CRL |= GPIO_CRL_CNF0_1;
    GPIOA->ODR |= GPIO_ODR_ODR0;

    /*
     * TIM2: шаг 1 мкс при тактовой частоте таймера 72 МГц.
     * Если clock_init() задаёт другую частоту TIM2, PSC нужно изменить.
     */
    TIM2->CR1 = 0U;
    TIM2->PSC = 71U;
    TIM2->ARR = 0xFFFFU;
    TIM2->CNT = 0U;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;
    TIM2->CR1 = TIM_CR1_CEN;

    /* EXTI0 маршрутизируется на PA0. SWJ_CFG не трогаем. */
    AFIO->EXTICR[0] &= ~AFIO_EXTICR1_EXTI0;
    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->FTSR |= EXTI_FTSR_TR0;
    EXTI->PR = EXTI_PR_PR0;

    nec_state = NEC_IDLE;
    nec_bit_count = 0U;
    nec_data = 0U;
    nec_last_edge = 0U;
    nec_have_edge = 0U;
    nec_last_command = 0U;
    nec_have_command = 0U;

    ik_out = 0U;
    debug_pulse = 0U;
    debug_pause = 0U;
    debug_stor = 0U;

    NVIC_SetPriority(EXTI0_IRQn, 5U);
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

void EXTI0_IRQHandler(void) {
    BaseType_t higher_priority_task_woken = pdFALSE;
    uint16_t now;
    uint16_t duration;
    uint8_t level;

    if ((EXTI->PR & EXTI_PR_PR0) == 0U) {
        return;
    }

    /* На STM32F1 флаг EXTI очищается записью единицы. */
    EXTI->PR = EXTI_PR_PR0;

    now = (uint16_t)TIM2->CNT;
    level = ((GPIOA->IDR & GPIO_IDR_IDR0) != 0U) ? 1U : 0U;

    if (nec_have_edge == 0U) {
        nec_last_edge = now;
        nec_have_edge = 1U;
        return;
    }

    /* Беззнаковая 16-битная разность учитывает переполнение TIM2. */
    duration = (uint16_t)(now - nec_last_edge);
    nec_last_edge = now;

    /*
     * level == 1: закончился LOW-импульс.
     * level == 0: закончилась HIGH-пауза.
     * Обновляем оба диагностических значения на соответствующих фронтах.
     */
    if (level != 0U) {
        debug_pulse = duration;
    } else {
        debug_pause = duration;
    }

    if (duration > NEC_FRAME_TIMEOUT) {
        nec_reset_frame();
    }

    if (level != 0U) {
        /* Завершился LOW-импульс */
        if (nec_state == NEC_IDLE) {
            if (nec_between(duration, NEC_LEADER_MIN, NEC_LEADER_MAX)) {
                nec_state = NEC_HEADER_SPACE;
            }
        } else if (nec_state == NEC_BIT_MARK) {
            if (nec_between(duration, NEC_MARK_MIN, NEC_MARK_MAX)) {
                nec_state = NEC_BIT_SPACE;
            } else {
                nec_reset_frame();
            }
        } else if (nec_state == NEC_REPEAT_MARK) {
            if (nec_between(duration, NEC_MARK_MIN, NEC_MARK_MAX)) {
                if (nec_have_command != 0U) {
                    ik_out = nec_last_command;
                    debug_stor = nec_last_command;
                    nec_notify_task(&higher_priority_task_woken);
                }
                nec_reset_frame();
            } else {
                nec_reset_frame();
            }
        } else if ((nec_state != NEC_HEADER_SPACE) &&
                   (nec_state != NEC_BIT_SPACE)) {
            nec_reset_frame();
        }
    } else {
        /* Завершилась HIGH-пауза */
        if (nec_state == NEC_HEADER_SPACE) {
            if (nec_between(duration, NEC_HEADER_MIN, NEC_HEADER_MAX)) {
                nec_state = NEC_BIT_MARK;
                nec_bit_count = 0U;
                nec_data = 0U;
            } else if (nec_between(duration, NEC_REPEAT_MIN, NEC_REPEAT_MAX)) {
                nec_state = NEC_REPEAT_MARK;
            } else {
                nec_reset_frame();
            }
        } else if (nec_state == NEC_BIT_SPACE) {
            if (nec_between(duration, NEC_ZERO_SPACE, NEC_ZERO_MAX)) {
                /* Бит 0: соответствующий бит уже равен нулю. */
            } else if (nec_between(duration, NEC_ONE_MIN, NEC_ONE_MAX)) {
                /* NEC передаёт биты младшим битом вперёд. */
                nec_data |= (1UL << nec_bit_count);
            } else {
                nec_reset_frame();
                portYIELD_FROM_ISR(higher_priority_task_woken);
                return;
            }

            nec_bit_count++;

            if (nec_bit_count >= 32U) {
                uint8_t address = (uint8_t)(nec_data);
                uint8_t address_inv = (uint8_t)(nec_data >> 8);
                uint8_t command = (uint8_t)(nec_data >> 16);
                uint8_t command_inv = (uint8_t)(nec_data >> 24);

                debug_stor = nec_data;

                /* Стандартный NEC: адрес и команда имеют инверсные байты. */
                if (((uint8_t)(address ^ address_inv) == 0xFFU) &&
                    ((uint8_t)(command ^ command_inv) == 0xFFU)) {
                    nec_last_command = command;
                    nec_have_command = 1U;
                    ik_out = command;
                    nec_notify_task(&higher_priority_task_woken);
                }

                nec_reset_frame();
            } else {
                nec_state = NEC_BIT_MARK;
            }
        } else {
            /* Неожиданная пауза вне ожидаемого состояния. */
            nec_reset_frame();
        }
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
}
