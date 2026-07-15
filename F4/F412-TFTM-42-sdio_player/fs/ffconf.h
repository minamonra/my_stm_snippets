/*---------------------------------------------------------------------------/
/  FatFs - Configuration file
/---------------------------------------------------------------------------*/

#define FFCONF_DEF 89352	/* Revision ID */

/*---------------------------------------------------------------------------/
/ Function Configurations
/---------------------------------------------------------------------------*/

#define FF_FS_READONLY	1    // Оставить 1 — только чтение
#define FF_FS_MINIMIZE	0    // 0 — все функции доступны (включая f_opendir/f_readdir)
#define FF_USE_STRFUNC	1    // Оставить 1
#define FF_USE_FIND		0    // 0 — не нужно
#define FF_USE_MKFS		0    // 0
#define FF_USE_FASTSEEK	1    // Оставить 1
#define FF_USE_EXPAND	0    // 0
#define FF_USE_CHMOD	0    // 0
#define FF_USE_LABEL	0    // 0
#define FF_USE_FORWARD	0    // 0

/*---------------------------------------------------------------------------/
/ Locale and Namespace Configurations
/---------------------------------------------------------------------------*/

#define FF_CODE_PAGE	437    // Поменять на 437 (US English) для теста
// #define FF_CODE_PAGE	862  // Закомментировать Hebrew на время теста

#define FF_USE_LFN		1    // Поменять на 1 (статический буфер в BSS)
#define FF_MAX_LFN		255
/* FF_USE_LFN:
/   0: Disable LFN
/   1: Enable LFN with static working buffer on the BSS — НЕ на стеке, не на куче
/   2: Enable LFN with dynamic working buffer on the STACK
/   3: Enable LFN with dynamic working buffer on the HEAP
*/

#define FF_LFN_UNICODE	0    // 0 = ANSI/OEM
#define FF_LFN_BUF		255
#define FF_SFN_BUF		12
#define FF_STRF_ENCODE	3

#define FF_FS_RPATH		0

/*---------------------------------------------------------------------------/
/ Drive/Volume Configurations
/---------------------------------------------------------------------------*/

#define FF_VOLUMES		1
#define FF_STR_VOLUME_ID	0
#define FF_VOLUME_STRS		"RAM","NAND","CF","SD","SD2","USB","USB2","USB3"
#define FF_MULTI_PARTITION	0

#define FF_MIN_SS		512
#define FF_MAX_SS		512

#define FF_USE_TRIM		0
#define FF_FS_NOFSINFO	0

/*---------------------------------------------------------------------------/
/ System Configurations
/---------------------------------------------------------------------------*/

#define FF_FS_TINY		0    // 0 = Normal (свой буфер на файл)
#define FF_FS_EXFAT		1    // 0 — exFAT не нужен
#define FF_FS_NORTC		0
#define FF_NORTC_MON	1
#define FF_NORTC_MDAY	1
#define FF_NORTC_YEAR	2017
#define FF_FS_LOCK		0
#define FF_FS_REENTRANT	0
#define FF_FS_TIMEOUT	1000
#define FF_SYNC_t		HANDLE

/*--- End of configuration options ---*/