#include "wav_player.h"
#include "audio_i2s.h"
#include "uart.h"
#include "ff.h"
#include <string.h>

extern volatile uint32_t ttms;

// Структура WAV-заголовка (44 байта)
typedef struct {
    char     chunk_id[4];       // "RIFF"
    uint32_t chunk_size;        // Размер файла - 8
    char     format[4];         // "WAVE"
    char     subchunk1_id[4];   // "fmt "
    uint32_t subchunk1_size;    // 16 для PCM
    uint16_t audio_format;      // 1 = PCM
    uint16_t num_channels;      // 1 = моно, 2 = стерео
    uint32_t sample_rate;       // Частота дискретизации (Гц)
    uint32_t byte_rate;         // sample_rate * num_channels * bits_per_sample/8
    uint16_t block_align;       // num_channels * bits_per_sample/8
    uint16_t bits_per_sample;   // 8, 16 и т.д.
    char     subchunk2_id[4];   // "data"
    uint32_t subchunk2_size;    // Размер аудиоданных в байтах
} __attribute__((packed)) wav_header_t;

// Глобальные переменные файловой системы и состояния воспроизведения
static FATFS fs;
static FIL file;
static uint8_t playing = 0;
static uint8_t eof = 0;
static uint8_t stopping = 0;
static uint16_t channels = 0;
static uint32_t bytes_left = 0;

// Параметры текущего файла для перемотки
static uint32_t sample_rate = 44100;
static uint32_t current_position = 0;
static uint32_t total_data_size = 0;

static uint16_t buf_raw[AUDIO_CHUNK_SIZE * 2] __attribute__((aligned(4)));

volatile uint8_t need_fill = 0;
static uint16_t* fill_buf = NULL;



// ============================================================================
// === ВЫБОР РЕЖИМА РАСПАКОВКИ ================================================
// ============================================================================

// Раскомментируйте нужный режим:
#define WAV_MODE_FAST_HALF    // Быстрый: деление на 2 (1 такт)
// #define WAV_MODE_AMPLITUDE // Точный: умножение на коэффициент
// #define WAV_MODE_PASSTHROUGH // Прямой: без обработки

// ============================================================================
// === БЛОК РАСПАКОВКИ ========================================================
// ============================================================================

#if defined(WAV_MODE_FAST_HALF)

// Быстрый режим — деление на 2 (один такт ASR)
static void unpack_mono(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t samples_count) {
    int16_t* p_src = (int16_t*)src_ptr;
    uint16_t* p_dst = dest_ptr;
    for (uint32_t i = 0; i < samples_count; i++) {
        int16_t sample = (*p_src++) >> 1;
        *p_dst++ = (uint16_t)sample;
        *p_dst++ = (uint16_t)sample;
    }
}

static void unpack_stereo(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t bytes_read) {
    int16_t* p_src = (int16_t*)src_ptr;
    uint16_t* p_dst = dest_ptr;
    uint32_t total_samples = bytes_read / 2;
    for (uint32_t i = 0; i < total_samples; i++) {
        *p_dst++ = (uint16_t)((*p_src++) >> 1);
    }
}

#elif defined(WAV_MODE_AMPLITUDE)

// Точный режим — умножение на WAV_AMPLITUDE
#define WAV_AMPLITUDE 200

static void unpack_mono(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t samples_count) {
    int16_t* p_src = (int16_t*)src_ptr;
    uint16_t* p_dst = dest_ptr;
    for (uint32_t i = 0; i < samples_count; i++) {
        int32_t raw_sample = *p_src++;
        int16_t sample = (int16_t)((raw_sample * WAV_AMPLITUDE) >> 8);
        *p_dst++ = (uint16_t)sample;
        *p_dst++ = (uint16_t)sample;
    }
}

