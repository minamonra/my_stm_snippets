#include "demo.h"
#include "ili9488x.h"
#include "common.h"   
#include "pindefs.h"  
#include "lcd_io.h"
#include "lcd_font.h"

#include "FreeMono14.h"
#include "FreeMonoBoldOblique16.h"
#include "FreeSans14.h"
#include "FreeSansBold14.h"


#include "lcd_draw_u8g2.h" // Подключаем интерфейс нового шрифта


// ДЕМО: Проверка портированного фабричного ядра U8g2 на дисплее ILI9488
// ДЕМО: Проверка портированного фабричного ядра U8g2 на дисплее ILI9488
void demo_show_u8g2_font1(void) {
    // 1. Полная очистка экрана через ваш DMA конвейер в глубокий синий цвет
    ili9488_Clear(ILI9488_NAVY); 
    
    // 2. Рисуем бирюзовую рамку вокруг будущих тестовых строк
    ili9488_DrawRect(5, 5, 470, 310, ILI9488_CYAN);
    ili9488_DrawRect(7, 7, 466, 306, ILI9488_CYAN);

    // ============= ЗАГОЛОВОК =============
    lcd_draw_u8g2_string(15, 25, "U8G2 ФАБРИЧНЫЙ ДЕКОДЕР НА ILI9488", ILI9488_YELLOW, ILI9488_NAVY);
    ili9488_DrawFastHLine(15, 32, 450, ILI9488_CYAN);
    
    // ============= РУССКИЙ АЛФАВИТ (прописные и строчные) =============
    lcd_draw_u8g2_string(15, 55, "РУССКИЙ: АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ", ILI9488_WHITE, ILI9488_NAVY);
    lcd_draw_u8g2_string(15, 75, "         абвгдеёжзийклмнопрстуфхцчшщъыьэюя", ILI9488_LIGHTGREY, ILI9488_NAVY);
    
    // ============= ЛАТИНИЦА =============
    lcd_draw_u8g2_string(15, 98, "LATIN:   ABCDEFGHIJKLMNOPQRSTUVWXYZ", ILI9488_GREEN, ILI9488_NAVY);
    lcd_draw_u8g2_string(15, 118, "         abcdefghijklmnopqrstuvwxyz", ILI9488_GREEN, ILI9488_NAVY);
    
    // ============= ЦИФРЫ И СПЕЦСИМВОЛЫ =============
    lcd_draw_u8g2_string(15, 141, "DIGITS:  012345678901234567890123456789", ILI9488_CYAN, ILI9488_NAVY);
    lcd_draw_u8g2_string(15, 161, "SYMBOLS: !@#$%%^&*()_+-=[]{}|;:'\",.<>/?~`", ILI9488_MAGENTA, ILI9488_NAVY);
    
    // ============= ПРИКОЛЬНЫЕ ФРАЗЫ (плотно) =============
    ili9488_DrawFastHLine(15, 170, 450, ILI9488_ORANGE);
    
    lcd_draw_u8g2_string(15, 190, ">  Зачем мы здесь тут? А чтобы течь! 1337   <", ILI9488_YELLOW, ILI9488_NAVY);
    lcd_draw_u8g2_string(15, 210, ">          Чтобы течь и раздавать стиль!    <", ILI9488_LIME, ILI9488_NAVY);
    lcd_draw_u8g2_string(15, 230, "> Co6aka - 9to He geHbfu, co6aka - 9to gpyr <", ILI9488_ORANGE, ILI9488_NAVY);
    lcd_draw_u8g2_string(15, 250, "> А если OHO pa6oTaeT, He Tporaй! (c)       <", ILI9488_PINK, ILI9488_NAVY);
    lcd_draw_u8g2_string(15, 270, "> 42 - OTBET HA ГЛАВНЫЙ ВОПРОС              <", ILI9488_CYAN, ILI9488_NAVY);
    
    // ============= СТРОКА СТАТУСА (внизу) =============
    ili9488_DrawFastHLine(15, 280, 450, ILI9488_GOLD);
    lcd_draw_u8g2_string(15, 300, "SPEED:100 MHz|SPI:50 MHz| U8G2:PORTED|DMA: OK", ILI9488_GOLD, ILI9488_NAVY);
}



