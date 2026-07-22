#include "sdcard.h"
#include "common.h"
#include <stddef.h>
#include "sdcard_dma.h"
#include "uart.h"

// Глобальные переменные состояния SD карты
SD_CardInfo_t sd_card_info = {0};
uint32_t      SD_Card_RCA  = 0;

// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
static void SDIO_ClearFlags(void) {
  SDIO->ICR = 0xFFFFFFFF;
}

static uint8_t SDIO_WaitFlag(uint32_t flag, uint32_t timeout_ms) {
  uint32_t start = ttms;
  while (!(SDIO->STA & flag)) {
    if ((ttms - start) > timeout_ms) {
      return 1;
    }
  }
  return 0;
}

static uint8_t SDIO_WaitCmdReady(uint32_t timeout_cycles) {
  uint32_t timeout = 0;
  while (!(SDIO->STA & SDIO_STA_CMDREND)) {
    if (++timeout > timeout_cycles) {
      if (SDIO->STA & SDIO_STA_CTIMEOUT) {
        SDIO->ICR = SDIO_ICR_CTIMEOUTC;
        return 2;
      }
      return 1;
    }
  }
  return 0;
}

static uint8_t SDIO_WaitCmdSent(uint32_t timeout_cycles) {
  uint32_t timeout = 0;
  while (!(SDIO->STA & SDIO_STA_CMDSENT)) {
    if (++timeout > timeout_cycles) {
      return 1;
    }
  }
  return 0;
}

static uint8_t SDIO_WaitCmdActive(uint32_t timeout_cycles) {
  uint32_t timeout = 0;
  while (SDIO->STA & SDIO_STA_CMDACT) {
    if (++timeout > timeout_cycles) {
      return 1;
    }
  }
  return 0;
}

uint8_t sdio_sendcommand(uint32_t cmd, uint32_t arg, uint32_t response_type, uint8_t check_crc) {
  uint8_t status;

  SDIO_ClearFlags();

  status = SDIO_WaitCmdActive(100000);
  if (status) return 1;

  SDIO->ARG = arg;
  SDIO->CMD = cmd | response_type | SDIO_CMD_CPSMEN;
  __DSB();

  if (response_type != 0) {
    status = SDIO_WaitCmdReady(100000);
    if (status) return status;
  } else {
    status = SDIO_WaitCmdSent(100000);
    if (status) return 1;
  }

  if (check_crc && (SDIO->STA & SDIO_STA_CCRCFAIL)) {
    SDIO->ICR = SDIO_ICR_CCRCFAILC;
    return 3;
  }

  return 0;
}

uint8_t sdio_getresponseR1(void) {
  if (SDIO_WaitFlag(SDIO_STA_CMDREND, 100)) {
    return 0xFF;
  }
  return (uint8_t)(SDIO->RESP1 & 0xFF);
}

void sdio_initpins(void) {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN;
  __DSB();

  RCC->APB2ENR |= RCC_APB2ENR_SDIOEN;
  __DSB();

  delay_ms(10);

  GPIOC->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9 | GPIO_MODER_MODER10 |
                    GPIO_MODER_MODER11 | GPIO_MODER_MODER12);
  GPIOC->MODER |= (GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1 |
                   GPIO_MODER_MODER11_1 | GPIO_MODER_MODER12_1);

  GPIOC->OSPEEDR |= (GPIO_OSPEEDR_OSPEED8 | GPIO_OSPEEDR_OSPEED9 | GPIO_OSPEEDR_OSPEED10 |
                     GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12);

  GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR8 | GPIO_PUPDR_PUPDR9 | GPIO_PUPDR_PUPDR10 |
                    GPIO_PUPDR_PUPDR11 | GPIO_PUPDR_PUPDR12);
  GPIOC->PUPDR |= (GPIO_PUPDR_PUPDR8_0 | GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR10_0 |
                   GPIO_PUPDR_PUPDR11_0 | GPIO_PUPDR_PUPDR12_0);

  GPIOC->AFR[1] &= ~((0xF << 0) | (0xF << 4) | (0xF << 8) | (0xF << 12) | (0xF << 16));
  GPIOC->AFR[1] |= ((12 << 0) | (12 << 4) | (12 << 8) | (12 << 12) | (12 << 16));

  GPIOD->MODER &= ~GPIO_MODER_MODER2;
  GPIOD->MODER |= GPIO_MODER_MODER2_1;
  GPIOD->OSPEEDR |= GPIO_OSPEEDR_OSPEED2;
  GPIOD->PUPDR &= ~GPIO_PUPDR_PUPDR2;
  GPIOD->PUPDR |= GPIO_PUPDR_PUPDR2_0;
  GPIOD->AFR[0] &= ~(0xF << 8);
  GPIOD->AFR[0] |= (12 << 8);

  SDIO->POWER = 0x03;
  delay_ms(20);

  SDIO->CLKCR = (118 << 0) |
                (0 << 8)  |
                (0 << 9)  |
                (0 << 10) |
                (0 << 11) |
                (1 << 13) |
                (0 << 14);

  SDIO->CLKCR |= SDIO_CLKCR_CLKEN;
  __DSB();
  delay_ms(20);

  SDIO->ICR = 0xFFFFFFFF;
}

