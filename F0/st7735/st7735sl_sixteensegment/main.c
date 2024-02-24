#include "main.h"
#include "myfont.h"
#include "lcd7735sl.h"
#include "fonts/SixteenSegment24x36.h"
int main(void) 
{
  rcc_sysclockinit();
  SysTick_Config(48000); // 1ms if HSI
  gpio_init();
  spi_init();
  delay_ms(2000);
  //lcd7735_init(CBLUE0);
  st7735init(LANDSCAPE, CBLUE0);
  
  unsigned char x = 32;
  do // do main 
  { 
    if (count > ttms || ttms - count > 500) {
      
      unsigned char X1=8, Y1=8, W1=24;
      print_char_sl(   x,       X1,    Y1, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CORANGE,  CBLACK); 
      print_char_sl( x+1,    X1+W1,    Y1, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CGRAY,    CBLACK);
      print_char_sl( x+2,  X1+W1*2,    Y1, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CGREEN0,  CBLACK); 
      print_char_sl( x+3,  X1+W1*3,    Y1, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CGREEN1,  CBLACK);
      print_char_sl( x+4,  X1+W1*4,    Y1, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CYELLOW0, CBLACK); 
      print_char_sl( x+5,  X1+W1*5,    Y1, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CMAGENTA, CBLACK);
      
      print_char_sl( x+6,       X1, Y1+38, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CPURPLE,  CBLACK);
      print_char_sl( x+7,    X1+W1, Y1+38, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CGREEN2,  CBLACK);
      print_char_sl( x+8,  X1+W1*2, Y1+38, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CRED,     CBLACK);
      print_char_sl( x+9,  X1+W1*3, Y1+38, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CGREEN3,  CBLACK);
      print_char_sl( x+10, X1+W1*4, Y1+38, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CRED0,    CBLACK);
      print_char_sl( x+10, X1+W1*5, Y1+38, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CRUBY,    CBLACK);

      print_char_sl( x+11,      X1, Y1+76, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CMAGENTA,  CBLACK);
      print_char_sl( x+12,   X1+W1, Y1+76, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CYELLOW0,  CBLACK);
      print_char_sl( x+13, X1+W1*2, Y1+76, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CGRAY,     CBLACK);
      print_char_sl( x+14, X1+W1*3, Y1+76, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CYELLOW1,  CBLACK);
      print_char_sl( x+15, X1+W1*4, Y1+76, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CGREEN0,   CBLACK);
      print_char_sl( x+16, X1+W1*5, Y1+76, 24, 36, 108, SixteenSegment24x36, SixteenSegment24x36idx, CORANGE,   CBLACK);


      //print_char_temp(x, 20, 30, 24, 32, 96, ubuntu_nums, ubuntu_nums_index, CORANGE, CBLACK); 
      //print_char_temp(x, 20+24, 30, 24, 32, 96, ubuntu_nums, ubuntu_nums_index, CBLACK, CWHITE0); 
      //print_char_temp(x, 20, 30, 24, 32, 96, InconsolaNum24x32, InconsolaNum24x32index, CORANGE, CBLACK); 
      //print_char_temp(x, 20+24, 30, 24, 32, 96, InconsolaNum24x32, InconsolaNum24x32index, CBLACK, CWHITE0); 
      //print_char_temp(x, 0, 0, 18, 35, 90, font_consolas_22, consolas22index, CORANGE, CBLACK);
      count = ttms;
      x++; if (x > 95) x = 32;
    }
  } while (1); // main do
} // main

// EoF