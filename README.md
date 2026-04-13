# LedBuiltIn

Built-in LED management for Arduino/ESP32 boards.

## Features
- Thread-safe immediate `on()/off()` using FreeRTOS mutex
- Non-blocking blink manager task
- Default-aware generic `blink(...)`: pulses opposite of default, then restores default
- Active-low support via `LED_ACTIVE_LOW`
- Optional logging via LogQueue when available

## Configuration (build flags)
- `LED_PIN`: LED pin; defaults to `LED_BUILTIN` if defined
- `LED_ACTIVE_LOW`: `1` for active-low, `0` for active-high (default: `1`)
- `LED_DEFAULT_ON`: initial logical default (default: `1`)

## API
- `void init()` / `void init(bool defaultOn)`
- `void setDefaultOn(bool on)`
- `void setActiveLow(bool activeLow)`
- `void on()` / `void off()`
- `bool blink(uint32_t onMs, uint32_t offMs, int count = 1)`
- `bool blink(uint32_t ms, int count = 1)`
- `bool blinkOn(uint32_t onMs, uint32_t offMs, int count = 1)`
- `bool blinkOn(uint32_t ms, int count = 1)`
- `bool blinkOff(uint32_t offMs, uint32_t onMs, int count = 1)`
- `bool blinkOff(uint32_t ms, int count = 1)`
- `bool isBlinking()` / `void stopBlink()`
- `void task_led(void*)`

## Usage
```cpp
#include <LedBuiltIn.h>

void setup() {
  // Initialize with defaults
  Led::init();
  // or override default
  // Led::init(/*defaultOn=*/true);

  // Start the manager task (example stack/priority)
  xTaskCreatePinnedToCore(Led::task_led, "led", 2048, nullptr, 1, nullptr, APP_CPU_NUM);

  // Pulse opposite-of-default for 250ms
  Led::blink(250, 0, 1);
}

void loop() {
  // Your app
}
```

## Notes
- Generic `blink(...)` pulses opposite the configured default (`LED_DEFAULT_ON`) and restores default.
- If `LogQueue` is available, startup messages use it; otherwise logging is disabled.
