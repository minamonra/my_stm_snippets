#ifndef DEMO_H
#define DEMO_H

#include <stdint.h>

// --- ГРАФИЧЕСКИЕ ДЕМО-ЭФФЕКТЫ ---
void draw_infinity_vortex(uint16_t iterations);
void draw_chessboard(uint16_t size, uint16_t color1, uint16_t color2);
void draw_random_rects(uint16_t count);
void draw2_random_rects(uint16_t count);
void draw_nested_frames(uint16_t color1, uint16_t color2);
//void draw_dynamic_dashboard(uint16_t duration_sec);
void run_speed_test(void);
void demo_test_fonts(void);

void demo_show_font_FreeSans14(void);
void demo_show_font_FreeMono14(void);
void demo_show_font_FreeMonoBoldOblique16(void);
void demo_show_font_FreeSansBold14(void);


void demo_show_u8g2_font1(void);
void demo_show_u8g2_font1_dense(void);
void demo_show_u8g2_font1_maxdense(void);
#endif // DEMO_H