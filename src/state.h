// FlashBang V3.0 - global state shared between the modules.
#pragma once
#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

extern Config cfg;
extern Setup cur;
extern int curPreset;                 // slot the current setup came from, -1 = custom
extern Preset presets[MAX_PRESETS];
extern Preferences prefs;

extern Adafruit_SH1106G oled;
extern WebServer server;
extern DNSServer dns;

extern TaskHandle_t mainTask;
extern volatile bool hit;             // a trigger fired (set in the interrupt handlers)
extern volatile uint32_t hitUs;
extern volatile uint8_t hitSrc;       // which input set `hit` (SRC_*)
extern uint8_t lastSrc;               // what started the last sequence, for the status screen
extern volatile bool fireReq;         // manual fire button pressed
extern volatile uint32_t fireUs;
extern unsigned long fireLock;

extern bool holdoff;
extern unsigned long holdoffEnd;
extern bool apActive;                 // config or OTA mode is running
extern bool otaActive;                // ... and it is OTA mode
extern bool btnDown;

extern bool dispOn;
extern unsigned long dispOffAt;

extern bool dirty;                    // settings changed, not yet saved
extern unsigned long saveAt;

extern int shots;
extern int sensRaw, sensPct;
extern float battV;
extern int battPct;
extern float battHours;

extern uint32_t evT[MAX_EVENTS];      // flash schedule, see schedule.cpp
extern uint8_t  evM[MAX_EVENTS];
extern int      evN;
extern uint32_t evPw;
