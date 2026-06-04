// ЯВНО ПОДКЛЮЧАЕМ ШРИФТЫ НА САМОМ ВЕРХУ, ЧТОБЫ ВСЕ ФУНКЦИИ ИХ ВИДЕЛИ
#include "ter_k16b.h"
#include "ter_k16n.h"
#include "ter-k32b.h"
#include "ter-k32n.h"
#include "ter-k24b.h"
#include "ter-k24n.h"

#include "st7789v.h"
#include "hardware.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdlib.h>  // ДОБАВЛЕНО для abs()

// Глобальный буфер для распаковки строк крупного шрифта
uint16_t sym32_buf[32]; 

// =========================================================================
// МАКРОСЫ УПРАВЛЕНИЯ ПИНАМИ
// =========================================================================
#define ST7789_CS_LOW()   GPIOA->BSRR = GPIO_BSRR_BR2
#define ST7789_CS_HIGH()  GPIOA->BSRR = GPIO_BSRR_BS2
#define ST7789_DC_LOW()   GPIOA->BSRR = GPIO_BSRR_BR3
#define ST7789_DC_HIGH()  GPIOA->BSRR = GPIO_BSRR_BS3
#define ST7789_RST_LOW()  GPIOA->BSRR = GPIO_BSRR_BR1
#define ST7789_RST_HIGH() GPIOA->BSRR = GPIO_BSRR_BS1
#define ST7789_BL_ON()    GPIOC->BSRR = GPIO_BSRR_BS15
#define ST7789_BL_OFF()   GPIOC->BSRR = GPIO_BSRR_BR15

// Базовые хелперы обмена байтами по SPI1
static inline void SPI1_SendByte(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_RXNE));
    (void)SPI1->DR;
}

static void SPI1_WaitBusy(void) {
    while (!(SPI1->SR & SPI_SR_TXE));
    while (SPI1->SR & SPI_SR_BSY);
    volatile uint8_t dummy = SPI1->DR; 
    (void)dummy;
}

static void SPI1_SetDataSize16(uint8_t is_16bit) {
    SPI1_WaitBusy();
    SPI1->CR1 &= ~SPI_CR1_SPE; 
    if (is_16bit) {
        SPI1->CR1 |= SPI_CR1_DFF;
    } else {
        SPI1->CR1 &= ~SPI_CR1_DFF;
    }
    SPI1->CR1 |= SPI_CR1_SPE;
}

// Функции декомпрессии шрифтов
static void KOI8Rx24_decompress_sym(uint8_t sym, uint16_t *sym16_buf) {
    uint32_t idx = TERMINUS_K12x24B_ID[sym];
    uint8_t i = 0;
    while (i < 24) {
        uint16_t s = TERMINUS_K12x24B[idx];
        uint16_t j = (s >> 12);     
        s &= 0x0FFF;                
        if (j == 0) j = 16;
        for (uint8_t k = 0; k < j && i < 24; k++) {
            sym16_buf[i++] = s;
        }
        idx++;
    }
}

static void KOI8Rx32_decompress_sym(uint8_t sym) {
    uint16_t idx = TERMINUS_K16x32B_ID[sym];
    uint8_t i = 0;
    while (i < 32) {
        uint8_t index = TERMINUS_K16x32B[idx++];
        if ((index & 0x03) == 0x03) {           
            index = (index >> 2);
            if (index == 0) index = 0x20;
            for (uint8_t j = 0; j < index; j++) {
                sym32_buf[i++] = 0x0000;
            }
        } 
        else if (index & 0x80) {                
            index &= 0x7F; index = index >> 2;
            if (index == 0) index = 0x20;
            for (uint8_t j = 0; j < index; j++) {
                uint16_t a1 = TERMINUS_K16x32B[idx++];
                uint16_t a2 = TERMINUS_K16x32B[idx++];
                uint16_t mask = (a2 << 8) + a1;
                sym32_buf[i++] = mask;
            }
        } 
        else {                                  
            index = (index >> 2);
            if (index == 0) index = 0x20;
            uint16_t a1 = TERMINUS_K16x32B[idx++];
            uint16_t a2 = TERMINUS_K16x32B[idx++];
            uint16_t mask = (a2 << 8) + a1;
            for (uint8_t j = 0; j < index; j++) {
                sym32_buf[i++] = mask;
            }
        }
    }
}

