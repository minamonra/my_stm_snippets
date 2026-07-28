#ifndef AUDIO_GEN_H
#define AUDIO_GEN_H

#include <stdint.h>

// Новые типы сигналов
typedef enum {
    AUDIO_GEN_SINE = 0,          // Синусоида
    AUDIO_GEN_TRIANGLE,          // Треугольник (симметричный)
    AUDIO_GEN_SQUARE,            // Меандр (трапеция)
    AUDIO_GEN_SAWTOOTH           // Пила (линейный подъем)
} audio_gen_type_t;

// Две опции для амплитуды
typedef enum {
    AUDIO_VOLUME_HALF = 0,       // Половина громкости (16384)
    AUDIO_VOLUME_FULL            // Полная громкость (32767)
} audio_gen_volume_t;

// Публичный API генератора — ТЕПЕРЬ СТРОГО 3 АРГУМЕНТА
void    audio_gen_start(audio_gen_type_t type, audio_gen_volume_t volume, uint32_t duration_ms);
void    audio_gen_stop(void);
uint8_t audio_gen_is_active(void);
void    audio_gen_process(void);

#endif // AUDIO_GEN_H
