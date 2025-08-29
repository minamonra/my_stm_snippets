#include "common.h"
#include "dispmt16s.h"


// === main() ===
int main(void) {
    StartHSE();
    hardware_init();
    lcd_init();

    const char *utf8_str = "КХМъъ\\, мир! Ёё";
    char cp1251_str[16];

    utf8_to_cp1251(utf8_str, cp1251_str, sizeof(cp1251_str));


    const char str1[7] = {0xCF, 0xF0, 0xE8, 0xE2, 0xE5, 0xF2, 0x00};
    lcdCommand(0x80);       // 1-я строка
    lcdString(str1);

    lcdCommand(0xC0);       // 2-я строка
    lcdString(cp1251_str);

    while (1) {
        blink_pc13led(199);
    }
}

// === EoF === 