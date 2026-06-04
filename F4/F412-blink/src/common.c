#include "common.h"
#include "pindefs.h"

volatile uint32_t ttms = 0;

void SysTick_Handler(void) {
    ttms++;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ttms;
    while ((ttms - start) < ms) {
        __WFI();
    }
}

void delay_nop(uint32_t ticks) {
    while (ticks--) {
        __NOP();
    }
}

void blink_led(uint16_t freq) {
    static uint32_t last_time = 0;                                              // Хранит снимок системного времени ttms (статическая локальная)

    // Проверка на переполнение счетчика или истечение интервала freq
    if (last_time > ttms || (ttms - last_time) >= freq) {
        LED_SYSTEM_TOGGLE;                                                      // Переключаем светодиод через наш макрос
        last_time = ttms;                                                       // Делаем новый снимок глобального времени ttms
    }
}

#define DEBUG  // Закомментируй эту строчку, если отладка осциллографом на PA8 больше не нужна

static void system_clock_config_100MHz(void) {
    // 1. Включаем внутренний генератор HSI для безопасной коммутации шин
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    // Переводим систему временно на безопасный HSI
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

    // 2. Включаем внешний кварцевый резонатор HSE (8 МГц)
    RCC->CR |= RCC_CR_HSEON;
    uint32_t startup_counter = 0;
    while (!(RCC->CR & RCC_CR_HSERDY)) {
        startup_counter++;
        if (startup_counter > 0x10000) {
            // Аварийный выход на HSI, если кварц не завелся
            RCC->PLLCFGR = (16U << RCC_PLLCFGR_PLLM_Pos) | (200U << RCC_PLLCFGR_PLLN_Pos) | (0U << RCC_PLLCFGR_PLLP_Pos) | (4U << RCC_PLLCFGR_PLLQ_Pos) | 0U;
            goto pll_start;
        }
    }

    // ЭТАЛОННАЯ КОНФИГУРАЦИЯ ПОД КВАРЦ 8 МГц ДЛЯ ЧЕСТНЫХ 100 МГц ЯДРА:
    // Вход 8 МГц / M(8) = 1 МГц.
    // 1 МГц * N(200) = 200 МГц — внутренняя частота генератора VCO.
    // 200 МГц / P(2) = 100 МГц — номинальная скорость процессора SYSCLK!
    RCC->PLLCFGR = (8U << RCC_PLLCFGR_PLLM_Pos)    |                            // Делитель M = 8 (Входной сигнал приводим к 1 МГц)
                   (200U << RCC_PLLCFGR_PLLN_Pos)  |                            // Множитель VCO N = 200
                   (0U << RCC_PLLCFGR_PLLP_Pos)    |                            // Делитель ядра P = 2 (биты 00 дают деление на 2 -> 100 МГц)
                   (4U << RCC_PLLCFGR_PLLQ_Pos)    |                            // Делитель Q = 4
                   RCC_PLLCFGR_PLLSRC_HSE;                                      // Источник — внешний кварц HSE

pll_start:
    // 3. Настройка задержек Flash памяти под финальные 100 МГц (3 цикла ожидания WS)
    FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    // 4. Запускаем умножитель частоты PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));                                         // Жестко ждем стабилизации частоты PLL

    // 5. Настройка делителей системных шин периферии
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);            
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;                                           // APB1 = 50 МГц (Максимум шины)
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;                                           // APB2 = 100 МГц (Максимум шины, тут наш SPI1!)

    // 6. Переключаем тактирование процессора (SYSCLK) на выходной сигнал PLL
    RCC->CFGR &= ~RCC_CFGR_SW;                                                  
    RCC->CFGR |= RCC_CFGR_SW_PLL;                                               
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);                     // Ждем от аппаратного переключателя подтверждения перехода

#ifdef DEBUG
    // --- КОНТРОЛЬНЫЙ ВЫВОД СИГНАЛА НА НОЖКУ MCO1 (PA8) ---
    RCC->CFGR &= ~RCC_CFGR_MCO1;
    RCC->CFGR |= (3U << 21);                                                    // Выбираем источник MCO1 = PLL (0b11)
    
    RCC->CFGR &= ~RCC_CFGR_MCO1PRE;
    RCC->CFGR |= (6U << 24);                                                    // Аппаратный делитель MCO1 на 4 (100 / 4 = 25.00 МГц)
