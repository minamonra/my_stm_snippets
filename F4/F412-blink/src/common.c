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

static void system_clock_config_100MHz(void) {
    RCC->CR |= RCC_CR_HSEON;                                                    // Включаем внешний кварцевый резонатор HSE (25 МГц)
    while (!(RCC->CR & RCC_CR_HSERDY));                                         // Ожидаем стабилизации тактового сигнала от HSE

    // Для частоты 100 МГц при питании 3.3В по даташиту требуется 3 цикла ожидания Flash (3WS)
    FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN; // Настройка задержки Flash, включение префетча и кэша инструкций/данных

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);            // Очищаем старые делители системных шин AHB, APB1 и APB2
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;                                           // Делитель APB1 равен 2 (Частота шины периферии APB1 = 50 МГц, максимум 50)
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;                                           // Делитель APB2 равен 1 (Частота шины периферии APB2 = 100 МГц, максимум 100)

    // Конфигурация умножителя PLL: Вход 25 МГц / M(25) * N(200) / P(2) = 100 МГц системной частоты
    RCC->PLLCFGR = (25 << RCC_PLLCFGR_PLLM_Pos) |                               // Делитель входной частоты M=25 (Приводим вход к стабильной частоте 1 МГц)
                   (200 << RCC_PLLCFGR_PLLN_Pos) |                              // Множитель частоты генератора VCO N=200 (Получаем внутренние 200 МГц)
                   (0 << RCC_PLLCFGR_PLLP_Pos)  |                               // Делитель системного ядра P=2 (Значение битов 00 делит VCO на 2 -> 100 МГц)
                   (4 << RCC_PLLCFGR_PLLQ_Pos)  |                               // Делитель частоты для USB/SDIO Q=4 (Дает ровно необходимые 48 МГц)
                   RCC_PLLCFGR_PLLSRC_HSE;                                      // Источником опорной тактовой частоты для PLL выбираем HSE

    RCC->CR |= RCC_CR_PLLON;                                                    // Включаем основной блок умножителя частоты PLL
    while (!(RCC->CR & RCC_CR_PLLRDY));                                         // Ожидаем стабилизации и захвата частоты генератором PLL

    RCC->CFGR |= RCC_CFGR_SW_PLL;                                               // Переключаем тактирование процессора (SYSCLK) на выходной сигнал PLL
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);                     // Ожидаем от аппаратного контроллера подтверждения перехода на PLL

    // Ручной запуск системного таймера SysTick на прерывания каждые 1 мс
    SysTick->LOAD = (F_CPU / 1000) - 1;                                         // Задаем период перезагрузки счетчика на основе частоты из Makefile
    SysTick->VAL  = 0;                                                          // Принудительно обнуляем текущее значение счетчика SysTick
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |                                // Задаем источник тактов — полная частота процессора без делителей
                    SysTick_CTRL_TICKINT_Msk   |                                // Разрешаем генерацию прерывания SysTick_Handler при обнулении
                    SysTick_CTRL_ENABLE_Msk;                                    // Включаем системный таймер SysTick в работу
}

static void gpio_init(void) {
    // 1. Включаем тактование необходимых портов на шине AHB1
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
    __asm("nop");                                                               // Пауза для стабилизации тактов

    // Настройка системного светодиода PB2 на вывод
    GPIOB->MODER   &= ~GPIO_MODER_MODER2;                                       // Очищаем оба бита настройки пина PB2 (в 00)
    GPIOB->MODER   |=  GPIO_MODER_MODER2_0;                                     // Выставляем режим вывода (General purpose output: 01)
    GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR2;                                  // Сбрасываем скорость в Low speed (00)

    // Исправленная настройка кнопки PC13 на чистый Вход (No Pull-up, No Pull-down)
    GPIOC->MODER   &= ~GPIO_MODER_MODER13;                                      // Режим: Вход (00)
    GPIOC->PUPDR   &= ~GPIO_PUPDR_PUPDR13;                                      // Сбрасываем в 00 (No pull)
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