static void unpack_stereo(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t bytes_read) {
    int16_t* p_src = (int16_t*)src_ptr;
    uint16_t* p_dst = dest_ptr;
    uint32_t total_samples = bytes_read / 2;
    for (uint32_t i = 0; i < total_samples; i++) {
        int32_t raw_sample = *p_src++;
        *p_dst++ = (uint16_t)((raw_sample * WAV_AMPLITUDE) >> 8);
    }
}

#elif defined(WAV_MODE_PASSTHROUGH)

// Режим без обработки (100% громкость)
static void unpack_mono(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t samples_count) {
    uint16_t* p_src = src_ptr;
    uint16_t* p_dst = dest_ptr;
    for (uint32_t i = 0; i < samples_count; i++) {
        uint16_t sample = *p_src++;
        *p_dst++ = sample;
        *p_dst++ = sample;
    }
}

static void unpack_stereo(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t bytes_read) {
    memcpy(dest_ptr, src_ptr, bytes_read);
}

#else

// По умолчанию — быстрый режим
static void unpack_mono(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t samples_count) {
    int16_t* p_src = (int16_t*)src_ptr;
    uint16_t* p_dst = dest_ptr;
    for (uint32_t i = 0; i < samples_count; i++) {
        int16_t sample = (*p_src++) >> 1;
        *p_dst++ = (uint16_t)sample;
        *p_dst++ = (uint16_t)sample;
    }
}

static void unpack_stereo(uint16_t* dest_ptr, uint16_t* src_ptr, uint32_t bytes_read) {
    int16_t* p_src = (int16_t*)src_ptr;
    uint16_t* p_dst = dest_ptr;
    uint32_t total_samples = bytes_read / 2;
    for (uint32_t i = 0; i < total_samples; i++) {
        *p_dst++ = (uint16_t)((*p_src++) >> 1);
    }
}

#endif







// ============================================================================
// === КОЛЛБЭК ГОТОВНОСТИ БУФЕРА ==============================================
// ============================================================================

static void cb_buffer_ready(uint16_t* buf) {
    if (playing && !eof) {
        fill_buf = buf;
        need_fill = 1;
    } else if (playing && eof) {
        memset(buf, 0, AUDIO_CHUNK_SIZE * 2 * sizeof(uint16_t));
        stopping = 1;
        need_fill = 1;
        fill_buf = buf;
    }
}

// ============================================================================
// === ПУБЛИЧНЫЙ ИНТЕРФЕЙС ====================================================
// ============================================================================

