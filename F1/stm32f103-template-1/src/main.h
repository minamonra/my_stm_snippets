#ifndef __MAIN_H__
#define __MAIN_H__

#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include "task.h"

// Вызывается из configASSERT() при нарушении инварианта FreeRTOS
void vAssertHandler(const char* pcFile, unsigned long ulLine) {
    (void)pcFile;
    (void)ulLine;
    taskDISABLE_INTERRUPTS();
    for (;;) {
        __BKPT(0);  // остановиться в отладчике
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) {
        __BKPT(0);
    }
}

void vApplicationMallocFailedHook(void) {
    taskDISABLE_INTERRUPTS();
    for (;;) {
        __BKPT(0);
    }
}

#endif  // __MAIN_H__