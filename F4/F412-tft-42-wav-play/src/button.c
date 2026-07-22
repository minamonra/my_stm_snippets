#include "button.h"
#include "common.h"
#include "wav_player_ui.h"
#include "stm32f4xx.h"
#include "wav_player.h"

// ---- Настраиваемые тайминги ----
#define BTN_SAMPLE_PERIOD_MS   5
#define BTN_DEBOUNCE_MS        30
#define BTN_HOLD_MS            600
#define BTN_REPEAT_MS          100  // Для быстрой перемотки

// Определение пинов кнопок
const Button_Pin_t buttonPins[BTN_COUNT] = {
    { GPIOB, 7, 1 },  // Кнопка 1 - ПРЕДЫДУЩИЙ ТРЕК (с удержанием)
    { GPIOB, 6, 1 },  // Кнопка 2 - ВОСПР/ПАУЗА
    { GPIOB, 5, 1 },  // Кнопка 3 - СЛЕДУЮЩИЙ ТРЕК (с удержанием)  <-- ИСПРАВЛЕНО
    { GPIOB, 4, 0 },  // Кнопка 4
    { GPIOB, 3, 0 },  // Кнопка 5
};

typedef void (*ButtonCallback)(void);

static const ButtonCallback clickHandlers[BTN_COUNT] = {
    button1_click, 
    button2_click, 
    button3_click, 
    button4_click, 
    button5_click
};

// Обработчики длительных нажатий
static const ButtonCallback holdHandlers[BTN_COUNT] = {
    button1_hold,   // Кнопка 1 - перемотка назад
    NULL,           // Кнопка 2 - нет длинного нажатия
    button3_hold,   // Кнопка 3 - перемотка вперед
    NULL,           // Кнопка 4 - нет длинного нажатия
    NULL            // Кнопка 5 - нет длинного нажатия
};

static volatile uint8_t clickFlags = 0;

typedef struct {
    uint32_t pressStartTime;
    uint32_t lastRepeatTime;
    uint8_t  debounced;
    uint8_t  holdTriggered;
    uint8_t  clickFlag;  // Флаг, разрешающий клик
} ButtonState_t;

static ButtonState_t btnState[BTN_COUNT] = {0};

// === ИНИЦИАЛИЗАЦИЯ GPIO =====================================================
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

// === ЧТЕНИЕ СОСТОЯНИЯ КНОПОК ================================================
static uint16_t GET_BUTTONS_STATE(void) {
    uint16_t state = 0;
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        if ((buttonPins[i].port->IDR & (1U << buttonPins[i].pin)) == 0) {
            state |= (1U << i);
        }
    }
    return state;
}

// === ОПРОС КНОПОК ===========================================================
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
                s->holdTriggered = 0;
                s->clickFlag = 1;  // Разрешаем клик по умолчанию
            }

            uint32_t heldFor = ttms - s->pressStartTime;

            // Debounce
            if (!s->debounced && heldFor >= BTN_DEBOUNCE_MS) {
                s->debounced = 1;
                s->lastRepeatTime = ttms;
            } 
            // Обработка длительного нажатия
            else if (s->debounced && !s->holdTriggered && 
                     buttonPins[i].repeatEnabled && heldFor >= BTN_HOLD_MS) {
                if (holdHandlers[i] != NULL) {
                    holdHandlers[i]();
                    s->holdTriggered = 1;
                    s->lastRepeatTime = ttms;
                    s->clickFlag = 0;  // Отменяем клик
                }
            }
            // Повторные вызовы для перемотки
            else if (s->debounced && s->holdTriggered && 
                     buttonPins[i].repeatEnabled &&
                     (ttms - s->lastRepeatTime) >= BTN_REPEAT_MS) {
                if (holdHandlers[i] != NULL) {
                    holdHandlers[i]();
                    s->lastRepeatTime = ttms;
                }
            }
        } else {
            // Кнопка отпущена - если был клик и не было длительного нажатия
            if (s->debounced && !s->holdTriggered && s->clickFlag) {
                clickFlags |= (1U << i);
            }
            s->pressStartTime = 0;
            s->debounced = 0;
            s->holdTriggered = 0;
            s->clickFlag = 1;
        }
    }
}

// === ГЛАВНЫЙ ЦИКЛ ОБРАБОТКИ КНОПОК =========================================
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

// === БЛОКИРУЮЩЕЕ ОЖИДАНИЕ ЛЮБОЙ КНОПКИ =====================================
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

// === ОБРАБОТЧИКИ КНОПОК =====================================================

// Кнопка 1: ПРЕДЫДУЩИЙ ТРЕК (клик) / ПЕРЕМОТКА НАЗАД (удержание)
void button1_click(void) {
    wav_playerui_prev();
}

// Кнопка 2: ВОСПРОИЗВЕДЕНИЕ / ПАУЗА
void button2_click(void) {
    wav_playerui_play_pause();
}

// Кнопка 3: СЛЕДУЮЩИЙ ТРЕК (клик) / ПЕРЕМОТКА ВПЕРЕД (удержание)
void button3_click(void) {
    wav_playerui_next();
}

void button1_hold(void) {
        if (!wav_is_playing())
        return;

    uint32_t pos = wav_get_position();
    uint32_t step = wav_get_sample_rate() * wav_get_channels() * 2 * 5;

    if (pos > step)
        wav_seek(pos - step);
    else
        wav_seek(0);
}
void button3_hold(void) {
        if (!wav_is_playing())
        return;

    uint32_t pos  = wav_get_position();
    uint32_t size = wav_get_total_size();

    uint32_t step = wav_get_sample_rate() * wav_get_channels() * 2 * 5;

    pos += step;

    if (pos >= size)
        pos = size - 1;

    wav_seek(pos);
}

// Кнопка 4
void button4_click(void) {
    // Можно добавить функцию позже
}

// Кнопка 5 (переключение каталога)
void button5_click(void) {
    wav_playerui_switch_directory();
}