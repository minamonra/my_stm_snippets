#include <stm32f0xx.h>
// Тоже софт реализация TM1538 SPI
// была написана кем-то для PIC, переделал на STM32F0
// PB3 = CLK (clock), PB4 = chipselect (MISO), PB5 = data (MOSI)
// Подключение к TM1638
// PB3 -> CLK, PB4 -> STB, PB5 -> DIO
#define DIO_SET GPIOB->BSRR |= GPIO_BSRR_BS_5
#define DIO_CLR GPIOB->BSRR |= GPIO_BSRR_BR_5
#define CLK_SET GPIOB->BSRR |= GPIO_BSRR_BS_3
#define CLK_CLR GPIOB->BSRR |= GPIO_BSRR_BR_3
#define CS_SET  GPIOB->BSRR |= GPIO_BSRR_BS_4
#define CS_CLR  GPIOB->BSRR |= GPIO_BSRR_BR_4
#define CHECK_DIO GPIOB->IDR & (1<<5)

volatile uint32_t ttms   = 0; // системный тикер
volatile uint32_t ddms   = 0; // системный тикер

void gpio_init(void)
{
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
  GPIOA->MODER |= GPIO_MODER_MODER2_0;
  GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR2_0;
  GPIOB->MODER |= (GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0);
  GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR3_0 | GPIO_OSPEEDER_OSPEEDR4_0 | GPIO_OSPEEDER_OSPEEDR5_0);
}

//
const char FontNumber[] =
{
  0x3F, //  0  0
  0x06, //  1  1
  0x5B, //  2  2
  0x4F, //  3  3
  0x66, //  4  4
  0x6D, //  5  5
  0x7D, //  6  6
  0x07, //  7  7
  0x7F, //  8  8
  0x6F, //  9  9
  0x77, // 10  A
  0x7C, // 11  B
  0x39, // 12  C
  0x5E, // 13  D
  0x79, // 14  E
  0x71, // 15  F
  0x3D, // 16  G
  0x76, // 17  H
  0x00, // 18
  0x40, // 19  -
  0x80, // 20  .
  0x63, // 21  *
  0x78, // 22  t
  0x1E, // 23  J
  0x38, // 24  L
};

const char FontChar[] =
{
  0b00000000, // (32)  <space>
  0b10000110, // (33)        !
  0b00100010, // (34)        "
  0b01111110, // (35)        #
  0b01101101, // (36)        $
  0b00000000, // (37)        %
  0b00000000, // (38)        &
  0b00000010, // (39)        '
  0b00110000, // (40)        (
  0b00000110, // (41)        )
  0b01100011, // (42)        *
  0b00000000, // (43)        +
  0b00000100, // (44)        ,
  0b01000000, // (45)        -
  0b10000000, // (46)        .
  0b01010010, // (47)        /
  0b00111111, // (48)        0
  0b00000110, // (49)        1
  0b01011011, // (50)        2
  0b01001111, // (51)        3
  0b01100110, // (52)        4
  0b01101101, // (53)        5
  0b01111101, // (54)        6
  0b00100111, // (55)        7
  0b01111111, // (56)        8
  0b01101111, // (57)        9
  0b00000000, // (58)        :
  0b00000000, // (59)        ;
  0b00000000, // (60)        <
  0b01001000, // (61)        =
  0b00000000, // (62)        >
  0b01010011, // (63)        ?
  0b01011111, // (64)        @
  0b01110111, // (65)        A
  0b01111111, // (66)        B
  0b00111001, // (67)        C
  0b00111111, // (68)        D
  0b01111001, // (69)        E
  0b01110001, // (70)        F
  0b00111101, // (71)        G
  0b01110110, // (72)        H
  0b00000110, // (73)        I
  0b00011111, // (74)        J
  0b01101001, // (75)        K
  0b00111000, // (76)        L
  0b00010101, // (77)        M
  0b00110111, // (78)        N
  0b00111111, // (79)        O
  0b01110011, // (80)        P
  0b01100111, // (81)        Q
  0b00110001, // (82)        R
  0b01101101, // (83)        S
  0b01111000, // (84)        T
  0b00111110, // (85)        U
  0b00101010, // (86)        V
  0b00011101, // (87)        W
  0b01110110, // (88)        X
  0b01101110, // (89)        Y
  0b01011011, // (90)        Z
  0b00111001, // (91)        [
  0b01100100, // (92)        \ (this can't be the last char on a line, even in comment or it'll concat)
  0b00001111, // (93)        ]
  0b00000000, // (94)        ^
  0b00001000, // (95)        _
  0b00100000, // (96)        `
  0b01011111, // (97)        a
  0b01111100, // (98)        b
  0b01011000, // (99)        c
  0b01011110, // (100)       d
  0b01111011, // (101)       e
  0b00110001, // (102)       f
  0b01101111, // (103)       g
  0b01110100, // (104)       h
  0b00000100, // (105)       i
  0b00001110, // (106)       j
  0b01110101, // (107)       k
  0b00110000, // (108)       l
  0b01010101, // (109)       m
  0b01010100, // (110)       n
  0b01011100, // (111)       o
  0b01110011, // (112)       p
  0b01100111, // (113)       q
  0b01010000, // (114)       r
  0b01101101, // (115)       s
  0b01111000, // (116)       t
  0b00011100, // (117)       u
  0b00101010, // (118)       v
  0b00011101, // (119)       w
  0b01110110, // (120)       x
  0b01101110, // (121)       y
  0b01000111, // (122)       z
  0b01000110, // (123)       {
  0b00000110, // (124)       |
  0b01110000, // (125)       }
  0b00000001, // (126)       ~
};

