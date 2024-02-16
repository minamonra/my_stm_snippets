#include "stm32f0xx.h"
#include "main.h"
#include "lcd7735.h"
#define SEGB_COLOR  ST77XX_BLUE
#define SEGF_COLOR  ST77XX_WHITE
#include "Ubuntu.h"
#include "Grotesk16x32.h"
#include "Grotesk24x48.h"
//#include "Inconsola.h"

void print_char_24x32_land(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor) {
  uint8_t j, i, b, d;
  uint16_t d1, color;
  CS_DN;
  lcd7735_at(X, Y, X + 31, Y + 23); // 24 x 32
  lcd7735_sendCmd(0x2C);
  SPI2SIXTEEN;
  DC_UP;
  for (i = 0; i < 3; i++) { // 3 байта в строке матрици 3*8 = 24
    for (j = 8; j > 0; j--) { 
      for (b = 32; b > 0; b--) { // 32 строки в матрице
        d1 = CH * 96 + ((b - 1) * 4 + i); // 96 байт
        
        //d = Inconsola[d1];
        d = Ubuntu[d1];
        if (d & (1 << (j - 1))) color = fcolor; else color = bcolor;
        while (!(SPI1->SR & SPI_SR_TXE)){};
        SPI1->DR = color;
      }
    }
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY)){};
  CS_UP;
  SPI2EIGHT;
}

void print_char_16x32_land(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor) {
  uint8_t j, i, b, d;
  uint16_t d1, color;
  CS_DN;
  lcd7735_at(X, Y, X + 31, Y + 15); // 16 x 32
  lcd7735_sendCmd(0x2C);
  SPI2SIXTEEN;
  DC_UP;
  for (i = 0; i < 2; i++) { // 2 байта в строке матрици 2*8 = 16
    for (j = 8; j > 0; j--) { 
      for (b = 32; b > 0; b--) { // 32 строки в матрице
        d1 = CH * 64 + ((b - 1) * 4 + i); // 64 байта в одном символе
        
        d = Grotesk16x32[d1];
        if (d & (1 << (j - 1))) color = fcolor; else color = bcolor;
        while (!(SPI1->SR & SPI_SR_TXE)){};
        SPI1->DR = color;
      }
    }
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY)){};
  CS_UP;
  SPI2EIGHT;
}

void print_char_24x48_land(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor) {
  uint8_t j, i, b, d;
  uint16_t d1, color;
  CS_DN;
  lcd7735_at(X, Y, X + 47, Y + 23); // 24 x 32 с поворотом
  lcd7735_sendCmd(0x2C);
  SPI2SIXTEEN;
  DC_UP;
  for (i = 0; i < 3; i++) { // 3 байта в строке матрици 3*8 = 24
    for (j = 8; j > 0; j--) { 
      for (b = 48; b > 0; b--) { // 32 строки в матрице
        d1 = CH * 144 + ((b - 1) * 4 + i); // 96 байт
        
        d = Grotesk24x48[d1];
        if (d & (1 << (j - 1))) color = fcolor; else color = bcolor;
        while (!(SPI1->SR & SPI_SR_TXE)){};
        SPI1->DR = color;
      }
    }
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY)){};
  CS_UP;
  SPI2EIGHT;
}

int main(void) {
  rcc_sysclockinit();
  SysTick_Config(48000); // 1ms if HSI
  gpio_init();
  spi_init();
  lcd7735_init(ST77XX_BLUE);
  delay_ms(10);
  uint8_t x = 0;
  do {
  if (count > ttms || ttms - count > 400)
  {
    
    //print_char_50x32_land(x, 40, 80, ST77XX_BLACK, ST77XX_WHITE);
    //print_char_50x32_port(x, 50,  8, ST77XX_BLACK, ST77XX_WHITE);
    
    print_char_24x32_land( x, 50, 10, ST77XX_BLACK, ST77XX_WHITE);
    print_char_24x48_land( x, 50, 60, ST77XX_BLACK, ST77XX_WHITE);
    print_char_16x32_land( x, 50, 90, ST77XX_BLACK, ST77XX_WHITE);
    count = ttms;
    x++; if (x > 12) x = 0;
  }
  blink_(300);
  } while (1);
}


// End of file