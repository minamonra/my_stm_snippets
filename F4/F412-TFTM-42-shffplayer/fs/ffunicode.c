#include "ff.h"

#if FF_USE_LFN

/* Перевод символа в верхний регистр */
DWORD ff_wtoupper (DWORD uni) {
    if (uni >= 'a' && uni <= 'z') {
        uni -= 0x20;
    }
    return uni;
}

/* Конвертация Unicode в OEM/ASCII */
WCHAR ff_uni2oem (DWORD uni, WORD cp) {
    if (uni < 0x80) {
        return (WCHAR)uni;
    }
    return 0; 
}

/* Конвертация OEM/ASCII в Unicode */
WCHAR ff_oem2uni (WCHAR oem, WORD cp) {
    if (oem < 0x80) {
        return (WCHAR)oem;
    }
    return 0;
}

#endif