// Выдаём один байт на SPI, строб не трогаем
void Write1638(uint8_t data)
{
  for (uint8_t i = 0; i < 8; i++) {
    CLK_CLR; 
    if (data & 0x01)
      DIO_SET;
    else
      DIO_CLR; 
    data >>= 1;
    CLK_SET; 
  }
}

// Строб опустили, выдали байт, строб подняли
void WriteCmd(uint8_t cmd)
{
  CS_CLR;
  Write1638(cmd);
  CS_SET;
}

// Read buttons, значения:
// hex 1F 17 0F 07 1B 13 0B 03
// dec 31 23 15 07 27 19 11 03
uint8_t ReadKey(void)
{
  uint8_t temp=32;
  CS_CLR;
  Write1638(0x42);    // Switch to read mode
  DIO_SET;            // Set DIO = input
  for(uint8_t i=0; i<32; i++) // Read 4 bytes
  {
    temp--;
    CLK_CLR;
    //delay_us(1);    //
    if(CHECK_DIO)
      temp<<=1;
    CLK_SET;
  }
  DIO_CLR;            // Set DIO = output
  CS_SET;
  return temp;        // 0x00 ~ 0xFF
}

void WriteData(uint8_t Adress,uint8_t Data)
{
  WriteCmd(0x44);
  CS_CLR;
  Write1638(0xC0 | Adress);
  Write1638(Data);
  CS_SET;
}

// Clear display, turn leds off
void LedReset (void)
{
  CLK_SET;
  WriteCmd(0x40);
  CS_CLR;
  Write1638(0xC0);
  for(uint8_t i=0 ; i<32 ; i++)
  {
    Write1638(0x00);
  }
  CS_SET;
  CLK_SET;
}

// Led - positions 1,3,5,7,9,11,13,15
// 0 - turn led ON, 1 - turn OFF
// 7Seg - positions 0,2,4,6,8,10,12,14
void WriteDigit(char number,char pos)
{
  WriteCmd(0x44);
  CS_CLR;
  Write1638(0xC0 | pos);
  Write1638(FontNumber[number]);
  CS_SET;
}

