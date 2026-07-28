#include "audio_gen.h"
#include "audio_i2s.h"
#include "common.h"
#include <math.h>

#define SAMPLING_RATE   44100
#define TARGET_FREQ     1050
#define SAMPLES_PER_PERIOD (SAMPLING_RATE / TARGET_FREQ) // Ровно 42

// Состояние генератора
static uint8_t            gen_active    = 0;
static uint8_t            gen_stopping  = 0;
static uint32_t           gen_remaining = 0; // Осталось сэмплов (0 = бесконечно)
static audio_gen_type_t   gen_type      = AUDIO_GEN_SINE;
static int32_t            gen_amplitude = 16384;
static uint32_t           sample_counter = 0;

// Переменные для треугольника
static int32_t triangle_val = 0;
static int32_t triangle_direction = 1; 
static int32_t triangle_step = 0;      

// Функция заполнения DMA-буфера (вызывается из коллбэка)
static void gen_fill_buffer(uint16_t* buf) {
    uint32_t count = AUDIO_CHUNK_SIZE; // 4096 стерео-кадров
    uint32_t limit = (gen_remaining && gen_remaining < count) ? gen_remaining : count;

    for (uint32_t i = 0; i < limit; i++) {
        int32_t sample = 0;

        switch (gen_type) {
            case AUDIO_GEN_SINE: {
                float radians = ((float)sample_counter / (float)SAMPLES_PER_PERIOD) * 2.0f * 3.14159265f;
                sample = (int32_t)(sinf(radians) * (float)gen_amplitude);
                break;
            }

            case AUDIO_GEN_TRIANGLE: {
                triangle_val += (triangle_direction * triangle_step);
                if (triangle_val >= gen_amplitude) {
                    triangle_val = gen_amplitude;
                    triangle_direction = -1; 
                } else if (triangle_val <= -gen_amplitude) {
                    triangle_val = -gen_amplitude;
                    triangle_direction = 1;  
                }
                sample = triangle_val;
                break;
            }

            case AUDIO_GEN_SQUARE: {
                // Возвращаем прошлый проверенный вариант трапеции (3 сэмпла)
                uint32_t half_period = SAMPLES_PER_PERIOD / 2; 
                uint32_t rise_fall_samples = 3; 

                if (sample_counter < (half_period - rise_fall_samples)) {
                    sample = gen_amplitude; 
                } else if (sample_counter < half_period) {
                    uint32_t step = sample_counter - (half_period - rise_fall_samples);
                    sample = gen_amplitude - ((2 * gen_amplitude * (int32_t)step) / (int32_t)rise_fall_samples);
                } else if (sample_counter < (SAMPLES_PER_PERIOD - rise_fall_samples)) {
                    sample = -gen_amplitude; 
                } else {
                    uint32_t step = sample_counter - (SAMPLES_PER_PERIOD - rise_fall_samples);
                    sample = -gen_amplitude + ((2 * gen_amplitude * (int32_t)step) / (int32_t)rise_fall_samples);
                }
                break;
            }

            case AUDIO_GEN_SAWTOOTH: {
                // Возвращаем прошлый проверенный вариант пилы со спадом в 4 сэмпла
                uint32_t ramp_up_samples = SAMPLES_PER_PERIOD - 4; 

                if (sample_counter < ramp_up_samples) {
                    sample = -gen_amplitude + ((2 * gen_amplitude * (int32_t)sample_counter) / (int32_t)(ramp_up_samples - 1));
                } else {
                    uint32_t step = sample_counter - ramp_up_samples;
                    sample = gen_amplitude - ((2 * gen_amplitude * (int32_t)(step + 1)) / 4);
                }
                break;
            }

            default:
                break;
        }

        // Вывод стерео
        uint16_t usample = (uint16_t)sample;
        *buf++ = usample; 
        *buf++ = usample; 

        // Инкремент циклического счетчика
        sample_counter++;
        if (sample_counter >= SAMPLES_PER_PERIOD) {
            sample_counter = 0;
        }
    }

    // Если кастомная длительность закончилась раньше конца буфера — заполняем остаток тишиной
    if (limit < count) {
        uint32_t silent_words = (count - limit) * 2;
        for (uint32_t i = 0; i < silent_words; i++) {
            *buf++ = 0;
        }
        gen_stopping = 1;
    }

    // Декремент счетчика оставшегося времени работы
    if (gen_remaining) {
        gen_remaining -= limit;
        if (gen_remaining == 0) {
            gen_stopping = 1;
        }
    }
}

// Оригинальный коллбэк от аудио-драйвера I2S
static void cb_gen_buffer_ready(uint16_t* buf) {
    if (!gen_active) return;
    gen_fill_buffer(buf);
}

// Первичная накачка буферов
static void gen_prime_buffers(void) {
    gen_fill_buffer(audio_get_buffer1());
    gen_fill_buffer(audio_get_buffer2());
}

// ПУБЛИЧНЫЙ API ЗАПУСКА
void audio_gen_start(audio_gen_type_t type, audio_gen_volume_t volume, uint32_t duration_ms) {
    if (gen_active) {
        audio_gen_stop();
    }

    gen_type = type;
    
    // Установка амплитуды по вашим двум опциям
    if (volume == AUDIO_VOLUME_FULL) {
        gen_amplitude = 32767;
    } else {
        gen_amplitude = 16384;
    }
    
    // Сброс всех внутренних счетчиков
    sample_counter = 0;
    gen_stopping   = 0;
    
    // Инициализация треугольника
    triangle_step = (4 * gen_amplitude) / SAMPLES_PER_PERIOD;
    triangle_val = 0;
    triangle_direction = 1;

    // Расчет длительности в сэмплах
    gen_remaining = duration_ms
        ? (uint32_t)(((uint64_t)duration_ms * (uint64_t)SAMPLING_RATE) / 1000ULL)
        : 0;

    // Накачка и старт периферии
    gen_prime_buffers();
    
    audio_callbacks_t cb = { .on_buffer_ready = cb_gen_buffer_ready };
    audio_set_callbacks(&cb);
    
    gen_active = 1;
    audio_start();
}

void audio_gen_stop(void) {
    if (!gen_active) return;
    audio_stop();
    gen_active    = 0;
    gen_stopping  = 0;
    gen_remaining = 0;
}

uint8_t audio_gen_is_active(void) {
    return gen_active;
}

void audio_gen_process(void) {
    if (gen_stopping && gen_active) {
        audio_gen_stop();
    }
}
