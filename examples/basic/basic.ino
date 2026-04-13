#include <Arduino.h>
#include <LedBuiltIn/LedBuiltIn.h>

LedBuiltIn led;

void setup() {
    led.begin();
}

void loop() {
    led.toggle();
    delay(500);
}