void sdio_setclockdivider(uint8_t divider) {
  SDIO->CLKCR &= ~SDIO_CLKCR_CLKEN;
  __DSB();
  delay_ms(2);

  SDIO->CLKCR &= ~SDIO_CLKCR_CLKDIV;
  SDIO->CLKCR |= (divider << 0);

  SDIO->CLKCR |= SDIO_CLKCR_CLKEN;
  __DSB();
  delay_ms(5);
}

static uint8_t sd_cmd0_go_idle(void) {
  uint32_t timeout = 0;

  SDIO->ARG = 0x00000000;
  SDIO->CMD = (0 << 0) | (0 << 6) | SDIO_CMD_CPSMEN;
  __DSB();

  while (!(SDIO->STA & SDIO_STA_CMDSENT)) {
    if (++timeout > 800000) {
      SDIO->ICR = 0xFFFFFFFF;
      return 0x01;
    }
  }
  SDIO->ICR = SDIO_ICR_CMDSENTC;
  delay_ms(30);

  return 0;
}

static uint8_t sd_cmd8_send_if_cond(void) {
  uint32_t timeout = 0;
  uint32_t resp;

  SDIO->ICR = 0xFFFFFFFF;
  __DSB();
  delay_ms(10);

  SDIO->ARG = 0x000001AA;
  SDIO->CMD = (8 << 0) | (1 << 6) | SDIO_CMD_CPSMEN;
  __DSB();

  while (!(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))) {
    if (++timeout > 800000) {
      return 0x02;
    }
  }

  if (SDIO->STA & SDIO_STA_CTIMEOUT) {
    SDIO->ICR = SDIO_ICR_CTIMEOUTC;
    return 0x00;
  }

  if (SDIO->STA & SDIO_STA_CCRCFAIL) {
    SDIO->ICR = SDIO_ICR_CCRCFAILC;
    return 0x03;
  }

  resp = SDIO->RESP1;
  if ((resp & 0xFF) != 0xAA) {
    SDIO->ICR = 0xFFFFFFFF;
    return 0x05;
  }

  SDIO->ICR = 0xFFFFFFFF;
  delay_ms(20);
  return 0;
}

static uint8_t sd_acmd41_send_op_cond(void) {
  uint32_t retry = 0;
  uint32_t resp;

  while (retry < 600) {
    SDIO->ICR = 0xFFFFFFFF;
    __DSB();

    SDIO->ARG = 0x00000000;
    SDIO->CMD = (55 << 0) | (1 << 6) | SDIO_CMD_CPSMEN;
    __DSB();

    uint32_t sub_retry = 0;
    while (!(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))) {
      if (++sub_retry > 300000) break;
    }

    if (SDIO->STA & SDIO_STA_CTIMEOUT) {
      SDIO->ICR = SDIO_ICR_CTIMEOUTC;
      delay_ms(10);
      retry++;
      continue;
    }

    if (SDIO->STA & SDIO_STA_CCRCFAIL) {
      SDIO->ICR = SDIO_ICR_CCRCFAILC;
      delay_ms(10);
      retry++;
      continue;
    }

    SDIO->ICR = 0xFFFFFFFF;
    __DSB();

    SDIO->ARG = 0x40FF8000;
    SDIO->CMD = (41 << 0) | (1 << 6) | SDIO_CMD_CPSMEN;
    __DSB();

    sub_retry = 0;
    while (!(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT))) {
      if (++sub_retry > 300000) break;
    }

    if (SDIO->STA & SDIO_STA_CTIMEOUT) {
      SDIO->ICR = SDIO_ICR_CTIMEOUTC;
      delay_ms(10);
      retry++;
      continue;
    }

    resp = SDIO->RESP1;
    if (resp & 0x80000000) {
      sd_card_info.card_type = (resp & 0x40000000) ? 1 : 0;
      SDIO->ICR = 0xFFFFFFFF;
      return 0;
    }

    SDIO->ICR = 0xFFFFFFFF;
    delay_ms(10);
    retry++;
  }

  return 0x06;
}

