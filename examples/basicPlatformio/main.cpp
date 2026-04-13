#include <Arduino.h>
#include <LedBuiltIn.h>

void setup() {
    Led::init(true);
    xTaskCreatePinnedToCore(Led::task_led, "led", 2048, nullptr, 1, nullptr, APP_CPU_NUM);
}

void loop() {
    Led::blink(250, -1);
    vTaskDelay(pdMS_TO_TICKS(500));
}