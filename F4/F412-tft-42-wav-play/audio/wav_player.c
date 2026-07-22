#include "wav_player.h"
#include "audio_i2s.h"
#include "uart.h"
#include "ff.h"
#include <string.h>

/*
Сейчас структура очень хорошая:

wav_play()
    |
    +--> открыть файл
    +--> прочитать заголовок
    +--> wav_prime_buffers()
    +--> audio_start()

DMA IRQ
    |
    +--> cb_buffer_ready()
            |
            +--> wav_fill_buffer()

wav_seek()
    |
    +--> остановить DMA
    +--> f_lseek()
    +--> wav_prime_buffers()
    +--> audio_start()

wav_process()
    |
    +--> только обработка остановки

То есть чтением SD-карты занимаются только две функции:
wav_fill_buffer() — во время воспроизведения;
wav_prime_buffers() — при старте и перемотке
 
*/


extern volatile uint32_t ttms;
uint32_t                 wav_data_offset = 44;

// Структура WAV-заголовка (44 байта)
typedef struct {
  char     chunk_id[4];      // "RIFF"
  uint32_t chunk_size;       // Размер файла - 8
  char     format[4];        // "WAVE"
  char     subchunk1_id[4];  // "fmt "
  uint32_t subchunk1_size;   // 16 для PCM
  uint16_t audio_format;     // 1 = PCM
  uint16_t num_channels;     // 1 = моно, 2 = стерео
  uint32_t sample_rate;      // Частота дискретизации (Гц)
  uint32_t byte_rate;        // sample_rate * num_channels * bits_per_sample/8
  uint16_t block_align;      // num_channels * bits_per_sample/8
  uint16_t bits_per_sample;  // 8, 16 и т.д.
  char     subchunk2_id[4];  // "data"
  uint32_t subchunk2_size;   // Размер аудиоданных в байтах
} __attribute__((packed)) wav_header_t;

// Глобальные переменные файловой системы и состояния воспроизведения
static FATFS    fs;
static FIL      file;
static uint8_t  playing    = 0;
static uint8_t  eof        = 0;
static uint8_t  stopping   = 0;
static uint16_t channels   = 0;
static uint32_t bytes_left = 0;

// Параметры текущего файла для перемотки
static uint32_t sample_rate      = 44100;
static uint32_t current_position = 0;
static uint32_t total_data_size  = 0;

static uint16_t buf_raw[AUDIO_CHUNK_SIZE * 2] __attribute__((aligned(4)));

 // очищаем все флаги DMA Stream4; можно записать так: DMA1->HIFCR = 0x3DU;
#define DMA_STREAM4_CLR_ALL_FLAGS ( \
    DMA_HIFCR_CFEIF4  | \
    DMA_HIFCR_CDMEIF4 | \
    DMA_HIFCR_CTEIF4  | \
    DMA_HIFCR_CHTIF4  | \
    DMA_HIFCR_CTCIF4)


static inline void audio_stop_dma1_stream4(void) {
    SPI2->CR2 &= ~SPI_CR2_TXDMAEN;
    DMA1_Stream4->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream4->CR & DMA_SxCR_EN) __NOP();
    DMA1->HIFCR = (     \
    DMA_HIFCR_CFEIF4  | \
    DMA_HIFCR_CDMEIF4 | \
    DMA_HIFCR_CTEIF4  | \
    DMA_HIFCR_CHTIF4  | \
    DMA_HIFCR_CTCIF4);
    SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
}