static uint8_t sd_cmd2_get_cid(void) {
  uint32_t timeout = 0;

  SDIO->ICR = 0xFFFFFFFF;
  SDIO->ARG = 0x00000000;
  SDIO->CMD = (2 << 0) | (3 << 6) | SDIO_CMD_CPSMEN;
  __DSB();

  while (!(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT))) {
    if (++timeout > 800000) return 0x12;
  }

  if (SDIO->STA & SDIO_STA_CTIMEOUT) {
    SDIO->ICR = SDIO_ICR_CTIMEOUTC;
    return 0x13;
  }

  sd_card_info.cid[0] = SDIO->RESP1;
  sd_card_info.cid[1] = SDIO->RESP2;
  sd_card_info.cid[2] = SDIO->RESP3;
  sd_card_info.cid[3] = SDIO->RESP4;

  SDIO->ICR = 0xFFFFFFFF;
  delay_ms(10);
  return 0;
}

static uint8_t sd_cmd3_get_rca(void) {
  SDIO->ICR = 0xFFFFFFFF;
  uint8_t status = sdio_sendcommand(3, 0, (1 << 6), 1);
  if (status) return 0x22 | (status << 4);

  SD_Card_RCA = SDIO->RESP1 & 0xFFFF0000;
  SDIO->ICR = 0xFFFFFFFF;
  delay_ms(10);
  return 0;
}

static uint8_t sd_cmd7_select_card(void) {
  SDIO->ICR = 0xFFFFFFFF;
  uint8_t status = sdio_sendcommand(7, SD_Card_RCA, (1 << 6), 1);
  if (status) return 0x32 | (status << 4);

  SDIO->ICR = 0xFFFFFFFF;
  delay_ms(10);
  return 0;
}

static uint8_t sd_cmd16_set_blocklen(void) {
  SDIO->ICR = 0xFFFFFFFF;
  uint8_t status = sdio_sendcommand(16, 512, (1 << 6), 1);
  if (status) return 0x52 | (status << 4);

  SDIO->ICR = 0xFFFFFFFF;
  delay_ms(10);
  return 0;
}

static uint8_t sd_set_4bit_mode(void) {
  uint8_t status = sdio_sendcommand(55, SD_Card_RCA, (1 << 6), 1);
  if (status) return 0x40 | (status << 4);

  status = sdio_sendcommand(6, 0x00000002, (1 << 6), 1);
  if (status) return 0x44 | (status << 4);

  SDIO->ICR = 0xFFFFFFFF;
  delay_ms(10);
  return 0;
}

static void sd_enable_high_speed(void) {
  if (sd_card_info.card_type != 1) return;

  SDIO->ICR = 0xFFFFFFFF;
  sdio_sendcommand(6, 0x80FFFFEF, (1 << 6), 1);
  SDIO->ICR = 0xFFFFFFFF;
  delay_ms(10);
}

