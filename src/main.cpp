// FlashBang V3.0 - sound/light/camera-triggered 4-channel flash controller
// Board: LilyGO TTGO T-Energy (ESP32-WROVER), 18650, 128x64 SH1106 OLED, rotary encoder
//
// Sound module DO, an external trigger input and/or the camera's PC sync contact -> interrupt.
// Four optos fire four flashes.
// Rotary encoder + button drive an on-device menu. Full settings and named presets
// live on a WiFi config page (start it from the menu, leave it with a long press).

#include "app.h"

void setup() {
  loadAll();
  setCpuFrequencyMhz(cfg.cpu_mhz);

  for (int c = 0; c < NCH; c++) { pinMode(FLASH_PIN[c], OUTPUT); digitalWrite(FLASH_PIN[c], LOW); }
  pinMode(LED, OUTPUT);   digitalWrite(LED, LOW);
  pinMode(SND, INPUT);
  pinMode(EXT_TRIG, INPUT);
  pinMode(PC_SYNC, INPUT);
  pinMode(FIRE_BTN, INPUT_PULLUP);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  analogSetPinAttenuation(SENS_ADC, ADC_11db);
  analogSetPinAttenuation(BAT_ADC, ADC_11db);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  oled.begin(OLED_ADDR, true);
  showSplash();
  unsigned long splashEnd = millis() + 2000;     // the rest of setup runs while it is shown

  sensRaw = readSensRaw();
  sensPct = rawToPct(sensRaw);
  readBattery();
  buildSchedule();

  webSetupRoutes();

  mainTask = xTaskGetCurrentTaskHandle();
  encOld = (uint8_t)((digitalRead(ENC_A) << 1) | digitalRead(ENC_B));
  attachInterrupt(digitalPinToInterrupt(BTN), onButton, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_A), onEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), onEncoder, CHANGE);
  while ((long)(millis() - splashEnd) < 0) delay(10);   // keep the splash up for 2 s in total
  armTriggers();
  show(stateText());
  displayWake();                                 // lit after startup, then times out
}

// Remaining ms until `deadline`, at least 1.
unsigned long msUntil(unsigned long deadline) {
  long rem = (long)(deadline - millis());
  return rem < 1 ? 1UL : (unsigned long)rem;
}

void loop() {
  static unsigned long lastSens = 0;
  static unsigned long lastBat = millis();

  // Sleep until an interrupt wakes us or something is due.
  unsigned long wait = apActive ? 10UL : (unsigned long)POT_PERIOD_MS;
  if (btnDown && wait > 20) wait = 20;
  if (!apActive) {
    if (ui != UI_STATUS) wait = msUntil(lastInput + MENU_TIMEOUT_MS);         // menu: only the timeout
    else if (dispOn && msUntil(dispOffAt) < wait) wait = msUntil(dispOffAt);
    if (holdoff && msUntil(holdoffEnd) < wait) wait = msUntil(holdoffEnd);
    if (saveAt && msUntil(saveAt) < wait) wait = msUntil(saveAt);
  }
  ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(wait));

  BtnEvent be = pollButton();
  int steps = takeEncoder();

  // --- WiFi config mode: long press leaves it ---
  if (apActive) {
    if (be == BTN_LONG) stopAp();
    else netService();
    return;
  }

  // --- Menu and quick adjust ---
  if (be != BTN_NONE || steps != 0) {
    lastInput = millis();
    handleUi(be, steps);
    if (apActive) return;                        // the menu started config mode
  }

  // --- Manual fire button: works on the status screen and in the menu (and while editing) ---
  if (fireReq) {
    fireReq = false;
    if (evN > 0 && (long)(millis() - fireLock) >= 0) {
      fireSchedule(fireUs);                      // delays count from the moment of the press
      lastSrc = SRC_MANUAL;
      shots++;
      fireLock = millis() + 250;                 // ignores contact bounce when the button is released
      if (ui == UI_STATUS) {
        holdoff = true;                          // sound and other triggers wait, like after any shot
        hit = true;
        holdoffEnd = millis() + cfg.holdoff_ms;
        show("FIRED");
        displayWake();
      } else {
        lastInput = millis();                    // keep the menu open
      }
    }
  }

  if (ui != UI_STATUS) {
    hit = false;                                 // sound, external and PC sync stay off while a menu is open
    if (millis() - lastInput > MENU_TIMEOUT_MS) closeMenu();
    return;
  }

  if (saveAt && (long)(millis() - saveAt) >= 0) saveAllIfDirty();
  if (dispOn && (long)(millis() - dispOffAt) >= 0) displaySleep();

  // --- Sound detected: play the flash schedule, timed from the interrupt ---
  if (hit && !holdoff) {
    if (evN == 0) {
      hit = false;                               // no channel enabled
    } else {
      fireSchedule(hitUs);
      lastSrc = hitSrc;
      shots++;
      holdoff = true;                            // 'hit' stays true, so the ISR ignores sound
      holdoffEnd = millis() + cfg.holdoff_ms;
      show("FIRED");
      displayWake();                             // light up after the flashes fire
    }
  }

  if (holdoff && (long)(millis() - holdoffEnd) >= 0) {
    holdoff = false;
    hit = false;                                 // re-arm
    if (dispOn) show(stateText());               // only redraw if the display is lit
  }

  // --- Idle housekeeping. A redraw blocks for ~25 ms, so only draw when needed. ---
  if ((trigMask(cur.trig_src) & TRIGB_SOUND) && millis() - lastSens >= (unsigned long)POT_PERIOD_MS) {
    lastSens = millis();
    int raw = readSensRaw();
    if (abs(raw - sensRaw) > POT_DEADBAND) {
      sensRaw = raw;
      sensPct = rawToPct(raw);
      show(stateText());
      displayWake();                             // light up while the trimpot is being turned
    }
  }

  if (millis() - lastBat >= BAT_PERIOD_MS) {
    lastBat = millis();
    readBattery();                               // values update silently...
    if (dispOn) show(stateText());               // ...and are drawn only if lit
  }
}
