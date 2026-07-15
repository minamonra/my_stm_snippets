#include "audio_i2s.h"
//#define AUDIO_CHUNK_SIZE   2048
// Единый ЦИКЛИЧЕСКИЙ буфер на ДВА полу-чанка (по аналогии с HAL: audio_buffer[AUDIO_CHUNK_SIZE*4]
// half-word, первая половина играет пока заполняется вторая, и наоборот).
// Формат на каждый стерео-фрейм: [L, R] (16 бит в 32-бит слот, HAL I2S Philips, DataFormat=16b в Frame=32b).
// ДАННЫЕ ЗНАКОВЫЕ (int16_t), упакованы подряд без заполнителя.
// STM32 в режиме Philips с CHLEN=1 и DATLEN=00 автоматически размещает 16 бит
// в старших 16 битах 32-битного слота, младшие 16 бит = 0x0000.
__attribute__((aligned(4))) static uint16_t audio_buffer[AUDIO_CHUNK_SIZE * 4];

static audio_callbacks_t callbacks = {0};

uint16_t* audio_get_buffer1(void) {
    return &audio_buffer[0];                         // Первая половина
}

uint16_t* audio_get_buffer2(void) {
    return &audio_buffer[AUDIO_CHUNK_SIZE * 2];       // Вторая половина
}

void audio_set_callbacks(audio_callbacks_t* cb) {
    if (cb) {
        callbacks.on_buffer_ready = cb->on_buffer_ready;
    }
}

void audio_init(void) {
    // 1. PLLI2S
    RCC->CR &= ~RCC_CR_PLLI2SON;
    // M=8, N=192, R=2 -> 96 МГц (на самом деле 98.304 МГц для точного аудио)
    // При делителе 17 даёт Fs = 98.304 / (32 * 17) ≈ 48 кГц для 32-бит фрейма
    // Корректная формула для 48 кГц: PLLI2S = 48кГц * 256 * 2 = 98.304 МГц
    //RCC->PLLI2SCFGR = (8U << 0) | (192U << 6) | (2U << 28); // N=192, R=2 -> 98.304 МГц

    // PLLI2S: HSE=8 / M=8 * N=192 / R=2 = 96 МГц
    // I2SPR=17 (I2SDIV=17, ODD=0) → делитель = 128 * 17 = 2176
    // Fs = 96 000 000 / 2176 = 44 117.6 Гц ≈ 44.1 кГц
    // BCK = 96 000 000 / (2 * 17) = 2 823 529 Гц ≈ 2.82 МГц
    RCC->PLLI2SCFGR = (8U << RCC_PLLI2SCFGR_PLLI2SM_Pos)
                | (192U << RCC_PLLI2SCFGR_PLLI2SN_Pos)
                | (2U << RCC_PLLI2SCFGR_PLLI2SR_Pos);

    RCC->CR |= RCC_CR_PLLI2SON;
    while (!(RCC->CR & RCC_CR_PLLI2SRDY));

    RCC->DCKCFGR &= ~(3U << 25); // I2S1SRC (биты [26:25] для I2S2/I2S3 на APB1) = 00 = PLLI2S

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    __DSB();

    // 2. Аппаратный сброс SPI2
    SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
    SPI2->CR1 = 0;
    RCC->APB1RSTR |= RCC_APB1RSTR_SPI2RST;
    __NOP(); __NOP(); __NOP();
    RCC->APB1RSTR &= ~RCC_APB1RSTR_SPI2RST;

    // 3. GPIO PB12, PB13, PB15 -> AF5 (I2S2)
    GPIOB->MODER   &= ~((3U << (12 * 2)) | (3U << (13 * 2)) | (3U << (15 * 2)));
    GPIOB->MODER   |=  ((2U << (12 * 2)) | (2U << (13 * 2)) | (2U << (15 * 2)));
    GPIOB->OSPEEDR |=  ((3U << (12 * 2)) | (3U << (13 * 2)) | (3U << (15 * 2)));
    GPIOB->PUPDR   &= ~((3U << (12 * 2)) | (3U << (13 * 2)) | (3U << (15 * 2)));
    GPIOB->AFR[1]  &= ~((0xFU << ((12U - 8U) * 4)) | (0xFU << ((13U - 8U) * 4)) | (0xFU << ((15U - 8U) * 4)));
    GPIOB->AFR[1]  |=  ((0x5U << ((12U - 8U) * 4)) | (0x5U << ((13U - 8U) * 4)) | (0x5U << ((15U - 8U) * 4)));

    // 4. I2S Philips Master Transmit, 16 бит данных в 32-битном фрейме
    // CHLEN=1 (32 бита на канал), DATLEN=00 (16 бит данных в 32-битном слоте)
    // В режиме Philips STM32 автоматически:
    //   - Сдвигает данные на 1 бит после фронта WS
    //   - Размещает 16-битные данные в СТАРШИХ 16 битах 32-битного слота
    //   - Младшие 16 бит заполняются нулями (0x0000)
    // Это ТОЧНО соответствует формату PCM5102: 16-bit I2S Philips в 32-бит фрейме
    SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD                      // I2S Mode
                  | (2U << SPI_I2SCFGR_I2SCFG_Pos)          // Master Transmit
                  | SPI_I2SCFGR_CHLEN                        // CHLEN=1: 32-битный фрейм
                  | (0U << SPI_I2SCFGR_DATLEN_Pos);         // DATLEN=00: 16 бит данных (явно)

    SPI2->I2SPR = 17;       // I2SDIV=17, ODD=0; Fs = PLLI2S / (128*17) ≈ 44.1 кГц
    // TXDMAEN включается ПОСЛЕДНИМ шагом в audio_start()
    // (точный порядок официального HAL: DMA_EN -> I2SE -> TXDMAEN)

    // 5. DMA: обычный CIRCULAR режим (без DBM), PSIZE=16bit/MSIZE=16bit.
    // Прямой аналог настройки CubeMX "Mem to Periph, Mode Circular, no FIFO,
    // Data width: half word / half word".
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    __NOP();

    DMA1_Stream4->CR = 0;
    while (DMA1_Stream4->CR & DMA_SxCR_EN);

    DMA1->HIFCR = 0x3DU;                                 // Сброс всех флагов Stream4
    DMA1_Stream4->FCR = 0x00000000;                      // Direct mode, FIFO выключен

    DMA1_Stream4->PAR  = (uint32_t)&(SPI2->DR);           // Адрес периферии — регистр данных SPI2
    DMA1_Stream4->M0AR = (uint32_t)audio_buffer;          // Адрес памяти — полный буфер целиком
    DMA1_Stream4->NDTR = AUDIO_CHUNK_SIZE * 4;            // Длина в half-word (PSIZE=16bit)

    DMA1_Stream4->CR = (0U << DMA_SxCR_CHSEL_Pos)        // Channel 0 = SPI2_TX (см. RM0090 Table 42)
                     | (3U << DMA_SxCR_PL_Pos)            // Приоритет Very High
                     | (1U << DMA_SxCR_DIR_Pos)           // Memory-to-Peripheral
                     | DMA_SxCR_MINC                      // Инкремент указателя памяти
                     | (1U << DMA_SxCR_PSIZE_Pos)         // 16-bit периферия (DR реально 16 бит)
                     | (1U << DMA_SxCR_MSIZE_Pos)         // 16-bit память
                     | DMA_SxCR_CIRC;                     // Циклический режим на ВСЁМ буфере

    DMA1_Stream4->CR |= DMA_SxCR_TCIE | DMA_SxCR_HTIE;   // Прерывания и на половину, и на полный проход

    NVIC_SetPriority(DMA1_Stream4_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Stream4_IRQn);
}