static void sd_configure_speed(void) {
  // 1. Отключаем тактирование шины для безопасного изменения конфигурации
  SDIO->CLKCR &= ~SDIO_CLKCR_CLKEN;
  __DSB();
  delay_ms(1);

  // 2. Настраиваем разрядность шины данных: переключаем в 4-битный режим
  SDIO->CLKCR &= ~SDIO_CLKCR_WIDBUS;
  SDIO->CLKCR |= SDIO_CLKCR_WIDBUS_0;

  // 3. Включаем аппаратный контроль потока (Hardware Flow Control) для предотвращения FIFO Overrun/Underrun
  SDIO->CLKCR |= SDIO_CLKCR_HWFC_EN;

  // 4. Максимальный разгон: отключаем делитель частоты и включаем режим BYPASS.
  // Тактовая частота шины становится равной частоте ядра SDIOCLK (48 МГц).
  // Потенциальные микрозадержки exFAT-кластеризации полностью нивелируются
  // за счет увеличенного до 4096 фреймов AUDIO_CHUNK_SIZE в audio_i2s.h.
  SDIO->CLKCR &= ~SDIO_CLKCR_CLKDIV;
  SDIO->CLKCR |= SDIO_CLKCR_BYPASS;

  // 5. Включаем тактирование шины обратно и ждем стабилизации линии
  SDIO->CLKCR |= SDIO_CLKCR_CLKEN;
  __DSB();
  delay_ms(1);
}

uint8_t sd_init(void) {
  uint8_t status;

  if (!(SDIO->CLKCR & SDIO_CLKCR_CLKEN)) {
    return 0x99;
  }

  SDIO->CMD = 0;
  delay_ms(30);
  SDIO->ICR = 0xFFFFFFFF;
  __DSB();

  status = sd_cmd0_go_idle();
  if (status) return status;

  status = sd_cmd8_send_if_cond();
  if (status && status != 0x00) return status;

  status = sd_acmd41_send_op_cond();
  if (status) return status;

  status = sd_cmd2_get_cid();
  if (status) return status;

  status = sd_cmd3_get_rca();
  if (status) return status;

  status = sd_cmd7_select_card();
  if (status) return status;

  status = sd_cmd16_set_blocklen();
  if (status) return status;

  status = sd_set_4bit_mode();
  if (status) return status;

  sd_enable_high_speed();

  sd_configure_speed();

  sd_card_info.is_initialized = 1;
  return 0;
}

#define SDIO_DTIMER_VALUE (24000000 / 4)
#define CMD_TIMEOUT_CYCLES  800000
#define DATA_TIMEOUT_CYCLES 500000

static uint8_t sd_wait_cmd_response(uint32_t response_mask) {
    uint32_t timeout = 0;

    while (!(SDIO->STA & (response_mask | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))) {
        if (++timeout > CMD_TIMEOUT_CYCLES) {
            SDIO->DCTRL = 0;
            return 0x11;
        }
    }

    if (SDIO->STA & SDIO_STA_CTIMEOUT) {
        SDIO->ICR = SDIO_ICR_CTIMEOUTC;
        SDIO->DCTRL = 0;
        return 0x12;
    }

    SDIO->ICR = 0x000007FF;
    __DSB();
    return 0;
}



static uint8_t sd_wait_dma_and_dataend(void) {
    uint32_t timeout = 0;

    // Активное ожидание без сна
    while (!(SDIO->STA & SDIO_STA_DATAEND)) {
        if (SDIO->STA & (SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL)) {
            uint8_t err = (SDIO->STA & SDIO_STA_DCRCFAIL) ? 0x15 : 0x16;
            sd_dma_abort();
            SDIO->ICR = 0xFFFFFFFF;
            SDIO->DCTRL = 0;
            return err;
        }

        if (++timeout > DATA_TIMEOUT_CYCLES) {
            sd_dma_abort();
            SDIO->DCTRL = 0;
            return 0x14;
        }
    }

    // DATAEND пришёл — ждём ещё чуть-чуть чтобы DMA точно добил последнее слово
    for (volatile int i = 0; i < 100; i++) __NOP();

    return 0;
}


