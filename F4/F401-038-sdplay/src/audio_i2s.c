#include "audio_i2s.h"
#include "stm32f4xx.h"
#include <string.h>

// Создаем два независимых буфера в ОЗУ, выровнены по 4 байтам

__attribute__((aligned(4))) uint16_t audio_buff1[AUDIO_CHUNK_SIZE * 2];
__attribute__((aligned(4))) uint16_t audio_buff2[AUDIO_CHUNK_SIZE * 2];

// Указатели плеера
uint16_t* signal_play_buff = audio_buff1;
uint16_t* signal_read_buff = audio_buff2;

volatile uint8_t read_next_chunk = 0;
volatile uint8_t end_of_file_reached = 0;

void Audio_I2S_Init_Advanced(uint32_t sample_rate, uint16_t channels) {
    (void)sample_rate;
    (void)channels;
    
    read_next_chunk = 0;
    end_of_file_reached = 0;
    memset(audio_buff1, 0, sizeof(audio_buff1));
    memset(audio_buff2, 0, sizeof(audio_buff2));

    // 1. Полностью гасим периферию
    SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
    SPI2->CR1 = 0;

    // 2. Сброс тактового домена SPI2/I2S2
    RCC->APB1RSTR |= RCC_APB1RSTR_SPI2RST;  
    __NOP(); __NOP(); __NOP();
    RCC->APB1RSTR &= ~RCC_APB1RSTR_SPI2RST; 

    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    __NOP(); __NOP();

    // SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD | (2 << SPI_I2SCFGR_I2SCFG_Pos) | SPI_I2SCFGR_CHLEN;  // слот 32 бита
    
    // // 4. Делитель частоты как в оригинале (работает!)
    // SPI2->I2SPR = 17;  // Ваше родное значение, дающее 44.1 кГц

    // SPI2->CR2 |= SPI_CR2_TXDMAEN;
    // Настраиваем режим 32-битного слота кадра (CHLEN = 1), I2S Philips, Master Transmit
    SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD | (2 << SPI_I2SCFGR_I2SCFG_Pos) | SPI_I2SCFGR_CHLEN;  
    
    
    
        SPI2->I2SPR = 17;
    
        
    

    SPI2->CR2 |= SPI_CR2_TXDMAEN;

    // 5. НАСТРОЙКА DMA: 16-битные данные
    DMA1_Stream4->CR = 0; 
    while(DMA1_Stream4->CR & DMA_SxCR_EN); 
    DMA1->HIFCR = (0x3D << 6);

    DMA1_Stream4->PAR  = (uint32_t)&(SPI2->DR); 
    DMA1_Stream4->M0AR = (uint32_t)signal_play_buff; 
    DMA1_Stream4->NDTR = AUDIO_CHUNK_SIZE * 2;  // 4096 полуслов = 8192 байта
    
    DMA1_Stream4->CR = (0 << DMA_SxCR_CHSEL_Pos)          
                     | (3 << DMA_SxCR_PL_Pos)             
                     | (1 << DMA_SxCR_DIR_Pos)            
                     | DMA_SxCR_MINC                      
                     | (1 << DMA_SxCR_PSIZE_Pos)          // 16 бит
                     | (1 << DMA_SxCR_MSIZE_Pos);         // 16 бит
                     
    DMA1_Stream4->CR |= DMA_SxCR_TCIE;
    NVIC_SetPriority(DMA1_Stream4_IRQn, 0);
    NVIC_EnableIRQ(DMA1_Stream4_IRQn);
}



// Функция перезапуска передачи следующего куска буфера из прерывания
void Audio_I2S_Transmit_Next(uint32_t addr, uint16_t size) {
    DMA1_Stream4->CR &= ~DMA_SxCR_EN;
    while(DMA1_Stream4->CR & DMA_SxCR_EN);
    
    DMA1->HIFCR = (0x3D << 0); // ИСПРАВЛЕНО: Чистим флаги Stream4 в HIFCR
    DMA1_Stream4->M0AR = addr;
    DMA1_Stream4->NDTR = size;
    DMA1_Stream4->CR |= DMA_SxCR_EN; // Погнали!
}

void Audio_Start(void) {
    SPI2->I2SCFGR |= SPI_I2SCFGR_I2SE; // Включаем I2S2
    DMA1_Stream4->CR |= DMA_SxCR_EN;   // Запускаем DMA первого куска
}

void Audio_Stop(void) {
    SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE; 
    DMA1_Stream4->CR &= ~DMA_SxCR_EN;   
}

// Обработчик прерывания DMA — полностью защищен от зависаний
void DMA1_Stream4_IRQHandler(void) {
    uint32_t hisr = DMA1->HISR;
    DMA1->HIFCR = (0x3D << 6); // Очистка флагов 4-го стрима

    // Проверяем аппаратный TCIF4 (Бит 5 в HISR)
    if (hisr & DMA_HISR_TCIF4) { 
        if (end_of_file_reached) return;

        // Переключаем указатели Пинг-Понга
        uint16_t* temp = signal_play_buff;
        signal_play_buff = signal_read_buff;
        signal_read_buff = temp;

        // Мгновенно толкаем новый буфер в ЦАП, пока процессор свободен
        Audio_I2S_Transmit_Next((uint32_t)signal_play_buff, AUDIO_CHUNK_SIZE * 2);
        
        // Взводим флаг для main.c
        read_next_chunk = 1;
    }
}

// Математически точный генератор синусоиды (схема Фискова)
void Fill_Buffer_With_Tone(uint16_t start_ptr[], uint16_t len_samples) {
    static const int16_t sine_44_points[44] = {
        0, 4671, 9225, 13552, 17545, 21102, 24134, 26563, 28318, 29342,
        29601, 29112, 27896, 26002, 23495, 20455, 16972, 13143, 9070, 4859,
        574, -3726, -7971, -12075, -15934, -19448, -22521, -25068, -27014, -28303,
        -28901, -28795, -28003, -26568, -24548, -22026, -19095, -15859, -12429, -8919,
        -5437, -2091, 1146, 4276
    };
    
    uint32_t sine_idx = 0;
    uint32_t dst_idx = 0;
    
    while (dst_idx < len_samples) {
        int16_t sample = sine_44_points[sine_idx];
        
        start_ptr[dst_idx + 0] = (uint16_t)sample; // Левый канал данные (16 бит)
        start_ptr[dst_idx + 1] = 0x0000;           // Выравнивание слота Philips до 32 бит
        start_ptr[dst_idx + 2] = (uint16_t)sample; // Правый канал дубликат (16 бит)
        start_ptr[dst_idx + 3] = 0x0000;           // Выравнивание слота Philips до 32 бит
        
        dst_idx += 4;
        sine_idx++;
        if (sine_idx >= 44) sine_idx = 0;
    }
}