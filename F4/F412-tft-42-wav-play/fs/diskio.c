#include <stdint.h>
#include "diskio.h"
#include "sdcard.h"

// FatFs Disk Functions

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv) return STA_NOINIT;
    return 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv) return STA_NOINIT;

    static uint8_t sd_already_configured = 0;

    if (!sd_already_configured) {
        sd_init();
        sd_already_configured = 1;
    } else {
        __NOP();
    }

    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
    if (pdrv) return RES_PARERR;
    if (sd_readsectors((uint32_t)sector, buff, (uint32_t)count) != 0) {
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
    (void)pdrv;
    (void)buff;
    (void)sector;
    (void)count;
    return RES_WRPRT;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv) return RES_PARERR;

    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(DWORD*)buff = 4194304;
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 512;   // Было 1 — должно совпадать с размером сектора
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

DWORD get_fattime(void) {
    return ((2026 - 1980) << 25) | (6 << 21) | (19 << 16) | (18 << 11) | (32 << 5) | 0;
}