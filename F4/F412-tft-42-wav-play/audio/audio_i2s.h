#ifndef __AUDIO_I2S_H__
#define __AUDIO_I2S_H__

#include "stm32f4xx.h"

// AUDIO_CHUNK_SIZE = число стерео-фреймов в ОДНОЙ половине циклического буфера.
// Совпадает со значением, использованным в CubeMX/HAL-версии.
// #define AUDIO_CHUNK_SIZE   1024
#define AUDIO_CHUNK_SIZE 4096

// Производные размеры буфера
#define AUDIO_HALF_SIZE (AUDIO_CHUNK_SIZE * 2)  // half-word в одной половине
#define AUDIO_BUF_SIZE  (AUDIO_CHUNK_SIZE * 4)  // half-word во всём буфере

// Пины I2S2 (SPI2 в режиме I2S)
#define I2S2_WS_PIN 12
#define I2S2_CK_PIN 13
#define I2S2_SD_PIN 15

// Все флаги прерываний DMA1 Stream4
#define DMA_STREAM4_ALL_FLAGS (DMA_HIFCR_CFEIF4 | DMA_HIFCR_CDMEIF4 | \
                               DMA_HIFCR_CTEIF4 | DMA_HIFCR_CHTIF4 |  \
                               DMA_HIFCR_CTCIF4)
typedef void (*audio_buff_ready_callback_t)(uint16_t* buf);

typedef struct {
  audio_buff_ready_callback_t on_buffer_ready;
} audio_callbacks_t;

// Возвращают указатели на ПОЛОВИНЫ единого циклического буфера.
// Каждая половина: AUDIO_CHUNK_SIZE стерео-фреймов * 2 канала * 1 half-word
// на канал (16 бит данных в 32-бит I2S frame, без явного padding-слова в памяти).
uint16_t* audio_get_buffer1(void);
uint16_t* audio_get_buffer2(void);

void audio_set_callbacks(audio_callbacks_t* cb);

void audio_init(void);
void audio_start(void);
void audio_stop(void);

#endif  // __AUDIO_I2S_H__