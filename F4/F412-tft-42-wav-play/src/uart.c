#include "stm32f4xx.h"
#include "uart.h"

// Жесткие константы частот шин на основе конфигурации тактирования (SYSCLK = 96 МГц)
#define ACTUAL_APB2_CLOCK 96000000U  // Для USART1 (так как делитель PPRE2 = DIV1)
#define ACTUAL_APB1_CLOCK 48000000U  // Для USART3 (так как делитель PPRE1 = DIV2)

volatile uint8_t uart_command = 0;   // В начале файла, после includes

// Потокобезопасный кольцевой буфер для приема UART1
static volatile uint8_t  rx1_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx1_head = 0;
static volatile uint16_t rx1_tail = 0;

// Потокобезопасный кольцевой буфер для приема UART3
static volatile uint8_t  rx3_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx3_head = 0;
static volatile uint16_t rx3_tail = 0;

// Вспомогательная функция для рекурсивного вывода числа
static void uart1_write_num_recursive(uint32_t num) {
    if (num / 10) {
        uart1_write_num_recursive(num / 10);
    }
    uart1_write((uint8_t)('0' + (num % 10)));
}

// Расчет BRR для OVER8 = 0 (семплирование 16 раз для максимальной помехоустойчивости)
static void usart_set_brr(USART_TypeDef* usart, uint32_t baud, uint32_t apb_clk) {
    usart->BRR = (apb_clk + baud / 2U) / baud;
}

// ============================================================================
// === USART1 (PA9/TX, PA10/RX) ===============================================
// ============================================================================
void uart1_init(uint32_t baudrate) {
    // 1. Тактирование GPIOA и USART1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    // 2. Конфигурация выводов (Альтернативная функция AF7)
    // PA9 – TX
    GPIOA->MODER   &= ~(3U << (9 * 2));
    GPIOA->MODER   |=  (2U << (9 * 2));
    GPIOA->OSPEEDR &= ~(3U << (9 * 2));
    GPIOA->OSPEEDR |=  (1U << (9 * 2));  // Medium Speed (Убирает ВЧ-звон и выбросы в линии)
    GPIOA->PUPDR   &= ~(3U << (9 * 2));
    GPIOA->OTYPER  &= ~(1U << 9);        // Push-pull
    GPIOA->AFR[1]  &= ~(0xF << ((9U - 8U) * 4));
    GPIOA->AFR[1]  |=  (0x7U << ((9U - 8U) * 4));

    // PA10 – RX
    GPIOA->MODER   &= ~(3U << (10 * 2));
    GPIOA->MODER   |=  (2U << (10 * 2));
    GPIOA->OSPEEDR &= ~(3U << (10 * 2));
    GPIOA->OSPEEDR |=  (1U << (10 * 2));  // Medium Speed
    GPIOA->PUPDR   |=  (1U << (10 * 2));  // Pull-up для стабильного удержания линии Idle (высокий уровень)
    GPIOA->AFR[1]  &= ~(0xF << ((10U - 8U) * 4));
    GPIOA->AFR[1]  |=  (0x7U << ((10U - 8U) * 4));

    // 3. Конфигурация USART1 (8N1, 1 стоп-бит)
    USART1->CR1 = 0;
    USART1->CR2 = 0;
    USART1->CR3 = 0;

    usart_set_brr(USART1, baudrate, ACTUAL_APB2_CLOCK);

    // Принудительное чтение регистров для сброса мусора, возникшего при старте питания
    volatile uint32_t dummy = USART1->SR;
    dummy = USART1->DR;
    (void)dummy;

    // Включение TX, RX и прерывания по приему (RXNE)
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    USART1->CR1 |= USART_CR1_UE;  // Включение модуля в работу

    // 4. Настройка приоритетов и включение прерывания в NVIC
    NVIC_SetPriority(USART1_IRQn, 5);
    NVIC_EnableIRQ(USART1_IRQn);
}

void uart1_write(uint8_t ch) {
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = ch;
}

void uart1_write_str(const char* str) {
    while (*str) {
        uart1_write((uint8_t)*str++);
    }
}

void uart1_write_bytes(const uint8_t* data, uint16_t size) {
    for (uint16_t i = 0U; i < size; ++i) {
        uart1_write(data[i]);
    }
}

void uart1_write_num(uint32_t num) {
    if (num == 0) {
        uart1_write('0');
    } else {
        uart1_write_num_recursive(num);
    }
}

void uart1_write_hex(uint8_t byte) {
    static const char hex_chars[] = "0123456789ABCDEF";
    uart1_write(hex_chars[(byte >> 4) & 0x0F]);
    uart1_write(hex_chars[byte & 0x0F]);
}

uint8_t uart1_available(void) {
    return (uint8_t)((RX_BUFFER_SIZE + rx1_head - rx1_tail) % RX_BUFFER_SIZE);
}

uint8_t uart1_read(void) {
    if (rx1_head == rx1_tail) {
        return 0;
    }
    uint8_t ch = rx1_buffer[rx1_tail];
    rx1_tail = (rx1_tail + 1) % RX_BUFFER_SIZE;
    return ch;
}

uint16_t uart1_read_bytes(uint8_t* dest, uint16_t max_len) {
    uint16_t count = 0;
    uint16_t current_head = rx1_head;  // Локальный снимок для защиты от гонки данных

    while (current_head != rx1_tail && count < max_len) {
        dest[count] = rx1_buffer[rx1_tail];
        rx1_tail = (rx1_tail + 1) % RX_BUFFER_SIZE;
        count++;
    }
    return count;
}

