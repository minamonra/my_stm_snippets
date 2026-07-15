#ifndef TM1637_H
#define TM1637_H

#include <stdint.h>
#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TM1637_CLK_PIN    (1U << 9)
#define TM1637_DIO_PIN    (1U << 8)
#define TM1637_CLK_HIGH() GPIOB->BSRR = TM1637_CLK_PIN
#define TM1637_CLK_LOW()  GPIOB->BRR = TM1637_CLK_PIN
#define TM1637_DIO_HIGH() GPIOB->BSRR = TM1637_DIO_PIN
#define TM1637_DIO_LOW()  GPIOB->BRR = TM1637_DIO_PIN
#define TM1637_DIO_READ() ((GPIOB->IDR & TM1637_DIO_PIN) != 0)

// Инициализация
void tm1637_init(void);
void tm1637_set_brightness(uint8_t brightness);
void tm1637_clear(void);

// Отображение
void tm1637_display_dec(uint16_t number, uint8_t showColon);
void tm1637_display_int(int16_t number, uint8_t showColon);
void tm1637_display_str(const char* str);
void tm1637_display_seg(const uint8_t* segments);
void tm1637_display_float(float number, uint8_t decimal_places);

// Управление сегментами
void    tm1637_set_segment(uint8_t position, uint8_t value);
uint8_t tm1637_get_segment(uint8_t position);
void    tm1637_set_colon(uint8_t position, uint8_t enabled);
void    tm1637_set_dot(uint8_t position, uint8_t enabled);

#ifdef __cplusplus
}
#endif

#endif  // TM1637_H