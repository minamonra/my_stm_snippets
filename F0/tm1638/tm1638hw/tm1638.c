#include "tm1638.h"
#include "code_tab.h"

#define CS_SET GPIOB->BSRR |= GPIO_BSRR_BS_4 // GPIOB->ODR &= ~GPIO_ODR_4;
#define CS_CLR GPIOB->BSRR |= GPIO_BSRR_BR_4 // GPIOB->ODR |= GPIO_ODR_4;

// PB3 - SPI1_SCK (clock)
// PB4 - CS (chip select)
// PB5 - SPI1_MOSI (data)

#define SPI1_DR_8bit  (*(__IO uint8_t *)((uint32_t)&(SPI1->DR)))
#define EMPTYCODE  17

struct 
{
  uint8_t  tens, hundreds, thousands;
  uint16_t units;
} bcd4x;


void spi_init(void)
{
  RCC->AHBENR   |= RCC_AHBENR_GPIOBEN;
  // SCK  PB3
  GPIOB->MODER |= GPIO_MODER_MODER3_1;  // alternate function
  // GPIOB->OTYPER default push-pull
  // GPIOB->AFR[] default AF 0: SPI1_SCK,
  
  // MOSI  PB5
  GPIOB->MODER |= GPIO_MODER_MODER5_1;  // alternate function
  
  // soft nCS   PB4
  GPIOB->MODER |= GPIO_MODER_MODER4_0; // PB4 as output
  GPIOB->OSPEEDR  |=  ( GPIO_OSPEEDER_OSPEEDR4_0);
  CS_SET; //GPIOB->ODR = GPIO_ODR_4; // deselect


  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  // SPI configuration
  SPI1->CR1 |= SPI_CR1_LSBFIRST; // !! Frame Format младшим битов вперед (little endian)
  SPI1->CR1 |= SPI_CR1_MSTR; // бит мастера (SPI_CR1_MSTR)
  SPI1->CR1 |= SPI_CR1_BR;   // SPI_CR1_BR_2; // spi_sck = SystemCoreClock /
  SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI; // software slave CS management & internal slave select
  SPI1->CR2 |= SPI_CR2_FRXTH; // бит наличия данных в очереди на четверть длины - 8 бит (SPI_CR2_FRXTH)
  SPI1->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; //8-bit
  SPI1->CR1 |= SPI_CR1_SPE; // SPI вкл
}

uint8_t spi_send_byte(uint8_t data)
{
  while (!(SPI1->SR & SPI_SR_TXE)); // TX buffer is empty
  SPI1_DR_8bit = data;
  while (!(SPI1->SR & SPI_SR_RXNE)); // while (!(SPI1->SR & SPI_SR_BSY)); //
  return SPI1_DR_8bit;
}

uint8_t tm1638_get_key(void)
{
  uint8_t c[4], i, key_value = 0;
  CS_CLR;
  spi_send_byte(0x42);
  SPI1->CR1 |= SPI_CR1_BIDIMODE;  // вкл bi-directional mode
  for (i = 0; i < 4; i++)
    key_value |= spi_send_byte(0xFF) << i;
  CS_SET;
  SPI1->CR1 &= ~SPI_CR1_BIDIMODE; // выкл bi-directional mode
  return key_value;
}

static void write_cmd(uint8_t cmd)
{
  CS_CLR;
  spi_send_byte(cmd);
  CS_SET;
}

static void write_data(uint8_t add, uint8_t data)
{
  write_cmd(0x44);
  CS_CLR;
  spi_send_byte(0xc0 | add);
  spi_send_byte(data);
  CS_SET;
}

void tm1638_write_led(uint8_t led_flag)
{
  unsigned char i;
  for (i = 0; i < 8; i++) {
    if (led_flag & (1 << i))
      write_data(2 * i + 1, 1);
    else
      write_data(2 * i + 1, 0);
  }
}

void tm1638_seg(uint8_t seg_num, uint8_t data, uint8_t dot) // seg_num 0-7; ключить точку 1
{
  write_cmd(0x44);
  CS_CLR;
  spi_send_byte(0xc0 + seg_num * 2); // 0,2,4,6,8,10....
  spi_send_byte(code_tab[data] | (dot ? 0x80 : 0));
  CS_SET;
}

