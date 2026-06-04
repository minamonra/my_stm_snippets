#include <stddef.h>
#include "common.h"
#include "ili9488x.h"
#include "lcd_font.h"
#include "demo.h"
#include "lcd_draw_u8g2.h"
#include "lcd_io.h"
//#include "FreeMonoBoldOblique16.h"
//extern const GFXfont FreeMonoBoldOblique16pt8b; // Просто ссылка, память не дублируется!








int main(void) {
    System_Init();
    ili9488_Init();
    
    for (int i = 0; i < 3; i++) {
        GPIOB->BSRR = GPIO_BSRR_BS_2; delay_ms(100);
        GPIOB->BSRR = GPIO_BSRR_BR_2; delay_ms(100);
    }
    
     while (1) {
        GPIOB->BSRR = GPIO_BSRR_BS_2; delay_ms(15);
        GPIOB->BSRR = GPIO_BSRR_BR_2; delay_ms(15);
        //draw_dynamic_dashboard(6);                delay_ms(2500);
demo_show_u8g2_font1();  delay_ms(400000);
demo_show_u8g2_font1_dense(); delay_ms(4000);
demo_show_u8g2_font1_maxdense(); delay_ms(4000);
        
//        test_u8g2_rle_perfect(); // Запуск нашего теста RLE
//delay_ms(100400);
//         demo_show_font_FreeSansBold14();delay_ms(4000);
//         demo_show_font_FreeSans14(); delay_ms(4000);
//         demo_show_font_FreeMono14(); delay_ms(4000);
//         demo_show_font_FreeMonoBoldOblique16(); delay_ms(4000);


//demo_show_u8g2_font(); delay_ms(4000);

        //demo_test_fonts(); delay_ms(4000);
        //draw_chessboard(20, ILI9488_BLACK, ILI9488_YELLOW);   delay_ms(500);
        //draw_chessboard(30, ILI9488_BLACK, ILI9488_RED);      delay_ms(500);
        //draw_chessboard(40, ILI9488_GREEN, ILI9488_RED);      delay_ms(500);
//        draw_chessboard(50, ILI9488_GREEN, ILI9488_BLUE);     delay_ms(500);
        
        
        //run_speed_test();          delay_ms(1500); // Запуск теста скорости!

    }
}

        // 2. Крупная шахматка
        //draw_chessboard(40, 0xF800, 0x0000);      delay_ms(1500);

        // 3. Вызов перенесенной функции дешборда (Линкер теперь свяжет адрес идеально!)
        
        // 4. Остальные железные тесты
        //draw_infinity_vortex(12);                 delay_ms(500);  
        //draw_chessboard(20, 0x001F, 0xFFFF);      delay_ms(1500); 
        
        //ili9488_FillRect(0, 0, 480, 320, 0xFFFF); delay_ms(200);  
        //draw2_random_rects(550);                  delay_ms(1000); 

        //ili9488_FillRect(0, 0, 480, 320, 0xFFFF); delay_ms(200);  
        //draw2_random_rects(550);                  delay_ms(1000); 