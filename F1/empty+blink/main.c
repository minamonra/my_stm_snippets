#include "main.h"

volatile uint32_t ttms   = 0;
volatile uint32_t pc13ms = 0;

#define  LEDTOGGLE GPIOC->ODR ^= (1<<13)

// прерывание системного тикера
void SysTick_Handler(void) 
{
  ++ttms;
  //if (ddms) ddms--;
}

void blink_pc13led(uint16_t freq)
{
  if (pc13ms > ttms || ttms - pc13ms > freq)
  {
    LEDTOGGLE;
    pc13ms = ttms;
  }
}

// MAIN
int main(void) {
  StartHSE();
  hardware_init();

  do {
    blink_pc13led(1499);
  } while (1);
}

// End of file
