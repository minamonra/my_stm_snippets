#include <stdlib.h>
#include "ili9488x.h"
#include "common.h"
#include "pindefs.h"
#include "lcd_io.h"
#include "lcd_font.h"

// Привязываемся к вашему буферу строки, который лежит в lcd_io.c или common.c
extern uint16_t dma_buffer[];
#define dma_buffer8 ((uint8_t*)dma_buffer)

//extern uint16_t get_random(uint16_t max);

// Прошивка фабричных регистров ILI9488 в режиме 18-бит (RGB666)
void ili9488_Init(void) {
    ILI9486_LED_ON;                                                             
    ILI9486_RST_CLR; delay_nop(1000);
    ILI9486_RST_SET; delay_nop(1000);
    
    lcd_send_cmd(0x01); //delay_ms(1); 
    lcd_send_cmd(0x11); //delay_ms(1); 
    
    lcd_send_cmd(0x3A); lcd_send_data8(0x66);
    delay_ms(10); 

    lcd_send_cmd(0x36); lcd_send_data8(0xF8);
    delay_ms(10);

    lcd_send_cmd(0x29); //delay_ms(1); 
    ili9488_Clear(ILI9488_BLACK);
    delay_ms(10);
}

// Установка координат графического окна на дисплее
void ili9488_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize) {
    uint16_t x_end = Xpos + Xsize - 1;
    uint16_t y_end = Ypos + Ysize - 1;

    // Задаем столбцы (X) — диапазон от 0 до 479
    lcd_send_cmd(0x2A); 
    lcd_send_data8((uint8_t)(Xpos >> 8));   lcd_send_data8((uint8_t)(Xpos & 0xFF));
    lcd_send_data8((uint8_t)(x_end >> 8));  lcd_send_data8((uint8_t)(x_end & 0xFF));
    
    // Задаем строки (Y) — диапазон от 0 до 319
    lcd_send_cmd(0x2B); 
    lcd_send_data8((uint8_t)(Ypos >> 8));   lcd_send_data8((uint8_t)(Ypos & 0xFF));
    lcd_send_data8((uint8_t)(y_end >> 8));  lcd_send_data8((uint8_t)(y_end & 0xFF));
}

// Скоростная заливка прямоугольника через 8-битный DMA конвейер
void ili9488_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode) {
    if ((Xpos + Xsize) > ILI9488_WIDTH)  Xsize = ILI9488_WIDTH - Xpos;
    if ((Ypos + Ysize) > ILI9488_HEIGHT) Ysize = ILI9488_HEIGHT - Ypos;
    if (Xsize == 0 || Ysize == 0) return;

    // Задаем окно координат
    ili9488_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
    lcd_send_cmd(0x2C); // RAMWR
    ILI9486_DC_SET; 

    // Вычисляем байты для ОДНОЙ строки (по 3 байта на пиксель)
    uint32_t bytes_per_line = Xsize * 3;
    
    // Переводим RGB565 в честный RGB888
    uint8_t r = (uint8_t)((RGBCode & 0xF800) >> 8);
    uint8_t g = (uint8_t)((RGBCode & 0x07E0) >> 3);
    uint8_t b = (uint8_t)((RGBCode & 0x001F) << 3);
    
    // Наш расширенный dma_buffer8 (размер теперь 2048 байт благодаря LCD_DMA_BUFFER_SIZE = 1024)
    // Хватит на всю ширину 480 * 3 = 1440 байт с огромным запасом!
    for (uint32_t i = 0; i < bytes_per_line; i += 3) {
        dma_buffer8[i]     = r;
        dma_buffer8[i + 1] = g;
        dma_buffer8[i + 2] = b;
    }

    // Зажимаем CS, чтобы убрать лишние паузы процессора между строками
    ILI9486_CS_CLR;                                  

    for (uint32_t i = 0; i < Ysize; i++) {
        RESET_DMA_STREAM5();
        
        DMA2_Stream5->M0AR = (uint32_t)dma_buffer8;            
        DMA2_Stream5->NDTR = bytes_per_line; // Шлем ровную порцию байт одной строки
        
        DMA2_Stream5->CR |= DMA_SxCR_EN;                 
        
        // Ждем флаг окончания передачи памяти
        while (!(DMA2->HISR & DMA_HISR_TCIF5));          
    }

    // В самом конце один раз ждем опустошения шины SPI
    while (SPI1->SR & SPI_SR_BSY);                  
    ILI9486_CS_SET;                                  
}

