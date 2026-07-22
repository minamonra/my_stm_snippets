#ifndef WAV_PLAYER_UI_H
#define WAV_PLAYER_UI_H
#include <stdint.h>

#define MAX_TRACKS 255

// Параметры перемотки (можно менять)
#define SEEK_STEP_MS 200      // Шаг перемотки в миллисекундах (50-200)
#define SEEK_REPEAT_MS 50     // Интервал повторения при удержании (50-150)
// ПАРАМЕТРЫ ПЕРЕМОТКИ
#define SEEK_BYTES_PER_MS 400

void wav_playerui_init(void);
void wav_playerui_process(void);

// Функции управления плеером
void wav_playerui_play_pause(void);
void wav_playerui_next(void);
void wav_playerui_prev(void);
void wav_playerui_switch_directory(void);

#endif // WAV_PLAYER_UI_H
