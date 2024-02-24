#include "main.h"
#include "myfont.h"
#include "lcd7735sl.h"
#include "SixteenSegment16x24.h"
#include "SixteenSegment24x36.h"
int main(void) 
{
  rcc_sysclockinit();
  SysTick_Config(48000); // 1ms if HSI
  gpio_init();
  spi_init();
  st7735init(LANDSCAPE, CBLUE0);
  
  unsigned char x = 32, width = 24, height = 36, lenght = 108;
  // unsigned char x = 0, width = 16, height = 24, lenght = 48;
  unsigned char X1=8, Y1=8, W1=24;
  
  const unsigned char *Font = SixteenSegment24x36;
  const unsigned int  *Fidx = SixteenSegment24x36idx;
  // const unsigned char *Font = SixteenSegment16x24;
  // const unsigned int  *Fidx = SixteenSegment16x24dx;


  do // do main 
  { 
    if (count > ttms || ttms - count > 500) {
      
      print_char_sl(    x,       X1,    Y1, width, height, 108, Font, Fidx, CORANGE,  CBLACK); 
      print_char_sl(  x+1,    X1+W1,    Y1, width, height, 108, Font, Fidx, CGRAY,    CBLACK);
      print_char_sl(  x+2,  X1+W1*2,    Y1, width, height, 108, Font, Fidx, CGREEN0,  CBLACK); 
      print_char_sl(  x+3,  X1+W1*3,    Y1, width, height, 108, Font, Fidx, CGREEN1,  CBLACK);
      print_char_sl(  x+4,  X1+W1*4,    Y1, width, height, 108, Font, Fidx, CYELLOW0, CBLACK); 
      print_char_sl(  x+5,  X1+W1*5,    Y1, width, height, 108, Font, Fidx, CMAGENTA, CBLACK);
      
      print_char_sl(  x+6,       X1, Y1+38, width, height, 108, Font, Fidx, CPURPLE,  CBLACK);
      print_char_sl(  x+7,    X1+W1, Y1+38, width, height, 108, Font, Fidx, CGREEN2,  CBLACK);
      print_char_sl(  x+8,  X1+W1*2, Y1+38, width, height, 108, Font, Fidx, CRED,     CBLACK);
      print_char_sl(  x+9,  X1+W1*3, Y1+38, width, height, 108, Font, Fidx, CGREEN3,  CBLACK);
      print_char_sl( x+10, X1+W1*4, Y1+38, width, height, 108, Font, Fidx, CRED0,    CBLACK);
      print_char_sl( x+11, X1+W1*5, Y1+38, width, height, 108, Font, Fidx, CRUBY,    CBLACK);

      print_char_sl( x+12,      X1, Y1+76, width, height, 108, Font, Fidx, CMAGENTA,  CBLACK);
      print_char_sl( x+13,   X1+W1, Y1+76, width, height, 108, Font, Fidx, CYELLOW0,  CBLACK);
      print_char_sl( x+14, X1+W1*2, Y1+76, width, height, 108, Font, Fidx, CGRAY,     CBLACK);
      print_char_sl( x+15, X1+W1*3, Y1+76, width, height, 108, Font, Fidx, CYELLOW1,  CBLACK);
      print_char_sl( x+16, X1+W1*4, Y1+76, width, height, 108, Font, Fidx, CGREEN0,   CBLACK);
      print_char_sl( x+17, X1+W1*5, Y1+76, width, height, 108, Font, Fidx, CORANGE,   CBLACK);

      count = ttms;
      x++; if (x > 95) x = 32;
    }
  } while (1); // main do
} // main

// EoF