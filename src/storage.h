// FlashBang V3.0 - settings in flash (NVS), clamping
#pragma once
#include "config.h"
#include "state.h"

void clampSetup(Setup& s);
int totalCount();
void clampConfig();
void loadAll();
void saveConfig();
void saveSetup();
void savePreset(int i);
void saveAllIfDirty();
