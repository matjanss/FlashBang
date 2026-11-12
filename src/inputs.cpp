// FlashBang V3.0 - trigger, button and encoder interrupts; arming; button polling
#include "app.h"
#include "soc/gpio_struct.h"

// ================= interrupts =================

// Sound: stamp the time and wake the main loop.
void IRAM_ATTR onSound() {
  if (!hit) {
    hitUs = micros();
    hitSrc = SRC_SOUND;
    hit = true;
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(mainTask, &woken);
    if (woken) portYIELD_FROM_ISR();
  }
}

// External trigger: same job as the sound interrupt. Whichever fires first wins.
void IRAM_ATTR onExt() {
  if (!hit) {
    hitUs = micros();
    hitSrc = SRC_EXT;
    hit = true;
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(mainTask, &woken);
    if (woken) portYIELD_FROM_ISR();
  }
}

// Camera sync contact: same job again. The contact closes when the shutter is fully open.
void IRAM_ATTR onPcSync() {
  if (!hit) {
    hitUs = micros();
    hitSrc = SRC_PC;
    hit = true;
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(mainTask, &woken);
    if (woken) portYIELD_FROM_ISR();
  }
}

// Manual fire button. It has its own flag, so it also works while the menu is open.
void IRAM_ATTR onFireBtn() {
  if (!fireReq) {
    fireUs = micros();
    fireReq = true;
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(mainTask, &woken);
    if (woken) portYIELD_FROM_ISR();
  }
}

// Button edge: just wake the main loop, it reads the pin itself.
void IRAM_ATTR onButton() {
  BaseType_t woken = pdFALSE;
  vTaskNotifyGiveFromISR(mainTask, &woken);
  if (woken) portYIELD_FROM_ISR();
}

// Encoder: quadrature decoder with a state table, accumulates quarter steps.
DRAM_ATTR static const int8_t ENC_TABLE[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
volatile uint8_t encOld = 0;
volatile int encQuarter = 0;
portMUX_TYPE encMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onEncoder() {
  uint8_t a = (GPIO.in >> ENC_A) & 1;
  uint8_t b = (GPIO.in >> ENC_B) & 1;
  uint8_t nw = (uint8_t)((a << 1) | b);
  int8_t d = ENC_TABLE[((encOld << 2) | nw) & 0x0F];
  encOld = nw;
  if (d) {
    portENTER_CRITICAL_ISR(&encMux);
    encQuarter += d;
    portEXIT_CRITICAL_ISR(&encMux);
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(mainTask, &woken);
    if (woken) portYIELD_FROM_ISR();
  }
}

// Whole encoder clicks since the last call (signed).
int takeEncoder() {
  portENTER_CRITICAL(&encMux);
  int whole = encQuarter / ENC_DIV;
  encQuarter -= whole * ENC_DIV;
  portEXIT_CRITICAL(&encMux);
  return ENC_INVERT ? -whole : whole;
}

// Round down to 1, 2 or 5 times a power of ten.
long niceStep(long x) {
  if (x < 1) return 1;
  long p = 1;
  while (p * 10 <= x) p *= 10;
  if (x >= 5 * p) return 5 * p;
  if (x >= 2 * p) return 2 * p;
  return p;
}

// Step for a delay (in us) per encoder click.
//   Turning slowly:   fine steps, 0.1 ms (1 ms from 10 s up, where the display only shows ms).
//   Turning faster:   steps grow with the value, about 3 % of it per click, and about 10 % when really fast.
long timeStep(int valueUs) {
  static unsigned long last = 0;
  unsigned long now = millis();
  unsigned long dt = now - last;
  last = now;
  long base = (valueUs >= 10000000) ? 1000 : 100;
  if (dt >= 80) return base;
  long st = niceStep(valueUs / ((dt < 30) ? 10 : 30));
  return st > base ? st : base;
}

// Faster turning = bigger steps (for values that do not grow, like intervals).
int accelMult() {
  static unsigned long last = 0;
  unsigned long now = millis();
  unsigned long dt = now - last;
  last = now;
  if (dt < 25) return 20;
  if (dt < 70) return 5;
  return 1;
}

void detachTriggers() {
  detachInterrupt(digitalPinToInterrupt(SND));
  detachInterrupt(digitalPinToInterrupt(EXT_TRIG));
  detachInterrupt(digitalPinToInterrupt(PC_SYNC));
  detachInterrupt(digitalPinToInterrupt(FIRE_BTN));
}

// (Re)connect the trigger inputs according to the settings.
void armTriggers() {
  hit = false;
  fireReq = false;
  holdoff = false;
  detachTriggers();
  int mask = trigMask(cur.trig_src);

  if (mask & TRIGB_SOUND) {
    attachInterrupt(digitalPinToInterrupt(SND), onSound, ACTIVE_LOW ? FALLING : RISING);
  }
  if (mask & TRIGB_EXT) {
    // Low = event pulls the pin to GND (internal pull-up keeps it high otherwise).
    // High = event pulls the pin to 3.3 V (internal pull-down keeps it low otherwise).
    pinMode(EXT_TRIG, cur.ext_high ? INPUT_PULLDOWN : INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EXT_TRIG), onExt, cur.ext_high ? RISING : FALLING);
  } else {
    pinMode(EXT_TRIG, INPUT);                    // not used: no pull resistor
  }
  if (mask & TRIGB_PC) {
    // The camera's X-sync contact closes to ground when the shutter is fully open, so this input is
    // always active low. The 10k pull-up from the schematic does the work; the internal one in parallel is harmless.
    pinMode(PC_SYNC, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PC_SYNC), onPcSync, FALLING);
  } else {
    pinMode(PC_SYNC, INPUT);
  }

  // The manual fire button is always connected, whatever the trigger source is.
  pinMode(FIRE_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FIRE_BTN), onFireBtn, FALLING);
}


// Short press fires on release, long press fires as soon as the hold time is reached.
BtnEvent pollButton() {
  static bool down = false, longSent = false;
  static unsigned long downSince = 0;
  bool now = (digitalRead(BTN) == LOW);
  unsigned long t = millis();
  BtnEvent ev = BTN_NONE;

  if (now && !down) { down = true; longSent = false; downSince = t; }
  else if (!now && down) {
    down = false;
    if (!longSent && t - downSince >= DEBOUNCE_MS) ev = BTN_SHORT;
  }
  if (down && !longSent && t - downSince >= LONG_PRESS_MS) { longSent = true; ev = BTN_LONG; }

  btnDown = down;
  return ev;
}