void tm1638_init(void)
{
  write_cmd(0x8E);
  write_cmd(0x44);
  CS_CLR;
  spi_send_byte(0xc0);
  for (uint8_t i = 0; i < 16; i++)
    spi_send_byte(0x00);
  CS_SET;
}

// 0-all; 1-segments; 2-led
void tm1638_clear(uint8_t par)
{
  uint8_t i = 0;
  CS_CLR;
  if (!par)
    spi_send_byte(0x40);
  else
    spi_send_byte(0x44);
  CS_SET;

  switch (par) {
  case 0: // all
    CS_CLR;
    spi_send_byte(0xc0);
    for (; i < 16; i++)
      spi_send_byte(0);
    CS_SET;
    break;
  case 1: // segments
    for (; i < 8; i++) {
      CS_CLR;
      spi_send_byte(0xc0 + i * 2);
      spi_send_byte(0);
      CS_SET;
    }
    break;
  case 2: // led
    for (; i < 8; i++) {
      CS_CLR;
      spi_send_byte(0xc1 + i * 2);
      spi_send_byte(0);
      CS_SET;
    }
    break;
  }
}

void tm1638_initc(uint8_t bright)
{
  CS_CLR;
  spi_send_byte(0x88 | bright); //0x88 включаем режим команд и сам дисплей
  CS_SET;
  tm1638_clear(0);
}

void tm1638_tube_dip(uint16_t pos, uint16_t data)
{
  write_data(pos * 2, code_tab[data]);
  tm1638_write_led(1 << data);
}

// Раскладываем 9999 на цифры
static void bin_bcd4x(uint_fast16_t a)
{
  bcd4x.tens = 0;
  bcd4x.hundreds = 0;
  bcd4x.thousands = 0;
  bcd4x.units = a;
  while(bcd4x.units >= 1000) {
    bcd4x.units -= 1000;
    bcd4x.thousands++;
  }
  while(bcd4x.units >= 100) {
    bcd4x.units -= 100;
    bcd4x.hundreds++;
  }
  while(bcd4x.units >= 10) {
    bcd4x.units -= 10;
    bcd4x.tens++;
  }
}

static void tm1638_disp_4arr1(uint8_t *array)
{
  tm1638_seg(0, array[0], 0);
  tm1638_seg(1, array[1], 0);
  tm1638_seg(2, array[2], 0);
  tm1638_seg(3, array[3], 0);
}

static void tm1638_disp_4arr2(uint8_t *array)
{
  tm1638_seg(4, array[0], 0);
  tm1638_seg(5, array[1], 0);
  tm1638_seg(6, array[2], 0);
  tm1638_seg(7, array[3], 0);
}
void tm1638_disp_4dec1(uint16_t y)
{
  uint8_t x[4];
  bin_bcd4x(y);
  if (!bcd4x.thousands)  x[0] = EMPTYCODE; else x[0] = bcd4x.thousands;
  if ((!bcd4x.hundreds)&&(!bcd4x.thousands)) x[1] = EMPTYCODE; else x[1] = bcd4x.hundreds;
  if ((!bcd4x.tens)&&(!bcd4x.hundreds)&&(!bcd4x.thousands)) x[2] = EMPTYCODE; else x[2] = bcd4x.tens;
  x[3] = bcd4x.units;
  tm1638_disp_4arr1(x);
}

void tm1638_disp_4dec2(uint16_t y)
{
  uint8_t x[4];
  bin_bcd4x(y);
  if (!bcd4x.thousands)  x[0] = EMPTYCODE; else x[0] = bcd4x.thousands;
  if ((!bcd4x.hundreds)&&(!bcd4x.thousands)) x[1] = EMPTYCODE; else x[1] = bcd4x.hundreds;
  if ((!bcd4x.tens)&&(!bcd4x.hundreds)&&(!bcd4x.thousands)) x[2] = EMPTYCODE; else x[2] = bcd4x.tens;
  x[3] = bcd4x.units;
  tm1638_disp_4arr2(x);
}

// EOF
