#ifndef __LCD_ST7735__
#define __LCD_ST7735__

#include "stm32f0xx.h"

#define LCD_SOFT_RST_DELAY 120  // пауза после программного сброса дисплея по даташиту пауза должна быть 120 мс
#define LCD_RST_DLY        010	// пауза в мс при сбросе дисплея, по даташиту должно быть 120 мс, но зачастую работает и 10 мс
// -------------------------------------------------------------------------------------------------------------------
// ОПРЕДЕЛЕНИЕ ПОРЯДКА КОДИРОВАНИЯ ЦВЕТА
// -------------------------------------------------------------------------------------------------------------------
// если закоментировать параметр ниже, то порядок кодирования будет 5B - 6G - 5R
//#define RGB					     // цвета кодируются 5R - 6G - 5B
// Примечание:
// число перед буквой показывает сколько бит какой цвет кодируют
// латинской буквой определяется цвет: R - Red (красный), G - Green (зеленый), B - Blue (синий)

// PB3 - SPI1_SCK (clock)
// PB4 - CS (chip select)
// PB5 - SPI1_MOSI (data)

// Chip select PB4
#define CS_UP GPIOB->BSRR |= GPIO_BSRR_BS_4
#define CS_DN GPIOB->BSRR |= GPIO_BSRR_BR_4

// DC (RS) PB1
#define DC_UP GPIOB->BSRR |= GPIO_BSRR_BS_1
#define DC_DN GPIOB->BSRR |= GPIO_BSRR_BR_1

// Reset PB0
#define RST_UP GPIOB->BSRR |= GPIO_BSRR_BS_0
#define RST_DN GPIOB->BSRR |= GPIO_BSRR_BR_0

#define SPI2SIXTEEN SPI1->CR2 &= ~SPI_CR2_FRXTH; SPI1->CR2 |=  SPI_CR2_DS_3; // переключаемся на 16 бит
#define SPI2EIGHT   SPI1->CR2 |=  SPI_CR2_FRXTH; SPI1->CR2 &= ~SPI_CR2_DS_3; // обратно на 8 бит  

#define SPIDR8BIT (*(__IO uint8_t *)((uint32_t)&SPI1->DR))

// Управление линией LCD_RST
#define LCD_RST1  GPIOB->BSRR |= GPIO_BSRR_BS_0
#define LCD_RST0  GPIOB->BSRR |= GPIO_BSRR_BR_0
// Управление линией LCD_DC // DC (RS) PB1
#define LCD_DC1   GPIOB->BSRR |= GPIO_BSRR_BS_1
#define LCD_DC0   GPIOB->BSRR |= GPIO_BSRR_BR_1
// Управление линией LCD_CS // Chip select PB4
#define LCD_CS1   GPIOB->BSRR |= GPIO_BSRR_BS_4
#define LCD_CS0   GPIOB->BSRR |= GPIO_BSRR_BR_4

// Some ready-made 16-bit ('565') color settings:
#define ST77XX_BLACK      0x0000
#define ST77XX_WHITE      0xFFFF
#define ST77XX_RED        0xF800
#define ST77XX_GREEN      0x07E0
#define ST77XX_BLUE       0x001F
#define ST77XX_CYAN       0x07FF
#define ST77XX_MAGENTA    0xF81F
#define ST77XX_YELLOW     0xFFE0
#define ST77XX_ORANGE     0xFC00

void lcd7735_init(uint16_t color); // инициализация дисплея
void lcd7735_sendCmd(unsigned char cmd); // отправка команды на дисплей
void lcd7735_sendData(unsigned char data) ;
void lcd7735_at(unsigned char startX, unsigned char startY, unsigned char stopX, unsigned char stopY);
void lcd7735_fillrect(unsigned char startX, unsigned char startY, unsigned char stopX, unsigned char stopY, unsigned int color); // заполнение прямоугольной области экрана
void lcd7735_putpix(unsigned char x, unsigned char y, unsigned int Color); // вывод пиксела
void lcd7735_line(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned int color); // вывод линии
void lcd7735_rect(char x1,char y1,char x2,char y2, unsigned int color); // рисование прямоугольника (не заполненного)
void lcd7735_putchar(unsigned char x, unsigned char y, unsigned char chr, unsigned int charColor, unsigned int bkgColor); // вывод символа в цвете по координатам
void lcd7735_putstr(unsigned char x, unsigned char y, const unsigned char str[], unsigned int charColor, unsigned int bkgColor); // вывод строки в цвете по координатам
void LCD7735_dec(unsigned int numb, unsigned char dcount, unsigned char x, unsigned char y,unsigned int fntColor, unsigned int bkgColor); // печать десятичного числа

void print_char_50x32_land(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor);
void print_char_50x32_port(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor);

#endif // __LCD_ST7735__