#endif

    // Возвращаем SysTick на честные 100 МГц
    SysTick->LOAD = (100000000 / 1000) - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}


static void gpio_init(void) {
    // 1. Включаем тактование портов на шине AHB1 (ОБЯЗАТЕЛЬНО добавляем GPIOAEN!)
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
    
    // Фиктивное чтение для жесткой стабилизации тактов на шине AHB1
    volatile uint32_t dummy = RCC->AHB1ENR; 
    (void)dummy;
    __asm("nop");

    // Настройка системного светодиода PB2 на вывод (01)
    GPIOB->MODER   &= ~GPIO_MODER_MODER2;                                       
    GPIOB->MODER   |=  GPIO_MODER_MODER2_0;                                     
    GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR2;                                  

    // Настройка кнопки PC13 на чистый Вход (00, No pull)
    GPIOC->MODER   &= ~GPIO_MODER_MODER13;                                      
    GPIOC->PUPDR   &= ~GPIO_PUPDR_PUPDR13;                                      

#ifdef DEBUG
    // --- НАСТРОЙКА ПИНА PA8 ПОД ВЫВОД ТАКТОВ MCO1 ---
    // 1. Переводим пин PA8 в режим Альтернативной Функции (биты 10)
    GPIOA->MODER   &= ~(3U << (8 * 2));
    GPIOA->MODER   |=  (2U << (8 * 2));
    
    // 2. Выставляем максимальную скорость (Very High Speed: 11), чтобы ножка тянула 24-50 МГц
    GPIOA->OSPEEDR |=  (3U << (8 * 2));

    // 3. Привязываем PA8 к альтернативной функции AF00 (MCO1)
    GPIOA->AFR[1]  &= ~(GPIO_AFRH_AFSEL8);                                      // PA8 сидит в верхнем регистре AFR[1]
    GPIOA->AFR[1]  |=  (0U << GPIO_AFRH_AFSEL8_Pos);                            // Записываем код AF0
#endif
}

// Главная публичная функция инициализации
void System_Init(void) {
    system_clock_config_100MHz();                                               // Настраиваем клок и SysTick внутри модуля
    gpio_init();                                                                // Настраиваем порты внутри модуля
}

// --- НЕБЛОКИРУЮЩИЙ АВТОМАТ МОРЗЯНКИ ---
// Структура для кодирования одного символа Морзе в 1 байт
typedef struct {
    uint8_t len;                                                                // Количество элементов (точек и тире) в символе
    uint8_t code;                                                               // Битовая маска (0 - точка, 1 - тире), выровненная вправо
} MorseChar_t;

// Таблица Морзе: индексы 0..25 соответствуют буквам A..Z
static const MorseChar_t morse_alphabet[] = {
    {2, 0b01},    // A (.-)
    {4, 0b1000},  // B (-...)
    {4, 0b1010},  // C (-.-.)
    {3, 0b100},   // D (-..)
    {1, 0b0},     // E (.)
    {4, 0b0010},  // F (..-.)
    {3, 0b110},   // G (--.)
    {4, 0b0000},  // H (....)
    {2, 0b00},    // I (..)
    {4, 0b0111},  // J (.---)
    {3, 0b101},   // K (-.-)
    {4, 0b0100},  // L (.-..)
    {2, 0b11},    // M (--)
    {2, 0b10},    // N (-.)
    {3, 0b111},   // O (---)
    {4, 0b0110},  // P (.--.)
    {4, 0b1101},  // Q (--.-)
    {3, 0b010},   // R (.-.)
    {3, 0b000},   // S (...)
    {1, 0b1},     // T (-)
    {3, 0b001},   // U (..-)
    {4, 0b0001},  // V (...-)
    {3, 0b011},   // W (.--)
    {4, 0b1001},  // X (-..-)
    {4, 0b1011},  // Y (-.--)
    {4, 0b1100}   // Z (--..)
};

