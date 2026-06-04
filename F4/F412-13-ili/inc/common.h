#ifndef COMMON_H
#define COMMON_H

#include "stm32f4xx.h"

extern volatile uint32_t ttms;

// Макрос для безопасного сброса потока DMA
#define RESET_DMA_STREAM5() { \
    DMA2_Stream5->CR &= ~DMA_SxCR_EN; \
    while (DMA2_Stream5->CR & DMA_SxCR_EN); \
    DMA2->HIFCR = (DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5); \
}

uint16_t get_random(uint16_t max);
void delay_ms(uint32_t ms);
void delay_nop(uint32_t ticks);
void gpio_init(void);
void spi_init(void);
void dma_init(void);
void System_Init(void);
uint8_t decode_utf8(const char **str);

// Заглушка для совместимости кода Adafruit с STM32
#ifndef PROGMEM
#define PROGMEM
#endif

#endif // COMMON_H