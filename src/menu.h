// FlashBang V3.0 - on-device menu
#pragma once
#include "config.h"
#include "state.h"
#include "inputs.h"

enum UiState { UI_STATUS, UI_MENU, UI_EDIT };
extern UiState ui;
extern unsigned long lastInput;

void openMenu();
void closeMenu();
void loadPreset(int slot);
void changed(bool setupChanged);
void scheduleSave();
void handleUi(BtnEvent be, int steps);
