#include "lcd_io.h"
#include "uart.h"

/////////////////////////
// ST7796 Display driver
/////////////////////////

_lcd_dev lcddev;
u16 POINT_COLOR = 0x0000;
u16 BACK_COLOR  = 0xFFFF;

// Аппаратная инициализация портов STM32F412 (Исправлены ошибки AFR)
void lcd_gpio_spi_init(void) {
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN);
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    __NOP(); __NOP();

    // Настройка подсветки на PA1 (Выход, High Speed)
    GPIOA->MODER &= ~GPIO_MODER_MODER1;
    GPIOA->MODER |= GPIO_MODER_MODER1_0;
    GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED1;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR1;

    // Настройка SPI1 пинов (PA5 - SCK, PA6 - MISO, PA7 - MOSI) -> AF5
    GPIOA->AFR[0] &= ~((0xFU << GPIO_AFRL_AFSEL5_Pos) | (0xFU << GPIO_AFRL_AFSEL6_Pos) | (0xFU << GPIO_AFRL_AFSEL7_Pos));
    GPIOA->AFR[0] |= (5U << GPIO_AFRL_AFSEL5_Pos) | (5U << GPIO_AFRL_AFSEL6_Pos) | (5U << GPIO_AFRL_AFSEL7_Pos);

    GPIOA->MODER &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
    GPIOA->MODER |= (GPIO_MODER_MODER5_1 | GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1); // AF Mode
    GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR5 | GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7);

    // Настройка управляющих пинов на PORTB (PB0 - CS, PB1 - RST, PB2 - DC)
    GPIOB->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2);
    GPIOB->MODER |= (GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0);
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED0 | GPIO_OSPEEDR_OSPEED1 | GPIO_OSPEEDR_OSPEED2);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR0 | GPIO_PUPDR_PUPDR1 | GPIO_PUPDR_PUPDR2);

    // Конфигурация SPI1 (PCLK2=96МГц, делитель /2 => 48 МГц, Mode 3)
    // Замечание по частоте: BR="000" — это МИНИМАЛЬНО возможный делитель /2 у STM32 SPI,
    // делителя /1 не существует. Т.е. 48 МГц — уже потолок при текущей PCLK2, дальше
    // разгонять некуда без поднятия самой шины APB2 (а она у F412 и так на максимуме ~100 МГц).
    SPI1->CR1 = 0;
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;

    SPI1->CR1 &= ~SPI_CR1_BR_Msk; // 000 = /2 => SPI1 = PCLK2/2 = 48 МГц

    SPI1->CR1 |= SPI_CR1_CPOL | SPI_CR1_CPHA; // Mode 3
    SPI1->CR1 |= SPI_CR1_SPE;                 // Включение SPI1

    ST7796_CS_SET;
    ST7796_RST_SET;
    ST7796_DC_SET;
    ST7796_LED_ON;
}

// ПОЛНОДУПЛЕКСНАЯ передача байта: ждём и TXE, и RXNE, читаем DR.
// Используется ТОЛЬКО там, где реально нужен ответ по MISO (сейчас — только lcd_read_id()).
// Для обычной отправки команд/данных НЕ используйте эту функцию — лишний цикл ожидания
// RXNE ничего не даёт, если принятый байт не читается смыслово, а только тратит время.
u8 lcd_spi_transfer(u8 byte) {
    while (!(SPI1->SR & SPI_SR_TXE));
    *(volatile u8*)&SPI1->DR = byte;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return *(volatile u8*)&SPI1->DR;
}

// Быстрая отправка байта БЕЗ ожидания RXNE и без дерганья CS (внутренняя, write-only).
// Это основной рабочий путь для команд/данных: не ждём ответа по MISO, т.к. он не нужен.
void spi_raw_write(uint8_t byte) {
    while (!(SPI1->SR & SPI_SR_TXE));
    *(volatile uint8_t*)&SPI1->DR = byte;
}

void LCD_WR_REG(u8 data) {
    ST7796_CS_CLR;
    ST7796_DC_CLR;
    spi_raw_write(data);              // write-only: RXNE не ждём
    while (SPI1->SR & SPI_SR_BSY);
    ST7796_CS_SET;
}

void LCD_WR_DATA8(u8 data) {
    ST7796_CS_CLR;
    ST7796_DC_SET;
    spi_raw_write(data);              // write-only: RXNE не ждём
    while (SPI1->SR & SPI_SR_BSY);
    ST7796_CS_SET;
}