// Отправка CMD17 (чтение одного сектора) с настройкой SDIO на DMA
static uint8_t sd_send_cmd17(uint32_t sector) {
    SDIO->ICR = 0xFFFFFFFF;
    __DSB();

    SDIO->DTIMER = SDIO_DTIMER_VALUE;
    SDIO->DLEN = 512;

    // Включаем чтение, размер блока 512 байт, включаем передачу данных и DMAEN
    SDIO->DCTRL = SDIO_DCTRL_DTDIR | (9 << 4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;
    __DSB();

    uint32_t arg = (sd_card_info.card_type == 0) ? sector * 512 : sector;
    SDIO->ARG = arg;
    SDIO->CMD = (17 << 0) | (1 << 6) | SDIO_CMD_CPSMEN;
    __DSB();

    return sd_wait_cmd_response(SDIO_STA_CMDREND);
}

// Чтение одного сектора через DMA
// Чтение одного сектора через DMA
uint8_t sd_readsector(uint32_t sector, uint8_t* buffer) {
    if (!sd_card_info.is_initialized) return 0xFF;
    if ((uint32_t)buffer & 0x03) return 0xFE;

    sd_dma_prepare_read(buffer);

    uint8_t status = sd_send_cmd17(sector);
    if (status) {
        sd_dma_abort();
        return status;
    }

    status = sd_wait_dma_and_dataend();
    if (status) return status;

    SDIO->ICR = 0xFFFFFFFF;
    SDIO->DCTRL = 0;
    __DSB();
    return 0;
}

// Чтение секторов через DMA (CMD18)
uint8_t sd_readsectors(uint32_t sector, uint8_t* buffer, uint32_t count) {
    if (!sd_card_info.is_initialized) return 0xFF;
    if ((uint32_t)buffer & 0x03) return 0xFE;
    if (count == 0) return 0;
    if (count == 1) return sd_readsector(sector, buffer);

    // Настройка DMA
    DMA2_Stream3->CR = 0;
    while (DMA2_Stream3->CR & DMA_SxCR_EN);
    DMA2->LIFCR = (0x3DUL << 22);

    DMA2_Stream3->M0AR = (uint32_t)buffer;
    DMA2_Stream3->NDTR = (512 * count) / 4;
    sd_dma_transfer_done = 0;

    DMA2_Stream3->CR = (4U << DMA_SxCR_CHSEL_Pos)
                     | (1U << DMA_SxCR_MBURST_Pos)
                     | (1U << DMA_SxCR_PBURST_Pos)
                     | (0U << DMA_SxCR_PL_Pos)
                     | (2U << DMA_SxCR_MSIZE_Pos)
                     | (2U << DMA_SxCR_PSIZE_Pos)
                     | DMA_SxCR_MINC
                     | (0U << DMA_SxCR_DIR_Pos)
                     | DMA_SxCR_PFCTRL
                     | DMA_SxCR_TCIE;
    DMA2_Stream3->CR |= DMA_SxCR_EN;

    SDIO->ICR = 0xFFFFFFFF;
    __DSB();
    SDIO->DTIMER = SDIO_DTIMER_VALUE * count;
    SDIO->DLEN = 512 * count;
    SDIO->DCTRL = SDIO_DCTRL_DTDIR | (9 << 4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;
    __DSB();

    uint32_t arg = (sd_card_info.card_type == 0) ? sector * 512 : sector;
    SDIO->ARG = arg;
    SDIO->CMD = (18 << 0) | (1 << 6) | SDIO_CMD_CPSMEN;
    __DSB();

    uint8_t status = sd_wait_cmd_response(SDIO_STA_CMDREND);
    if (status) {
        DMA2_Stream3->CR = 0;
        SDIO->DCTRL = 0;
        return status;
    }

    status = sd_wait_dma_and_dataend();

    SDIO->ICR = 0xFFFFFFFF;
    SDIO->ARG = 0;
    SDIO->CMD = (12 << 0) | (1 << 6) | SDIO_CMD_CPSMEN;
    __DSB();
    sd_wait_cmd_response(SDIO_STA_CMDREND);

    SDIO->ICR = 0xFFFFFFFF;
    SDIO->DCTRL = 0;
    __DSB();

    return status;
}


void sd_deinit(void) {
  SDIO->CLKCR &= ~SDIO_CLKCR_CLKEN;
  SDIO->POWER = 0;

  RCC->APB2RSTR |= RCC_APB2RSTR_SDIORST;
  __DSB();
  RCC->APB2RSTR &= ~RCC_APB2RSTR_SDIORST;
  __DSB();

  sd_card_info.is_initialized = 0;
}

uint8_t sd_writesector(uint32_t sector, const uint8_t* buffer) {
  (void)sector;
  (void)buffer;
  return 0;
}

uint8_t sd_is_inserted(void) {
  return SD_CARD_DETECT();
}

uint8_t sd_is_initialized(void) {
    return sd_card_info.is_initialized;
}

void sdio_printstatus(void) {}


