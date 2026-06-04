#include "wav_player.h"
#include "audio_i2s.h"
#include "ff.h" 
#include "hardware.h"
#include <string.h>

extern uint16_t audio_buff1[];
extern uint16_t audio_buff2[];
extern uint16_t* signal_play_buff;
extern uint16_t* signal_read_buff;
extern volatile uint8_t read_next_chunk;
extern volatile uint8_t end_of_file_reached;

// Объявляем универсальный внешний Callback-переключатель, который живет в main.c
extern const char* WAV_Next_Track_Callback(void);

static FATFS fs;
static FIL wav_file;       
static uint8_t wav_is_playing = 0;

volatile uint16_t wav_display_ch = 0;   
volatile uint16_t wav_display_bits = 0; 
volatile uint32_t dbg_file_fs = 0;      
volatile uint32_t wav_sectors_played = 0;
volatile uint32_t wav_audio_bytes_left = 0;
volatile uint32_t total_audio_bytes = 0;
volatile uint32_t wav_byte_rate = 176400; 

#define RAW_BUF_SIZE (AUDIO_BUF_SIZE * 2)  
__attribute__((aligned(4))) static uint16_t raw_pcm_buf[RAW_BUF_SIZE];


uint32_t WAV_Get_Current_Time_Sec(void) {
    if (total_audio_bytes == 0 || wav_byte_rate == 0) return 0;
    uint32_t bytes_played = total_audio_bytes - wav_audio_bytes_left;
    return bytes_played / wav_byte_rate;
}

static void Apply_Fiskov_Unpacking_Mono(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t samples_count) {
    uint16_t* p_src = src_ptr;                                                  
    uint16_t* p_dst = dest_ptr;                                                 
    for (uint32_t i = 0; i < samples_count; i++) {
        uint16_t sample = *p_src++;                                             
        *p_dst++ = sample;                                                      
        *p_dst++ = sample;                                                      
    }
}

static void Apply_Fiskov_Unpacking_Stereo(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t bytes_read) {
    memcpy(dest_ptr, src_ptr, bytes_read);
}

uint8_t WAV_Play(const char* filename) {
    UINT bytes_read;
    __attribute__((aligned(4))) static uint8_t header_buf[44]; 
    
    static uint8_t fs_mounted = 0;
    if (!fs_mounted) {
        if (f_mount(&fs, "", 1) != FR_OK) return 1;
        fs_mounted = 1;
    }
    
    if (f_open(&wav_file, filename, FA_READ) != FR_OK) return 2; 
    
    f_read(&wav_file, header_buf, 44, &bytes_read);
    if (bytes_read < 44) { f_close(&wav_file); return 5; }
    
    if (header_buf[0] != 'R' || header_buf[1] != 'I' || header_buf[2] != 'F' || header_buf[3] != 'F') {
        f_close(&wav_file); return 3; 
    }
    
    uint16_t num_channels   = ((uint16_t)header_buf[23] << 8) | header_buf[22];
    uint32_t sample_rate    = ((uint32_t)header_buf[27] << 24) | ((uint32_t)header_buf[26] << 16) | ((uint32_t)header_buf[25] << 8) | header_buf[24];
    uint16_t bits_sample    = ((uint16_t)header_buf[35] << 8) | header_buf[34];
    uint32_t subchunk2_size = ((uint32_t)header_buf[43] << 24) | ((uint32_t)header_buf[42] << 16) | ((uint32_t)header_buf[41] << 8) | header_buf[40];
    uint32_t byte_rate      = ((uint32_t)header_buf[31] << 24) | ((uint32_t)header_buf[30] << 16) | ((uint32_t)header_buf[29] << 8) | header_buf[28];

    wav_display_ch = num_channels;
    wav_display_bits = bits_sample;
    dbg_file_fs = sample_rate;
    wav_byte_rate = byte_rate;
    
    if (wav_byte_rate == 0) wav_byte_rate = (num_channels == 1) ? 88200 : 176400;

    total_audio_bytes = subchunk2_size;
    wav_audio_bytes_left = subchunk2_size;
    wav_sectors_played = 0;

    static uint32_t last_sample_rate = 0;
    static uint16_t last_channels = 0;
    if (sample_rate != last_sample_rate || num_channels != last_channels) {
        Audio_I2S_Init_Advanced(sample_rate, num_channels);
        last_sample_rate = sample_rate;
        last_channels = num_channels;
    }

    f_lseek(&wav_file, 44);
    
    if (num_channels == 1) {
        f_read(&wav_file, (uint8_t*)raw_pcm_buf, AUDIO_BUF_SIZE, &bytes_read);
        wav_audio_bytes_left -= bytes_read;
        Apply_Fiskov_Unpacking_Mono(audio_buff1, raw_pcm_buf, bytes_read / 2);
    } else {
        f_read(&wav_file, (uint8_t*)raw_pcm_buf, AUDIO_BUF_SIZE * 2, &bytes_read);
        wav_audio_bytes_left -= bytes_read;
        Apply_Fiskov_Unpacking_Stereo(audio_buff1, raw_pcm_buf, bytes_read);
    }

    if (num_channels == 1) {
        f_read(&wav_file, (uint8_t*)raw_pcm_buf, AUDIO_BUF_SIZE, &bytes_read);
        wav_audio_bytes_left -= bytes_read;
        Apply_Fiskov_Unpacking_Mono(audio_buff2, raw_pcm_buf, bytes_read / 2);
    } else {
        f_read(&wav_file, (uint8_t*)raw_pcm_buf, AUDIO_BUF_SIZE * 2, &bytes_read);
        wav_audio_bytes_left -= bytes_read;
        Apply_Fiskov_Unpacking_Stereo(audio_buff2, raw_pcm_buf, bytes_read);  
    }
    
    read_next_chunk = 0;
    end_of_file_reached = 0;
    signal_play_buff = audio_buff1;
    signal_read_buff = audio_buff2;
    wav_is_playing = 1;
    
    Audio_Start(); 
    return 0;
}

