#include "pindefs.h"
#include "button.h"
#include "common.h"   // ttms, delay_ms при необходимости

// ---- Настраиваемые тайминги (в миллисекундах, не зависят от частоты опроса) ----
#define BTN_SAMPLE_PERIOD_MS   5     // с какой периодичностью реально сэмплируем пины
#define BTN_DEBOUNCE_MS        30    // сколько держать нажатой, чтобы засчитать клик
#define BTN_HOLD_MS            600   // после этого времени начинается авто-повтор
#define BTN_REPEAT_MS          150   // период авто-повтора при удержании

typedef void (*ButtonCallback)(void);
static const ButtonCallback clickHandlers[BTN_COUNT] = {
    Button1_Click, Button2_Click, Button3_Click, Button4_Click, Button5_Click
    // при добавлении кнопки дописать сюда ButtonN_Click
};

static volatile uint8_t clickFlags = 0; // бит i = кнопка i только что кликнула (обработать и сбросить)

// Состояние каждой кнопки для дребезга/удержания
typedef struct {
    uint32_t pressStartTime;   // ttms в момент начала нажатия (0, если не нажата)
    uint32_t lastRepeatTime;   // ttms последнего авто-повтора
    uint8_t  debounced;        // 1, если клик уже засчитан на текущее нажатие
} ButtonState_t;

static ButtonState_t btnState[BTN_COUNT] = {0};

// ============================================================================
// === ИНИЦИАЛИЗАЦИЯ GPIO ПОД КНОПКИ ==========================================
// ============================================================================
// Порт уже затактирован (RCC_AHB1ENR_GPIOBEN включен ранее).
// Настраиваем каждую ножку как вход с подтяжкой к VCC (кнопка замыкает на GND).
//
// ВАЖНО про JTAG: PB4 (NJTRST) и PB3 (JTDO/TRACESWO) после сброса находятся
// в режиме альтернативной функции JTAG. Как только мы переводим их в режим
// GPIO Input (MODER = 00), альтернативная функция автоматически отключается —
// отдельно лезть в DBGMCU не нужно. SWD (PA13/PA14) при этом не трогается,
// отладка через SWD продолжит работать.
void Buttons_Init(void) {
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
// === ЧТЕНИЕ СОСТОЯНИЯ КНОПОК (активный уровень = 0, кнопка на GND) =========
// ============================================================================
static uint16_t GET_BUTTONS_STATE(void) {
    uint16_t state = 0;
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        if ((buttonPins[i].port->IDR & (1U << buttonPins[i].pin)) == 0) {
            state |= (1U << i); // 1 = кнопка нажата
        }
    }
    return state;
}

// ============================================================================
// === ОПРОС КНОПОК (дребезг + клик + авто-повтор при удержании) ============
// ============================================================================
// Внутри сама себя ограничивает по времени (BTN_SAMPLE_PERIOD_MS), поэтому
// её можно безопасно дёргать хоть в каждой итерации главного цикла —
// лишние вызовы просто ничего не делают. Это убирает прежнюю проблему
// рассинхронизации между Button_Process(interval_ms) и delay_ms(10)
// внутри Button_WaitAnyPress().
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
            // Кнопка физически нажата
            if (s->pressStartTime == 0) {
                s->pressStartTime = ttms;   // фиксируем начало нажатия
                s->debounced = 0;
            }

            uint32_t heldFor = ttms - s->pressStartTime;

            if (!s->debounced && heldFor >= BTN_DEBOUNCE_MS) {
                // Прошёл дребезг — засчитываем клик
                clickFlags |= (1U << i);
                s->debounced = 1;
                s->lastRepeatTime = ttms;
            } else if (s->debounced && buttonPins[i].repeatEnabled &&
                       heldFor >= BTN_HOLD_MS &&
                       (ttms - s->lastRepeatTime) >= BTN_REPEAT_MS) {
                // Удержание -> авто-повтор клика
                clickFlags |= (1U << i);
                s->lastRepeatTime = ttms;
            }
        } else {
            // Кнопка отпущена — сброс состояния
            s->pressStartTime = 0;
            s->debounced = 0;
        }
    }
}

// ============================================================================
// === ГЛАВНЫЙ ЦИКЛ: обработка кликов =========================================
// ============================================================================
void Button_Process(uint32_t interval_ms) {
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
// === БЛОКИРУЮЩЕЕ ОЖИДАНИЕ ЛЮБОЙ КНОПКИ =======================================
// ============================================================================
void Button_WaitAnyPress(void) {
    // Шаг 1: дождаться, что все кнопки отпущены (если что-то было зажато)
    while (GET_BUTTONS_STATE() != 0) {
        Buttons_Poll();
    }
    clickFlags = 0;

    // Шаг 2: ждать клика по любой кнопке
    while (1) {
        Buttons_Poll();
        if (clickFlags) {
            clickFlags = 0;
            break;
        }
    }

    // Шаг 3: дождаться полного отпускания, чтобы не улетело в главный цикл
    while (GET_BUTTONS_STATE() != 0) {
        Buttons_Poll();
    }
    clickFlags = 0;
}