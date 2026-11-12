// FlashBang V3.0 - sensitivity pot and battery
#pragma once
#include "config.h"
#include "state.h"

int readSensRaw();
int rawToPct(int raw);
int voltToPct(float v);
void readBattery();