// Таблица Морзе: индексы 0..9 соответствуют цифрам 0..9
static const MorseChar_t morse_numbers[] = {
    {5, 0b11111}, // 0 (-----)
    {5, 0b01111}, // 1 (.----)
    {5, 0b00111}, // 2 (..---)
    {5, 0b00011}, // 3 (...--)
    {5, 0b00001}, // 4 (....-)
    {5, 0b00000}, // 5 (.....)
    {5, 0b10000}, // 6 (-....)
    {5, 0b11000}, // 7 (--...)
    {5, 0b11100}, // 8 (---..)
    {5, 0b11110}  // 9 (----.)
};

// --- ПОЛНОСТЬЮ УНИВЕРСАЛЬНЫЙ НЕБЛОКИРУЮЩИЙ АВТОМАТ МОРЗЯНКИ ---
void morse_send_nb(const char* str) {
    static enum { IDLE, NEXT_CHAR, SIGNAL_START, SIGNAL_HOLD, AP_GAP } state = IDLE;
    static const char* ptr = NULL;
    static uint32_t last_morse_time = 0;
    static uint32_t wait_duration = 0;
    static uint8_t morse_code = 0;
    static uint8_t morse_len = 0;

    // Сброс автомата извне при передаче NULL
    if (str == NULL) {
        LED_SYSTEM_OFF;
        state = IDLE;
        return;
    }

    if (state == IDLE) {
        ptr = str;
        state = NEXT_CHAR;
        last_morse_time = ttms;
        return;
    }

    switch (state) {
        case IDLE: break;
        case NEXT_CHAR:
            if (*ptr == '\0') {                                                 // Строка закончилась — пускаем её по кругу
                ptr = str;
            }
            char c = *ptr++;
            
            // Переводим символ в верхний регистр
            if (c >= 'a' && c <= 'z') c -= 32;

            if (c >= 'A' && c <= 'Z') {
                // Извлекаем конфигурацию символа из алфавитного массива
                morse_code = morse_alphabet[c - 'A'].code;
                morse_len  = morse_alphabet[c - 'A'].len;
            } 
            else if (c >= '0' && c <= '9') {
                // Извлекаем конфигурацию из массива цифр
                morse_code = morse_numbers[c - '0'].code;
                morse_len  = morse_numbers[c - '0'].len;
            } 
            else if (c == ' ') {
                // Пауза между словами (7 точек = 700 мс)
                wait_duration = 700;
                last_morse_time = ttms;
                state = AP_GAP;
                return;
            } 
            else {
                state = NEXT_CHAR;                                              // Игнорируем знаки препинания и неизвестные символы
                return;
            }

            state = SIGNAL_START;
            break;

        case SIGNAL_START:
            if (morse_len == 0) {
                wait_duration = 300;                                            // Пауза между буквами в слове (3 точки = 300 мс)
                last_morse_time = ttms;
                state = AP_GAP;
                break;
            }

            // Выделяем текущий бит маски (смотрим с конца)
            uint8_t is_dash = (morse_code >> (morse_len - 1)) & 1U;
            morse_len--;

            LED_SYSTEM_ON;                                                      // Зажигаем светодиод без использования delay
            wait_duration = is_dash ? 300 : 100;                                // Длительность: Тире = 300 мс, Точка = 100 мс
            last_morse_time = ttms;
            state = SIGNAL_HOLD;
            break;

        case SIGNAL_HOLD:
            if (ttms - last_morse_time >= wait_duration) {
                LED_SYSTEM_OFF;                                                 // Время удержания сигнала вышло — гасим диод
                wait_duration = 100;                                            // Пауза между элементами одной буквы (1 точка = 100 мс)
                last_morse_time = ttms;
                state = AP_GAP;
            }
            break;

        case AP_GAP:
            if (ttms - last_morse_time >= wait_duration) {
                // Если буква не закончилась — шлем следующую точку/тире, иначе переходим к новой букве
                state = (morse_len > 0) ? SIGNAL_START : NEXT_CHAR;
            }
            break;
    }
}