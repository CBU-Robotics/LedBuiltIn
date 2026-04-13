#include "LedBuiltIn.h"

// Optional logging via LogQueue; compile-time detection
#if defined(__has_include)
#  if __has_include(<LogQueue.h>)
#    include <LogQueue.h>
#    define LED_LOG_AVAILABLE 1
#  else
#    define LED_LOG_AVAILABLE 0
#  endif
#else
#  ifdef HAS_LOGQUEUE
#    include <LogQueue.h>
#    define LED_LOG_AVAILABLE 1
#  else
#    define LED_LOG_AVAILABLE 0
#  endif
#endif

#if LED_LOG_AVAILABLE
// #  define LED_LOGE(...) LOGE(__VA_ARGS__)
// #  define LED_LOGW(...) LOGW(__VA_ARGS__)
// #  define LED_LOGB(...) LOGB(__VA_ARGS__)
#  define LED_LOGI(...) LOGI(__VA_ARGS__)
// #  define LED_LOGD(...) LOGD(__VA_ARGS__)
// #  define LED_LOGF(...) LOGF(__VA_ARGS__)
#else
// #  define LED_LOGE(...) ((void)0)
// #  define LED_LOGW(...) ((void)0)
// #  define LED_LOGB(...) ((void)0)
#  define LED_LOGI(...) ((void)0)
// #  define LED_LOGD(...) ((void)0)
// #  define LED_LOGF(...) ((void)0)
#endif

namespace Led {

static SemaphoreHandle_t s_lock = nullptr;
static volatile bool s_blinkActive = false;
static volatile bool s_activeLow = LED_ACTIVE_LOW;
static volatile bool s_defaultOn = LED_DEFAULT_ON;
struct BlinkCfg { bool blinkOn = true; uint32_t onMs = 0; uint32_t offMs = 0; int count = 0; };
static BlinkCfg s_cfg;

// --- Internal helpers ---
static inline void setPin(bool on) {
  // Map logical ON/OFF to electrical level based on active-low flag
  uint8_t level;
  if (s_activeLow) {
    level = on ? LOW : HIGH;
  } else {
    level = on ? HIGH : LOW;
  }
  digitalWrite(LED_BUILTIN_PIN, level);
}

// --- Initialization ---
void init() {
  if (!s_lock) s_lock = xSemaphoreCreateMutex();
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  blinkOn(125, 3); // startup pattern: 125ms on/off, 3 times
}

void init(bool defaultOn) {
  if (!s_lock) s_lock = xSemaphoreCreateMutex();
  if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
  s_defaultOn = defaultOn;
  if (s_lock) xSemaphoreGive(s_lock);
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  blinkOn(125, 3); // startup pattern: 125ms on/off, 3 times
}

// --- Immediate control ---
void on() {
  if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
  setPin(true);
  if (s_lock) xSemaphoreGive(s_lock);
}

void off() {
  if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
  setPin(false);
  if (s_lock) xSemaphoreGive(s_lock);
}

// --- Blink core & wrappers ---
bool blink(bool blinkOn, uint32_t onMs, uint32_t offMs, int count) {
  if (!s_lock) s_lock = xSemaphoreCreateMutex();
  if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
  if (s_blinkActive) {
    if (s_lock) xSemaphoreGive(s_lock);
    return false; // already blinking; do not override
  }
  s_cfg.onMs = onMs;
  s_cfg.offMs = offMs;
  s_cfg.count = (count == 0) ? 1 : count; // treat 0 as 1
  s_cfg.blinkOn = blinkOn;
  s_blinkActive = true;
  if (s_lock) xSemaphoreGive(s_lock);
  return true;
}

bool blink(uint32_t onMs, uint32_t offMs, int count) {
  return blink(!s_defaultOn, onMs, offMs, count);
}

bool blink(uint32_t ms, int count) {
  return blink(!s_defaultOn, ms, ms, count);
}

bool blinkOn(uint32_t onMs, uint32_t offMs, int count) {
  return blink(true, onMs, offMs, count);
}

bool blinkOn(uint32_t ms, int count) {
  return blink(true, ms, ms, count);
}

bool blinkOff(uint32_t offMs, uint32_t onMs, int count) {
  return blink(false, offMs, onMs, count);
}

bool blinkOff(uint32_t ms, int count) {
  return blink(false, ms, ms, count);
}

// --- State helpers ---
bool isBlinking() {
  // Return blink state; lock for consistency if available
  if (s_lock) {
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool active = s_blinkActive;
    xSemaphoreGive(s_lock);
    return active;
  }
  return s_blinkActive;
}

void stopBlink() {
  if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
  s_blinkActive = false;
  if (s_lock) xSemaphoreGive(s_lock);
  setPin(s_defaultOn); // restore default state
}

// --- Configuration ---
void setActiveLow(bool activeLow) {
  if (!s_lock) s_lock = xSemaphoreCreateMutex();
  if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
  s_activeLow = activeLow;
  if (s_lock) xSemaphoreGive(s_lock);
  // Re-assert current default state according to new mapping
  setPin(s_defaultOn);
}

void setDefaultOn(bool on) {
  if (!s_lock) s_lock = xSemaphoreCreateMutex();
  bool wasBlinking = false;
  if (s_lock) {
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_defaultOn = on;
    wasBlinking = s_blinkActive;
    xSemaphoreGive(s_lock);
  } else {
    s_defaultOn = on;
  }
  // Apply immediately only when not blinking to avoid racing with pattern
  if (!wasBlinking) setPin(s_defaultOn);
}

// --- Manager task ---
void task_led(void*) {
  // Ensure pin setup and default state
  init();
  LED_LOGI("LED: task started; running startup pattern");

  // Manage blink requests non-blocking
  for (;;) {
    if (!s_blinkActive) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    // Snapshot config under lock
    BlinkCfg cfg;
    if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
    cfg = s_cfg;
    if (s_lock) xSemaphoreGive(s_lock);

    if (cfg.count < 0) {
      // Permanent blink
      setPin(!s_defaultOn);
      vTaskDelay(pdMS_TO_TICKS(cfg.onMs));
      setPin(s_defaultOn);
      vTaskDelay(pdMS_TO_TICKS(cfg.offMs));
      continue; // stay active
    }

    int remaining = cfg.count;
    while (remaining > 0 && s_blinkActive) {
      setPin(cfg.blinkOn);
      if (cfg.onMs) vTaskDelay(pdMS_TO_TICKS(cfg.onMs));
      setPin(!cfg.blinkOn);
      if (cfg.offMs) vTaskDelay(pdMS_TO_TICKS(cfg.offMs));
      --remaining;
    }

    // Restore default state and mark inactive
    setPin(s_defaultOn);
    s_blinkActive = false;
  }
}

} // namespace Led
