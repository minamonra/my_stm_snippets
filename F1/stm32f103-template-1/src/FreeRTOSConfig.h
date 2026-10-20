/**
 * @file    FreeRTOSConfig.h
 * @brief   Конфигурация ядра FreeRTOS. Шаблон STM32F103C8T6.
 *
 * FreeRTOS V11 требует ряд макросов задавать ЯВНО (без значений по умолчанию).
 * При переносе на другой МК проверьте configCPU_CLOCK_HZ, configPRIO_BITS,
 * configTICK_RATE_HZ, configTOTAL_HEAP_SIZE, configMAX_PRIORITIES.
 *
 * Единицы: стеки - в словах (StackType_t = 4 B), куча - в байтах.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ==========================================================================
 * 1. Планировщик
 * ========================================================================== */

#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0 /* Cortex-M3 не поддерживает */
#define configUSE_TICKLESS_IDLE                 0 /* нужен portSUPPRESS_TICKS_AND_SLEEP */

#define configCPU_CLOCK_HZ         (72000000UL)
#define configTICK_RATE_HZ         ((TickType_t)1000U)
#define configUSE_16_BIT_TICKS     0 /* обязательно при 1000 Hz */
#define configMAX_PRIORITIES       (16)
#define configMINIMAL_STACK_SIZE   (128) /* 128 слов = 512 B */
#define configIDLE_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE)
#define configMAX_TASK_NAME_LEN    (16)
#define configIDLE_SHOULD_YIELD    1

/* ==========================================================================
 * 2. Память
 * ========================================================================== */

#define configSUPPORT_STATIC_ALLOCATION  0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configTOTAL_HEAP_SIZE            ((size_t)(8 * 1024))
#define configAPPLICATION_ALLOCATED_HEAP 0

/* ==========================================================================
 * 3. Синхронизация и IPC
 * ========================================================================== */

#define configUSE_MUTEXES                     1
#define configUSE_RECURSIVE_MUTEXES           1
#define configUSE_COUNTING_SEMAPHORES         1
#define configUSE_QUEUE_SETS                  0
#define configUSE_TASK_NOTIFICATIONS          1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1

/* ==========================================================================
 * 4. Группы событий, таймеры
 * ========================================================================== */

#define configUSE_EVENT_GROUPS       1
#define configUSE_TIMERS             1
#define configTIMER_TASK_PRIORITY    (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH     (10)
#define configTIMER_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE)

/* ==========================================================================
 * 5. Хуки  (FreeRTOS V11: обязательно задать ЯВНО)
 * ========================================================================== */

#define configUSE_IDLE_HOOK                0
#define configUSE_TICK_HOOK                0
#define configUSE_MALLOC_FAILED_HOOK       0
#define configCHECK_FOR_STACK_OVERFLOW     0 /* 2 = включить проверку */
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

/* ==========================================================================
 * 6. Дополнительные API задач
 *    Без этих макросов функции НЕ КОМПИЛИРУЮТСЯ вообще.
 * ========================================================================== */

#define INCLUDE_vTaskPrioritySet       1
#define INCLUDE_uxTaskPriorityGet      1
#define INCLUDE_vTaskDelete            1
#define INCLUDE_vTaskCleanUpResources  1
#define INCLUDE_vTaskSuspend           1
#define INCLUDE_vTaskDelayUntil        1
#define INCLUDE_vTaskDelay             1
#define INCLUDE_xTaskGetSchedulerState 1 /* нужен при configUSE_TIMERS = 1 */

/* ==========================================================================
 * 7. Диагностика
 * ========================================================================== */

#define configUSE_TRACE_FACILITY             1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1
#define configGENERATE_RUNTIME_STATS         0
#define configUSE_APPLICATION_TASK_TAG       0

/* ==========================================================================
 * 8. Совместимость (V11)
 * ========================================================================== */

#define configENABLE_BACKWARD_COMPATIBILITY     1
#define configUSE_MINI_LIST_ITEM                0
#define configUSE_POSIX_ERRNO                   0
#define configUSE_CO_ROUTINES                   0 /* устарело в V11 */
#define configUSE_NEWLIB_REENTRANT              0 /* 1 = требует правки sysmem.c */
#define configUSE_PICOLIBC_REENTRANT            0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0

/* ==========================================================================
 * 9. Приоритеты прерываний
 *    Байт приоритета NVIC -> сдвиг на (8 - configPRIO_BITS).
 *    Cortex-M3 (STM32F1): configPRIO_BITS = 4.
 * ========================================================================== */

#define configPRIO_BITS                              4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY      (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ==========================================================================
 * 10. Связка с обработчиками Cortex-M3
 * ========================================================================== */

#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/* ==========================================================================
 * 11. Проверки
 * ========================================================================== */

extern void vAssertHandler(const char* pcFile, unsigned long ulLine);

#define configASSERT(x)                         \
    do {                                        \
        if ((x) == 0) {                         \
            taskDISABLE_INTERRUPTS();           \
            vAssertHandler(__FILE__, __LINE__); \
        }                                       \
    } while (0)

#endif /* FREERTOS_CONFIG_H */