uint8_t wav_play(const char* filename) {
    wav_header_t hdr;
    UINT br;

    if (playing) {
        SPI2->CR2 &= ~SPI_CR2_TXDMAEN;
        DMA1_Stream4->CR &= ~DMA_SxCR_EN;
        while (DMA1_Stream4->CR & DMA_SxCR_EN) __NOP();
        DMA1->HIFCR = 0x3DU; // Очищаем зависшие флаги HT/TC
        SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
        f_close(&file);
        playing = 0;
    }

    eof = 0;
    stopping = 0;
    need_fill = 0;
    bytes_left = 0;
    channels = 0;
    current_position = 0;
    total_data_size = 0;

    static uint8_t mounted = 0;
    if (!mounted) {
        if (f_mount(&fs, "0:", 0) != FR_OK) return 1;
        mounted = 1;
    }

    if (f_open(&file, filename, FA_READ) != FR_OK) return 2;

    f_read(&file, &hdr, sizeof(hdr), &br);
    if (br < sizeof(hdr) || memcmp(hdr.chunk_id, "RIFF", 4) != 0) {
        f_close(&file);
        return 3;
    }

    channels = hdr.num_channels;
    sample_rate = hdr.sample_rate;

    uint32_t real_data_size = f_size(&file) - 44;
    if (hdr.subchunk2_size > 0 && hdr.subchunk2_size <= real_data_size) {
        bytes_left = hdr.subchunk2_size;
    } else {
        bytes_left = real_data_size;
    }

    total_data_size = bytes_left;
    current_position = 0;
    f_lseek(&file, 44);

    uint32_t chunk_bytes = AUDIO_CHUNK_SIZE * (channels == 1 ? 2 : 4);

    uint32_t n = (chunk_bytes > bytes_left) ? bytes_left : chunk_bytes;
    f_read(&file, buf_raw, n, &br);
    bytes_left -= br;
    current_position += br;
    if (channels == 1) unpack_mono(audio_get_buffer1(), buf_raw, br / 2);
    else unpack_stereo(audio_get_buffer1(), buf_raw, br);

    if (br < chunk_bytes) {
        uint16_t* remaining = audio_get_buffer1() + (channels == 1 ? (br / 2) * 2 : br / 2);
        memset(remaining, 0, (chunk_bytes - br) / 2 * (channels == 1 ? 2 : 1) * sizeof(uint16_t));
    }

    n = (chunk_bytes > bytes_left) ? bytes_left : chunk_bytes;
    if (n > 0) {
        f_read(&file, buf_raw, n, &br);
        bytes_left -= br;
        current_position += br;
        if (channels == 1) unpack_mono(audio_get_buffer2(), buf_raw, br / 2);
        else unpack_stereo(audio_get_buffer2(), buf_raw, br);

        if (br < chunk_bytes) {
            uint16_t* remaining = audio_get_buffer2() + (channels == 1 ? (br / 2) * 2 : br / 2);
            memset(remaining, 0, (chunk_bytes - br) / 2 * (channels == 1 ? 2 : 1) * sizeof(uint16_t));
        }

        if (bytes_left == 0) eof = 1;
    } else {
        memset(audio_get_buffer2(), 0, AUDIO_CHUNK_SIZE * 2 * sizeof(uint16_t));
        eof = 1;
    }

    audio_callbacks_t cb = {cb_buffer_ready};
    audio_set_callbacks(&cb);

    need_fill = 0;
    playing = 1;
    audio_start();
    return 0;
}

void wav_process(void) {
    if (stopping && playing) {
        SPI2->CR2 &= ~SPI_CR2_TXDMAEN;
        DMA1_Stream4->CR &= ~DMA_SxCR_EN;
        while (DMA1_Stream4->CR & DMA_SxCR_EN) __NOP();
        DMA1->HIFCR = 0x3DU; // Очищаем зависшие флаги HT/TC
        SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
        f_close(&file);
        playing = 0;
        stopping = 0;
        eof = 0;
        return;
    }

    if (!playing || !need_fill || eof) {
        return;
    }

    need_fill = 0;

    uint32_t chunk_bytes = AUDIO_CHUNK_SIZE * (channels == 1 ? 2 : 4);
    uint32_t n = (chunk_bytes > bytes_left) ? bytes_left : chunk_bytes;

    if (n > 0) {
        UINT br = 0;
        FRESULT res = f_read(&file, buf_raw, n, &br);
        if (res == FR_OK && br > 0) {
            bytes_left -= br;
            current_position += br;

            if (channels == 1) {
                unpack_mono(fill_buf, buf_raw, br / 2);
                if (br < n) {
                    uint16_t* remaining = fill_buf + (br / 2) * 2;
                    memset(remaining, 0, (n - br) / 2 * 2 * sizeof(uint16_t));
                }
            } else {
                unpack_stereo(fill_buf, buf_raw, br);
                if (br < n) {
                    uint16_t* remaining = fill_buf + br / 2;
                    memset(remaining, 0, (n - br) / 2 * sizeof(uint16_t));
                }
            }

            if (bytes_left == 0) {
                eof = 1;
                f_close(&file);
            }
            return;
        }
    }

    eof = 1;
    f_close(&file);
}

uint8_t wav_is_playing(void) {
    return playing;
}

// ============================================================================
// === ФУНКЦИИ УПРАВЛЕНИЯ ВОСПРОИЗВЕДЕНИЕМ ====================================
// ============================================================================