void USART1_IRQHandler(void) {
    uint32_t sr = USART1->SR;

    if (sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
        uint8_t ch = (uint8_t)(USART1->DR & 0xFF);

        if (!(sr & USART_SR_FE)) {
            // Проверка ВСЕХ команд управления
            switch (ch) {
                case 'n':
                case 'N':
                    uart_command = 'n';
                    break;
                    
                case 'p':
                case 'P':
                    uart_command = 'p';
                    break;
                    
                case '+':
                case '=':
                    uart_command = '+';
                    break;
                    
                case '-':
                case '_':
                    uart_command = '-';
                    break;
                    
                default:
                    // Другие символы не являются командами
                    break;
            }
            
            // Сохраняем символ в кольцевой буфер (для всех символов)
            uint16_t next_head = (rx1_head + 1) % RX_BUFFER_SIZE;
            if (next_head != rx1_tail) {
                rx1_buffer[rx1_head] = ch;
                rx1_head = next_head;
            }
        }
    }
}
// ============================================================================
// === USART3 (PC10/TX, PC11/RX) ==============================================
// ============================================================================
void uart3_init(uint32_t baudrate) {
    // 1. Тактирование GPIOC и USART3
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

    // 2. Конфигурация выводов (Альтернативная функция AF7)
    // PC10 – TX
    GPIOC->MODER   &= ~(3U << (10 * 2));
    GPIOC->MODER   |=  (2U << (10 * 2));
    GPIOC->OSPEEDR &= ~(3U << (10 * 2));
    GPIOC->OSPEEDR |=  (1U << (10 * 2));  // Medium Speed
    GPIOC->PUPDR   &= ~(3U << (10 * 2));
    GPIOC->OTYPER  &= ~(1U << 10);        // Push-pull
    GPIOC->AFR[1]  &= ~(0xF << ((10U - 8U) * 4));
    GPIOC->AFR[1]  |=  (0x7U << ((10U - 8U) * 4));

    // PC11 – RX
    GPIOC->MODER   &= ~(3U << (11 * 2));
    GPIOC->MODER   |=  (2U << (11 * 2));
    GPIOC->OSPEEDR &= ~(3U << (11 * 2));
    GPIOC->OSPEEDR |=  (1U << (11 * 2));  // Medium Speed
    GPIOC->PUPDR   |=  (1U << (11 * 2));  // Pull-up для стабильного удержания Idle
    GPIOC->AFR[1]  &= ~(0xF << ((11U - 8U) * 4));
    GPIOC->AFR[1]  |=  (0x7U << ((11U - 8U) * 4));

    // 3. Конфигурация USART3 (8N1, 1 стоп-бит)
    USART3->CR1 = 0;
    USART3->CR2 = 0;
    USART3->CR3 = 0;

    usart_set_brr(USART3, baudrate, ACTUAL_APB1_CLOCK);

    // Принудительное чтение регистров для сброса мусора, возникшего при старте питания
    volatile uint32_t dummy = USART3->SR;
    dummy = USART3->DR;
    (void)dummy;

    // Включение TX, RX и прерывания по приему (RXNE)
    USART3->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    USART3->CR1 |= USART_CR1_UE;

    // 4. Настройка приоритетов и включение прерывания в NVIC (такой же приоритет, как у UART1)
    NVIC_SetPriority(USART3_IRQn, 5);
    NVIC_EnableIRQ(USART3_IRQn);
}

void uart3_write(uint8_t ch) {
    while (!(USART3->SR & USART_SR_TXE));
    USART3->DR = ch;
}

void uart3_write_str(const char* str) {
    while (*str) {
        uart3_write((uint8_t)*str++);
    }
}

void uart3_write_bytes(const uint8_t* data, uint16_t size) {
    for (uint16_t i = 0U; i < size; ++i) {
        uart3_write(data[i]);
    }
}

uint8_t uart3_available(void) {
    return (uint8_t)((RX_BUFFER_SIZE + rx3_head - rx3_tail) % RX_BUFFER_SIZE);
}

uint8_t uart3_read(void) {
    if (rx3_head == rx3_tail) {
        return 0;
    }
    uint8_t ch = rx3_buffer[rx3_tail];
    rx3_tail = (rx3_tail + 1) % RX_BUFFER_SIZE;
    return ch;
}

uint16_t uart3_read_bytes(uint8_t* dest, uint16_t max_len) {
    uint16_t count = 0;
    uint16_t current_head = rx3_head;

    while (current_head != rx3_tail && count < max_len) {
        dest[count] = rx3_buffer[rx3_tail];
        rx3_tail = (rx3_tail + 1) % RX_BUFFER_SIZE;
        count++;
    }
    return count;
}

void USART3_IRQHandler(void) {
    uint32_t sr = USART3->SR;

    // Обработка прерывания по аналогии с USART1 — защищает от Overrun-зависания шины RS-485
    if (sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
        uint8_t ch = (uint8_t)(USART3->DR & 0xFF);

        if (!(sr & USART_SR_FE)) {
            uint16_t next_head = (rx3_head + 1) % RX_BUFFER_SIZE;
            if (next_head != rx3_tail) {
                rx3_buffer[rx3_head] = ch;
                rx3_head = next_head;
            }
        }
    }
}

void uart3_flush(void) {
    while (!(USART3->SR & USART_SR_TC));
}