void ST7789_Reset(void) {
    ST7789_RST_LOW();
    delay_ms(10);
    ST7789_RST_HIGH();
    delay_ms(20);
}

static void ST7789_SendCommand(uint8_t cmd) {
    ST7789_CS_LOW();
    ST7789_DC_LOW();
    SPI1_SendByte(cmd);
    SPI1_WaitBusy();
    ST7789_CS_HIGH();
}

static void ST7789_SendData(uint8_t data) {
    ST7789_CS_LOW();
    ST7789_DC_HIGH();
    SPI1_SendByte(data);
    SPI1_WaitBusy();
    ST7789_CS_HIGH();
}

void ST7789_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    ST7789_CS_LOW();
    ST7789_DC_LOW();  SPI1_SendByte(0x2A); SPI1_WaitBusy(); 
    ST7789_DC_HIGH();
    SPI1_SendByte(x1 >> 8);   SPI1_SendByte(x1 & 0xFF);
    SPI1_SendByte(x2 >> 8);   SPI1_SendByte(x2 & 0xFF);
    SPI1_WaitBusy();
    ST7789_DC_LOW();  SPI1_SendByte(0x2B); SPI1_WaitBusy(); 
    ST7789_DC_HIGH();
    SPI1_SendByte(y1 >> 8);   SPI1_SendByte(y1 & 0xFF);
    SPI1_SendByte(y2 >> 8);   SPI1_SendByte(y2 & 0xFF);
    SPI1_WaitBusy();
    ST7789_DC_LOW();  SPI1_SendByte(0x2C); SPI1_WaitBusy(); 
    ST7789_DC_HIGH();
}

void ST7789_Init(void) {
    ST7789_Reset();
    ST7789_SendCommand(0x01); delay_ms(10);
    ST7789_SendCommand(0x11); delay_ms(10);
    ST7789_SendCommand(0x3A); ST7789_SendData(0x05); delay_ms(10);
    ST7789_SendCommand(0x36); ST7789_SendData(0xA0); delay_ms(10);
    ST7789_SendCommand(0x29); delay_ms(10);
    ST7789_BL_ON();
}

void ST7789_DMA_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;  
    DMA2_Stream5->CR &= ~DMA_SxCR_EN;    
    while (DMA2_Stream5->CR & DMA_SxCR_EN);
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    DMA2->HIFCR = (0x3D << 6);
}

// Запуск DMA передачи
static void Start_DMA(uint32_t ram_addr, uint16_t words_count, uint8_t is_last) {
    if (DMA2_Stream5->CR & DMA_SxCR_EN) {
        while (!(DMA2->HISR & (1 << 11))); 
        DMA2->HIFCR = (1 << 11);
    }
    
    SPI1_SetDataSize16(1);
    SPI1->CR2 &= ~SPI_CR2_TXDMAEN;
    
    DMA2_Stream5->CR &= ~DMA_SxCR_EN;
    while (DMA2_Stream5->CR & DMA_SxCR_EN);
    DMA2->HIFCR = (0x3D << 6); 
    
    DMA2_Stream5->PAR  = (uint32_t)&(SPI1->DR);
    DMA2_Stream5->M0AR = ram_addr;
    DMA2_Stream5->NDTR = words_count;
    
    DMA2_Stream5->CR = (3 << DMA_SxCR_CHSEL_Pos) | (1 << DMA_SxCR_MSIZE_Pos) | 
                       (1 << DMA_SxCR_PSIZE_Pos) | (1 << DMA_SxCR_DIR_Pos) | 
                       DMA_SxCR_MINC | (1 << DMA_SxCR_PL_Pos);
                       
    DMA2_Stream5->CR |= DMA_SxCR_EN;
    __NOP(); __NOP();
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    
    if (is_last) {
        while (!(DMA2->HISR & (1 << 11)));
        while (!(SPI1->SR & SPI_SR_TXE));
        while (SPI1->SR & SPI_SR_BSY);
        ST7789_CS_HIGH();
        SPI1_SetDataSize16(0);
    }
}

// ==================== БАЗОВЫЕ ПРИМИТИВЫ ====================

void ST7789_FillScreen_DMA(uint16_t color) {
    ST7789_ClearScreen_DMA(color);
}

