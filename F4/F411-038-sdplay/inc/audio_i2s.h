#ifndef AUDIO_I2S_H
#define AUDIO_I2S_H

#include "stm32f4xx.h"

// Размер куска буфера в полусловах (2048 слов = 4096 байт)
//#define AUDIO_BUF_SIZE     4096   // Было 2048, стало 4096
//#define AUDIO_BUF_SIZE     3072  // Между 2048 и 4096
//#define AUDIO_BUF_SIZE     8192
//#define AUDIO_BUF_SIZE     2048
#define AUDIO_BUF_SIZE     1024
#define AUDIO_CHUNK_SIZE   AUDIO_BUF_SIZE

// Экспорт буферов и указателей для wav_player.c
extern uint16_t audio_buff1[AUDIO_CHUNK_SIZE * 2];
extern uint16_t audio_buff2[AUDIO_CHUNK_SIZE * 2];
extern uint16_t* signal_play_buff;
extern uint16_t* signal_read_buff;

extern volatile uint8_t read_next_chunk;
extern volatile uint8_t end_of_file_reached;

void Audio_I2S_Init_Advanced(uint32_t sample_rate, uint16_t channels);
void Audio_I2S_Transmit_Next(uint32_t addr, uint16_t size);
void Audio_Start(void);
void Audio_Stop(void);

#endif // AUDIO_I2S_H