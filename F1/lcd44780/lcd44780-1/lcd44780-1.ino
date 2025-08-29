//https://eax.me/hd44780-protocol
#include <Arduino.h>

const int PIN_RS = PA15;
const int PIN_E  = PB3;
const int PIN_D4 = PB4;
const int PIN_D5 = PB5;
const int PIN_D6 = PB8;
const int PIN_D7 = PB9;

const int LCD_DELAY_MS = 1;

void lcdSend(uint8_t isCommand, uint8_t data) {
  if (isCommand == 1) digitalWrite(PIN_RS, LOW); else digitalWrite(PIN_RS, HIGH);
  delay(LCD_DELAY_MS);
  if (data&0x080) digitalWrite(PIN_D7, HIGH); else digitalWrite(PIN_D7, LOW);
  if (data&0x040) digitalWrite(PIN_D6, HIGH); else digitalWrite(PIN_D6, LOW);
  if (data&0x020) digitalWrite(PIN_D5, HIGH); else digitalWrite(PIN_D5, LOW);
  if (data&0x010) digitalWrite(PIN_D4, HIGH); else digitalWrite(PIN_D4, LOW);
  // Wnen writing to the display, data is transfered only
  // on the high to low transition of the E signal.
  digitalWrite(PIN_E, HIGH);
  delay(LCD_DELAY_MS);
  digitalWrite(PIN_E, LOW);
  if (data&0x08) digitalWrite(PIN_D7, HIGH); else digitalWrite(PIN_D7, LOW);
  if (data&0x04) digitalWrite(PIN_D6, HIGH); else digitalWrite(PIN_D6, LOW);
  if (data&0x02) digitalWrite(PIN_D5, HIGH); else digitalWrite(PIN_D5, LOW);
  if (data&0x01) digitalWrite(PIN_D4, HIGH); else digitalWrite(PIN_D4, LOW);
  digitalWrite(PIN_E, HIGH);
  delay(LCD_DELAY_MS);
  digitalWrite(PIN_E, LOW);
}

void lcdCommand(uint8_t cmd) {
  lcdSend(1, cmd);
}

void lcdChar(const char chr) {
  lcdSend(0, (uint8_t)chr);
}

void lcdString(const char* str) {
  while (*str != '\0') {
    lcdChar(*str);
    str++;
  }
}

void setCursor(char x, char y) {
  uint8_t base = 0;
  if(y==1) {
    base = 0x40;
  }else {
    base = 0;
  }
  lcdCommand( 0x80 | (base + x));
}

void setup() {
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);
  pinMode(PIN_RS, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_D4, OUTPUT);
  pinMode(PIN_D5, OUTPUT);
  pinMode(PIN_D6, OUTPUT);
  pinMode(PIN_D7, OUTPUT);

  // 4-bit mode, 2 lines, 5x7 format
  lcdCommand(0b00110000);
  //  lcdCommand(0x00);
  //  lcdCommand(0x08);
  //  lcdCommand(0x80);
  //lcdCommand(30);
  // display & cursor home (keep this!)
  lcdCommand(0b00000010);
  // display on, right shift, underline off, blink off
  lcdCommand(0b00001100);
  // clear display (optional here)
  lcdCommand(0x01);

  lcdCommand(0b10000000); // set address to 0x00
  lcdString("Using HD44780");
  lcdCommand(0b11000000); // set address to 0x40
  lcdString("7777777777777");
  delay(1000);
  lcdCommand(0x01);
  setCursor(4,1);
  delay(100);
  lcdString("0000000000000");
}

void loop() {
  /* do nothing */
  delay(100);
}