void wav_pause(void) {
    if (!playing) return;

    SPI2->CR2 &= ~SPI_CR2_TXDMAEN;
    DMA1_Stream4->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream4->CR & DMA_SxCR_EN) __NOP();
    DMA1->HIFCR = 0x3DU; // Очищаем зависшие флаги HT/TC, чтобы избежать ложного IRQ при следующем audio_start()
    SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
}

void wav_resume(void) {
    if (!playing) return;
    audio_start();
}

void wav_seek_forward(uint32_t seconds) {
    if (!playing) {
        uart1_write_str("SEEK: not playing!\r\n");
        return;
    }

    uint32_t bytes_per_second = sample_rate * channels * 2;
    uint32_t bytes_to_skip = seconds * bytes_per_second;

    uart1_write_str("SEEK: cur=");
    uart1_write_num(current_position);
    uart1_write_str(" total=");
    uart1_write_num(total_data_size);
    uart1_write_str(" skip=");
    uart1_write_num(bytes_to_skip);
    uart1_write_str("\r\n");

    if (current_position + bytes_to_skip >= total_data_size) {
        bytes_to_skip = total_data_size - current_position;
        if (bytes_to_skip < bytes_per_second / 2) {
            uart1_write_str("SEEK: too close to end\r\n");
            return;
        }
    }

    FSIZE_t new_pos = 44 + current_position + bytes_to_skip;

    SPI2->CR2 &= ~SPI_CR2_TXDMAEN;
    DMA1_Stream4->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream4->CR & DMA_SxCR_EN) __NOP();
    DMA1->HIFCR = 0x3DU; // Очищаем зависшие флаги HT/TC, чтобы избежать ложного IRQ при следующем audio_start()

    if (f_lseek(&file, new_pos) != FR_OK) {
        uart1_write_str("SEEK: f_lseek failed!\r\n");
        audio_start();
        return;
    }

    current_position += bytes_to_skip;
    bytes_left = total_data_size - current_position;
    eof = 0;
    stopping = 0;

    uint32_t chunk_bytes = AUDIO_CHUNK_SIZE * (channels == 1 ? 2 : 4);
    UINT br;

    uint32_t n = (chunk_bytes > bytes_left) ? bytes_left : chunk_bytes;
    if (n > 0) {
        f_read(&file, buf_raw, n, &br);
        bytes_left -= br;
        current_position += br;
        if (channels == 1) unpack_mono(audio_get_buffer1(), buf_raw, br / 2);
        else unpack_stereo(audio_get_buffer1(), buf_raw, br);

        if (br < chunk_bytes) {
            uint16_t* remaining = audio_get_buffer1() + (channels == 1 ? (br / 2) * 2 : br / 2);
            memset(remaining, 0, (chunk_bytes - br) / 2 * (channels == 1 ? 2 : 1) * sizeof(uint16_t));
        }
    }

    n = (chunk_bytes > bytes_left) ? bytes_left : chunk_bytes;
    if (n > 0) {
        f_read(&file, buf_raw, n, &br);
        bytes_left -= br;
        current_position += br;
        if (channels == 1) unpack_mono(audio_get_buffer2(), buf_raw, br / 2);
        else unpack_stereo(audio_get_buffer2(), buf_raw, br);

        if (br < chunk_bytes) {
            uint16_t* remaining = audio_get_buffer2() + (channels == 1 ? (br / 2) * 2 : br / 2);
            memset(remaining, 0, (chunk_bytes - br) / 2 * (channels == 1 ? 2 : 1) * sizeof(uint16_t));
        }

        if (bytes_left == 0) eof = 1;
    } else {
        memset(audio_get_buffer2(), 0, AUDIO_CHUNK_SIZE * 2 * sizeof(uint16_t));
        eof = 1;
    }

    need_fill = 0;
    audio_start();

    uart1_write_str("SEEK: OK, new pos=");
    uart1_write_num(current_position);
    uart1_write_str("/");
    uart1_write_num(total_data_size);
    uart1_write_str("\r\n");
}