// === БЛОК РАСПАКОВКИ ========================================================
#if defined(WAV_MODE_FAST_HALF)
// Быстрый режим — деление на 2 (один такт ASR)
static void unpack_mono(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t samples_count) {
  int16_t*  p_src = (int16_t*)src_ptr;
  uint16_t* p_dst = dest_ptr;
  for (uint32_t i = 0; i < samples_count; i++) {
    int16_t sample = (*p_src++) >> 1;
    *p_dst++       = (uint16_t)sample;
    *p_dst++       = (uint16_t)sample;
  }
}
static void unpack_stereo(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t bytes_read) {
  int16_t*  p_src         = (int16_t*)src_ptr;
  uint16_t* p_dst         = dest_ptr;
  uint32_t  total_samples = bytes_read / 2;
  for (uint32_t i = 0; i < total_samples; i++) {
    *p_dst++ = (uint16_t)((*p_src++) >> 1);
  }
}
#elif defined(WAV_MODE_AMPLITUDE)
// Точный режим — умножение на WAV_AMPLITUDE
#define WAV_AMPLITUDE 200
static void unpack_mono(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t samples_count) {
  int16_t*  p_src = (int16_t*)src_ptr;
  uint16_t* p_dst = dest_ptr;
  for (uint32_t i = 0; i < samples_count; i++) {
    int32_t raw_sample = *p_src++;
    int16_t sample     = (int16_t)((raw_sample * WAV_AMPLITUDE) >> 8);
    *p_dst++           = (uint16_t)sample;
    *p_dst++           = (uint16_t)sample;
  }
}
static void unpack_stereo(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t bytes_read) {
  int16_t*  p_src         = (int16_t*)src_ptr;
  uint16_t* p_dst         = dest_ptr;
  uint32_t  total_samples = bytes_read / 2;
  for (uint32_t i = 0; i < total_samples; i++) {
    int32_t raw_sample = *p_src++;
    *p_dst++           = (uint16_t)((raw_sample * WAV_AMPLITUDE) >> 8);
  }
}
#elif defined(WAV_MODE_PASSTHROUGH)
// Режим без обработки (100% громкость)
static void unpack_mono(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t samples_count) {
  uint16_t* p_src = src_ptr;
  uint16_t* p_dst = dest_ptr;
  for (uint32_t i = 0; i < samples_count; i++) {
    uint16_t sample = *p_src++;
    *p_dst++        = sample;
    *p_dst++        = sample;
  }
}
static void unpack_stereo(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t bytes_read) {
  memcpy(dest_ptr, src_ptr, bytes_read);
}
#else
// По умолчанию — быстрый режим
static void unpack_mono(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t samples_count) {
  int16_t*  p_src = (int16_t*)src_ptr;
  uint16_t* p_dst = dest_ptr;
  for (uint32_t i = 0; i < samples_count; i++) {
    int16_t sample = (*p_src++) >> 1;
    *p_dst++       = (uint16_t)sample;
    *p_dst++       = (uint16_t)sample;
  }
}
static void unpack_stereo(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t bytes_read) {
  int16_t*  p_src         = (int16_t*)src_ptr;
  uint16_t* p_dst         = dest_ptr;
  uint32_t  total_samples = bytes_read / 2;
  for (uint32_t i = 0; i < total_samples; i++) {
    *p_dst++ = (uint16_t)((*p_src++) >> 1);
  }
}
#endif

// === ФУНКЦИИ ПОЛУЧЕНИЯ ИНФОРМАЦИИ ==========================================
uint32_t wav_get_position(void) { return current_position; }
uint32_t wav_get_total_size(void) { return total_data_size; }
uint32_t wav_get_sample_rate(void) { return sample_rate; }
uint16_t wav_get_channels(void) { return channels; }
uint8_t  wav_is_playing(void) { return playing; }

void wav_get_file_info(uint32_t* pos, uint32_t* total, uint32_t* sr, uint16_t* ch) {
  if (pos) *pos = current_position;
  if (total) *total = total_data_size;
  if (sr) *sr = sample_rate;
  if (ch) *ch = channels;
}

// функция которая умеет заполнить один DMA-полу-буфер
static void wav_fill_buffer(uint16_t* buf) {
  uint32_t chunk = AUDIO_CHUNK_SIZE * (channels == 1 ? 2 : 4);
  UINT     br = 0;
  uint32_t n  = (bytes_left > chunk) ? chunk : bytes_left;
  if (n == 0) {
    memset(buf, 0, AUDIO_CHUNK_SIZE * 2 * sizeof(uint16_t));
    eof      = 1;
    stopping = 1;
    return;
  }
  if (f_read(&file, buf_raw, n, &br) != FR_OK) {
    memset(buf, 0, AUDIO_CHUNK_SIZE * 2 * sizeof(uint16_t));
    eof      = 1;
    stopping = 1;
    return;
  }
  bytes_left -= br;
  current_position += br;
  if (channels == 1) unpack_mono(buf, buf_raw, br / 2);
  else unpack_stereo(buf, buf_raw, br);

  if (br < chunk) {
    if (channels == 1) {
      uint16_t* p = buf + (br / 2) * 2;
      memset(p, 0, (chunk - br) * 2);
    } else {
      uint16_t* p = buf + br / 2;
      memset(p, 0, chunk - br);
    }
    eof = 1;
  }
}

