// LedBuiltIn: Built-in LED management for Arduino/ESP32.
//
// Features
// - Thread-safe immediate `on()/off()` with FreeRTOS mutex.
// - Non-blocking blink manager task with default-aware semantics.
// - Default-aware generic `blink(...)`: pulses opposite of default, then restores.
// - Active-low support via `LED_ACTIVE_LOW` (maps logical to electrical levels).
// - Optional logging via LogQueue if available (no-op otherwise).
//
// Configuration macros (override via build flags):
// - LED_BUILTIN_PIN: pin number; falls back to `LED_BUILTIN` if defined.
// - LED_ACTIVE_LOW: 1 for active-low LEDs; 0 for active-high (default: 1).
// - LED_DEFAULT_ON: logical default state asserted at init (default: 1).
//
// Usage
// - Initialize in setup: `Led::init();` or `Led::init(/*defaultOn=*/true);`
// - Pulse opposite-of-default: `Led::blink(250, 0, 1);`
// - Explicit patterns: `Led::blinkOn(onMs, offMs, count)` / `blinkOff(...)`
// - Change default at runtime: `Led::setDefaultOn(true);`
// - Run manager task: create a FreeRTOS task with `Led::task_led`.
#pragma once

#include <Arduino.h>

// Define LED_BUILTIN_PIN if not provided by board or build flags
#ifndef LED_BUILTIN_PIN
  #ifdef LED_BUILTIN
    #define LED_BUILTIN_PIN LED_BUILTIN
  #else
    // This can be overridden via build flags or a board-specific header
    // #define LED_BUILTIN_PIN 21
    #error "LED_BUILTIN not defined for this board. Please define LED_BUILTIN_PIN via build flags or in a board-specific header."
  #endif
#endif

// Active-low LEDs are common on ESP32 boards. Allow override via build flags.
#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW 1
#endif

#ifndef LED_DEFAULT_ON
#define LED_DEFAULT_ON 1
#endif

namespace Led {
  // Initialization
  void init();                    // Initialize with compile-time default
  void init(bool defaultOn);      // Initialize with provided default state

  // Configuration
  void setDefaultOn(bool on);     // Update runtime default; applied if not blinking
  void setActiveLow(bool activeLow);

  // Immediate control
  void on();
  void off();

  // Blinking (non-blocking). Returns true if started; false if already active.
  // Generic `blink(...)` pulses opposite of the default and restores the default.
  bool blink(uint32_t onMs, uint32_t offMs, int count = 1);
  bool blink(uint32_t ms, int count = 1);
  bool blinkOn(uint32_t onMs, uint32_t offMs, int count = 1);
  bool blinkOn(uint32_t ms, int count = 1);
  bool blinkOff(uint32_t offMs, uint32_t onMs, int count = 1);
  bool blinkOff(uint32_t ms, int count = 1);

  // State helpers
  bool isBlinking();
  void stopBlink();

  // Manager task
  void task_led(void*);
}

