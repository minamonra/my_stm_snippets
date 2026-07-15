#include "stm32f4xx.h"
#include "common.h"
#include "shuffle_player.h"
#include "button.h"

int main(void) {
  hw_init();                   // 1. Глобальный запуск всей аппаратной периферии микроконтроллера
  buttons_init();              // 2. Инициализация кнопок (GPIO)
  shuffle_player_init();       // 3. Инициализация внутренней логики Shuffle-плеера и сканирование папки

  while (1) {
    blink_led(500);            // Фоновое мигание статус-диода от SysTick
    button_process(20);        // Обработка кнопок (каждые 20 мс)
    shuffle_player_process();  // Основная логика плеера
  }
}