void LCD_WriteReg(u8 LCD_Reg, u16 LCD_RegValue) {
    LCD_WR_REG(LCD_Reg);
    LCD_WR_DATA8(LCD_RegValue);
}

void LCD_WriteRAM_Prepare(void) {
    LCD_WR_REG(lcddev.wramcmd);
}

void LCD_WR_DATA16(u16 Data) {
    ST7796_CS_CLR;
    ST7796_DC_SET;
    spi_raw_write(Data >> 8);         // write-only: RXNE не ждём
    spi_raw_write(Data & 0xFF);
    while (SPI1->SR & SPI_SR_BSY);
    ST7796_CS_SET;
}

// ИСПРАВЛЕННАЯ ФУНКЦИЯ УСТАНОВКИ ОКНА (Удерживает CS в LOW для всех 4 байт данных)
void lcd_setwindows(u16 xStar, u16 yStar, u16 xEnd, u16 yEnd) {
    // === CASET (столбцы) ===
    ST7796_CS_CLR;
    ST7796_DC_CLR;
    spi_raw_write(lcddev.setxcmd); // 0x2A
    while (SPI1->SR & SPI_SR_BSY);
    ST7796_DC_SET;
    spi_raw_write(xStar >> 8);
    spi_raw_write(xStar & 0xFF);
    spi_raw_write(xEnd >> 8);
    spi_raw_write(xEnd & 0xFF);
    while (SPI1->SR & SPI_SR_BSY);
    ST7796_CS_SET;

    // === RASET (строки) ===
    ST7796_CS_CLR;
    ST7796_DC_CLR;
    spi_raw_write(lcddev.setycmd); // 0x2B
    while (SPI1->SR & SPI_SR_BSY);
    ST7796_DC_SET;
    spi_raw_write(yStar >> 8);
    spi_raw_write(yStar & 0xFF);
    spi_raw_write(yEnd >> 8);
    spi_raw_write(yEnd & 0xFF);
    while (SPI1->SR & SPI_SR_BSY);
    ST7796_CS_SET;

    // === RAMWR ===
    ST7796_CS_CLR;
    ST7796_DC_CLR;
    spi_raw_write(lcddev.wramcmd); // 0x2C
    while (SPI1->SR & SPI_SR_BSY);
    ST7796_CS_SET;
}


void lcd_setcursor(u16 Xpos, u16 Ypos) {
    lcd_setwindows(Xpos, Ypos, Xpos, Ypos);
}

void lcd_direction(u8 direction) {
    lcddev.setxcmd = 0x2A;
    lcddev.setycmd = 0x2B;
    lcddev.wramcmd = 0x2C;
    lcddev.rramcmd = 0x2E;
    lcddev.dir = direction % 4;

    switch(lcddev.dir) {
        case 0: // Портретная (0°)
            lcddev.width  = LCD_W;
            lcddev.height = LCD_H;
            LCD_WriteReg(0x36, (1<<3) | (1<<6));
        break;

        case 1: // Альбомная (90°)
            lcddev.width  = LCD_H;
            lcddev.height = LCD_W;
            LCD_WriteReg(0x36, (1<<3) | (1<<5));
        break;

        case 2: // Портретная перевернутая (180°)
            lcddev.width  = LCD_W;
            lcddev.height = LCD_H;
            LCD_WriteReg(0x36, (1<<3) | (1<<7));
        break;

        case 3: // Альбомная перевернутая (270°)
            lcddev.width  = LCD_H;
            lcddev.height = LCD_W;
            LCD_WriteReg(0x36, (1<<3) | (1<<7) | (1<<6) | (1<<5));
        break;

        default: break;
    }
}

