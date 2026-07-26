#include "stm32f4xx.h"
#include "common.h"
#include "wav_player_ui.h"
#include "button.h"

int main(void) {
  hw_init();            // 1. Глобальный запуск всей аппаратной периферии микроконтроллера
  buttons_init();       // 2. Инициализация кнопок (GPIO)
  wav_playerui_init();  // 3. Инициализация внутренней логики Shuffle-плеера и сканирование папки

  while (1) {
    blink_led(500);          // Фоновое мигание статус-диода от SysTick
    button_process(20);      // Обработка кнопок (каждые 20 мс)
    wav_playerui_process();  // Основная логика плеера
  }
}

/*
Для теста в main.c сделать:
#include "audio_gen.h"
// в main() после hw_init():
audio_gen_start(AUDIO_GEN_SINE, 1000, 8000, 5000);  // 1 кГц, 5 сек
// и в while(1) добавить:
audio_gen_process();
*/