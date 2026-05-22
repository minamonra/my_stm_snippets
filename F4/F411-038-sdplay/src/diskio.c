#include <stdint.h>   
#include "diskio.h"   
#include "ff.h"       
#include "sdcard.h"   

// СТРОГО: Все упоминания DMA2 и флагов блокировок SPI дисплея полностью удалены!
// Теперь FatFs работает по своим программным линиям, никак не затрагивая видеоядро.
DRESULT disk_read(uint8_t pdrv, uint8_t* buff, uint32_t sector, uint16_t count) {
    if (pdrv) return RES_PARERR;
    
    // Программный ногодрыг последовательно и безопасно вычитывает секторы FatFs
    for (; count > 0; count--) {
        if (SD_ReadSector(sector, buff) != 0) {
            return RES_ERROR;
        }
        sector++;
        buff += 512;
    }
    return RES_OK;
}

// Заглушки для остальных обязательных системных функций FatFs
DSTATUS disk_status(BYTE pdrv) { return pdrv ? STA_NOINIT : 0; }
DSTATUS disk_initialize(BYTE pdrv) { return pdrv ? STA_NOINIT : 0; }
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) { return RES_OK; }
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) { return RES_OK; }