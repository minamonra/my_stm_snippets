#include "audio_gen.h"
#include "audio_i2s.h"
#include <string.h>

// Константы генератора
#define PHASE_MAX   0x10000000U                     // 268435456 — размер фазового аккумулятора
#define PHASE_SHIFT 28                              // сдвиг для выделения фазы

// Состояние генератора
static uint8_t  gen_active    = 0;
static uint8_t  gen_stopping  = 0;
static uint32_t gen_remaining = 0;                  // осталось сэмплов (0 = бесконечно)
static uint32_t gen_phase     = 0;                  // текущая фаза
static uint32_t gen_step      = 0;                  // приращение фазы на сэмпл
static uint16_t gen_amplitude = 0;                  // амплитуда (0..32767)
static audio_gen_type_t gen_type = AUDIO_GEN_SINE;

// Таблица синуса 1/4 периода (256 точек)
// Содержит sin(0..PI/2) * 32767
static const uint16_t sin_quarter[256] = {
      0,   804,  1608,  2412,  3216,  4019,  4821,  5623,
   6423,  7223,  8022,  8819,  9615, 10410, 11203, 11994,
  12784, 13571, 14356, 15139, 15919, 16697, 17471, 18243,
  19012, 19777, 20539, 21297, 22051, 22801, 23547, 24289,
  25026, 25759, 26487, 27210, 27928, 28640, 29347, 30048,
  30744, 31433, 32117, 32795, 33466, 34131, 34790, 35442,
  36087, 36726, 37357, 37982, 38599, 39209, 39812, 40407,
  40995, 41575, 42147, 42711, 43267, 43815, 44355, 44887,
  45410, 45925, 46431, 46929, 47418, 47898, 48370, 48833,
  49287, 49732, 50168, 50595, 51013, 51422, 51822, 52213,
  52594, 52967, 53330, 53684, 54029, 54364, 54691, 55008,
  55316, 55615, 55904, 56184, 56455, 56717, 56969, 57213,
  57447, 57672, 57888, 58095, 58293, 58482, 58662, 58833,
  58995, 59148, 59292, 59428, 59554, 59672, 59781, 59882,
  59974, 60057, 60132, 60198, 60256, 60305, 60346, 60378,
  60402, 60418, 60425, 60424, 60415, 60397, 60372, 60338,
  60296, 60246, 60188, 60122, 60048, 59966, 59876, 59779,
  59674, 59561, 59441, 59313, 59178, 59035, 58885, 58728,
  58563, 58392, 58213, 58027, 57835, 57635, 57429, 57216,
  56996, 56770, 56537, 56298, 56053, 55801, 55543, 55279,
  55009, 54733, 54452, 54164, 53871, 53573, 53269, 52960,
  52646, 52327, 52003, 51674, 51340, 51002, 50659, 50312,
  49960, 49605, 49245, 48882, 48514, 48143, 47769, 47391,
  47010, 46625, 46238, 45847, 45454, 45058, 44659, 44258,
  43854, 43448, 43040, 42630, 42218, 41804, 41388, 40971,
  40552, 40132, 39711, 39288, 38865, 38440, 38015, 37589,
  37162, 36735, 36307, 35879, 35451, 35022, 34594, 34165,
  33737, 33309, 32881, 32454, 32027, 31601, 31175, 30750,
  30326, 29903, 29481, 29060, 28640, 28222, 27805, 27389,
  26975, 26563, 26152, 25743, 25336, 24931, 24528, 24127,
  23728, 23332, 22938, 22546, 22157, 21770, 21386, 21005,
  20626, 20251, 19878, 19509, 19142, 18779, 18419, 18062,
};

// Получить значение синуса из таблицы (фаза 0..PHASE_MAX-1)
static int16_t sine_sample(uint32_t phase) {
    uint32_t idx = phase >> (PHASE_SHIFT - 8);      // 8 бит на квадрант
    uint32_t quad = (idx >> 8) & 3;                 // 0..3
    uint8_t  pos  = idx & 0xFF;                     // 0..255

    uint16_t val;
    switch (quad) {
        case 0: val =  sin_quarter[pos];         break;  // 0..PI/2
        case 1: val =  sin_quarter[255 - pos];   break;  // PI/2..PI
        case 2: val =  sin_quarter[pos];         break;  // PI..3PI/2 (инвертируем)
        default:val =  sin_quarter[255 - pos];   break;  // 3PI/2..2PI (инвертируем)
    }
    return (quad & 2) ? -(int16_t)val : (int16_t)val;
}

