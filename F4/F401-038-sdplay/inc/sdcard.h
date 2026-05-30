#ifndef SDCARD_H
#define SDCARD_H

#include "stm32f4xx.h"
#include <stdint.h>


#define R1_READY_STATE      0x00
#define R1_IDLE_STATE       0x01
#define R1_ILLEGAL_COMMAND  0x04

// Команды SD (SPI mode)
#define CMD0   0   // GO_IDLE_STATE
#define CMD8   8   // SEND_IF_COND
#define CMD16  16  // SET_BLOCKLEN
#define CMD17  17  // READ_SINGLE_BLOCK
#define CMD55  55  // APP_CMD
#define ACMD41 41  // APP_SEND_OP_COND

// Функции
uint8_t SD_Init(void);
uint8_t SD_ReadSector(uint32_t sector, uint8_t *buffer);

// Быстрая неблокирующая проверка физического наличия карты на шине
uint8_t SD_CheckPresence(void);

#endif // SDCARD_H