// Полная очистка экрана выбранным цветом
void ili9488_Clear(uint16_t RGBCode) {
    ili9488_FillRect(0, 0, ILI9488_WIDTH, ILI9488_HEIGHT, RGBCode);
}

// Одиночная точка (Пиксель). Самая медленная функция из-за накладных расходов SPI на 1 пиксель.
void ili9488_DrawPixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode) {
    if (Xpos >= ILI9488_WIDTH || Ypos >= ILI9488_HEIGHT) return;

    ili9488_SetDisplayWindow(Xpos, Ypos, 1, 1);
    lcd_send_cmd(0x2C); // RAMWR
    ILI9486_DC_SET;

    // Конвертация RGB565 -> RGB888
    uint8_t r = (uint8_t)((RGBCode & 0xF800) >> 8);
    uint8_t g = (uint8_t)((RGBCode & 0x07E0) >> 3);
    uint8_t b = (uint8_t)((RGBCode & 0x001F) << 3);

    // Выводим 3 байта напрямую без DMA (для 1 пикселя DMA инициализировать дольше)
    ILI9486_CS_CLR;
    lcd_send_data8(r);
    lcd_send_data8(g);
    lcd_send_data8(b);
    while (SPI1->SR & SPI_SR_BSY);
    ILI9486_CS_SET;
}

// Быстрая горизонтальная линия через FillRect (использует DMA под капотом)
void ili9488_DrawFastHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length, uint16_t RGBCode) {
    ili9488_FillRect(Xpos, Ypos, Length, 1, RGBCode);
}

// Быстрая вертикальная линия через FillRect (использует DMA под капотом)
void ili9488_DrawFastVLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length, uint16_t RGBCode) {
    ili9488_FillRect(Xpos, Ypos, 1, Length, RGBCode);
}

// Контур прямоугольника (состоит из 4 быстрых линий)
void ili9488_DrawRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode) {
    if (Xsize == 0 || Ysize == 0) return;
    ili9488_DrawFastHLine(Xpos, Ypos, Xsize, RGBCode);
    ili9488_DrawFastHLine(Xpos, Ypos + Ysize - 1, Xsize, RGBCode);
    ili9488_DrawFastVLine(Xpos, Ypos, Ysize, RGBCode);
    ili9488_DrawFastVLine(Xpos + Xsize - 1, Ypos, Ysize, RGBCode);
}

// Универсальная линия произвольного направления (Алгоритм Брезенхема)
void ili9488_DrawLine(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t RGBCode) {
    int16_t dx = abs((int16_t)X2 - (int16_t)X1);
    int16_t dy = abs((int16_t)Y2 - (int16_t)Y1);
    int16_t sx = (X1 < X2) ? 1 : -1;
    int16_t sy = (Y1 < Y2) ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        ili9488_DrawPixel(X1, Y1, RGBCode);
        if (X1 == X2 && Y1 == Y2) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            X1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            Y1 += sy;
        }
    }
}

// Контур окружности (Алгоритм Брезенхема/Мидпоинта)
void ili9488_DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint16_t RGBCode) {
    int16_t x = 0;
    int16_t y = Radius;
    int16_t d = 3 - 2 * Radius;

    while (y >= x) {
        ili9488_DrawPixel(Xpos + x, Ypos + y, RGBCode);
        ili9488_DrawPixel(Xpos - x, Ypos + y, RGBCode);
        ili9488_DrawPixel(Xpos + x, Ypos - y, RGBCode);
        ili9488_DrawPixel(Xpos - x, Ypos - y, RGBCode);
        ili9488_DrawPixel(Xpos + y, Ypos + x, RGBCode);
        ili9488_DrawPixel(Xpos - y, Ypos + x, RGBCode);
        ili9488_DrawPixel(Xpos + y, Ypos - x, RGBCode);
        ili9488_DrawPixel(Xpos - y, Ypos - x, RGBCode);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

// Заполненная окружность (Оптимизирована: вместо точек рисует быстрые горизонтальные линии)
void ili9488_FillCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint16_t RGBCode) {
    int16_t x = 0;
    int16_t y = Radius;
    int16_t d = 3 - 2 * Radius;

    while (y >= x) {
        ili9488_DrawFastHLine(Xpos - x, Ypos + y, 2 * x + 1, RGBCode);
        ili9488_DrawFastHLine(Xpos - x, Ypos - y, 2 * x + 1, RGBCode);
        ili9488_DrawFastHLine(Xpos - y, Ypos + x, 2 * y + 1, RGBCode);
        ili9488_DrawFastHLine(Xpos - y, Ypos - x, 2 * y + 1, RGBCode);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