void demo_show_u8g2_font1_dense(void) {
    ili9488_Clear(ILI9488_BLACK);
    
    // Тонкая рамка
    ili9488_DrawRect(2, 2, 476, 316, ILI9488_DARKGREY);
    
    // СТРОКИ С МИНИМАЛЬНЫМИ ОТСТУПАМИ (шаг ~18-20 пикселей)
    lcd_draw_u8g2_string(5, 20,  "=== U8G2 FONT ENGINE v2.33 ===", ILI9488_YELLOW, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 38,  "ABCDEFGHIJKLMNOPQRSTUVWXYZ", ILI9488_GREEN, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 56,  "abcdefghijklmnopqrstuvwxyz", ILI9488_GREEN, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 74,  "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ", ILI9488_CYAN, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 92,  "абвгдеёжзийклмнопрстуфхцчшщъыьэюя", ILI9488_CYAN, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 110, "0123456789 0123456789 0123456789", ILI9488_MAGENTA, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 128, "!@#$%%^&*()_+[]{}|;:'\",.<>?/`~", ILI9488_ORANGE, ILI9488_BLACK);
    
    // РАЗДЕЛИТЕЛЬ
    for(int i = 0; i < 476; i+=10) ili9488_DrawPixel(5+i, 138, ILI9488_GRAY);
    
    // ПРИКОЛЬНЫЕ СТРОКИ
    lcd_draw_u8g2_string(5, 158, "Knock, knock, Neo... The U8G2 has you...", ILI9488_GREEN, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 176, "I know kung fu. - Show me. (u8g2_DrawStr)", ILI9488_YELLOW, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 194, "There is no spoon... There is only buffer!", ILI9488_CYAN, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 212, "What is real? How do you define 'real'?", ILI9488_MAGENTA, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 230, "If by 'real' you mean what you can draw...", ILI9488_ORANGE, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 248, "...then 'real' is simply pixels in your RAM.", ILI9488_ORANGE, ILI9488_BLACK);
    
    // СМАЙЛИКИ И ПРИКОЛЫ
    lcd_draw_u8g2_string(5, 268, ">_  8==3  ^_^  (◕‿◕)  [*_*]  (>_<)  (¬_¬)  ", ILI9488_PINK, ILI9488_BLACK);
    
    // СТРОКА СТАТУСА
    lcd_draw_u8g2_string(5, 290, "[ STM32F412 ] [ 100 MHz ] [ SPI ] [ ILI9488 ] [ OK ]", ILI9488_GREEN, ILI9488_BLACK);
    lcd_draw_u8g2_string(5, 310, "U8G2 ported by Klaus + HVLine hack - Now with cursive!", ILI9488_GRAY, ILI9488_BLACK);
}


// ДЕМО: Абсолютно максимальная плотность (20 строк текста!)
void demo_show_u8g2_font1_maxdense(void) {
    ili9488_Clear(ILI9488_BLACK);
    
    // Цвета для разных строк (радуга)
    const uint16_t colors[] = {
        ILI9488_RED, ILI9488_ORANGE, ILI9488_YELLOW, ILI9488_GREEN, 
        ILI9488_CYAN, ILI9488_BLUE, ILI9488_MAGENTA, ILI9488_PINK,
        ILI9488_LIME, ILI9488_GOLD, ILI9488_WHITE, ILI9488_LIGHTGREY
    };
    
    const char *lines[] = {
        "1. U8G2 + ILI9488 = BEST FRIENDS FOREVER",
        "2. PoPuP - g0nNa gIvE yOu Up!!!",
        "3. Never gonna let you down (c) 1987",
        "4. 0xDEADBEEF - dinner is ready!",
        "5. rm -rf / --no-preserve-root <-- DONT!",
        "6. 42 - answer to life, universe, everything",
        "7. sudo make me a sandwich (permission denied)",
        "8. Windows is loading... Just kidding!",
        "9. 404: sense of humor not found",
        "10. I'm 16 kilobytes of raw text power!",
        "11. U8g2: turning pixels into poetry since 2014",
        "12. SPI bus goes brrrrrrrrr...",
        "13. DMA: I'm fast as lightning BOY!",
        "14. Stack overflow? Never heard of her.",
        "15. Q: Why STM32? A: Why NOT?",
        "16. This text is so dense, it bends light!",
        "17. 95% of statistics are made up on spot",
        "18. Trust me, I'm an engineer (c)",
        "19. Keep calm and printf(\"Hello World\");",
        "20. THE END... Or is it just the beginning?"
    };
    
    int num_lines = sizeof(lines) / sizeof(lines[0]);
    int y_pos = 18;  // Стартовая позиция (минимально возможная)
    
    for(int i = 0; i < num_lines && y_pos < 310; i++) {
        lcd_draw_u8g2_string(3, y_pos, lines[i], colors[i % (sizeof(colors)/sizeof(colors[0]))], ILI9488_BLACK);
        y_pos += 17;  // Шаг 17 пикселей - супер плотно!
    }
}