void wav_seek_backward(uint32_t seconds) {
    if (!playing) return;

    uint32_t bytes_per_second = sample_rate * channels * 2;
    uint32_t bytes_to_rewind = seconds * bytes_per_second;

    if (bytes_to_rewind > current_position) {
        bytes_to_rewind = current_position;
    }

    FSIZE_t new_pos = 44 + current_position - bytes_to_rewind;

    uint8_t was_playing = playing;
    if (was_playing) {
        SPI2->CR2 &= ~SPI_CR2_TXDMAEN;
        DMA1_Stream4->CR &= ~DMA_SxCR_EN;
        while (DMA1_Stream4->CR & DMA_SxCR_EN) __NOP();
        DMA1->HIFCR = 0x3DU; // Очищаем зависшие флаги HT/TC
    }

    if (f_lseek(&file, new_pos) != FR_OK) {
        if (was_playing) audio_start();
        return;
    }

    current_position -= bytes_to_rewind;
    bytes_left = total_data_size - current_position;

    eof = 0;
    stopping = 0;

    uint32_t chunk_bytes = AUDIO_CHUNK_SIZE * (channels == 1 ? 2 : 4);
    UINT br;

    uint32_t n = (chunk_bytes > bytes_left) ? bytes_left : chunk_bytes;
    if (n > 0) {
        f_read(&file, buf_raw, n, &br);
        bytes_left -= br;
        current_position += br;
        if (channels == 1) unpack_mono(audio_get_buffer1(), buf_raw, br / 2);
        else unpack_stereo(audio_get_buffer1(), buf_raw, br);

        if (br < chunk_bytes) {
            uint16_t* remaining = audio_get_buffer1() + (channels == 1 ? (br / 2) * 2 : br / 2);
            memset(remaining, 0, (chunk_bytes - br) / 2 * (channels == 1 ? 2 : 1) * sizeof(uint16_t));
        }
    }

    n = (chunk_bytes > bytes_left) ? bytes_left : chunk_bytes;
    if (n > 0) {
        f_read(&file, buf_raw, n, &br);
        bytes_left -= br;
        current_position += br;
        if (channels == 1) unpack_mono(audio_get_buffer2(), buf_raw, br / 2);
        else unpack_stereo(audio_get_buffer2(), buf_raw, br);

        if (br < chunk_bytes) {
            uint16_t* remaining = audio_get_buffer2() + (channels == 1 ? (br / 2) * 2 : br / 2);
            memset(remaining, 0, (chunk_bytes - br) / 2 * (channels == 1 ? 2 : 1) * sizeof(uint16_t));
        }

        if (bytes_left == 0) eof = 1;
    } else {
        memset(audio_get_buffer2(), 0, AUDIO_CHUNK_SIZE * 2 * sizeof(uint16_t));
        eof = 1;
    }

    need_fill = 0;
    if (was_playing) {
        audio_start();
    }

    uart1_write_str("Seek -");
    uart1_write_num(seconds);
    uart1_write_str("s, pos: ");
    uart1_write_num(current_position);
    uart1_write_str("/");
    uart1_write_num(total_data_size);
    uart1_write_str(" (");
    uart1_write_num((uint32_t)((uint64_t)current_position * 100 / total_data_size));
    uart1_write_str("%)\r\n");
}

// ============================================================================
// === ФУНКЦИИ ПОЛУЧЕНИЯ ИНФОРМАЦИИ ==========================================
// ============================================================================

uint32_t wav_get_position(void) {
    return current_position;
}

uint32_t wav_get_total_size(void) {
    return total_data_size;
}

uint32_t wav_get_sample_rate(void) {
    return sample_rate;
}

uint16_t wav_get_channels(void) {
    return channels;
}

void wav_get_file_info(uint32_t* pos, uint32_t* total, uint32_t* sr, uint16_t* ch) {
    if (pos)   *pos   = current_position;
    if (total) *total = total_data_size;
    if (sr)    *sr    = sample_rate;
    if (ch)    *ch    = channels;
}