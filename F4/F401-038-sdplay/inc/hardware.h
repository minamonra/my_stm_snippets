#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include "stm32f4xx.h"

extern volatile uint8_t sd_activity_flag; 

// Светодиод PC13 (Black Pill: low = ON, high = OFF)
#define LED2_ON    GPIOC->BSRR = GPIO_BSRR_BR13
#define LED2_OFF   GPIOC->BSRR = GPIO_BSRR_BS13
#define LED2TOGGLE GPIOC->ODR ^= GPIO_ODR_OD13

// Системное время (миллисекунды)
extern volatile uint32_t ttms;

// Переменные диагностики аудио-модулей (для вывода на экран)
extern volatile uint16_t wav_display_ch;                                        // Количество каналов WAV файла
extern volatile uint16_t wav_display_bits;                                      // Битность аудио (16 или 24 бита)
extern volatile uint32_t dbg_file_fs;                                           // Частота дискретизации из заголовка
extern volatile uint32_t wav_sectors_played;                                   // Количество проигранных секторов
extern volatile uint32_t wav_audio_bytes_left;                                 // Сколько байт аудио осталось воспроизвести
extern volatile uint32_t total_audio_bytes;                                     // Общий размер аудиоданных трека
extern volatile uint8_t read_next_chunk;                                       // Флаг запроса следующей порции данных
extern volatile uint32_t wav_byte_rate;                                         // Скорость потока байт в секунду

// Оптимизированные функции задержки и тактирования
void SystemInit(void);                                                          // Обязательная функция для стартапа
void SystemClock_Config_84MHz(void);                                            // Настройка тактирования ядра и шин
void Periph_Init(void);                                                         // Инициализация пинов и таймеров
void delay_ms(uint32_t ms);                                                     // Надежная задержка на миллисекунды
void delay_nop(uint32_t ticks);                                                 // Быстрая задержка на пустых тактах
void blink_pc13led(uint16_t freq);                                              // Асинхронное мигание светодиодом платы
void itoa32(uint32_t num, char* str);                                           // Преобразование числа в текстовую строку
void format_time(uint32_t total_seconds, char* out_str);
// Заглушки для опроса периферии
void encoder_poll(void);                                                        // Функция опроса энкодера
void update_button_state(void);                                                 // Функция обработки состояния кнопок

#endif // HARDWARE_H