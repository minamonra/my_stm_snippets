#ifndef DISKIO_H
#define DISKIO_H

#include <stdint.h>

// Типы (совместимые с FatFs)
typedef uint8_t      BYTE;
typedef uint16_t     WORD;
typedef uint32_t     DWORD;
typedef unsigned int UINT;
typedef uint8_t      DSTATUS;
typedef uint8_t      DRESULT;

// Статусы диска
#define STA_NOINIT  0x01
#define STA_NODISK  0x02
#define STA_PROTECT 0x04

// Результаты операций
#define RES_OK     0
#define RES_ERROR  1
#define RES_WRPRT  2
#define RES_NOTRDY 3
#define RES_PARERR 4

// Функции
DSTATUS disk_status(BYTE pdrv);
DSTATUS disk_initialize(BYTE pdrv);
DRESULT disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff);
DWORD   get_fattime(void);

// Команды disk_ioctl
#define CTRL_SYNC        0
#define GET_SECTOR_COUNT 1
#define GET_SECTOR_SIZE  2
#define GET_BLOCK_SIZE   3
#define CTRL_TRIM        4

#endif  // DISKIO_H