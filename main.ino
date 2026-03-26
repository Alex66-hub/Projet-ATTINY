#include <TinyWireM.h>
#include <Tiny4kOLED.h>

#define BUTTON_PIN A3

int lireMoyenne(uint8_t pin) {
  long somme = 0;
  for (int i = 0; i < 20; i++) {
    somme += analogRead(pin);
    delay(2);
  }
  return somme / 20;
}

void setup() {
  pinMode(BUTTON_PIN, INPUT);

  oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);
  oled.setFont(FONT6X8);
  oled.clear();
  oled.on();
}

void loop() {
  int val = lireMoyenne(BUTTON_PIN);

  oled.clear();
  oled.setCursor(25, 3);
  oled.print(F("Valeur ADC :"));
  oled.setCursor(40, 5);
  oled.print(val);

  delay(100);
}
