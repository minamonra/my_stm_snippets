#include <stddef.h>
#include "common.h"
#include "ili9488x.h"
#include "demo.h"


int main(void) {
    System_Init();
    ili9488_Init();

    while (1) {
        demo_show_terminus_maxout();
        delay_ms(5000);
    }
}