// функция "подготовить два DMA буфера"
static void wav_prime_buffers(void) {
  eof      = 0;
  stopping = 0;
  wav_fill_buffer(audio_get_buffer1());
  wav_fill_buffer(audio_get_buffer2());
}

// === КОЛЛБЭК ГОТОВНОСТИ БУФЕРА ==============================================
static void cb_buffer_ready(uint16_t* buf) {
  if (!playing) return;
  wav_fill_buffer(buf);
}

// === ПУБЛИЧНЫЙ ИНТЕРФЕЙС ====================================================
uint8_t wav_play(const char* filename) {
  wav_header_t hdr;
  UINT         br;
  if (playing) {
    audio_stop_dma1_stream4();
    f_close(&file);
    playing = 0;
  }
  eof                    = 0;
  stopping               = 0;
  bytes_left             = 0;
  channels               = 0;
  current_position       = 0;
  total_data_size        = 0;
  static uint8_t mounted = 0;
  if (!mounted) {
    if (f_mount(&fs, "0:", 0) != FR_OK) return 1;
    mounted = 1;
  }
  if (f_open(&file, filename, FA_READ) != FR_OK) return 2;
  if (f_read(&file, &hdr, sizeof(hdr), &br) != FR_OK ||
      br != sizeof(hdr) ||
      memcmp(hdr.chunk_id, "RIFF", 4) != 0) {
    f_close(&file);
    return 3;
  }
  channels                = hdr.num_channels;
  sample_rate             = hdr.sample_rate;
  uint32_t real_data_size = f_size(&file) - wav_data_offset;
  if (hdr.subchunk2_size > 0 && hdr.subchunk2_size <= real_data_size) {
    total_data_size = hdr.subchunk2_size;
  } else {
    total_data_size = real_data_size;
  }
  bytes_left = total_data_size;
  if (f_lseek(&file, wav_data_offset) != FR_OK) {
    f_close(&file);
    return 4;
  }
  // заполнить оба DMA-буфера
  wav_prime_buffers();
  audio_callbacks_t cb = { .on_buffer_ready = cb_buffer_ready };
  audio_set_callbacks(&cb);
  playing = 1;
  current_position = 0;
  audio_start();
  return 0;
}

//
void wav_process(void) {
  if (stopping && playing) {
    void audio_stop_dma1_stream4();
    f_close(&file);
    playing  = 0;
    stopping = 0;
    eof      = 0;
  }
}

// ФУНКЦИИ УПРАВЛЕНИЯ ВОСПРОИЗВЕДЕНИЕМ
void wav_pause(void) {
  if (!playing) return;
  audio_stop_dma1_stream4();
}

void wav_resume(void) {
  if (!playing) return;
  audio_start();
}

// ОСТАНОВКА ВОСПРОИЗВЕДЕНИЯ
void wav_stop(void) {
  if (!playing) return;
  audio_stop_dma1_stream4(); // Останавливаем DMA и I2S
  f_close(&file);
  playing          = 0;
  eof              = 0;
  stopping         = 0;
  bytes_left       = 0;
  channels         = 0;
  current_position = 0;
  total_data_size  = 0;
}

// === УНИВЕРСАЛЬНАЯ ПЕРЕМОТКА (ДЛЯ ДЛИТЕЛЬНЫХ НАЖАТИЙ) =======================
void wav_seek(uint32_t new_position) {
  if (!playing) return;
  uint32_t block_align = channels * 2;  // 16-bit PCM
  // выравнивание по аудиокадру
  new_position -= new_position % block_align;
  if (new_position >= total_data_size) {
    if (total_data_size > block_align)
      new_position = total_data_size - block_align;
    else
      new_position = 0;
  }
  
  audio_stop_dma1_stream4(); // Остановить передачу

  if (f_lseek(&file, wav_data_offset + new_position) != FR_OK) {
    audio_start();
    return;
  }
  current_position = new_position;
  bytes_left       = total_data_size - current_position;
  eof              = 0;
  stopping         = 0;
  // заново заполнить оба DMA-буфера
  wav_prime_buffers();
  audio_start();
}