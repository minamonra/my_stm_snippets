#include "common.h"
#include "FreeRTOS.h"
#include "task.h"

// Режимы светодиода
typedef enum {
    LED_SLOW = 0, // Медленно
    LED_FAST,     // Быстро
    LED_ON        // Всегда горит
} led_mode_t;

// Тайминги и константы кнопки
#define T_DEBOUNCE_MS     20U  // Антидребезг
#define T_LONG_PRESS_MS   1000U // Порог удержания
#define T_POLL_MS         20U  // Период опроса
#define T_BLINK_SLOW_MS   500U // Период медленный
#define T_BLINK_FAST_MS   150U // Период быстрый
#define T_ALWAYS_ON_MS    50U  // Период удержания ON
#define PRIORITY_NORMAL   1U   // Равный приоритет задач

static volatile led_mode_t current_mode = LED_SLOW; // Текущий режим

#define BUTTON_PRESSED   ((GPIOC->IDR & GPIO_IDR_IDR_13) != 0) // Кнопка нажата (High)

void task_led(void *pvParameters) {
    (void)pvParameters; // Без варнингов
    while (1) {
        switch (current_mode) {
            case LED_SLOW:
                LED_SYSTEM_TOGGLE; // Мигаем
                vTaskDelay(pdMS_TO_TICKS(T_BLINK_SLOW_MS)); // Спим
                break;
            case LED_FAST:
                LED_SYSTEM_TOGGLE; // Мигаем
                vTaskDelay(pdMS_TO_TICKS(T_BLINK_FAST_MS)); // Спим
                break;
            case LED_ON:
                LED_SYSTEM_ON; // Включаем
                vTaskDelay(pdMS_TO_TICKS(T_ALWAYS_ON_MS)); // Спим
                break;
        }
    }
}

void task_btn(void *pvParameters) {
    (void)pvParameters; // Без варнингов
    u32 pressed_time = 0; // Время зажатия
    bool was_pressed = false; // Предыдущее состояние

    while (1) {
        bool is_pressed = BUTTON_PRESSED; // Текущее состояние
        if (is_pressed) {
            if (!was_pressed) {
                vTaskDelay(pdMS_TO_TICKS(T_DEBOUNCE_MS)); // Антидребезг
                if (BUTTON_PRESSED) {
                    was_pressed = true; // Нажата
                    pressed_time = 0; // Сброс времени
                }
            } else {
                pressed_time += T_POLL_MS; // Копим время
                if (pressed_time >= T_LONG_PRESS_MS && current_mode != LED_ON) {
                    current_mode = LED_ON; // Режим: всегда горит
                }
            }
        } else {
            if (was_pressed) { // Отпущена
                if (pressed_time < T_LONG_PRESS_MS) { // Короткий клик
                    current_mode = (current_mode == LED_SLOW) ? LED_FAST : LED_SLOW; // Смена темпа
                }
                was_pressed = false; // Сброс флага
                pressed_time = 0; // Сброс счетчика
                vTaskDelay(pdMS_TO_TICKS(T_DEBOUNCE_MS)); // Антидребезг отпускания
            }
        }
        vTaskDelay(pdMS_TO_TICKS(T_POLL_MS)); // Шаг опроса
    }
}

int main(void) {
    clock_init(); // Запуск 96МГц
    hw_init(); // Запуск GPIO

    xTaskCreate(task_led, "led", configMINIMAL_STACK_SIZE, NULL, PRIORITY_NORMAL, NULL); // Таск LED
    xTaskCreate(task_btn, "btn", configMINIMAL_STACK_SIZE, NULL, PRIORITY_NORMAL, NULL); // Таск кнопки

    vTaskStartScheduler(); // Старт ОС

    while (1); // Сюда не дойдем
}
