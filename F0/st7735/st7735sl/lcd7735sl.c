#include "lcd7735sl.h"      // объявления модуля

// some flags for initR() :(
#define INITR_GREENTAB    0x00
#define INITR_REDTAB      0x01
#define INITR_BLACKTAB    0x02
#define INITR_18GREENTAB  INITR_GREENTAB
#define INITR_18REDTAB    INITR_REDTAB
#define INITR_18BLACKTAB  INITR_BLACKTAB
#define INITR_144GREENTAB 0x01
#define INITR_MINI160x80  0x04
#define INITR_HALLOWING   0x05

// Some register settings
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH  0x04

#define ST7735_FRMCTR1    0xB1
#define ST7735_FRMCTR2    0xB2
#define ST7735_FRMCTR3    0xB3
#define ST7735_INVCTR     0xB4
#define ST7735_DISSET5    0xB6

#define ST7735_PWCTR1     0xC0
#define ST7735_PWCTR2     0xC1
#define ST7735_PWCTR3     0xC2
#define ST7735_PWCTR4     0xC3
#define ST7735_PWCTR5     0xC4
#define ST7735_VMCTR1     0xC5

#define ST7735_PWCTR6     0xFC

#define ST7735_GMCTRP1    0xE0
#define ST7735_GMCTRN1    0xE1

#define ST_CMD_DELAY      0x80 // special signifier for command lists

#define ST77XX_NOP        0x00
#define ST77XX_SWRESET    0x01
#define ST77XX_RDDID      0x04
#define ST77XX_RDDST      0x09

#define ST77XX_SLPIN      0x10
#define ST77XX_SLPOUT     0x11
#define ST77XX_PTLON      0x12
#define ST77XX_NORON      0x13

#define ST77XX_INVOFF     0x20
#define ST77XX_INVON      0x21
#define ST77XX_DISPOFF    0x28
#define ST77XX_DISPON     0x29
#define ST77XX_CASET      0x2A
#define ST77XX_RASET      0x2B
#define ST77XX_RAMWR      0x2C
#define ST77XX_RAMRD      0x2E

#define ST77XX_PTLAR      0x30
#define ST77XX_TEOFF      0x34
#define ST77XX_TEON       0x35
#define ST77XX_MADCTL     0x36
#define ST77XX_COLMOD     0x3A

#define ST77XX_MADCTL_MY  0x80
#define ST77XX_MADCTL_MX  0x40
#define ST77XX_MADCTL_MV  0x20
#define ST77XX_MADCTL_ML  0x10
#define ST77XX_MADCTL_RGB 0x00

#define ST77XX_RDID1      0xDA
#define ST77XX_RDID2      0xDB
#define ST77XX_RDID3      0xDC
#define ST77XX_RDID4      0xDD

extern void delay_ms(uint32_t ms);
#define SPIDR8BIT (*(__IO uint8_t *)((uint32_t)&SPI1->DR))

// ===================================================== //
// NEW: ================================================ //

// посылка байта команды/данных
static inline void st7735send(uint8_t dc, uint8_t data)
{
  if (dc == DATA) DC_UP;
  else            DC_DN;

  while (!(SPI1->SR & SPI_SR_TXE));
  SPIDR8BIT = data;
  while (!(SPI1->SR & SPI_SR_RXNE));
  uint8_t temp = SPIDR8BIT;
}

// посылка байта команды/данных
void send(unsigned char data, unsigned char dc)
{
  if (dc == DATA) DC_UP;  //
  else            DC_DN;  //

  while (!(SPI1->SR & SPI_SR_TXE));
  SPIDR8BIT = data;
  while (!(SPI1->SR & SPI_SR_RXNE));
  uint8_t temp = SPIDR8BIT;
}


