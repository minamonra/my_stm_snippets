#ifndef WAV_PLAYER_H
#define WAV_PLAYER_H

#include "stm32f4xx.h"

// ИСПРАВЛЕНО: Добавлен атрибут __attribute__((packed)) для жесткого сжатия в 44 байта!
typedef struct {
    char     chunk_id[4];     // "RIFF"
    uint32_t chunk_size;
    char     format[4];       // "WAVE"
    char     subchunk1_id[4]; // "fmt "
    uint32_t subchunk1_size;  
    uint16_t audio_format;    
    uint16_t num_channels;    
    uint32_t sample_rate;     
    uint32_t byte_rate;       
    uint16_t block_align;
    uint16_t bits_per_sample; 
    char     subchunk2_id[4]; // "data"
    uint32_t subchunk2_size;  
} __attribute__((packed)) WAV_Header_t; 

uint8_t WAV_Play(const char* filename);
void WAV_Process_Buffer(void);
uint32_t WAV_Get_Current_Time_Sec(void);


#endif // WAV_PLAYER_H