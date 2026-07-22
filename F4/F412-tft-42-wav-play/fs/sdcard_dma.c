#include "sdcard_dma.h"

// Определение глобального флага
volatile uint8_t sd_dma_transfer_done = 0;

void sd_dma_init(void) {
    // 1. Включаем тактирование контроллера DMA2
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    __DSB();

    // 2. Сбрасываем и выключаем Стрим 3 перед настройкой
    DMA2_Stream3->CR = 0;
    while (DMA2_Stream3->CR & DMA_SxCR_EN);

    // 3. Очищаем все старые флаги прерываний Стрима 3 (биты 27:22 регистра LIFCR)
    DMA2->LIFCR = (0x3DUL << 22); 

    // 4. Задаем фиксированный адрес периферии — окно SDIO FIFO
    DMA2_Stream3->PAR = (uint32_t)&(SDIO->FIFO);

    // 5. Конфигурация FCR: включаем FIFO и ставим максимальный порог (Full)
    DMA2_Stream3->FCR = DMA_SxFCR_DMDIS | (3U << DMA_SxFCR_FTH_Pos);

    // 6. Включаем прерывание Стрима в NVIC (Кубовский наивысший приоритет 0)
    NVIC_SetPriority(DMA2_Stream3_IRQn, 0); 
    NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

void sd_dma_prepare_read(uint8_t* buffer) {
    DMA2_Stream3->CR = 0;
    while (DMA2_Stream3->CR & DMA_SxCR_EN); // Ждем аппаратного отключения
    DMA2->LIFCR = (0x3DUL << 22);           // Очистка флагов

    DMA2_Stream3->M0AR = (uint32_t)buffer;
    DMA2_Stream3->NDTR = 512 / 4;           // 128 слов по 32 бита
    sd_dma_transfer_done = 0;

    // Конфигурируем CR строго по регистрам CubeMX: Burst INC4, 32-бит, PFCTRL
    DMA2_Stream3->CR = (4U << DMA_SxCR_CHSEL_Pos)   // Канал 4 (SDIO)
                     | (1U << DMA_SxCR_MBURST_Pos)  // Memory Burst: INC4
                     | (1U << DMA_SxCR_PBURST_Pos)  // Peripheral Burst: INC4
                     | (0U << DMA_SxCR_PL_Pos)      // Priority: Low (как в Кубе)
                     | (2U << DMA_SxCR_MSIZE_Pos)   // Memory size: 32-bit (Word)
                     | (2U << DMA_SxCR_PSIZE_Pos)   // Periph size: 32-bit (Word)
                     | DMA_SxCR_MINC                // Авто-инкремент адреса памяти
                     | (0U << DMA_SxCR_DIR_Pos)     // Направление: Из периферии в память
                     | DMA_SxCR_PFCTRL              // Контроллер потока — ПЕРИФЕРИЯ (SDIO)
                     | DMA_SxCR_TCIE;               // Прерывание по окончанию (Transfer Complete)

    // Запускаем поток в работу
    DMA2_Stream3->CR |= DMA_SxCR_EN;
}

void sd_dma_abort(void) {
    DMA2_Stream3->CR = 0;
}

void DMA2_Stream3_IRQHandler(void) {
    if (DMA2->LISR & DMA_LISR_TCIF3) {
        DMA2->LIFCR = DMA_LIFCR_CTCIF3; // Очищаем флаг в регистре
        sd_dma_transfer_done = 1;       // Выставляем глобальный флаг готовности
    }
}

