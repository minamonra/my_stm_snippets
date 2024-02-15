#include "stm32f0xx.h"
#include "main.h"
#include "lcd7735.h"

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