// ЧТЕНИЕ ID: Полная официальная копия Alientek через тестовый режим 0xFB
// Тут СОЗНАТЕЛЬНО используется полнодуплексный lcd_spi_transfer(), т.к. нам
// реально нужен байт ответа по MISO. Плюс скорость временно снижена (см. ниже) —
// снижать частоту для операций ЧТЕНИЯ по-прежнему обязательно.
u16 lcd_read_id(void) {
    u8 i, val[3] = {0};

    // Временный переход на безопасную скорость чтения (Mode 0, ~6 МГц)
    u16 orig_cr1 = SPI1->CR1;
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);
    SPI1->CR1 &= ~SPI_CR1_BR_Msk;
    SPI1->CR1 |= (3U << SPI_CR1_BR_Pos); // /16
    SPI1->CR1 |= SPI_CR1_SPE;

    LCD_WR_REG(0xF0); LCD_WR_DATA8(0xC3);
    LCD_WR_REG(0xF0); LCD_WR_DATA8(0x96);

    ST7796_CS_CLR;
    for(i = 1; i < 4; i++) {
        ST7796_DC_CLR;
        lcd_spi_transfer(0xFB);
        while (SPI1->SR & SPI_SR_BSY);

        ST7796_DC_SET;
        lcd_spi_transfer(0x10 + i);
        while (SPI1->SR & SPI_SR_BSY);

        ST7796_DC_CLR;
        lcd_spi_transfer(0xD3);
        while (SPI1->SR & SPI_SR_BSY);

        ST7796_DC_SET;
        val[i-1] = lcd_spi_transfer(0xFF); // Считывание байта ID
        while (SPI1->SR & SPI_SR_BSY);

        ST7796_DC_CLR;
        lcd_spi_transfer(0xFB);
        while (SPI1->SR & SPI_SR_BSY);

        ST7796_DC_SET;
        lcd_spi_transfer(0x00);
        while (SPI1->SR & SPI_SR_BSY);
    }
    ST7796_CS_SET;

    LCD_WR_REG(0xF0); LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xF0); LCD_WR_DATA8(0x69);

    // Восстановление высокой скорости SPI для вывода графики
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 = orig_cr1;
    SPI1->CR1 |= SPI_CR1_SPE;

    lcddev.id = val[1];
    lcddev.id <<= 8;
    lcddev.id |= val[2];
    return lcddev.id;
}

// Глобальная переменная для хранения цвета текущей DMA транзакции заливки
// (DMA берет данные по указателю, переменная не должна пропадать из стека)
volatile uint16_t dma_color_buffer;

void lcd_clear(u16 Color) {
    u32 total_pixels = (u32)lcddev.width * lcddev.height;
    dma_color_buffer = Color;

    lcd_setwindows(0, 0, lcddev.width - 1, lcddev.height - 1);
    while (SPI1->SR & SPI_SR_BSY);

    ST7796_CS_CLR;
    ST7796_DC_SET;

    // Переключение SPI1 в 16-битный режим для DMA
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 |= SPI_CR1_DFF;
    SPI1->CR1 |= SPI_CR1_SPE;

    SPI1->CR2 |= SPI_CR2_TXDMAEN;

    u16 chunk_size = total_pixels / 4;
    DMA2_Stream5->M0AR = (uint32_t)&dma_color_buffer;

    for (int chunk = 0; chunk < 4; chunk++) {
        DMA2_Stream5->CR &= ~DMA_SxCR_EN;
        while (DMA2_Stream5->CR & DMA_SxCR_EN);

        DMA2_Stream5->NDTR = chunk_size;

        // Флаг чистим ПЕРЕД стартом (а не только используем как условие выхода) —
        // иначе если он случайно уже стоял от предыдущей операции, ожидание ниже
        // завершится мгновенно, не дождавшись реальной передачи.
        DMA2->HIFCR = DMA_HIFCR_CTCIF5;

        DMA2_Stream5->CR |= DMA_SxCR_EN;
        while (!(DMA2->HISR & DMA_HISR_TCIF5));
    }

    // ВАЖНО: после выхода из цикла флаг TCIF5 остаётся ВЗВЕДЁННЫМ (мы его только
    // проверяли, но не чистили после последнего чанка). Если не почистить его здесь,
    // это "протухший" флаг унаследует СЛЕДУЮЩИЙ вызов lcd_fillrect()/lcd_clear() —
    // именно это раньше вызывало проблему "первая закраска после lcd_clear() не рисуется".
    DMA2->HIFCR = DMA_HIFCR_CTCIF5;

    // === Корректное закрытие FIFO после серии DMA-передач ===
    while (SPI1->SR & SPI_SR_BSY);  // 1. Ждем, пока SPI полностью замолкнет
    SPI1->CR2 &= ~SPI_CR2_TXDMAEN;  // 2. Отключаем DMA от SPI

    // 3. Вычитываем мусор из DR, очищая FIFO
    volatile uint16_t dummy_read = SPI1->DR;
    (void)dummy_read;

    // 4. Сбрасываем и перезапускаем автомат SPI в 8-битном режиме
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 &= ~SPI_CR1_DFF;
    SPI1->CR1 |= SPI_CR1_SPE;

    ST7796_CS_SET;
}