// void demo_show_u8g2_font(void) {
//     // Чистим экран в синий, чтобы проверить четкость краев
//     ili9488_Clear(ILI9488_NAVY); 
    
//     // Печатаем строки
//     lcd_draw_u8g2_string(20, 60,  "ШРИФТ: X11 10X20 ТЕСТ",    ILI9488_CYAN,   ILI9488_NAVY);
//     lcd_draw_u8g2_string(20, 120, "СТАТУС ПЕРИФЕРИИ: ОК",   ILI9488_GREEN,  ILI9488_NAVY);
//     lcd_draw_u8g2_string(20, 180, "СТАБИЛЬНОСТЬ: Ь Л Я Э",  ILI9488_WHITE,  ILI9488_NAVY);
//     lcd_draw_u8g2_string(20, 240, "0123456789", ILI9488_YELLOW, ILI9488_NAVY);
// }

// ДЕМО 1: Чистый современный гротеск (Без засечек)
void demo_show_font_FreeSans14(void) {
    ili9488_Clear(ILI9488_BLACK);
    lcd_set_font(&FreeSans14pt8b);

    ili9488_DrawRect(10, 10, 460, 300, ILI9488_TEAL);

    lcd_draw_string(25, 45,  "FreeSans 14pt (Гротеск)",    ILI9488_CYAN, ILI9488_BLACK);
    ili9488_DrawFastHLine(25, 55, 430, ILI9488_TEAL);

    lcd_draw_string(25, 95,  "ЗАГЛАВНЫЕ: АБВГДЕЖЗИЙКЛМНОП", ILI9488_WHITE, ILI9488_BLACK);
    lcd_draw_string(25, 140, "строчные: фхцчшщъыьэюяё 123",  ILI9488_WHITE, ILI9488_BLACK);
    lcd_draw_string(25, 185, "English: Fox jumps over dog",  ILI9488_YELLOW, ILI9488_BLACK);
    lcd_draw_string(25, 230, "Цифры: 0123456789",            ILI9488_GREEN, ILI9488_BLACK);
    lcd_draw_string(25, 275, "Знаки: !@#$%^&*()_+-=",         ILI9488_LIGHTGREY, ILI9488_BLACK);
}

// ДЕМО 2: Строгий моноширинный (Тот, что на вашем фото)
void demo_show_font_FreeMono14(void) {
    ili9488_Clear(ILI9488_BLACK);
    lcd_set_font(&FreeMono14pt8b);

    ili9488_DrawRect(10, 10, 460, 300, ILI9488_GOLD);

    lcd_draw_string(25, 45,  "FreeMono 14pt (Моно)",       ILI9488_GOLD, ILI9488_BLACK);
    ili9488_DrawFastHLine(25, 55, 430, ILI9488_GOLD);

    lcd_draw_string(25, 95,  "CH1: 23.5 C [СТАБИЛЬНО]",    ILI9488_WHITE, ILI9488_BLACK);
    lcd_draw_string(25, 140, "CH2: 12.0 V [НОРМА]",         ILI9488_GREEN, ILI9488_BLACK);
    lcd_draw_string(25, 185, "CH3: 0.45 A [ВНИМАНИЕ]",      ILI9488_YELLOW, ILI9488_BLACK);
    lcd_draw_string(25, 230, "Пакеты UTF8: 1420 ок",        ILI9488_CYAN, ILI9488_BLACK);
    lcd_draw_string(25, 275, "Ошибки: 0 (0.0%)",            ILI9488_LIGHTGREY, ILI9488_BLACK);
}

