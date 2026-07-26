# Рефакторинг: устранение повторов в audio_i2s и wav_player

## 1. audio_i2s.h — добавить дефайны

```c
// Размеры буфера
#define AUDIO_HALF_SIZE  (AUDIO_CHUNK_SIZE * 2)  // half-word в одной половине
#define AUDIO_BUF_SIZE   (AUDIO_CHUNK_SIZE * 4)  // half-word во всём буфере

// Пины I2S2
#define I2S2_WS_PIN  12
#define I2S2_CK_PIN  13
#define I2S2_SD_PIN  15

// Флаги DMA Stream4
#define DMA_STREAM4_ALL_FLAGS  (DMA_HIFCR_CFEIF4 | DMA_HIFCR_CDMEIF4 | \
                                DMA_HIFCR_CTEIF4  | DMA_HIFCR_CHTIF4  | \
                                DMA_HIFCR_CTCIF4)
```

## 2. audio_i2s.c — правки

### 2.1. GPIO — макрос для настройки пина

```c
// Вспомогательный макрос для настройки одного пина на AF
#define GPIO_AF_CONFIG(port, pin, af) \
    do { \
        port->MODER   &= ~(3U << ((pin) * 2)); \
        port->MODER   |=  (2U << ((pin) * 2)); \
        port->OSPEEDR |=  (3U << ((pin) * 2)); \
        port->PUPDR   &= ~(3U << ((pin) * 2)); \
        if ((pin) < 8) { \
            port->AFR[0] &= ~(0xFU << ((pin) * 4)); \
            port->AFR[0] |=  ((af) << ((pin) * 4)); \
        } else { \
            port->AFR[1] &= ~(0xFU << (((pin) - 8) * 4)); \
            port->AFR[1] |=  ((af) << (((pin) - 8) * 4)); \
        } \
    } while (0)
```

Замена строк 60–65:
```c
GPIO_AF_CONFIG(GPIOB, I2S2_WS_PIN, 5);
GPIO_AF_CONFIG(GPIOB, I2S2_CK_PIN, 5);
GPIO_AF_CONFIG(GPIOB, I2S2_SD_PIN, 5);
```

### 2.2. Сброс флагов DMA — унифицировать

Строка 92: `DMA1->HIFCR = 0x3DU;` → `DMA1->HIFCR = DMA_STREAM4_ALL_FLAGS;`

Строки 131–136: заменить на `DMA1->HIFCR = DMA_STREAM4_ALL_FLAGS;`

Строка 145: `DMA1->HIFCR = 0x3DU;` → `DMA1->HIFCR = DMA_STREAM4_ALL_FLAGS;`

### 2.3. Заменить AUDIO_CHUNK_SIZE * 2 / * 4

- Строка 9: `audio_buffer[AUDIO_CHUNK_SIZE * 4]` → `audio_buffer[AUDIO_BUF_SIZE]`
- Строка 18: `&audio_buffer[AUDIO_CHUNK_SIZE * 2]` → `&audio_buffer[AUDIO_HALF_SIZE]`
- Строка 97: `NDTR = AUDIO_CHUNK_SIZE * 4` → `NDTR = AUDIO_BUF_SIZE`

## 3. wav_player.h — добавить дефайны

```c
#define WAV_FRAME_SIZE(ch)  ((ch) * 2)           // байт на один аудиофрейм
#define WAV_HALF_BYTES      (AUDIO_CHUNK_SIZE * 2 * sizeof(uint16_t))  // байт в полу-буфере
```

## 4. wav_player.c — правки

### 4.1. Функция fill_silence()

```c
static void fill_silence(uint16_t* buf) {
    memset(buf, 0, WAV_HALF_BYTES);
    eof      = 1;
    stopping = 1;
}
```

Замена:
- Строки 173–177 → `fill_silence(buf); return;`
- Строки 179–183 → `fill_silence(buf); return;`

### 4.2. Заменить AUDIO_CHUNK_SIZE * 2 * sizeof(uint16_t)

- Строка 174, 180: заменить на `WAV_HALF_BYTES`
- Строка 192: `buf + (br / 2) * 2` → `buf + br` (упрощение: br уже в байтах, uint16_t* арифметика)
- Строка 170: `AUDIO_CHUNK_SIZE * (channels == 1 ? 2 : 4)` → оставить или через `WAV_FRAME_SIZE`

### 4.3. wav_play() — заменить ручную остановку на wav_stop()

Строки 220–224:
```c
if (playing) {
    audio_stop();
    f_close(&file);
    playing = 0;
}
```
Заменить на:
```c
if (playing) {
    wav_stop();
}
```

### 4.4. Вынести сброс состояния в static функцию

```c
static void wav_reset_state(void) {
    playing          = 0;
    eof              = 0;
    stopping         = 0;
    bytes_left       = 0;
    channels         = 0;
    current_position = 0;
    total_data_size  = 0;
}
```

Использовать в `wav_stop()` (строки 293–299) и в `wav_process()` (строки 271–273).