#include "button.h"
#include "common.h"
#include "shuffle_player.h"


// ---- Настраиваемые тайминги ----
#define BTN_SAMPLE_PERIOD_MS   5
#define BTN_DEBOUNCE_MS        30
#define BTN_HOLD_MS            600
#define BTN_REPEAT_MS          150

typedef void (*ButtonCallback)(void);

static const ButtonCallback clickHandlers[BTN_COUNT] = {
    button1_click, 
    button2_click, 
    button3_click, 
    button4_click, 
    button5_click
};

static volatile uint8_t clickFlags = 0;

typedef struct {
    uint32_t pressStartTime;
    uint32_t lastRepeatTime;
    uint8_t  debounced;
} ButtonState_t;

static ButtonState_t btnState[BTN_COUNT] = {0};

// ============================================================================
// === ИНИЦИАЛИЗАЦИЯ GPIO =====================================================
// ============================================================================

void buttons_init(void) {
    // Включаем тактирование GPIOB
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        GPIO_TypeDef* port = buttonPins[i].port;
        uint8_t pin = buttonPins[i].pin;

        // Режим: Input (00)
        port->MODER &= ~(0x3UL << (pin * 2));

        // Подтяжка: Pull-up (01)
        port->PUPDR &= ~(0x3UL << (pin * 2));
        port->PUPDR |=  (0x1UL << (pin * 2));
    }
}

// ============================================================================
// === ЧТЕНИЕ СОСТОЯНИЯ КНОПОК ================================================
// ============================================================================

static uint16_t GET_BUTTONS_STATE(void) {
    uint16_t state = 0;
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        if ((buttonPins[i].port->IDR & (1U << buttonPins[i].pin)) == 0) {
            state |= (1U << i);
        }
    }
    return state;
}

// ============================================================================
// === ОПРОС КНОПОК ===========================================================
// ============================================================================

static void Buttons_Poll(void) {
    static uint32_t lastSampleTime = 0;
    if ((ttms - lastSampleTime) < BTN_SAMPLE_PERIOD_MS) {
        return;
    }
    lastSampleTime = ttms;

    uint16_t state = GET_BUTTONS_STATE();

    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        ButtonState_t* s = &btnState[i];

        if (state & (1U << i)) {
            if (s->pressStartTime == 0) {
                s->pressStartTime = ttms;
                s->debounced = 0;
            }

            uint32_t heldFor = ttms - s->pressStartTime;

            if (!s->debounced && heldFor >= BTN_DEBOUNCE_MS) {
                clickFlags |= (1U << i);
                s->debounced = 1;
                s->lastRepeatTime = ttms;
            } else if (s->debounced && buttonPins[i].repeatEnabled &&
                       heldFor >= BTN_HOLD_MS &&
                       (ttms - s->lastRepeatTime) >= BTN_REPEAT_MS) {
                clickFlags |= (1U << i);
                s->lastRepeatTime = ttms;
            }
        } else {
            s->pressStartTime = 0;
            s->debounced = 0;
        }
    }
}

// ============================================================================
// === ГЛАВНЫЙ ЦИКЛ ОБРАБОТКИ КНОПОК =========================================
// ============================================================================

void button_process(uint32_t interval_ms) {
    static uint32_t last_btn_time = 0;
    if ((ttms - last_btn_time) < interval_ms) {
        return;
    }
    last_btn_time = ttms;

    Buttons_Poll();

    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        if (clickFlags & (1U << i)) {
            clickFlags &= ~(1U << i);
            clickHandlers[i]();
        }
    }
}

// ============================================================================
// === БЛОКИРУЮЩЕЕ ОЖИДАНИЕ ЛЮБОЙ КНОПКИ =====================================
// ============================================================================

void button_waitanypress(void) {
    while (GET_BUTTONS_STATE() != 0) {
        Buttons_Poll();
    }
    clickFlags = 0;

    while (1) {
        Buttons_Poll();
        if (clickFlags) {
            clickFlags = 0;
            break;
        }
    }

    while (GET_BUTTONS_STATE() != 0) {
        Buttons_Poll();
    }
    clickFlags = 0;
}

// ============================================================================
// === ОБРАБОТЧИКИ КНОПОК =====================================================
// ============================================================================

// Кнопка 1: ПРЕДЫДУЩИЙ ТРЕК
void button1_click(void) {
    shuffle_player_prev();
}

// Кнопка 2: ВОСПРОИЗВЕДЕНИЕ / ПАУЗА
void button2_click(void) {
    shuffle_player_play_pause();
}

// Кнопка 3: СЛЕДУЮЩИЙ ТРЕК
void button3_click(void) {
    shuffle_player_next();
}

// Кнопка 4 (пока не используется)
void button4_click(void) {
    // Можно добавить функцию позже
}

// Кнопка 5 (пока не используется)
void button5_click(void) {
    shuffle_player_switch_directory();
}