// ДЕМО 3: Крупный жирный наклонный
void demo_show_font_FreeMonoBoldOblique16(void) {
    ili9488_Clear(ILI9488_BLACK);
    lcd_set_font(&FreeMonoBoldOblique16pt8b);

    ili9488_DrawRect(10, 10, 460, 300, ILI9488_ORANGE);

    lcd_draw_string(25, 55,  "FreeMonoBO 16pt",            ILI9488_ORANGE, ILI9488_BLACK);
    ili9488_DrawFastHLine(25, 70, 420, ILI9488_ORANGE);

    lcd_draw_string(25, 130, "СТАТУС: ТРЕВОГА!",           ILI9488_RED,   ILI9488_BLACK);
    lcd_draw_string(25, 195, "ЯДРО: 100 МГЦ",              ILI9488_GREEN, ILI9488_BLACK);
    lcd_draw_string(25, 260, "ПРОВЕРЬТЕ SPI1",             ILI9488_WHITE, ILI9488_BLACK);
}

// ДЕМО 4: Тест жирного шрифта без засечек FreeSansBold 14pt
void demo_show_font_FreeSansBold14(void) {
    ili9488_Clear(ILI9488_BLACK);
    
    // Включаем новый жирный шрифт
    lcd_set_font(&FreeSansBold14pt8b);

    // Рисуем рамку цвета Маджента (пурпурный)
    ili9488_DrawRect(10, 10, 460, 300, ILI9488_MAGENTA);

    // Заголовок экрана (жирным шрифтом выглядит очень солидно)
    lcd_draw_string(25, 45,  "FreeSansBold 14pt",          ILI9488_MAGENTA, ILI9488_BLACK);
    ili9488_DrawFastHLine(25, 55, 430, ILI9488_MAGENTA);

    // Тестовые строки с важными статусами приборной панели
    lcd_draw_string(25, 95,  "ГЛАВНОЕ МЕНЮ СИСТЕМЫ",       ILI9488_WHITE, ILI9488_BLACK);
    lcd_draw_string(25, 140, "СТАТУС: ПОДКЛЮЧЕНО",          ILI9488_GREEN, ILI9488_BLACK);
    lcd_draw_string(25, 185, "РЕЖИМ: АВТОМАТИКА",          ILI9488_YELLOW, ILI9488_BLACK);
    lcd_draw_string(25, 230, "English: Bold Sans Test",     ILI9488_CYAN, ILI9488_BLACK);
    lcd_draw_string(25, 275, "Цифры: 0123456789",            ILI9488_LIGHTGREY, ILI9488_BLACK);
}







void demo_test_fonts(void) {
    // Полная очистка экрана через DMA
    ili9488_Clear(ILI9488_BLACK);
    
    // Включаем новый шрифт
    lcd_set_font(&FreeSans14pt8b);

    // Сетка тестов по оси Y (помним, что Y — это базовая линия шрифта, а не верх)
    lcd_draw_string(20, 40,  "FreeSans 14pt (Тест)", ILI9488_CYAN,    ILI9488_BLACK);
    lcd_draw_string(20, 90,  "АБВГДЕЖЗИЙКЛМНОПРСТУ",  ILI9488_WHITE,   ILI9488_BLACK);
    lcd_draw_string(20, 140, "фхцчшщъыьэюяё 12345",  ILI9488_WHITE,   ILI9488_BLACK);
    lcd_draw_string(20, 190, "Quick brown fox jumps", ILI9488_YELLOW,  ILI9488_BLACK);
    lcd_draw_string(20, 240, "0123456789 !@#$%^&*()", ILI9488_GREEN,   ILI9488_BLACK);

    // Подчеркнем нижнюю границу красивой линией
    ili9488_DrawFastHLine(20, 270, 440, ILI9488_MAROON);
}


