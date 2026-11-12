// FlashBang V3.0 - OLED screens: status, splash, config and OTA; display on/off
#pragma once
#include "config.h"
#include "state.h"

const char* stateText();
void fmtDelayShort(int us, char* buf, size_t n);
void centerText(const char* s, int width, char* out, size_t n);
void fmtTime(int us, char* buf, size_t n);
void show(const char* state);
void showConfig();
void showOtaScreen();
void showUpdateStart();
void showUpdateProgress(int pct);
void showUpdateDone();
void showUpdateFailed(const char* why);
void displaySleep();
void displayOn();
void displayWake();
void centerText(const char* t, int y, int size);
void showSplash();
