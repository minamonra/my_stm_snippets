#ifndef ENC_H
#define ENC_H

#include "stm32f103xb.h"
#include <stdint.h>

// Твои оригинальные серийные коды Грея
#define ENCUPCNT  0x02  // Код шага вправо
#define ENCDNCNT  0x01  // Код шага влево

// Макросы быстрого чтения физического уровня пинов PB12 и PB13
#define ENC_A_READ() ((GPIOB->IDR & GPIO_IDR_IDR12) ? 1 : 0)
#define ENC_B_READ() ((GPIOB->IDR & GPIO_IDR_IDR13) ? 1 : 0)

void encoder_init(void);
void encoder_process(void); // Неблокирующий опрос для вызова в while(1)

#endif // ENC_H
