#ifndef __SDCARD_DMA_H__
#define __SDCARD_DMA_H__

#include "stm32f4xx.h"
#include <stdint.h>

// Глобальный флаг окончания транзакции, который будет читать sdcard.c
extern volatile uint8_t sd_dma_transfer_done;

// Прототипы функций управления DMA для SDIO
void sd_dma_init(void);
void sd_dma_prepare_read(uint8_t* buffer);
void sd_dma_abort(void);

#endif // __SDCARD_DMA_H__
