#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdint.h>
#include <string.h> 
#include <stdlib.h>
#include "inc/hardware.h"    // Добавлен префикс папки inc/
#include "inc/st7789v.h"     // Добавлен префикс папки inc/
#include "inc/wav_player.h"  // Добавлен префикс папки inc/
#include "inc/audio_i2s.h"   // Добавлен префикс папки inc/
#include "inc/ff.h"          // Добавлен префикс папки inc/
#include "inc/sdcard.h"      // Добавлен префикс папки inc/

// --- НАСТРОЙКА КАТАЛОГА МУЗЫКИ ---
#define AUDIO_DIR   "0:AU"

// --- ЦВЕТА СИСТЕМНОГО ИНТЕРФЕЙСА ---


// --- СОСТОЯНИЯ АВТОМАТА АУДИОПЛЕЕРА ---
typedef enum {
    STATE_INIT,         
    STATE_PLAY_NEXT,    // Поиск СЛУЧАЙНОГО трека и МГНОВЕННЫЙ СТАРТ звука
    STATE_PLAYING,      // Воспроизведение и обновление прогресс-бара
    STATE_ERROR         
} PlayerState_t;

// --- ПРОТОТИПЫ ГЛОБАЛЬНЫХ ФУНКЦИЙ ---
void Draw_Static_UI(uint16_t track_num);
void Display_Process(uint32_t sec_played, const char* file_name, uint8_t update_text);
uint8_t Find_Random_Wav_File(char* out_name);
uint32_t Get_Random_Number(uint32_t max_val);
void Process_Button_Events(void);
uint8_t Playlist_Init(void);
uint8_t Playlist_Load_Track(uint32_t* track_start_time);

#endif // MAIN_H