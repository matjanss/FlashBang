// FlashBang V3.0 - trigger, button and encoder inputs
#pragma once
#include "config.h"
#include "state.h"

enum BtnEvent { BTN_NONE, BTN_SHORT, BTN_LONG };
extern volatile uint8_t encOld;

void onButton();       // IRAM_ATTR is on the definition in inputs.cpp only
void onEncoder();
int takeEncoder();
long niceStep(long x);
long timeStep(int valueUs);
int accelMult();
void detachTriggers();
void armTriggers();
BtnEvent pollButton();
