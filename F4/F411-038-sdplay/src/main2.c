#include "hardware.h"
#include "st7789v.h"
#include "audio_i2s.h"

// Тестовый буфер: синусоида 440 Гц, стерео, оба канала
#define TEST_BUF_SIZE 1024
__attribute__((aligned(4))) static uint16_t test_buffer[TEST_BUF_SIZE * 2];

void GenerateTestTone(void) {
    // Заполняем буфер правильной упаковкой для 32-битного слота I2S
    // Формат: [L][0][R][0] где L и R - 16-битные семплы
    for (int i = 0; i < TEST_BUF_SIZE; i++) {
        // Простая синусоида 440 Гц при 44.1 кГц (примерно 100 семплов на период)
        int16_t sample = 32767 * (i % 100 < 50 ? 1 : -1);  // меандр для простоты
        
        test_buffer[i * 4 + 0] = (uint16_t)sample;  // левый канал
        test_buffer[i * 4 + 1] = 0x0000;             // паддинг
        test_buffer[i * 4 + 2] = (uint16_t)sample;  // правый канал
        test_buffer[i * 4 + 3] = 0x0000;             // паддинг
    }
}

int main(void) {
    SystemClock_Config_84MHz();
    Periph_Init();
    delay_ms(100);
    
    // Инициализация I2S с частотой 44.1 кГц
    Audio_I2S_Init_Advanced(44100, 2);
    
    // Генерируем тестовый сигнал
    GenerateTestTone();
    
    // Загружаем тестовый буфер в оба буфера плеера
    memcpy((uint8_t*)audio_buff1, (uint8_t*)test_buffer, sizeof(test_buffer));
    memcpy((uint8_t*)audio_buff2, (uint8_t*)test_buffer, sizeof(test_buffer));
    
    signal_play_buff = audio_buff1;
    signal_read_buff = audio_buff2;
    
    Audio_Start();
    
    while(1) {
        blink_pc13led(500);
        
        // Если DMA запросил новый буфер, подсовываем тестовый снова
        if (read_next_chunk) {
            read_next_chunk = 0;
            // Меняем буферы местами (уже сделано в прерывании)
            // Просто убеждаемся, что данные правильные
            if (signal_play_buff == audio_buff1) {
                memcpy((uint8_t*)audio_buff2, (uint8_t*)test_buffer, sizeof(test_buffer));
            } else {
                memcpy((uint8_t*)audio_buff1, (uint8_t*)test_buffer, sizeof(test_buffer));
            }
        }
    }
}



void _init(void) {}                                                             // Заглушка системной инициализации компилятора
void _fini(void) {} 