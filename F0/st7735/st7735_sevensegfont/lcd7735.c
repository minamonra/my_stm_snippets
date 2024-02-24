#ifndef __LCD7735_H__
#define __LCD7735_H__

#include "lcd7735.h" // объявления модуля
#include "font7x15.h" // шрифт
#include "SevenSegNumFont.h"
#include "SevenSegNumFontPCMDS.h"

extern void delay_ms(uint32_t ms);
#define SPIDR8BIT (*(__IO uint8_t *)((uint32_t)&SPI1->DR))

// отправка данных\команд на дисплей
void lcd7735_senddata(unsigned char data)
{
  while (!(SPI1->SR & SPI_SR_TXE));
  SPIDR8BIT = data;
  while (!(SPI1->SR & SPI_SR_RXNE));
  uint8_t temp = SPIDR8BIT;
}

// отправка команды на дисплей с ожиданием конца передачи
void lcd7735_sendCmd(unsigned char cmd)
{
  DC_DN;
  lcd7735_senddata(cmd);
  while(SPI1->SR & SPI_SR_BSY);
}

// отправка данных на дисплей с ожиданием конца передачи
void lcd7735_sendData(unsigned char data) 
{
  DC_UP;
  lcd7735_senddata(data);
  while(SPI1->SR & SPI_SR_BSY);
}

// определение области экрана для заполнения
void lcd7735_at(unsigned char startX, unsigned char startY, unsigned char stopX, unsigned char stopY) 
{
  lcd7735_sendCmd(0x2A);
  DC_UP;
  lcd7735_senddata(0x00);
  lcd7735_senddata(startX);
  lcd7735_senddata(0x00);
  lcd7735_sendData(stopX);

  lcd7735_sendCmd(0x2B);
  DC_UP;
  lcd7735_senddata(0x00);
  lcd7735_senddata(startY);
  lcd7735_senddata(0x00);
  lcd7735_sendData(stopY);
}

// Инициализация
void lcd7735_init(uint16_t color) 
{
  CS_DN; // CS=0  - начали сеанс работы с дисплеем
  // сброс дисплея
  // аппаратный сброс дисплея
  RST_DN;
  delay_ms(LCD_RST_DLY); // пауза
  RST_UP; // RST=1
  delay_ms(LCD_RST_DLY); // пауза
  
  // инициализация дисплея
  lcd7735_sendCmd(0x11); // после сброса дисплей спит - даем команду проснуться
  delay_ms(10); // пауза
  lcd7735_sendCmd (0x3A); // режим цвета:
  lcd7735_sendData(0x05); //             16 бит
  lcd7735_sendCmd (0x36); // направление вывода изображения:
#ifdef RGB
  lcd7735_sendData(0x1C);         // снизу вверх, справа на лево, порядок цветов RGB
#else
  lcd7735_sendData(0x14); // снизу вверх, справа на лево, порядок цветов BGR
#endif
  lcd7735_sendCmd (0x29); // включаем изображение
  CS_UP;
  lcd7735_fillrect(0, 0, 127, 159, color);
}

// процедура заполнения прямоугольной области экрана заданным цветом
void lcd7735_fillrect(unsigned char startX, unsigned char startY, unsigned char stopX, unsigned char stopY, unsigned int color)
{
  unsigned char y;
  unsigned char x;
  CS_DN;
  lcd7735_at(startX, startY, stopX, stopY);
  lcd7735_sendCmd(0x2C);
  DC_UP;
  SPI2SIXTEEN;

  for (y=startY;y<stopY+1;y++)
    for (x=startX;x<stopX+1;x++) 
    {
      while (!(SPI1->SR & SPI_SR_TXE));
      SPI1->DR = color;
    }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY));
  SPI2EIGHT;
  CS_UP;
}

// вывод пиксела
void lcd7735_putpix(unsigned char x, unsigned char y, unsigned int Color)
{
  CS_DN;

  lcd7735_at(x, y, x, y);
  lcd7735_sendCmd(0x2C);
  lcd7735_sendData((unsigned char)((Color & 0xFF00)>>8));
  lcd7735_sendData((unsigned char) (Color & 0x00FF));

  CS_UP;
}