// Глобальные неизменяемые строки во Flash-памяти (0% риска порчи и оптимизации)
static const char scroll_text[]   = "МОНИТОР ТЕЛЕМЕТРИИ ДИСПЛЕЯ ILI9488 И ПЛАТЫ WEACT STM32F412";
static const char telemetry_msg[] = "Ядро: 100 MHz";

// Глобальные буферы в ОЗУ (Защита от Stack Overflow и ложной оптимизации GCC 15)
static char fps_buf[16];

void draw_dynamic_dashboard_old(uint16_t duration_sec) {
    ili9488_Clear(ILI9488_BLACK);
    lcd_set_font(&FreeMonoBoldOblique16pt8b);

    // Рисуем бирюзовую рамку интерфейса
    ili9488_FillRect(5, 5, 470, 4, ILI9488_CYAN);
    ili9488_FillRect(5, 311, 470, 4, ILI9488_CYAN);
    ili9488_FillRect(5, 5, 4, 310, ILI9488_CYAN);
    ili9488_FillRect(471, 5, 4, 310, ILI9488_CYAN);
    
    // Статичные левые заголовки табло
    lcd_draw_string(20, 45,  "СИСТЕМА:",     ILI9488_WHITE, ILI9488_BLACK);
    lcd_draw_string(20, 110, "МОНИТОР:",     ILI9488_WHITE, ILI9488_BLACK);
    lcd_draw_string(20, 175, "ПОТОК ДАННЫХ:", ILI9488_WHITE, ILI9488_BLACK);

    // Считаем ширину бегущего текста один раз на входе
    uint16_t text_width = lcd_get_string_width(scroll_text);

    int16_t scroll_x = 450; 
    uint32_t frame_counter = 0;
    uint32_t total_frames = duration_sec * 25; 

    // Жестко фиксируем шаблон строки FPS в безопасной памяти ОЗУ
    fps_buf[0] = 'F'; fps_buf[1] = 'P'; fps_buf[2] = 'S'; fps_buf[3] = ':';
    fps_buf[4] = ' '; fps_buf[5] = '3'; fps_buf[6] = '0'; fps_buf[7] = 0;

    // СЕТКА КООРДИНАТ: Выравниваем всё строго по линеечке
    // Левые заголовки заканчиваются на X=170. Ставим значения на X=180
    uint16_t row1_val_x = 180; 
    uint16_t row2_val_x = 180; 
    // FPS сдвигаем левее, на честную координату X=300, чтобы он гарантированно влез на экран
    uint16_t fps_val_x  = 300; 

    for (uint32_t f = 0; f < total_frames; f++) {
        frame_counter++;
        
        // Безопасно обновляем динамическую цифру кадров внутри массива ОЗУ
        fps_buf[6] = '0' + (28 + (frame_counter % 3));

        // СТРОКА 1: Выводим частоту ядра из Flash памяти
        lcd_draw_string(row1_val_x, 45, telemetry_msg, ILI9488_GREEN, ILI9488_BLACK);

        // СТРОКА 2, ЛЕВАЯ ЧАСТЬ: Выводим стабильный статус
        if ((frame_counter / 12) & 1) {
            lcd_draw_string(row2_val_x, 110, "СТАТУС: ОК", ILI9488_GREEN, ILI9488_BLACK);
        } else {
            lcd_draw_string(row2_val_x, 110, "СТАТУС: ОК", ILI9488_WHITE, ILI9488_BLACK);
        }

        // СТРОКА 2, ПРАВАЯ ЧАСТЬ: Выводим тикающий FPS на координате 300
        lcd_draw_string(fps_val_x, 110, fps_buf, ILI9488_CYAN, ILI9488_BLACK);

        // --- ДИНАМИКА БЕГУЩЕЙ СТРОКИ ---
        scroll_x -= 4; // Сдвигаем влево на 4 пикселя
        
        // Сброс строки, когда она полностью уплывет за левую границу рамки (10 пикселей)
        if (scroll_x < (10 - text_width - 40)) {
            // Перед сбросом полностью чистим только горизонтальную зону бегущей строки
            ili9488_FillRect(10, 230, 450, 45, ILI9488_BLACK);
            scroll_x = 460; // Начинаем выплывать заново с правого борта
        }

        // Выводим бегущую строку (Y опущен на безопасные 245 пикселей)
        lcd_draw_string(scroll_x, 245, scroll_text, ILI9488_YELLOW, ILI9488_BLACK);

        // Стабильный фреймрейт анимации (~30 FPS)
        delay_ms(15);
    }
}





