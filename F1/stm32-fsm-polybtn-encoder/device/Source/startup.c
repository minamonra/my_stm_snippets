#include "stm32f103xb.h"

/* Объявляем внешние переменные из скрипта линкера */
extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __StackTop;

/* Прототипы основных функций */
extern int main(void);
extern void SystemInit(void);

/* Стандартный обработчик по умолчанию для прерываний (бесконечный цикл) */
void Default_Handler(void) {
    while (1);
}

/* Функция сброса микроконтроллера */
void Reset_Handler(void) {
    uint32_t *src, *dst;

    /* 1. Копируем секцию .data из Flash в RAM */
    src = &__etext;
    dst = &__data_start__;
    while (dst < &__data_end__) {
        *dst++ = *src++;
    }

    /* 2. Заполняем секцию .bss нулями в RAM */
    dst = &__bss_start__;
    while (dst < &__bss_end__) {
        *dst++ = 0;
    }

    /* 3. Вызываем системную инициализацию */
    SystemInit();

    /* 4. Переходим в главную программу */
    main();

    /* Если вышли из main — уходим в бесконечный цикл */
    while (1);
}

/* Слабые (weak) алиасы для системных исключений Cortex-M3 */
void NMI_Handler(void)          __attribute__ ((weak, alias("Default_Handler")));
void HardFault_Handler(void)    __attribute__ ((weak, alias("Default_Handler")));
void MemManage_Handler(void)    __attribute__ ((weak, alias("Default_Handler")));
void BusFault_Handler(void)     __attribute__ ((weak, alias("Default_Handler")));
void UsageFault_Handler(void)   __attribute__ ((weak, alias("Default_Handler")));
void SVC_Handler(void)          __attribute__ ((weak, alias("Default_Handler")));
void DebugMon_Handler(void)     __attribute__ ((weak, alias("Default_Handler")));
void PendSV_Handler(void)       __attribute__ ((weak, alias("Default_Handler")));
void SysTick_Handler(void)      __attribute__ ((weak, alias("Default_Handler")));

/* Таблица векторов прерываний (помещается в секцию .vectors) */
__attribute__((section(".vectors")))
const void *const __vector_table[] = {
    (const void *)&__StackTop,  /* Точка вершины стека */
    (const void *)Reset_Handler, /* Вектор сброса */
    (const void *)NMI_Handler,
    (const void *)HardFault_Handler,
    (const void *)MemManage_Handler,
    (const void *)BusFault_Handler,
    (const void *)UsageFault_Handler,
    0, 0, 0, 0,                  /* Зарезервировано */
    (const void *)SVC_Handler,
    (const void *)DebugMon_Handler,
    0,                           /* Зарезервировано */
    (const void *)PendSV_Handler,
    (const void *)SysTick_Handler
};