void st7735init(unsigned int orientation, unsigned int color) {
  CS_DN; // CS=0  - начали сеанс работы с дисплеем
  // аппаратный сброс дисплея
  RST_UP; // RST=1
  delay_ms(ST7735DLY); // пауза

  st7735send(COMM,ST77XX_SWRESET);
  delay_ms(ST7735DLY);

  st7735send(COMM, 0x11); // SLPOUT (0x11) после сброса дисплей спит - даем команду проснуться  // SLPIN (10h) / SLPOUT (11h)
  delay_ms(ST7735DLY);

  st7735send(COMM, 0x3A); // COLMOD (0x3A) режим цвета:
  st7735send(DATA, 0x05); // 16 бит
  
  // MADCTL (36h) – установка режима адресации и, соответственно, порядка вывода данных на дисплей, эта команда определяет ориентацию изображения на экране,
  // кроме того, бит RGB параметра этой команды ответственен за распределение интенсивности между субпикселями (красным, зелёным и синим)
  st7735send(COMM, 0x36); // MADCTL (0x36) направление вывода изображения:
  if (orientation ==  PORTRAIT)  st7735send(DATA, 0xC0);
  if (orientation == LANDSCAPE)  st7735send(DATA, 0x60);

  // 00 = upper left printing right
  // 10 = does nothing (MADCTL_ML)
  // 20 = upper left printing down (backwards) (Vertical flip)
  // 40 = upper right printing left (backwards) (X Flip)
  // 80 = lower left printing right (backwards) (Y Flip)
  // 04 = (MADCTL_MH)
  // 60 = 90 right rotation
  // C0 = 180 right rotation
  // A0 = 270 right rotation

  st7735send(COMM, ST7735_FRMCTR1);
  st7735send(DATA, 0x01);
  st7735send(DATA, 0x2C);
  st7735send(DATA, 0x2D);

  st7735send(COMM, ST7735_FRMCTR2);
  st7735send(DATA, 0x01);
  st7735send(DATA, 0x2C);
  st7735send(DATA, 0x2D);

  st7735send(COMM, ST7735_FRMCTR3);
  st7735send(DATA, 0x01);
  st7735send(DATA, 0x2C);
  st7735send(DATA, 0x2D);
  st7735send(DATA, 0x01);
  st7735send(DATA, 0x2C);
  st7735send(DATA, 0x2D);

  st7735send(COMM, ST7735_INVCTR);
  st7735send(DATA, 0x07);

  st7735send(COMM, ST7735_PWCTR1);
  st7735send(DATA, 0xA2);
  st7735send(DATA, 0x02);
  st7735send(DATA, 0x84);

  st7735send(COMM, ST7735_PWCTR2);
  st7735send(DATA, 0xC5);

  st7735send(COMM, ST7735_PWCTR3);
  st7735send(DATA, 0x0A);
  st7735send(DATA, 0x00);

  st7735send(COMM, ST7735_PWCTR4);
  st7735send(DATA, 0x8A);
  st7735send(DATA, 0x2A);

  st7735send(COMM, ST7735_PWCTR5);
  st7735send(DATA, 0x8A);
  st7735send(DATA, 0xEE);

  st7735send(COMM, ST7735_VMCTR1);
  st7735send(DATA, 0x0E);

  st7735send(COMM, ST77XX_INVOFF);  // INVOFF (20h) / INVON (21h) – выключение/включение инверсии дисплея. 

  st7735send(COMM, ST7735_GMCTRP1);
  st7735send(DATA, 0x02);
  st7735send(DATA, 0x1C);
  st7735send(DATA, 0x07);
  st7735send(DATA, 0x12);
  st7735send(DATA, 0x37);
  st7735send(DATA, 0x32);
  st7735send(DATA, 0x29);
  st7735send(DATA, 0x2D);
  st7735send(DATA, 0x29);
  st7735send(DATA, 0x25);
  st7735send(DATA, 0x2B);
  st7735send(DATA, 0x39);
  st7735send(DATA, 0x00);
  st7735send(DATA, 0x01);
  st7735send(DATA, 0x03);
  st7735send(DATA, 0x10);

  st7735send(COMM, ST7735_GMCTRN1);
  st7735send(DATA, 0x03);
  st7735send(DATA, 0x1D);
  st7735send(DATA, 0x07);
  st7735send(DATA, 0x06);
  st7735send(DATA, 0x2E);
  st7735send(DATA, 0x2C);
  st7735send(DATA, 0x29);
  st7735send(DATA, 0x2D);
  st7735send(DATA, 0x2E);
  st7735send(DATA, 0x2E);
  st7735send(DATA, 0x37);
  st7735send(DATA, 0x3F);
  st7735send(DATA, 0x00);
  st7735send(DATA, 0x00);
  st7735send(DATA, 0x02);
  st7735send(DATA, 0x10);
  
  st7735send(COMM, ST77XX_NORON);
  delay_ms(ST7735DLY);

  st7735send(COMM, ST77XX_DISPON);
  delay_ms(ST7735DLY);

  CS_DN; // chip_select_disable();

  if (orientation ==  PORTRAIT) st7735fillrect(0, 0, 127, 159, color);
  if (orientation == LANDSCAPE) st7735fillrect(0, 0, 159, 127, color);
}