void audio_start(void) {
    // ИСПРАВЛЕНО: порядок включения теперь точно соответствует официальному
    // HAL_I2S_Transmit_DMA() (stm32f4xx_hal_i2s.c): сначала включается DMA-стрим,
    // потом I2SE, и ТОЛЬКО ПОСЛЕДНИМ шагом — TXDMAEN в SPI_CR2.
    // Ранее TXDMAEN включался в Audio_Init() заранее, до старта DMA и I2SE -
    // это давало рассинхронизацию первого запроса данных относительно
    // реального запуска передачи, что, по всей видимости, и вызывало
    // постоянный, устойчивый артефакт на аналоговом выходе независимо
    // от архитектуры DMA (DBM/Circular) и формата данных.
    DMA1_Stream4->CR |= DMA_SxCR_EN;                    // 1. Запускаем DMA-стрим
    SPI2->I2SCFGR |= SPI_I2SCFGR_I2SE;                  // 2. Включаем I2S
    SPI2->CR2 |= SPI_CR2_TXDMAEN;                       // 3. Разрешаем запросы DMA от передатчика (ТОЛЬКО ПОСЛЕДНИМ)
}

// HTIF срабатывает в середине буфера (DMA играет вторую половину -> первая свободна).
// TCIF срабатывает в конце буфера (DMA зациклился на первую половину -> вторая свободна).
// Прямой аналог HAL_I2S_TxHalfCpltCallback / HAL_I2S_TxCpltCallback.
void DMA1_Stream4_IRQHandler(void) {
    uint32_t hisr = DMA1->HISR;
    DMA1->HIFCR = 0x3DU;

    if (hisr & DMA_HISR_HTIF4) {
        if (callbacks.on_buffer_ready) {
            callbacks.on_buffer_ready(audio_get_buffer1());
        }
    }

    if (hisr & DMA_HISR_TCIF4) {
        if (callbacks.on_buffer_ready) {
            callbacks.on_buffer_ready(audio_get_buffer2());
        }
    }
}