#ifndef __UART_H__
#define __UART_H__

#include "stm32f4xx.h"
#include <stdint.h>

// Размер буфера кратен 256 для идеальной работы кольца
#define RX_BUFFER_SIZE 256

extern volatile uint8_t uart_command;

// ============================================================================
// === USART1 (консоль PA9, PA10) =============================================
// ============================================================================
void     uart1_init(uint32_t baudrate);
void     uart1_write(uint8_t ch);
void     uart1_write_str(const char* str);
void     uart1_write_bytes(const uint8_t* data, uint16_t size);
void     uart1_write_num(uint32_t num);
void uart1_write_hex(uint8_t byte);
uint8_t  uart1_available(void);
uint8_t  uart1_read(void);
uint16_t uart1_read_bytes(uint8_t* dest, uint16_t max_len);

// ============================================================================
// === USART3 (RS485 PC10, PC11) ==============================================
// ============================================================================
void     uart3_init(uint32_t baudrate);
void     uart3_write(uint8_t ch);
void     uart3_write_str(const char* str);
void     uart3_write_bytes(const uint8_t* data, uint16_t size);
uint8_t  uart3_available(void);
uint8_t  uart3_read(void);
uint16_t uart3_read_bytes(uint8_t* dest, uint16_t max_len);
void     uart3_flush(void);

#endif  // __UART_H__