// Заполнить DMA-буфер сэмплами
static void gen_fill_buffer(uint16_t* buf) {
    uint32_t count = AUDIO_CHUNK_SIZE;               // число стерео-фреймов
    uint32_t limit = (gen_remaining && gen_remaining < count) ? gen_remaining : count;

    for (uint32_t i = 0; i < limit; i++) {
        int16_t sample;

        switch (gen_type) {
            case AUDIO_GEN_SINE:
                sample = sine_sample(gen_phase);
                break;

            case AUDIO_GEN_TRIANGLE: {
                // Треугольник: фаза 0..PHASE_MAX/2 = ramp up, PHASE_MAX/2..PHASE_MAX = ramp down
                uint32_t half = PHASE_MAX / 2;
                uint32_t p = gen_phase % PHASE_MAX;
                sample = (int16_t)((p < half)
                    ? ((int32_t)p * 2 * gen_amplitude / half - gen_amplitude)
                    : ((int32_t)(PHASE_MAX - p) * 2 * gen_amplitude / half - gen_amplitude));
                break;
            }

            case AUDIO_GEN_SQUARE: {
                // Меандр: фаза 0..PHASE_MAX/2 = +A, PHASE_MAX/2..PHASE_MAX = -A
                sample = (gen_phase < (PHASE_MAX / 2)) ? (int16_t)gen_amplitude : -(int16_t)gen_amplitude;
                break;
            }

            default:
                sample = 0;
                break;
        }

        // Стерео: L = R = sample (с приведением к uint16_t для I2S)
        uint16_t usample = (uint16_t)sample;
        *buf++ = usample;
        *buf++ = usample;

        gen_phase += gen_step;
    }

    // Если остаток меньше полного чанка — заполнить тишиной
    if (limit < count) {
        memset(buf, 0, (count - limit) * 2 * sizeof(uint16_t));
        gen_stopping = 1;
    }

    if (gen_remaining) {
        gen_remaining -= limit;
        if (gen_remaining == 0) gen_stopping = 1;
    }
}

// Коллбэк от audio_i2s
static void cb_gen_buffer_ready(uint16_t* buf) {
    if (!gen_active) return;
    gen_fill_buffer(buf);
}

// Заполнить оба DMA-буфера при старте
static void gen_prime_buffers(void) {
    gen_fill_buffer(audio_get_buffer1());
    gen_fill_buffer(audio_get_buffer2());
}

// ПУБЛИЧНЫЙ API
void audio_gen_start(audio_gen_type_t type, uint32_t freq_hz, uint16_t amplitude, uint32_t duration_ms) {
    // Остановить текущую генерацию, если активна
    if (gen_active) {
        audio_gen_stop();
    }

    // Проверка параметров
    if (freq_hz == 0 || freq_hz > 22050) freq_hz = 440;   // A4 по умолчанию
    if (amplitude == 0 || amplitude > 32767) amplitude = 8000;

    gen_type      = type;
    gen_amplitude = amplitude;
    gen_phase     = 0;
    gen_stopping  = 0;

    // Шаг фазового аккумулятора: step = freq * PHASE_MAX / sample_rate
    // sample_rate = 44100 (из audio_i2s)
    // Используем uint64_t для промежуточного результата
    gen_step = (uint32_t)((uint64_t)freq_hz * PHASE_MAX / 44100);

    // Длительность в сэмплах (стерео)
    gen_remaining = duration_ms
        ? (uint32_t)((uint64_t)duration_ms * 44100 / 1000)
        : 0;

    // Заполнить буферы и запустить
    gen_prime_buffers();
    audio_callbacks_t cb = { .on_buffer_ready = cb_gen_buffer_ready };
    audio_set_callbacks(&cb);
    gen_active = 1;
    audio_start();
}

void audio_gen_stop(void) {
    if (!gen_active) return;
    audio_stop();
    gen_active   = 0;
    gen_stopping = 0;
    gen_remaining = 0;
}

uint8_t audio_gen_is_active(void) {
    return gen_active;
}

// Обработка остановки по окончании — вызывать из main loop
void audio_gen_process(void) {
    if (gen_stopping && gen_active) {
        audio_gen_stop();
    }
}