void WAV_Process_Buffer(void) {
    if (!wav_is_playing) return;
    UINT bytes_read = 0;
    
    // === НАШ ПОБЕДНЫЙ БЕСШОВНЫЙ ПЕРЕКЛЮЧАТЕЛЬ НА ЛЕТУ ПО CALLBACK ===
    if (wav_audio_bytes_left == 0 || end_of_file_reached) {
        f_close(&wav_file); // Безопасно закрываем старый файл
        
        // Спрашиваем у main.c путь к следующему файлу через универсальное окно
        const char* next_path = WAV_Next_Track_Callback();
        
        if (next_path != NULL && f_open(&wav_file, next_path, FA_READ) == FR_OK) {
            f_lseek(&wav_file, 44); // Пропускаем заголовок
            total_audio_bytes = f_size(&wav_file) - 44;
            wav_audio_bytes_left = total_audio_bytes;
            end_of_file_reached = 0;
            read_next_chunk = 1; // Толкаем DMA дальше
        } else {
            // Если треков больше нет, глушим стрим
            Audio_Stop();
            wav_is_playing = 0;
        }
        return;
    }
    
    if (read_next_chunk == 1) {
        read_next_chunk = 0; 
        
        uint32_t bytes_to_read = (wav_display_ch == 1) ? RAW_BUF_SIZE : (RAW_BUF_SIZE * 2);
        if (bytes_to_read > wav_audio_bytes_left) bytes_to_read = wav_audio_bytes_left;

        FRESULT res = f_read(&wav_file, (uint8_t*)raw_pcm_buf, bytes_to_read, &bytes_read);
        
        if (res == FR_OK && bytes_read > 0) {
            wav_audio_bytes_left -= bytes_read;
            wav_sectors_played += (bytes_read / 512); 
            
            if (wav_display_ch == 1) {
                Apply_Fiskov_Unpacking_Mono((uint16_t*)signal_read_buff, raw_pcm_buf, bytes_read / 2);
            } else {
                Apply_Fiskov_Unpacking_Stereo((uint16_t*)signal_read_buff, raw_pcm_buf, bytes_read);
            }
        }

        if (bytes_read == 0 || wav_audio_bytes_left == 0) {
            if (wav_audio_bytes_left == 0) {
                read_next_chunk = 0;
            } else {
                end_of_file_reached = 1;
            }
        }
    }
}