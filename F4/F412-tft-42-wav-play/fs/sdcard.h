#pragma once
#ifndef SDCARD_H
#define SDCARD_H

#include "stm32f4xx.h"
#include <stdint.h>

// Аппаратные пины SD карты (SDIO)
#define SDIO_D0_PIN  (1U << 8)   // PC8
#define SDIO_D1_PIN  (1U << 9)   // PC9
#define SDIO_D2_PIN  (1U << 10)  // PC10
#define SDIO_D3_PIN  (1U << 11)  // PC11
#define SDIO_CLK_PIN (1U << 12)  // PC12
#define SDIO_CMD_PIN (1U << 2)   // PD2

// Проверка физического наличия карты по подтяжке линии D3
// D3 = 0 (LOW)  -> карта вставлена
// D3 = 1 (HIGH) -> карты нет
#define SD_CARD_DETECT() (!(GPIOC->IDR & SDIO_D3_PIN))

// СТАТУСЫ ОТВЕТОВ SD
#define SD_R1_READY_STATE     0x00
#define SD_R1_IDLE_STATE      0x01
#define SD_R1_ILLEGAL_COMMAND 0x04

// КОМАНДЫ SD (SDIO режим)
#define SD_CMD0   0   // GO_IDLE_STATE
#define SD_CMD2   2   // ALL_SEND_CID
#define SD_CMD3   3   // SEND_RELATIVE_ADDR
#define SD_CMD7   7   // SELECT_DESELECT_CARD
#define SD_CMD8   8   // SEND_IF_COND
#define SD_CMD9   9   // SEND_CSD
#define SD_CMD10  10  // SEND_CID
#define SD_CMD12  12  // STOP_TRANSMISSION
#define SD_CMD13  13  // SEND_STATUS
#define SD_CMD16  16  // SET_BLOCKLEN
#define SD_CMD17  17  // READ_SINGLE_BLOCK
#define SD_CMD18  18  // READ_MULTIPLE_BLOCK
#define SD_CMD24  24  // WRITE_BLOCK
#define SD_CMD25  25  // WRITE_MULTIPLE_BLOCK
#define SD_CMD55  55  // APP_CMD
#define SD_ACMD41 41  // APP_SEND_OP_COND

// СТРУКТУРА ДАННЫХ КАРТЫ
typedef struct {
  uint32_t rca;
  uint32_t cid[4];
  uint32_t csd[4];
  uint8_t  card_type;  // 0 - SDSC, 1 - SDHC/SDXC
  uint8_t  is_initialized;
} SD_CardInfo_t;

extern SD_CardInfo_t sd_card_info;
extern uint32_t      SD_Card_RCA;

// ФУНКЦИИ SDIO ДРАЙВЕРА
uint8_t sd_init(void);
uint8_t sd_readsector(uint32_t sector, uint8_t* buffer);
uint8_t sd_readsectors(uint32_t sector, uint8_t* buffer, uint32_t count);
uint8_t sd_writesector(uint32_t sector, const uint8_t* buffer);
void    sd_deinit(void);
void    sdio_initpins(void);
void    sdio_setclockdivider(uint8_t divider);

// Быстрая проверка: вставлена ли карта?
uint8_t sd_is_inserted(void);
// Полная проверка: можно ли работать с картой?
uint8_t sd_is_initialized(void);

#endif  // SDCARD_H