void run_speed_test(void) {
    lcd_set_font(&FreeMonoBoldOblique16pt8b);
    
    // Массив тестовых цветов RGB565
    uint16_t test_colors[4] = {0xF800, 0x07E0, 0x001F, 0xFFFF}; // Красный, Зеленый, Синий, Белый
    
    uint32_t start_time = ttms; // Запоминаем стартовое время в миллисекундах
    uint32_t iterations = 100;  // 100 полных заливок экрана 480х320
    
    for (uint32_t i = 0; i < iterations; i++) {
        // Меняем цвет на каждом шаге
        uint16_t current_color = test_colors[i % 4];
        
        // Вызываем нашу оптимизированную функцию заливки всего экрана
        ili9488_Clear(current_color);
    }
    
    uint32_t end_time = ttms; // Фиксируем время окончания
    uint32_t total_time = end_time - start_time; // Общее время в мс
    
    // Считаем чистый FPS: (Количество кадров * 1000) / Всего миллисекунд
    uint32_t fps = (iterations * 1000) / total_time;
    
    // Выводим результат теста прямо поверх получившегося фона
    ili9488_FillRect(30, 100, 420, 120, ILI9488_BLACK);
    ili9488_FillRect(35, 105, 410, 110, ILI9488_CYAN);
    
    static char result_buf[16];
    // Собираем строку FPS: "FPS: XX" без использования sprintf
    result_buf[0] = 'F'; result_buf[1] = 'P'; result_buf[2] = 'S'; result_buf[3] = ':'; result_buf[4] = ' ';
    result_buf[5] = (fps / 10) + '0';
    result_buf[6] = (fps % 10) + '0';
    result_buf[7] = 0;
    
    lcd_draw_string(60, 160, "ТЕСТ СКОРОСТИ:", ILI9488_BLACK, ILI9488_CYAN);
    lcd_draw_string(300, 160, result_buf, ILI9488_RED, ILI9488_CYAN);
    
    // Замираем на 5 секунд, чтобы рассмотреть результат скорости
    delay_ms(5000);
}



void draw_dynamic_dashboard(uint16_t duration_sec) {
    // 1. Быстрая очистка экрана через DMA
    ili9488_Clear(ILI9488_BLACK);
    
    // 2. Устанавливаем новый чистый шрифт без засечек
    lcd_set_font(&FreeSans14pt8b);

    // 3. Рисуем бирюзовую рамку интерфейса через наши оптимизированные DMA-линии
    // Вместо четырех FillRect используем DrawRect с толщиной, либо DrawFast
    // Для сохранения вашей точной толщины в 4 пикселя сделаем аккуратный контур:
    for (uint8_t i = 0; i < 4; i++) {
        ili9488_DrawRect(5 + i, 5 + i, 470 - (i * 2), 310 - (i * 2), ILI9488_CYAN);
    }
    
    // 4. Статичные левые заголовки табло
    // Внимание: у FreeSans14 базовая линия (Y) и высота глифов немного отличаются от FreeMono,
    // поэтому координаты 45, 110, 175 должны встать идеально ровно.
    lcd_draw_string(20, 45,  "СИСТЕМА:",      ILI9488_WHITE, ILI9488_BLACK);
    lcd_draw_string(20, 110, "МОНИТОР:",      ILI9488_WHITE, ILI9488_BLACK);
    lcd_draw_string(20, 175, "ПОТОК ДАННЫХ:", ILI9488_WHITE, ILI9488_BLACK);

    // Пример вывода динамических данных (зеленый цвет для статуса)
    lcd_draw_string(250, 45, "ОК (АКТИВНА)",  ILI9488_GREEN, ILI9488_BLACK);
}