// определение области экрана для заполнения
void st7735setwin(unsigned char startX, unsigned char startY, unsigned char stopX, unsigned char stopY)
{
  st7735send(COMM, 0x2A); // CASET
  DC_UP;
  st7735send(DATA, 0x00); // MADCTL_RGB // 00 = upper left printing right
  st7735send(DATA, startX);
  st7735send(DATA, 0x00);
  st7735send(DATA, stopX);

  st7735send(COMM, 0x2B); // RASET
  DC_UP;
  st7735send(DATA, 0x00);
  st7735send(DATA, startY);
  st7735send(DATA, 0x00);
  st7735send(DATA, stopY);
}

// вывод пиксела
void st7735pixel(unsigned char X, unsigned char Y, unsigned int color)
{
  CS_DN;
  st7735setwin(X, Y, X, Y);
  st7735send(COMM, 0x2C); // RAMWR
  st7735send(DATA, (unsigned char)((color & 0xFF00)>>8));
  st7735send(DATA, (unsigned char) (color & 0x00FF));
  CS_UP;
}

// процедура заполнения прямоугольной области экрана заданным цветом
void st7735fillrect(unsigned char startX, unsigned char startY, unsigned char stopX, unsigned char stopY, unsigned int color)
{
  unsigned char X;
  unsigned char Y;
  CS_DN;
  st7735setwin(startX, startY, stopX, stopY);
  st7735send(COMM, 0x2C); // RAMWR
  DC_UP;
  SPI2SIXTEEN;

  for (Y = startY; Y < stopY + 1; Y++)
    for (X = startX; X < stopX + 1; X++) 
    {
      while (!(SPI1->SR & SPI_SR_TXE));
      SPI1->DR = color;
    }
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY));

  SPI2EIGHT;
  CS_UP;
}

// процедура рисования линии
void st7735line(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned int color) {
  signed char   dx, dy, sx, sy;
  unsigned char  x,  y, mdx, mdy, l;

  if (x1==x2) { // быстрая отрисовка вертикальной линии
    st7735fillrect(x1,y1, x1,y2, color);
    return;
  }
  if (y1==y2) { // быстрая отрисовка горизонтальной линии
    st7735fillrect(x1,y1, x2,y1, color);
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
      else      { y=y1-mdy*(x-x1)/mdx; }
      st7735pixel(x,y,color);
      x=x+sx;
      l--;
    }
  } else {
    l=mdy;
    while (l>0) {
      if (dy>0) { x=x1+((mdx*(y-y1))/mdy); }
      else      { x=x1+((mdx*(y1-y))/mdy); }
      st7735pixel(x,y,color);
      y=y+sy;
      l--;
    }
  }
  st7735pixel(x2, y2, color);
}


