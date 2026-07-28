#include "stm32f4xx.h"
#include "common.h"
#include "wav_player_ui.h"
#include "button.h"
#include "audio_gen.h"

#define GENERATOR
//#define DEFPLAYER
int main(void) {
  hw_init();            
#if defined(DEFPLAYER)
  buttons_init();       
  wav_playerui_init();  
#elif defined(GENERATOR)
  // Запуск: 1050 Гц, Половина амплитуды, играть 95 секунд
  audio_gen_start(AUDIO_GEN_SAWTOOTH, AUDIO_VOLUME_FULL, 95000);  
#endif  
  while (1) {
    blink_led(500);          
#if defined(DEFPLAYER)
    button_process(20);      
    wav_playerui_process();  
#elif defined(GENERATOR)
    audio_gen_process(); // Автоматически остановит I2S через 95 секунд
#endif
  }
}