// Демо-эффект: бесконечный вортекс со строгой геометрией
void draw_infinity_vortex(uint16_t iterations) {
    for (uint16_t it = 0; it < iterations; it++) {
        uint16_t x = 0, y = 0, w = ILI9488_WIDTH, h = ILI9488_HEIGHT;
        uint16_t step = 6;
        uint16_t color_seed = (uint16_t)(it * 1350);
        
        uint16_t shift_x = step + (it % 2);
        uint16_t shift_y = step + ((it + 1) % 2);
        
        while (w > (shift_x * 2) && h > (shift_y * 2)) {
            uint16_t color = (uint16_t)(color_seed + (w * 7));
            
            ili9488_FillRect(x, y, w, step, color);                          
            ili9488_FillRect(x, y + h - step, w, step, color);               
            ili9488_FillRect(x, y + step, step, h - step * 2, color);        
            ili9488_FillRect(x + w - step, y + step, step, h - step * 2, color); 
            
            x += shift_x; 
            y += shift_y; 
            w -= (shift_x * 2); 
            h -= (shift_y * 2);
        }
        delay_ms(50);
    }
}


// Демо-эффект: скоростная шахматная доска
void draw_chessboard(uint16_t size, uint16_t color1, uint16_t color2) {
    if (size == 0) size = 10;
    
    for (uint16_t y = 0; y < ILI9488_HEIGHT; y += size) {
        uint16_t height = ((y + size) > ILI9488_HEIGHT) ? (ILI9488_HEIGHT - y) : size;
        uint16_t row_idx = y / size;
        
        for (uint16_t x = 0; x < ILI9488_WIDTH; x += size) {
            uint16_t width = ((x + size) > ILI9488_WIDTH) ? (ILI9488_WIDTH - x) : size;
            uint16_t col_idx = x / size;
            
            uint16_t current_color = ((row_idx + col_idx) & 1) ? color2 : color1;
            ili9488_FillRect(x, y, width, height, current_color);
        }
    }
}


// НОВЫЙ ТЕСТ 3: "Вертикальные Жалюзи" (Быстрая смена тонких вертикальных окон)
void draw_vertical_blinds(uint16_t loops) {
    uint16_t blind_width = 16;                                                  // Ширина одной створки жалюзи
    
    for (uint16_t lp = 0; lp < loops; lp++) {
        uint16_t global_color = (uint16_t)(lp * 4521);
        
        // Захлопываем створки слева направо по всему экрану
        for (uint16_t x = 0; x < 320; x += blind_width) {
            uint16_t color = (uint16_t)(global_color ^ (x * 3));
            ili9488_FillRect(x, 0, blind_width, 480, color);
            delay_ms(5);                                                        // Эффект плавного нарастания занавеса
        }
    }
}

void draw_random_rects(uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        uint16_t x = get_random(300);
        uint16_t y = get_random(460);
        uint16_t w = 10 + get_random(320 - x - 10);
        uint16_t h = 10 + get_random(480 - y - 10);
        uint16_t color = get_random(0xFFFF);
        ili9488_FillRect(x, y, w, h, color);
        delay_ms(5);
    }
}

void draw2_random_rects(uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        // Генерируем случайную начальную точку
        uint16_t x = get_random(ILI9488_WIDTH);
        uint16_t y = get_random(ILI9488_HEIGHT);
        
        // Ограничиваем максимальный размер прямоугольника (например, до 40x40 пикселей)
        uint16_t w = get_random(40) + 4; 
        uint16_t h = get_random(40) + 4;
        
        // Генерируем абсолютно любой случайный цвет RGB565
        uint16_t color = get_random(0xFFFF);
        
        // Рисуем элемент через наш безопасный и быстрый конвейер
        ili9488_FillRect(x, y, w, h, color);
    }
}

void draw_nested_frames(uint16_t color1, uint16_t color2) {
    uint16_t x = 0, y = 0, w = 320, h = 480, step = 8, toggle = 0;
    while (w > step * 2 && h > step * 2) {
        uint16_t current_color = (toggle++ % 2 == 0) ? color1 : color2;
        ili9488_FillRect(x, y, w, step, current_color);
        ili9488_FillRect(x, y + h - step, w, step, current_color);
        ili9488_FillRect(x, y + step, step, h - step * 2, current_color);
        ili9488_FillRect(x + w - step, y + step, step, h - step * 2, current_color);
        x += step; y += step; w -= step * 2; h -= step * 2;
        delay_ms(25);
    }
}

