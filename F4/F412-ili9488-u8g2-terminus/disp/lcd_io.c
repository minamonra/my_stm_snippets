#include "lcd_io.h"

// Выделение памяти под буфер
uint16_t dma_buffer[LCD_DMA_BUFFER_SIZE];

// Низкоуровневая отправка команды (блокирующая)
void lcd_send_cmd(uint8_t cmd) {
    ILI9486_DC_CLR; 
    ILI9486_CS_CLR;
    
    while (!(SPI1->SR & SPI_SR_TXE));
    *(volatile uint8_t *)&SPI1->DR = cmd;
    while (!(SPI1->SR & SPI_SR_RXNE));
    volatile uint8_t dummy = *(volatile uint8_t *)&SPI1->DR; (void)dummy;
    while (SPI1->SR & SPI_SR_BSY);
    
    ILI9486_CS_SET; 
    ILI9486_DC_SET;
}

// Низкоуровневая отправка одиночного байта данных (блокирующая)
void lcd_send_data8(uint8_t data) {
    ILI9486_DC_SET; 
    ILI9486_CS_CLR;
    
    while (!(SPI1->SR & SPI_SR_TXE));
    *(volatile uint8_t *)&SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_RXNE));
    volatile uint8_t dummy = *(volatile uint8_t *)&SPI1->DR; (void)dummy;
    while (SPI1->SR & SPI_SR_BSY);
    
    ILI9486_CS_SET;
}

// Построчный скоростной 8-битный конвейер DMA
void lcd_io_send_buf8_dma(uint8_t *pData, uint32_t bytes_per_transfer, uint32_t transfer_count) {
    for (uint32_t i = 0; i < transfer_count; i++) {
        // Быстро выключаем поток для перезаписи регистров длины и адреса
        DMA2_Stream5->CR &= ~DMA_SxCR_EN;
        while (DMA2_Stream5->CR & DMA_SxCR_EN); 
        
        // Моментально сбрасываем флаги прерываний Потока 5
        DMA2->HIFCR = (DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5);
        
        DMA2_Stream5->M0AR = (uint32_t)pData;            // Указатель на начало байтового буфера
        DMA2_Stream5->NDTR = bytes_per_transfer;         // Сколько байт выстрелить в этой строке
        
        ILI9486_CS_CLR;                                  // Опускаем CS — дисплей слушает строку
        DMA2_Stream5->CR |= DMA_SxCR_EN;                 // Пли!
        
        while (!(DMA2->HISR & DMA_HISR_TCIF5));          // Аппаратное ожидание: DMA докачал строку в регистр SPI
        while (SPI1->SR & SPI_SR_BSY);                  // Аппаратное ожидание: SPI физически вытолкнул последний бит в шину
        
        ILI9486_CS_SET;                                  // Поднимаем CS — фиксируем геометрию строки на дисплее
    }
}

// Возврат SPI1 и DMA в стандартный 8-битный режим (нужно для шрифтов и команд)
void lcd_io_set_8bit(void) {
    SPI1->CR1 &= ~SPI_CR1_SPE;
    
    SPI1->CR1 &= ~SPI_CR1_DFF; // Включаем 8-битный режим кадра (DFF = 0)
    
    // Перенастраиваем DMA2 Stream 5 обратно на 8-бит (MSIZE=8, PSIZE=8)
    DMA2_Stream5->CR &= ~(DMA_SxCR_MSIZE | DMA_SxCR_PSIZE);
    
    SPI1->CR1 |= SPI_CR1_SPE;
}

// Перевод SPI1 и DMA строго в 16-битный скоростной режим (тактовая частота удваивается до 100 МГц)
void lcd_io_set_16bit(void) {
    SPI1->CR1 &= ~SPI_CR1_SPE; // Выключаем SPI для перенастройки
    
    SPI1->CR1 |= SPI_CR1_DFF;  // Включаем 16-битный режим кадра (DFF = 1)
    
    // Перенастраиваем DMA2 Stream 5 на 16-битную передачу (MSIZE=16, PSIZE=16)
    DMA2_Stream5->CR &= ~(DMA_SxCR_MSIZE | DMA_SxCR_PSIZE);
    DMA2_Stream5->CR |= (1U << DMA_SxCR_MSIZE_Pos) | (1U << DMA_SxCR_PSIZE_Pos); 
    
    SPI1->CR1 |= SPI_CR1_SPE;  // Включаем SPI обратно
}