// процедура рисования линии
void lcd7735_line(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned int color) {
  signed char   dx, dy, sx, sy;
  unsigned char  x,  y, mdx, mdy, l;

  if (x1==x2) { // быстрая отрисовка вертикальной линии
	  lcd7735_fillrect(x1,y1, x1,y2, color);
	  return;
  }

  if (y1==y2) { // быстрая отрисовка горизонтальной линии
	  lcd7735_fillrect(x1,y1, x2,y1, color);
	  return;
  }

  dx=x2-x1; dy=y2-y1;

  if (dx>=0) { mdx=dx; sx=1; } else { mdx=x1-x2; sx=-1; }
  if (dy>=0) { mdy=dy; sy=1; } else { mdy=y1-y2; sy=-1; }

  x=x1; y=y1;

  if (mdx>=mdy) {
     l=mdx;
     while (l>0) {
         if (dy>0) { y=y1+mdy*(x-x1)/mdx; }
            else { y=y1-mdy*(x-x1)/mdx; }
         lcd7735_putpix(x,y,color);
         x=x+sx;
         l--;
     }
  } else {
     l=mdy;
     while (l>0) {
        if (dy>0) { x=x1+((mdx*(y-y1))/mdy); }
          else { x=x1+((mdx*(y1-y))/mdy); }
        lcd7735_putpix(x,y,color);
        y=y+sy;
        l--;
     }
  }
  lcd7735_putpix(x2, y2, color);
}

// рисование прямоугольника (не заполненного)
void lcd7735_rect(char x1,char y1,char x2,char y2, unsigned int color)
{
  lcd7735_fillrect(x1,y1, x2,y1, color);
  lcd7735_fillrect(x1,y2, x2,y2, color);
  lcd7735_fillrect(x1,y1, x1,y2, color);
  lcd7735_fillrect(x2,y1, x2,y2, color);
}

// печать десятичного числа
void LCD7735_dec(unsigned int numb, unsigned char dcount, unsigned char x, unsigned char y,unsigned int fntColor, unsigned int bkgColor)
{
  unsigned int divid=10000;
  unsigned char i;

  for (i=5; i!=0; i--) 
  {
    unsigned char res=numb/divid;
    if (i<=dcount)
    {
      lcd7735_putchar(x, y, res+'0', fntColor, bkgColor);
      y=y+6;
    }
    numb%=divid;
    divid/=10;
  }
}

// вывод символа на экран по координатам
void lcd7735_putchar(unsigned char x, unsigned char y, unsigned char chr, unsigned int charColor, unsigned int bkgColor) {
  unsigned char i;
  unsigned char j;
  CS_DN;
  lcd7735_at(x, y, x+12, y+8);
  lcd7735_sendCmd(0x2C);

  SPI2SIXTEEN;
  DC_UP;

  unsigned char k;
  for (i=0;i<7;i++)
  for (k=2;k>0;k--)
  {
    unsigned char chl=NewBFontLAT[ ( (chr-0x20)*14 + i+ 7*(k-1)) ];
    chl=chl<<2*(k-1); // нижнюю половину символа сдвигаем на 1 позицию влево (убираем одну линию снизу)
    unsigned char h;
    if (k==2) h=6; else h=7; // у нижней половины выведем только 6 точек вместо 7
    for (j=0;j<h;j++)
    {
      unsigned int color;
      if (chl & 0x80) color=charColor; else color=bkgColor;
      chl = chl<<1;
      while (!(SPI1->SR & SPI_SR_TXE));
      SPI1->DR = color;
    }
  };
  // рисуем справа от символа пустую вертикальную линию для бокового интервала
  for (j=0;j<13;j++)
  {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = bkgColor;
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY));
  CS_UP;
  SPI2EIGHT;
}

