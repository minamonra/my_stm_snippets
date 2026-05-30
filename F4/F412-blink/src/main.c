#include "common.h"
#include "pindefs.h"

int main(void) {
    system_clock_config_100MHz();   // Запуск ядра на 100 МГц и SysTick
    gpio_init();                   // Настройка периферии GPIO

    while(1) {
        LED_SYSTEM_TOGGLE;         // Мигаем системным диодом
        delay_ms(500);             // Спим 500 миллисекунд
    }
}