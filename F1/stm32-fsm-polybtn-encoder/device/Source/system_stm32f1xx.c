#include "system_stm32f1xx.h"

uint32_t SystemCoreClock = 8000000U;

void SystemInit(void) {
    /* Базовая инициализация ядра (очищаем регистры тактирования) */
    /* Чип стартует на внутреннем HSI генераторе 8 МГц */
}

void SystemCoreClockUpdate(void) {
    SystemCoreClock = 8000000U;
}