void lcd_drawpoint(u16 x, u16 y) {
    lcd_setcursor(x, y);
    LCD_WR_DATA16(POINT_COLOR);
}


// 1. ФУНКЦИЯ СПЛОШНОЙ ЗАЛИВКИ ОБЛАСТИ ОДНИМ ЦВЕТОМ (адрес источника в DMA НЕ инкрементируется —
//    в память по одному и тому же адресу все время пишется один и тот же цвет `color`)
void lcd_fillrect(u16 x1, u16 y1, u16 x2, u16 y2, u16 color) {
    if (x1 > x2) { u16 tmp = x1; x1 = x2; x2 = tmp; }
    if (y1 > y2) { u16 tmp = y1; y1 = y2; y2 = tmp; }

    u16 w = x2 - x1 + 1;
    u16 h = y2 - y1 + 1;
    u32 total_pixels = (u32)w * h;

    lcd_setwindows(x1, y1, x2, y2);
    while (SPI1->SR & SPI_SR_BSY);

    ST7796_CS_CLR;
    ST7796_DC_SET;

    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 |= SPI_CR1_DFF;  // 16-бит режим
    SPI1->CR1 |= SPI_CR1_SPE;

    DMA2_Stream5->CR &= ~DMA_SxCR_EN;
    while (DMA2_Stream5->CR & DMA_SxCR_EN);

    DMA2_Stream5->M0AR = (uint32_t)&color; // Передаем цвет напрямую, адрес не инкрементируется
    DMA2_Stream5->NDTR = total_pixels;

    // === ФИКС гонки DMA-флага: чистим TCIF3 ПЕРЕД стартом, а не только после ===
    // Если флаг остался взведён от предыдущей операции (например, от lcd_clear()),
    // ожидание "while (!(DMA2->HISR & TCIF5))" ниже завершится мгновенно, ещё до
    // реального завершения передачи — CS поднимется раньше времени, и данные
    // на экран не попадут (внешне выглядит как "первый вызов рисует пустоту").
    DMA2->HIFCR = DMA_HIFCR_CTCIF5;

    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    DMA2_Stream5->CR |= DMA_SxCR_EN;

    while (!(DMA2->HISR & DMA_HISR_TCIF5));
    DMA2->HIFCR = DMA_HIFCR_CTCIF5;

    while (SPI1->SR & SPI_SR_BSY);
    SPI1->CR2 &= ~SPI_CR2_TXDMAEN;

    volatile uint16_t dummy_read = SPI1->DR;
    (void)dummy_read;

    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 &= ~SPI_CR1_DFF; // Возврат в 8-бит
    SPI1->CR1 |= SPI_CR1_SPE;

    ST7796_CS_SET;
}

// 2. НОВАЯ ФУНКЦИЯ: DMA-блит произвольного буфера пикселей (адрес источника
//    ИНКРЕМЕНТИРУЕТСЯ — MINC=1), в отличие от lcd_fillrect (там MINC=0, один цвет).
//    Используется, например, для быстрой пересылки целого символа шрифта,
//    заранее собранного в RAM-буфере — одной непрерывной DMA-транзакцией
//    вместо десятков мелких SPI-передач с повторной установкой окна на каждый штрих.
void lcd_blit_buffer(u16 x1, u16 y1, u16 w, u16 h, const u16 *buf) {
    if (w == 0 || h == 0 || buf == NULL) return;

    u32 total_pixels = (u32)w * h;
    u16 x2 = x1 + w - 1;
    u16 y2 = y1 + h - 1;

    lcd_setwindows(x1, y1, x2, y2);
    while (SPI1->SR & SPI_SR_BSY);

    ST7796_CS_CLR;
    ST7796_DC_SET;

    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 |= SPI_CR1_DFF;  // 16-бит режим
    SPI1->CR1 |= SPI_CR1_SPE;

    DMA2_Stream5->CR &= ~DMA_SxCR_EN;
    while (DMA2_Stream5->CR & DMA_SxCR_EN);

    // Включаем инкремент адреса памяти — читаем буфер последовательно, пиксель за пикселем
    DMA2_Stream5->CR |= DMA_SxCR_MINC;
    DMA2_Stream5->M0AR = (uint32_t)buf;
    DMA2_Stream5->NDTR = total_pixels;

    // Чистим флаг ПЕРЕД стартом — та же логика, что и в lcd_fillrect()
    DMA2->HIFCR = DMA_HIFCR_CTCIF5;

    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    DMA2_Stream5->CR |= DMA_SxCR_EN;

    while (!(DMA2->HISR & DMA_HISR_TCIF5));
    DMA2->HIFCR = DMA_HIFCR_CTCIF5;

    while (SPI1->SR & SPI_SR_BSY);
    SPI1->CR2 &= ~SPI_CR2_TXDMAEN;

    volatile uint16_t dummy_read = SPI1->DR;
    (void)dummy_read;

    // ВАЖНО: возвращаем MINC обратно в 0, иначе lcd_fillrect()/lcd_clear() (которые
    // ожидают фиксированный адрес источника — один и тот же цвет на каждый пиксель)
    // начнут вместо этого читать и слать мусор из соседних областей памяти.
    DMA2_Stream5->CR &= ~DMA_SxCR_MINC;

    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 &= ~SPI_CR1_DFF;
    SPI1->CR1 |= SPI_CR1_SPE;

    ST7796_CS_SET;
}

