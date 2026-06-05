#ifndef LCD_IO_H
#define LCD_IO_H

#include <stdint.h>
#include "common.h"
#include "pindefs.h" // Убедитесь, что здесь объявлены ILI9486_CS_CLR/SET и ILI9486_DC_CLR/SET

// Глобальный байтовый буфер для строк (размер увеличен до 1024 байт для безопасности)
//#define LCD_DMA_BUFFER_SIZE  512
// Увеличиваем размер до 1024 слов (2048 байт). Хватит на всю ширину экрана 480 с запасом!
#define LCD_DMA_BUFFER_SIZE  1024
extern uint16_t dma_buffer[LCD_DMA_BUFFER_SIZE];
#define dma_buffer8         ((uint8_t*)dma_buffer)


// Прототипы функций отправки данных
void lcd_send_cmd(uint8_t cmd);
void lcd_send_data8(uint8_t data);
void lcd_io_send_buf8_dma(uint8_t *pData, uint32_t bytes_per_transfer, uint32_t transfer_count);
void lcd_io_set_8bit(void);
void lcd_io_set_16bit(void);

#endif // LCD_IO_H