// вывод строки
void lcd7735_putstr(unsigned char x, unsigned char y, const unsigned char str[], unsigned int charColor, unsigned int bkgColor)
{
  while (*str!=0) 
  {
    lcd7735_putchar(x, y, *str, charColor, bkgColor);
    y=y+8;
    str++;
  }
}

// Семисегментный шрифт 32х50. 
// Этот шрифт устроен так: 50 строк -0...49 сверху вниз, 
// В каждой строке 4 байта. Нумерация байтов 0...3 слева на право. 
// Нумерация битов в байте 7...0 слева на право. 
// В программе b-это байты, i-строки, j-биты. 


void print_char_32x50_land(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor) {
  uint8_t j, i, b, d;
  uint16_t d1, color;
  CS_DN;
  lcd7735_at(X, Y, X + 49, Y + 32); // Set window
  lcd7735_sendCmd(0x2C);
  SPI2SIXTEEN;
  DC_UP;
  for (i = 0; i < 4; i++) {
    for (j = 8; j > 0; j--) {
      for (b = 50; b > 0; b--) {
        d1 = (CH)*200 + ((b - 1) * 4 + i);
        d = SevenSegNumFont[d1];
        if (d & (1 << (j - 1))) color = fcolor; else color = bcolor;
        while (!(SPI1->SR & SPI_SR_TXE)){};
        SPI1->DR = color;
      }
    }
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY)){};
  CS_UP;
  SPI2EIGHT;
}

void print_char_32x50_port(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor) {
  uint8_t j, i, b, d;
  uint16_t d1, color;
  CS_DN;
  lcd7735_at(X, Y, X + 31, Y + 49);
  lcd7735_sendCmd(0x2C);
  SPI2SIXTEEN;
  DC_UP;
  for (b = 0; b < 50; b++) {
    for (i = 0; i < 4; i++) {
      for (j = 8; j > 0; j--) {
        d1 = (CH)*200 + (b * 4 + i);
        d = SevenSegNumFont[d1];
        if (d & (1 << (j - 1))) color = fcolor; else color = bcolor;
        d = d << 1;
        while (!(SPI1->SR & SPI_SR_TXE)){};
        SPI1->DR = color;
      }
    }
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY)){};
  CS_UP;
  SPI2EIGHT;
}


// 32x46
void print_char_32x46_port(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor) {
  uint8_t j, i, b, d;
  uint16_t d1, color;
  CS_DN;
  lcd7735_at(X, Y, X + 31, Y + 45);
  lcd7735_sendCmd(0x2C);
  SPI2SIXTEEN;
  DC_UP;
  for (b = 0; b < 46; b++) {
    for (i = 0; i < 4; i++) {
      for (j = 8; j > 0; j--) {
        d1 = (CH)*184 + (b * 4 + i);
        d = SevenSegNumFontPCMDS[d1];
        if (d & (1 << (j - 1))) color = fcolor; else color = bcolor;
        d = d << 1;
        while (!(SPI1->SR & SPI_SR_TXE)){};
        SPI1->DR = color;
      }
    }
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY)){};
  CS_UP;
  SPI2EIGHT;
}

void print_char_32x46_land(uint8_t CH, uint8_t X, uint8_t Y, uint16_t fcolor, uint16_t bcolor) {
  uint8_t j, i, b, d;
  uint16_t d1, color;
  CS_DN;
  lcd7735_at(X, Y, X + 45, Y + 31); // Set window
  lcd7735_sendCmd(0x2C);
  SPI2SIXTEEN;
  DC_UP;
  for (i = 0; i < 4; i++) {
    for (j = 8; j > 0; j--) {
      for (b = 46; b > 0; b--) {
        d1 = (CH)*184 + ((b - 1) * 4 + i);
        d = SevenSegNumFontPCMDS[d1];
        if (d & (1 << (j - 1))) color = fcolor; else color = bcolor;
        while (!(SPI1->SR & SPI_SR_TXE)){};
        SPI1->DR = color;
      }
    }
  }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY)){};
  CS_UP;
  SPI2EIGHT;
}


#endif // __LCD7735_H__