// 3. ФУНКЦИЯ КОНТУРНОЙ РАМКИ (Рисует только ПУСТУЮ рамку в 1 пиксель по углам X1,Y1 - X2,Y2)
void lcd_drawrectangle(u16 x1, u16 y1, u16 x2, u16 y2) {
    lcd_fillrect(x1, y1, x2, y1, POINT_COLOR); // Верхняя горизонтальная линия
    lcd_fillrect(x1, y2, x2, y2, POINT_COLOR); // Нижняя горизонтальная линия
    lcd_fillrect(x1, y1, x1, y2, POINT_COLOR); // Левая вертикальная линия
    lcd_fillrect(x2, y1, x2, y2, POINT_COLOR); // Правая вертикальная линия
}


void lcd_init(void) {
    // === ШАГ 1: АППАРАТНАЯ НАСТРОЙКА МК ===
    extern void lcd_gpio_spi_init(void);
    lcd_gpio_spi_init(); // Конфигурация портов ввода-вывода GPIO и шины SPI1 (48 МГц)
    lcd_dma_init();      // Настройка стрима DMA2 под высокоскоростную передачу графики

    // === ШАГ 2: АППАРАТНЫЙ СБРОС ДИСПЛЕЯ ===
    ST7796_RST_CLR;      // Опускаем пин RESET в ноль (аппаратный сброс контроллера экрана)
    delay_ms(100);       // Держим сброс 100 мс для перезагрузки внутренней логики чипа
    ST7796_RST_SET;      // Возвращаем пин RESET в единицу (рабочее состояние)
    delay_ms(50);        // Даем 50 мс на стабилизацию внутренних цепей матрицы

    // === ШАГ 3: ПОСЛЕДОВАТЕЛЬНОСТЬ ИНИЦИАЛИЗАЦИИ РЕГИСТРОВ ST7796S ===
    LCD_WR_REG(0x11);    // Sleep Out
    delay_ms(120);

    LCD_WR_REG(0x36); LCD_WR_DATA8(0x48); // Memory Access Control (MADCTL): начальная портретная ориентация
    LCD_WR_REG(0x3A); LCD_WR_DATA8(0x55); // Interface Pixel Format (COLMOD): 16-бит RGB565

    LCD_WR_REG(0xF0); LCD_WR_DATA8(0xC3); // Ключ доступа 1
    LCD_WR_REG(0xF0); LCD_WR_DATA8(0x96); // Ключ доступа 2

    LCD_WR_REG(0xB4); LCD_WR_DATA8(0x02); // Display Inversion Control
    LCD_WR_REG(0xB7); LCD_WR_DATA8(0xC6); // Entry Mode Set

    LCD_WR_REG(0xC0); LCD_WR_DATA8(0xC0); LCD_WR_DATA8(0x00); // Power Control 1
    LCD_WR_REG(0xC1); LCD_WR_DATA8(0x13);                     // Power Control 2
    LCD_WR_REG(0xC2); LCD_WR_DATA8(0xA7);                     // Power Control 3
    LCD_WR_REG(0xC5); LCD_WR_DATA8(0x21);                     // VCOM Control

    LCD_WR_REG(0xE8);
    LCD_WR_DATA8(0x40); LCD_WR_DATA8(0x8A); LCD_WR_DATA8(0x1B); LCD_WR_DATA8(0x1B);
    LCD_WR_DATA8(0x23); LCD_WR_DATA8(0x0A); LCD_WR_DATA8(0xAC); LCD_WR_DATA8(0x33);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0xD2); LCD_WR_DATA8(0x05); LCD_WR_DATA8(0x08); LCD_WR_DATA8(0x06);
    LCD_WR_DATA8(0x05); LCD_WR_DATA8(0x02); LCD_WR_DATA8(0x2A); LCD_WR_DATA8(0x44);
    LCD_WR_DATA8(0x46); LCD_WR_DATA8(0x39); LCD_WR_DATA8(0x15); LCD_WR_DATA8(0x15);
    LCD_WR_DATA8(0x2D); LCD_WR_DATA8(0x32);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0x96); LCD_WR_DATA8(0x08); LCD_WR_DATA8(0x0C); LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x09); LCD_WR_DATA8(0x25); LCD_WR_DATA8(0x2E); LCD_WR_DATA8(0x43);
    LCD_WR_DATA8(0x42); LCD_WR_DATA8(0x35); LCD_WR_DATA8(0x11); LCD_WR_DATA8(0x11);
    LCD_WR_DATA8(0x28); LCD_WR_DATA8(0x2E);

    LCD_WR_REG(0xF0); LCD_WR_DATA8(0x3C); // Закрыть ключ 1
    LCD_WR_REG(0xF0); LCD_WR_DATA8(0x69); // Закрыть ключ 2

    // === ПАТЧ РАЗВЕРТКИ: Открытие полных 480 строк по вертикали ===
    LCD_WR_REG(0x30); // Partial Area Command
    LCD_WR_DATA8(0x00); LCD_WR_DATA8(0x00); // Начальный адрес строки кадра (0)
    LCD_WR_DATA8(0x01); LCD_WR_DATA8(0xDF); // Конечный адрес строки кадра (479 = 0x01DF)

    delay_ms(120);

    LCD_WR_REG(0x21);    // Display Inversion ON
    LCD_WR_REG(0x29);    // Display ON

    // === ШАГ 4: ФИНАЛЬНАЯ КОНФИГУРАЦИЯ ЭКРАНА ===
    lcd_direction(USE_HORIZONTAL);

    lcd_clear(BLACK);
}