void ST7789_FillRect_CPU(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    if (x + w > ST7789_WIDTH) w = ST7789_WIDTH - x;
    if (y + h > ST7789_HEIGHT) h = ST7789_HEIGHT - y;
    
    SPI1_SetDataSize16(0);
    ST7789_SetWindow(x, y, x + w - 1, y + h - 1);
    ST7789_CS_LOW();
    ST7789_DC_HIGH();
    
    SPI1_SetDataSize16(1);
    uint32_t total = w * h;
    for (uint32_t i = 0; i < total; i++) {
        while (!(SPI1->SR & SPI_SR_TXE));
        SPI1->DR = color;
        while (!(SPI1->SR & SPI_SR_RXNE));
        (void)SPI1->DR;
    }
    
    while (!(SPI1->SR & SPI_SR_TXE));
    while (SPI1->SR & SPI_SR_BSY);
    ST7789_CS_HIGH();
    SPI1_SetDataSize16(0);
}

void ST7789_FillRect_DMA(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    if (x + w > ST7789_WIDTH) w = ST7789_WIDTH - x;
    if (y + h > ST7789_HEIGHT) h = ST7789_HEIGHT - y;
    
    #define DMA_BUF_SIZE 320
    __attribute__((aligned(4))) static uint16_t dma_buffer[DMA_BUF_SIZE];
    
    for (uint16_t i = 0; i < DMA_BUF_SIZE; i++) {
        dma_buffer[i] = color;
    }
    
    SPI1_SetDataSize16(0);
    ST7789_SetWindow(x, y, x + w - 1, y + h - 1);
    ST7789_CS_LOW();
    ST7789_DC_HIGH();
    SPI1_SetDataSize16(1);
    
    uint16_t remaining = w * h;
    while (remaining > 0) {
        uint16_t chunk = (remaining > DMA_BUF_SIZE) ? DMA_BUF_SIZE : remaining;
        
        if (DMA2_Stream5->CR & DMA_SxCR_EN) {
            while (!(DMA2->HISR & (1 << 11)));
            DMA2->HIFCR = (1 << 11);
        }
        
        SPI1->CR2 &= ~SPI_CR2_TXDMAEN;
        DMA2_Stream5->CR &= ~DMA_SxCR_EN;
        while (DMA2_Stream5->CR & DMA_SxCR_EN);
        DMA2->HIFCR = (0x3D << 6);
        
        DMA2_Stream5->PAR = (uint32_t)&(SPI1->DR);
        DMA2_Stream5->M0AR = (uint32_t)dma_buffer;
        DMA2_Stream5->NDTR = chunk;
        DMA2_Stream5->CR = (3 << DMA_SxCR_CHSEL_Pos) | (1 << DMA_SxCR_MSIZE_Pos) |
                          (1 << DMA_SxCR_PSIZE_Pos) | (1 << DMA_SxCR_DIR_Pos) |
                          DMA_SxCR_MINC | (1 << DMA_SxCR_PL_Pos);
        DMA2_Stream5->CR |= DMA_SxCR_EN;
        __NOP(); __NOP();
        SPI1->CR2 |= SPI_CR2_TXDMAEN;
        
        while (!(DMA2->HISR & (1 << 11)));
        remaining -= chunk;
    }
    
    while (!(SPI1->SR & SPI_SR_TXE));
    while (SPI1->SR & SPI_SR_BSY);
    ST7789_CS_HIGH();
    SPI1_SetDataSize16(0);
}

void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    ST7789_FillRect_DMA(x, y, w, h, color);
}

void ST7789_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    ST7789_FillRect_DMA(x, y, w, h, color);
}

void ST7789_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    ST7789_DrawHLine(x, y, w, color);
    ST7789_DrawHLine(x, y + h - 1, w, color);
    ST7789_DrawVLine(x, y, h, color);
    ST7789_DrawVLine(x + w - 1, y, h, color);
}

void ST7789_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    if (y >= ST7789_HEIGHT) return;
    if (x >= ST7789_WIDTH) return;
    if (x + w > ST7789_WIDTH) w = ST7789_WIDTH - x;
    ST7789_FillRect_DMA(x, y, w, 1, color);
}

void ST7789_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    if (x >= ST7789_WIDTH) return;
    if (y >= ST7789_HEIGHT) return;
    if (y + h > ST7789_HEIGHT) h = ST7789_HEIGHT - y;
    ST7789_FillRect_DMA(x, y, 1, h, color);
}