void print_char_sl(unsigned char CH,            // символ который выводим
                unsigned char X, unsigned char Y, // координаты
                unsigned char SymbolWidth,        // ширина символа
                unsigned char SymbolHeight,       // высота символа
                unsigned char MatrixLength,       // длина матрицы символа
                const unsigned char font[],       // шрифт
                const unsigned int index[],       // индексный массив шрифта
                unsigned int fcolor,              // цвет шрифта
                unsigned int bcolor)              // цвет фона
{
  unsigned char MatrixByte;   // временная для обработки байта матрицы
  unsigned char BitMask;      // маска, накладываемая на байт для получения значения одного бита
  unsigned char BitWidth;     // счётчик выведенных по горизонтали точек
  const unsigned char *MatrixPointer = font;
  unsigned char color;
  BitWidth = SymbolWidth;     // начальная установка счётчика
  MatrixPointer += index[CH]; // перемещаем указатель на начало символа CH
  // начало алгоритма вывода. Вывод без поворота
  // x матрицы = x символа (ширина)
  // y матрицы = y символа (высота)
  CS_DN;
  st7735setwin(X, Y, X + SymbolWidth - 1, Y - SymbolHeight - 1); // Ширина и высота шрифта от 0
  st7735send(COMM, 0x2C); // Команда RAMWR (0x2C) указывает контроллеру дисплея, что все данные, идущие после неё, нужно воспринимать, 
                          // как байты кодировки цвета (не забываем про 2 байта на цвет одного пикселя), которые после преобразования выводятся 
                          // поочерёдно в соответствующем месте области вывода.
  SPI2SIXTEEN; // SPI в 16 бит
  DC_UP;
  
  do {
    MatrixByte    = *MatrixPointer;     // чтение очередного байта матрицы
    MatrixPointer =  MatrixPointer + 1; // после чтения передвинем указатель на следующий байт матрицы
    MatrixLength  =  MatrixLength  - 1; // можно писать просто MatrixLength--; но мне так не нравится...
    BitMask       =  0b10000000;        // предустановка маски и ширины символа на начало для каждого прочитанного байта
    do {
      while (!(SPI1->SR & SPI_SR_TXE)) {}; // ждём TXE перед отправкой (проверка есть ли данные в SPI)
      if ((MatrixByte & BitMask) > 0) SPI1->DR = fcolor; // вывод в поток точки цвета символа
      else                            SPI1->DR = bcolor; // вывод в поток точки цвета фона
      BitMask  = BitMask >> 1; // как только бит выедет вправо, BitMask станет равен 0. Значит, вывели все 8 битов в поток
      BitWidth = BitWidth - 1; // как только значение станет равным 0. Значит, все биты строки вывели в поток
    } while ((BitWidth > 0) && (BitMask > 0)); // если хоть что-то стало 0, надо читать следующий байт - выходим из этого цикла
    if (BitWidth == 0) BitWidth = SymbolWidth; // если выход был по окончании вывода строки - снова предустановим счётчик
                                               // один раз на символ предустановка лишняя, но организовывать проверку
                                               // окажется много дороже, чем один лишний раз присвоить значение
  } while (MatrixLength > 0);
  // закончили
  while (!(SPI1->SR & SPI_SR_TXE) || (SPI1->SR & SPI_SR_BSY)) {};
  CS_UP;
  SPI2EIGHT;
} // print_char_sl()


// ===================================================== //
// OLD: ================================================ //

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
  lcd7735_sendCmd(0x2A); // ST77XX_CASET
  DC_UP;
  lcd7735_senddata(0x00); // ST77XX_MADCTL_RGB
  lcd7735_senddata(startX);
  lcd7735_senddata(0x00);
  lcd7735_sendData(stopX);

  lcd7735_sendCmd(0x2B); // ST77XX_RASET
  DC_UP;
  lcd7735_senddata(0x00);
  lcd7735_senddata(startY);
  lcd7735_senddata(0x00);
  lcd7735_sendData(stopY);
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


// Инициализация
void lcd7735_init(uint16_t color) 
{
  CS_DN; // CS=0  - начали сеанс работы с дисплеем
  // сброс дисплея
  // аппаратный сброс дисплея
  RST_DN;
  delay_ms(ST7735DLY); // пауза
  RST_UP; // RST=1
  delay_ms(ST7735DLY); // пауза
  
  // инициализация дисплея
  lcd7735_sendCmd(0x11); // после сброса дисплей спит - даем команду проснуться
  delay_ms(10); // пауза
  lcd7735_sendCmd (0x3A); // режим цвета:
  lcd7735_sendData(0x05); //             16 бит
  lcd7735_sendCmd (0x36); // направление вывода изображения:
#ifdef RGB
  lcd7735_sendData(0x1C); // снизу вверх, справа на лево, порядок цветов RGB
#else
  lcd7735_sendData(0x14); // снизу вверх, справа на лево, порядок цветов BGR
#endif
  lcd7735_sendCmd (0x29); // включаем изображение
  CS_UP;
  lcd7735_fillrect(0, 0, 127, 159, color);
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
      else      { y=y1-mdy*(x-x1)/mdx; }
      lcd7735_putpix(x,y,color);
      x=x+sx;
      l--;
    }
  } else {
    l=mdy;
    while (l>0) {
      if (dy>0) { x=x1+((mdx*(y-y1))/mdy); }
      else      { x=x1+((mdx*(y1-y))/mdy); }
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

// EOF