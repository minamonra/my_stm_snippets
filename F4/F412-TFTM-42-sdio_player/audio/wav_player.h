#ifndef __WAV_PLAYER_H__
#define __WAV_PLAYER_H__

#include "stm32f4xx.h"
extern volatile uint8_t need_fill; // флаг готовности буфера DMA
// флаг, который говорит, что нужно подлить новые данные в аудиобуфер, пока старые ещё играют
// В прерывании DMA (когда половина или весь буфер заполнен) устанавливается need_fill = 1;
// Это значит: "Эй, процессор, буфер проигрался наполовину, нужно залить новыми данными!"

uint8_t wav_play(const char* filename);
void    wav_process(void);
uint8_t wav_is_playing(void);

// Функции управления
void    wav_pause(void);
void    wav_resume(void);
void    wav_seek_forward(uint32_t seconds);
void    wav_seek_backward(uint32_t seconds);

// Функции получения информации
uint32_t wav_get_position(void);
uint32_t wav_get_total_size(void);
uint32_t wav_get_sample_rate(void);
uint16_t wav_get_channels(void);
void     wav_get_file_info(uint32_t* pos, uint32_t* total, uint32_t* sr, uint16_t* ch);
void wav_stop(void);

#endif