void ST7789_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    int16_t dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int16_t dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx + dy;
    
    while (1) {
        ST7789_FillRect_DMA(x1, y1, 1, 1, color);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

// ==================== ШРИФТЫ ====================

void ST7789_DrawChar_12x24(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg_color) {
    if ((x + 12 > ST7789_WIDTH) || (y + 24 > ST7789_HEIGHT)) return;
    
    if (DMA2_Stream5->CR & DMA_SxCR_EN) {
        while (!(DMA2->HISR & (1 << 11)));
        DMA2->HIFCR = (1 << 11);
    }
    
    uint16_t sym16_buf[24];
    KOI8Rx24_decompress_sym(c, sym16_buf);
    
    #define CHAR12_BUF_WORDS (24 * 12)
    __attribute__((aligned(4))) static uint16_t dma_buf_char12[CHAR12_BUF_WORDS]; 
    
    uint32_t buf_idx = 0;
    for (uint8_t i = 0; i < 24; i++) {
        uint16_t data = sym16_buf[i] << 4;
        for (uint8_t j = 0; j < 12; j++) {
            dma_buf_char12[buf_idx++] = (data & 0x8000) ? color : bg_color;
            data <<= 1;
        }
    }
    
    SPI1_SetDataSize16(0); 
    ST7789_SetWindow(x, y, x + 12 - 1, y + 24 - 1);
    ST7789_CS_LOW(); ST7789_DC_HIGH();
    
    Start_DMA((uint32_t)dma_buf_char12, CHAR12_BUF_WORDS, 1);
}

void ST7789_DrawChar_16x32(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg_color) {
    if ((x + 16 > ST7789_WIDTH) || (y + 32 > ST7789_HEIGHT)) return;
    
    if (DMA2_Stream5->CR & DMA_SxCR_EN) {
        while (!(DMA2->HISR & (1 << 11)));
        DMA2->HIFCR = (1 << 11);
    }
    
    extern uint16_t sym32_buf[32];
    KOI8Rx32_decompress_sym(c);
    
    #define CHAR16_BUF_WORDS (32 * 16)
    __attribute__((aligned(4))) static uint16_t dma_buf_char16[CHAR16_BUF_WORDS];
    
    uint32_t buf_idx = 0;
    for (uint8_t row = 0; row < 32; row++) {
        uint16_t data = sym32_buf[row];
        for (uint8_t col = 0; col < 16; col++) {
            dma_buf_char16[buf_idx++] = (data & 0x8000) ? color : bg_color;
            data <<= 1;
        }
    }
    
    SPI1_SetDataSize16(0); 
    ST7789_SetWindow(x, y, x + 16 - 1, y + 32 - 1);
    ST7789_CS_LOW(); ST7789_DC_HIGH();
    
    Start_DMA((uint32_t)dma_buf_char16, CHAR16_BUF_WORDS, 1);
}

void ST7789_DrawString_12x24(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color) {
    uint16_t start_x = x;
    while (*str) {
        uint8_t c = (uint8_t)*str; 
        if (c == 0xD0 || c == 0xD1) {
            str++; uint8_t n = (uint8_t)*str;
            if (c == 0xD0) {
                if (n == 0x81) c = 0xB3;
                else if (n >= 0x90 && n <= 0xBF) {
                    static const uint8_t koi8_table_d0[] = {0xE1,0xE2,0xF7,0xE7,0xE4,0xE5,0xF6,0xFA,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF2,0xF3,0xF4,0xF5,0xE6,0xE8,0xE3,0xFE,0xFB,0xFD,0xFF,0xF9,0xF8,0xFC,0xE0,0xF1,0xC1,0xC2,0xD7,0xC7,0xC4,0xC5,0xD6,0xDA,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0};
                    c = koi8_table_d0[n - 0x90];
                }
            } else if (c == 0xD1) {
                if (n == 0x91) c = 0xA3;
                else if (n >= 0x80 && n <= 0x8F) {
                    static const uint8_t koi8_table_d1[] = {0xD2,0xD3,0xD4,0xD5,0xC6,0xC8,0xC3,0xDE,0xDB,0xDD,0xDF,0xD9,0xD8,0xDC,0xC0,0xD1};
                    c = koi8_table_d1[n - 0x80];
                }
            }
        }
        ST7789_DrawChar_12x24(x, y, c, color, bg_color);
        x += 12; if (x + 12 > ST7789_WIDTH) { x = start_x; y += 24; }
        str++;
    }
}

void ST7789_DrawString_16x32(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color) {
    uint16_t start_x = x;
    while (*str) {
        uint8_t c = (uint8_t)*str;
        if (c == 0xD0 || c == 0xD1) {
            str++; uint8_t n = (uint8_t)*str;
            if (c == 0xD0) {
                if (n == 0x81) c = 0xB3;
                else if (n >= 0x90 && n <= 0xBF) {
                    static const uint8_t koi8_table_d0[] = {0xE1,0xE2,0xF7,0xE7,0xE4,0xE5,0xF6,0xFA,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF2,0xF3,0xF4,0xF5,0xE6,0xE8,0xE3,0xFE,0xFB,0xFD,0xFF,0xF9,0xF8,0xFC,0xE0,0xF1,0xC1,0xC2,0xD7,0xC7,0xC4,0xC5,0xD6,0xDA,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0};
                    c = koi8_table_d0[n - 0x90];
                }
            } else if (c == 0xD1) {
                if (n == 0x91) c = 0xA3;
                else if (n >= 0x80 && n <= 0x8F) {
                    static const uint8_t koi8_table_d1[] = {0xD2,0xD3,0xD4,0xD5,0xC6,0xC8,0xC3,0xDE,0xDB,0xDD,0xDF,0xD9,0xD8,0xDC,0xC0,0xD1};
                    c = koi8_table_d1[n - 0x80];
                }
            }
        }
        ST7789_DrawChar_16x32(x, y, c, color, bg_color);
        x += 16; if (x + 16 > ST7789_WIDTH) { x = start_x; y += 32; }
        str++;
    }
}

void ST7789_ClearScreen_DMA(uint16_t color) {
    SPI1_WaitBusy();
    SPI1_SetDataSize16(0); 
    ST7789_SetWindow(0, 0, 319, 239);
    ST7789_CS_LOW(); 
    ST7789_DC_HIGH();
    SPI1_SetDataSize16(1); 

    #define LINE_BUF_SIZE 320
    __attribute__((aligned(4))) static uint16_t line_dma_buffer[LINE_BUF_SIZE];
    
    for (uint16_t i = 0; i < LINE_BUF_SIZE; i++) {
        line_dma_buffer[i] = color;
    }

    SPI1->CR2 &= ~SPI_CR2_TXDMAEN;    
    DMA2_Stream5->CR &= ~DMA_SxCR_EN; 
    while (DMA2_Stream5->CR & DMA_SxCR_EN);
    
    DMA2_Stream5->PAR  = (uint32_t)&(SPI1->DR);    
    DMA2_Stream5->M0AR = (uint32_t)line_dma_buffer; 
    
    uint32_t dma_cr_config = (3 << DMA_SxCR_CHSEL_Pos) | (1 << DMA_SxCR_MSIZE_Pos) | 
                             (1 << DMA_SxCR_PSIZE_Pos) | (1 << DMA_SxCR_DIR_Pos) | 
                             DMA_SxCR_MINC | (1 << DMA_SxCR_PL_Pos);

    for (uint16_t row = 0; row < 240; row++) {
        DMA2_Stream5->CR = 0;
        while (DMA2_Stream5->CR & DMA_SxCR_EN);
        DMA2->HIFCR = (0x3D << 6);
        DMA2_Stream5->NDTR = LINE_BUF_SIZE;
        DMA2_Stream5->CR = dma_cr_config;
        DMA2_Stream5->CR |= DMA_SxCR_EN;
        SPI1->CR2 |= SPI_CR2_TXDMAEN;
        while (!(DMA2->HISR & (1 << 11))); 
    }

    while (!(SPI1->SR & SPI_SR_TXE));
    while (SPI1->SR & SPI_SR_BSY);
    volatile uint16_t dummy = SPI1->DR;
    (void)dummy;
    ST7789_CS_HIGH();
    SPI1_SetDataSize16(0); 
}

void ST7789_ClearScreen_NoDMA(uint16_t color) {
    SPI1_SetDataSize16(0);
    ST7789_SetWindow(0, 0, 319, 239);
    ST7789_CS_LOW();
    ST7789_DC_HIGH(); 
    
    uint8_t hi = (color >> 8) & 0xFF;
    uint8_t lo = color & 0xFF;
    uint32_t total_pixels = 320 * 240;
    
    for (uint32_t i = 0; i < total_pixels; i++) {
        SPI1_SendByte(hi);
        SPI1_SendByte(lo);
    }
    SPI1_WaitBusy();
    ST7789_CS_HIGH();
}