#include "stm32f0xx.h"
#include "main.h"
#include "lcd7735.h"

void print_char_32x24_land(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor) {
  uint8_t j, i, b, d;
  uint16_t d1, color;
  CS_DN;
  lcd7735_at(X, Y, X + 31, Y + 23); // Set window 
  lcd7735_sendCmd(0x2C);
  SPI2SIXTEEN;
  DC_UP;
  for (i = 0; i < 4; i++) {
    for (j = 8; j > 0; j--) {
      for (b = 24; b > 0; b--) {
        d1 = CH * 96 + ((b - 1) * 4 + i);
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

int main(void) {
  rcc_sysclockinit();
  SysTick_Config(48000); // 1ms if HSI
  gpio_init();
  spi_init();
  lcd7735_init(ST77XX_CYAN);
  delay_ms(10);
  uint8_t x = 0;
  do {
    if (count > ttms || ttms - count > 400)
    {
      print_char_50x32_land(x, 40, 80, ST77XX_ORANGE, ST77XX_BLUE);
      print_char_50x32_port(x, 50, 8, ST77XX_ORANGE, ST77XX_BLUE);
      count = ttms;
      x++; if (x >12) x = 0;
    }
    blink_(300);
  } while (1);
}