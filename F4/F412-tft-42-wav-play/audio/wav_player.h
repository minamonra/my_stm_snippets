#ifndef __WAV_PLAYER_H__
#define __WAV_PLAYER_H__
#include "stm32f4xx.h"
#include "audio_i2s.h"  // для AUDIO_CHUNK_SIZE

// === ВЫБОР РЕЖИМА РАСПАКОВКИ ================================================
// Раскомментируйте нужный режим:
// #define WAV_MODE_FAST_HALF  // Быстрый: деление на 2 (1 такт)
// #define WAV_MODE_AMPLITUDE // Точный: умножение на коэффициент
#define WAV_MODE_PASSTHROUGH  // Прямой: без обработки

// Размер одного аудиофрейма в байтах (16-bit PCM)
#define WAV_FRAME_SIZE(ch) ((ch) * 2)

// Размер одного DMA-полубуфера в байтах
#define WAV_HALF_BYTES (AUDIO_CHUNK_SIZE * 2 * sizeof(uint16_t))

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