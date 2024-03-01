#include "main.h"
#include "myfont.h"
#include "lcd7735sl.h"
//#include "consolas_22_font.h"
//#include "ubuntunums.h"
#include "fonts/SixteenSegment24x36.h"
#include "SixteenSegment16x24.h"
#include "Arial_round_16x24.h"
#include "consolas_22_font.h"
#include "consolas_18_font.h"
#include "gost_type_a_18_font.h"
int main(void) 
{
  rcc_sysclockinit();
  SysTick_Config(48000); // 1ms if HSI
  gpio_init();
  spi_init();
  delay_ms(1000);
  st7735init(LANDSCAPE, CBLUE0);
  
  //unsigned char x = 32, width = 24, height = 36, lenght = 108;; // SixteenSegment24x36
  //unsigned char x = 0, width = 16, height = 24, lenght = 48;// SixteenSegment16x24 Arial_round_16x24
  //unsigned char x =32, width = 15, height = 26, lenght = 52;  // consolas 22
  //unsigned char x =32, width = 18, height = 34, lenght = 102;  // consolas 22
  unsigned char x =0, width = 17, height = 23, lenght = 69;  // consolas 22
  unsigned char X1=8, Y1=8, W1=24;
  
  //const unsigned char *Font = SixteenSegment16x24;
  //const unsigned int  *Fidx = SixteenSegment16x24dx;
  //const unsigned char *Font = Arial_round_16x24;
  //const unsigned int  *Fidx = Arial_round_16x24idx;
  //const unsigned char *Font = font_consolas_18;
  //const unsigned int  *Fidx = font_consolas_18idx;
  //const unsigned char *Font = font_consolas_22;
  //const unsigned int  *Fidx = font_consolas_22idx;

  const unsigned char *Font = font_gost_type_a_18;
  const unsigned int  *Fidx = font_gost_type_a_18idx;


  do // do main 
  { 
    if (count > ttms || ttms - count > 500) {
      
      
      print_char_sl_rb(   x,       X1,    Y1, width, height, lenght, Font, Fidx, CORANGE,  CBLACK); 
      print_char_sl_rb( x+1,    X1+W1,    Y1, width, height, lenght, Font, Fidx, CGRAY,    CBLACK);
      print_char_sl_rb( x+2,  X1+W1*2,    Y1, width, height, lenght, Font, Fidx, CGREEN0,  CBLACK); 
      print_char_sl_rb( x+3,  X1+W1*3,    Y1, width, height, lenght, Font, Fidx, CGREEN1,  CBLACK);
      print_char_sl_rb( x+4,  X1+W1*4,    Y1, width, height, lenght, Font, Fidx, CYELLOW0, CBLACK); 
      print_char_sl_rb( x+5,  X1+W1*5,    Y1, width, height, lenght, Font, Fidx, CMAGENTA, CBLACK);
      
      print_char_sl_rb( x+6,       X1, Y1+38, width, height, lenght, Font, Fidx, CPURPLE,  CBLACK);
      print_char_sl_rb( x+7,    X1+W1, Y1+38, width, height, lenght, Font, Fidx, CGREEN2,  CBLACK);
      print_char_sl_rb( x+8,  X1+W1*2, Y1+38, width, height, lenght, Font, Fidx, CRED,     CBLACK);
      print_char_sl_rb( x+9,  X1+W1*3, Y1+38, width, height, lenght, Font, Fidx, CGREEN3,  CBLACK);
      print_char_sl_rb( x+10, X1+W1*4, Y1+38, width, height, lenght, Font, Fidx, CRED0,    CBLACK);
      print_char_sl_rb( x+11, X1+W1*5, Y1+38, width, height, lenght, Font, Fidx, CRUBY,    CBLACK);

      print_char_sl_rb( x+12,      X1, Y1+76, width, height, lenght, Font, Fidx, CMAGENTA,  CBLACK);
      print_char_sl_rb( x+13,   X1+W1, Y1+76, width, height, lenght, Font, Fidx, CYELLOW0,  CBLACK);
      print_char_sl_rb( x+14, X1+W1*2, Y1+76, width, height, lenght, Font, Fidx, CGRAY,     CBLACK);
      print_char_sl_rb( x+15, X1+W1*3, Y1+76, width, height, lenght, Font, Fidx, CYELLOW1,  CBLACK);
      print_char_sl_rb( x+16, X1+W1*4, Y1+76, width, height, lenght, Font, Fidx, CGREEN0,   CBLACK);
      print_char_sl_rb( x+17, X1+W1*5, Y1+76, width, height, lenght, Font, Fidx, CORANGE,   CBLACK);


 
      count = ttms;
      x++; if (x > 222) x = 0;
    }
  } while (1); // main do
} // main

// EoF