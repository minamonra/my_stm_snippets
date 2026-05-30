#include "common.h"
#include "pindefs.h"

int main(void) {
    System_Init();   

    while(1) {
        blink_led(500);                                                         // Мигаем светодиодом каждые 500 мс
    }
}