void DisplayLongNumber(signed long number)
{
  uint8_t und,dec,cnt,mil,decmil,cntmil,mlh,decmlh;
  signed long tmp;
  und     = number%10;
  tmp     = number/10;
  dec     = tmp%10;
  tmp     = tmp/10;
  cnt     = tmp%10;
  tmp     = tmp/10;
  mil     = tmp%10;
  tmp     = tmp/10;
  decmil  = tmp%10;
  tmp     = tmp/10;
  cntmil  = tmp%10;
  tmp     = tmp/10;
  mlh     = tmp%10;
  tmp     = tmp/10;
  decmlh  = tmp%10;
  CLK_SET;
  WriteCmd(0x40);
  CS_CLR;
  Write1638(0xC0);
  Write1638(FontNumber[decmlh]);
  Write1638(0x00);
  Write1638(FontNumber[mlh]);
  Write1638(0x00);
  Write1638(FontNumber[cntmil]);
  Write1638(0x00);
  Write1638(FontNumber[decmil]);
  Write1638(0x00);
  Write1638(FontNumber[mil]);
  Write1638(0x00);
  Write1638(FontNumber[cnt]);
  Write1638(0x00);
  Write1638(FontNumber[dec]);
  Write1638(0x00);
  Write1638(FontNumber[und]);
  Write1638(0x00);
  CS_SET;
  CLK_SET;
}

void DisplayString(const char *msg)
{
  CLK_SET;
  WriteCmd(0x40);
  CS_CLR;
  Write1638(0xC0);
  Write1638(FontChar[msg[0]-32]);
  Write1638(0x00);
  Write1638(FontChar[msg[1]-32]);
  Write1638(0x00);
  Write1638(FontChar[msg[2]-32]);
  Write1638(0x00);
  Write1638(FontChar[msg[3]-32]);
  Write1638(0x00);
  Write1638(FontChar[msg[4]-32]);
  Write1638(0x00);
  Write1638(FontChar[msg[5]-32]);
  Write1638(0x00);
  Write1638(FontChar[msg[6]-32]);
  Write1638(0x00);
  Write1638(FontChar[msg[7]-32]);
  Write1638(0x00);
  CS_SET;
  CLK_SET;
}

void delay_ms(uint32_t ms)
{
  ddms = ms;
  while(ddms) {};
}

int main(void)
{
  SysTick_Config(8000);
  gpio_init();

  delay_ms(1000);               // Power-Up recovery delay
  WriteCmd(0x8F);               // Light level (8,9,A,B,C,D,E,F)
  LedReset();

// DEMO TEST
  while(1)
  {
    DisplayLongNumber(76543210);
    delay_ms(2000);
    DisplayString("--OFF---");
    delay_ms(1000);
    WriteCmd(0x80);             // Display off
    delay_ms(1000);
    WriteCmd(0x88);             // Light min
    DisplayString("-LLL-001");
    delay_ms(1000);
    WriteCmd(0x8B);
    DisplayString("-LLL-004");
    delay_ms(1000);
    WriteCmd(0x8F);             // Light max
    DisplayString("-LLL-008");
    delay_ms(1000);
    LedReset();
    delay_ms(1000);
    WriteDigit (0,1);           // Leds...
    WriteDigit (10,0);
    delay_ms(200);
    WriteDigit (0,3);
    WriteDigit (11,2);
    delay_ms(200);
    WriteDigit (0,5);
    WriteDigit (12,4);
    delay_ms(200);
    WriteDigit (0,7);
    WriteDigit (13,6);
    delay_ms(200);
    WriteDigit (0,9);
    WriteDigit (14,8);
    delay_ms(200);
    WriteDigit (0,11);
    WriteDigit (15,10);
    delay_ms(200);
    WriteDigit (0,13);
    WriteDigit (16,12);
    delay_ms(200);
    WriteDigit (0,15);
    WriteDigit (17,14);
    delay_ms(2000);
    {                           // Keys test
      for(uint8_t i=0 ; i<100 ; i++)
      {
        DisplayLongNumber(ReadKey());
        delay_ms(200);
      }
    }
    DisplayString("SOFt OFF");
    delay_ms(1000);
    LedReset();
    delay_ms(1000);
  }
}

void SysTick_Handler(void) 
{
  ++ttms;
  if (ddms) ddms--;
}

// EOF
