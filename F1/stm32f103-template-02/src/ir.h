#ifndef __IR_H__
#define __IR_H__

#include "stm32f1xx.h"

// Коды кнопок ИК-пульта (NEC)
#define IR_KEY_0 0x16
#define IR_KEY_1 0x0C
#define IR_KEY_2 0x18
#define IR_KEY_3 0x5E
#define IR_KEY_4 0x08
#define IR_KEY_5 0x1C
#define IR_KEY_6 0x5A
#define IR_KEY_7 0x42
#define IR_KEY_8 0x52
#define IR_KEY_9 0x4A

#define IR_KEY_POWER       0x45
#define IR_KEY_MODE        0x46
#define IR_KEY_MUTE        0x47
#define IR_KEY_PLAY_PAUSE  0x44
#define IR_KEY_PREV        0x40
#define IR_KEY_NEXT        0x43
#define IR_KEY_EQ          0x07
#define IR_KEY_VOLUME_DOWN 0x15
#define IR_KEY_VOLUME_UP   0x09
#define IR_KEY_REPEAT      0x19
#define IR_KEY_SCAN        0x0D

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================
// Содержит 8 бит чистой команды пульта NEC, принятой в прерывании
extern volatile uint32_t ik_out;

// ============================================================================
// ФУНКЦИИ ИНИЦИАЛИЗАЦИИ
// ============================================================================
// Инициализация аппаратного декодера TIM2_CH1 на пине PA0
void ir_receiver_init(void);

#endif  // __IR_H__