#ifndef __AUDIO_GEN_H__
#define __AUDIO_GEN_H__

#include "stm32f4xx.h"

// Типы генерируемых сигналов
typedef enum {
    AUDIO_GEN_SINE,       // Синус
    AUDIO_GEN_TRIANGLE,   // Треугольник
    AUDIO_GEN_SQUARE,     // Меандр
} audio_gen_type_t;

// Запустить генерацию сигнала.
// freq_hz    — частота в Гц (1 .. 22050)
// amplitude  — амплитуда (0..32767), 8000 = ~25% от макс.
// duration_ms — длительность в мс (0 = бесконечно)
// type       — форма сигнала
void audio_gen_start(audio_gen_type_t type, uint32_t freq_hz, uint16_t amplitude, uint32_t duration_ms);

// Остановить генерацию
void audio_gen_stop(void);

// Проверить, активна ли генерация
uint8_t audio_gen_is_active(void);

// Вызывать в main loop для обработки остановки по окончании
void audio_gen_process(void);

#endif // __AUDIO_GEN_H__