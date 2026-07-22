#ifndef __WAV_PLAYER_H__
#define __WAV_PLAYER_H__
#include "stm32f4xx.h"

// === ВЫБОР РЕЖИМА РАСПАКОВКИ ================================================
// Раскомментируйте нужный режим:
#define WAV_MODE_FAST_HALF  // Быстрый: деление на 2 (1 такт)
// #define WAV_MODE_AMPLITUDE // Точный: умножение на коэффициент
//#define WAV_MODE_PASSTHROUGH  // Прямой: без обработки

static inline void dma_stream4_wait_stop(void) {
    while (DMA1_Stream4->CR & DMA_SxCR_EN)
        __NOP();
}

uint8_t wav_play(const char* filename);
void    wav_stop(void);
void    wav_process(void);
uint8_t wav_is_playing(void);

// Функции управления
void wav_pause(void);
void wav_resume(void);

// Функции получения информации
uint32_t wav_get_position(void);
uint32_t wav_get_total_size(void);
uint32_t wav_get_sample_rate(void);
uint16_t wav_get_channels(void);
void     wav_get_file_info(uint32_t* pos, uint32_t* total, uint32_t* sr, uint16_t* ch);

// Универсальная функция перемотки (для длительных нажатий)
void wav_seek(uint32_t new_position);

#endif