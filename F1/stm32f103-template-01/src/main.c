#include "main.h"

// Тестовая задача-мигалка для проверки работоспособности шаблона
void vBlinkTask(void* pvParameters) {
    (void)pvParameters;

    // Включаем тактирование порта GPIOC
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // Конфигурируем вывод PC13 на выход (General purpose output push-pull, 50 MHz)
    // Очищаем биты конфигурации пина 13 (в регистре CRH пины 8-15)
    GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
    // Выставляем MODE13 в 0b11 (Выход, 50 МГц), CNF13 в 0b00 (Push-Pull)
    GPIOC->CRH |= (0b11 << GPIO_CRH_MODE13_Pos);

    while (1) {
        // Инвертируем состояние светодиода на PC13 через регистр ODR
        GPIOC->ODR ^= GPIO_ODR_ODR13;

        // Потокобезопасная задержка FreeRTOS на 500 миллисекунд
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Инициализация тактирования микроконтроллера на 72 МГц (Официальный CMSIS STM32F1)
void clock_init(void) {
    // 1. Включаем внешний кварцевый резонатор (HSE)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // 2. Настройка Flash: 2 цикла ожидания (Latency), так как частота > 48 МГц
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    // 3. Настройка шин APB1 и APB2:
    // APB1 не должна превышать 36 МГц, поэтому делим системную частоту на 2 (72 / 2 = 36 МГц)
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
    // APB2 может работать на 72 МГц, оставляем делитель 1 (по умолчанию)

    // 4. Конфигурация PLL:
    // RCC_CFGR_PLLSRC - выбирает HSE в качестве входного сигнала для PLL
    // RCC_CFGR_PLLMULL9 - умножитель частоты на 9 (8 МГц * 9 = 72 МГц)
    RCC->CFGR |= RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9;

    // 5. Включаем PLL и ждем фиксации частоты
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // 6. Переключаем системное тактирование (SYSCLK) на выход PLL
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    // Ждем, пока статус источника SYSCLK не подтвердит переключение на PLL
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

int main(void) {
    // Базовая инициализация железа
    clock_init();

    // Создаем тестовую задачу FreeRTOS
    xTaskCreate(vBlinkTask, "Blink", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    // Запуск планировщика операционной системы
    vTaskStartScheduler();

    // Сюда мы никогда не должны попасть
    while (1) {
        __NOP();
    }
}