// ================================================================
// ИНИЦИАЛИЗАЦИЯ DMA2 ДЛЯ SPI1_TX
// ================================================================
// ВНИМАНИЕ: SPI1_TX использует DMA2_Stream5, Channel 3.
// Это НЕ конфликтует с SDIO (DMA2_Stream3, Channel 4) и I2S (DMA1_Stream4, Channel 0).
// ================================================================
void lcd_dma_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    __NOP(); __NOP();

    // === Настройка DMA2_Stream5 для SPI1_TX ===
    DMA2_Stream5->CR = 0;
    while (DMA2_Stream5->CR & DMA_SxCR_EN);

    // Очищаем флаги для Stream5 (HIFCR)
    DMA2->HIFCR = DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 |
                  DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5;

    // Channel 3 — для SPI1_TX
    DMA2_Stream5->CR |= (3U << DMA_SxCR_CHSEL_Pos);
     // Варианты: 0-Low, 1-Medium, 2-High, 3-VeryHigh
    DMA2_Stream5->CR |= (0U << DMA_SxCR_PL_Pos);     // Приоритет: High

    // 16-бит данные
    DMA2_Stream5->CR |= (1U << DMA_SxCR_MSIZE_Pos);  // 16-бит в памяти
    DMA2_Stream5->CR |= (1U << DMA_SxCR_PSIZE_Pos);  // 16-бит на периферии (SPI1_DR)

    // MINC по умолчанию ВЫКЛЮЧЕН — для lcd_fillrect()/lcd_clear()
    // (один и тот же цвет пишется много раз).
    // lcd_blit_buffer() включает MINC на время своей передачи
    // и обязательно выключает обратно после.
    DMA2_Stream5->CR &= ~DMA_SxCR_MINC;
    DMA2_Stream5->CR &= ~DMA_SxCR_PINC;

    DMA2_Stream5->CR |= (1U << DMA_SxCR_DIR_Pos);    // Memory-to-Peripheral

    DMA2_Stream5->PAR = (uint32_t)&(SPI1->DR);
}