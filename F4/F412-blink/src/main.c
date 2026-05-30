#include <stddef.h>
#include "common.h"
#include "pindefs.h"

int main(void) {
    System_Init();                                                              // Инициализация всего железа платы WeAct

    uint8_t last_button_state = 0;                                              // Храним прошлое состояние кнопки

    while(1) {
        if (BUTTON_K1_PRESSED) {
            morse_send_nb("SOS");                                               // Крутим неблокирующий автомат Морзе, пока зажата кнопка C13
            last_button_state = 1;                                              // Запоминаем, что кнопка была нажата
        } else {
            // Если кнопку только что отпустили (была нажата, а теперь нет)
            if (last_button_state == 1) {
                morse_send_nb(NULL);                                            // Сбрасываем автомат Морзе в IDLE
                last_button_state = 0;                                          // Сбрасываем триггер состояния кнопки
            }
            
            blink_led(500);                                                     // В покое спокойно мигаем раз в 500 мс
        }
    }
}