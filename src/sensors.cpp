// FlashBang V3.0 - sensitivity pot and battery
#include "app.h"

// ================= sensitivity pot and battery =================

int readSensRaw() {
  long s = 0;
  for (int i = 0; i < POT_SAMPLES; i++) s += analogRead(SENS_ADC);
  return s / POT_SAMPLES;
}

int rawToPct(int raw) {
  int p = map(raw, 0, 4095, 0, 100);
  return SENS_INVERT ? 100 - p : p;
}

// Rough resting-voltage curve for a Li-ion cell; approximate under load.
int voltToPct(float v) {
  static const float V[]   = {3.30, 3.50, 3.60, 3.70, 3.80, 3.90, 4.00, 4.10, 4.20};
  static const int   PCT[] = {   0,   10,   20,   35,   50,   65,   80,   90,  100};
  const int N = sizeof(V) / sizeof(V[0]);
  if (v <= V[0]) return 0;
  if (v >= V[N - 1]) return 100;
  for (int i = 1; i < N; i++) {
    if (v <= V[i]) {
      return (int)(PCT[i - 1] + (PCT[i] - PCT[i - 1]) * (v - V[i - 1]) / (V[i] - V[i - 1]));
    }
  }
  return 100;
}

void readBattery() {
  uint32_t mv = 0;
  for (int i = 0; i < 8; i++) mv += analogReadMilliVolts(BAT_ADC);
  battV = (mv / 8.0f) * BAT_DIV * cfg.bat_cal / 1000.0f;
  battPct = voltToPct(battV);
  battHours = (cfg.batt_mah * battPct / 100.0f) / cfg.avg_ma;
}
