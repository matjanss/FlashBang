// FlashBang V3.0 - pins, constants, enums and the settings structs.
// Everything here is const or a type, so every file can include it.
#pragma once
#include <Arduino.h>

// ---- Pins ----
// T-Energy: GPIO16/17 are used by the PSRAM and GPIO35 is the battery divider.
// ENC_A / ENC_B must be below GPIO32 (they are read straight from the GPIO register in the ISR).
const int SND      = 23;   // sound module DO
const int SENS_ADC = 34;   // wiper of the sound module's trimpot (ADC1, input only)
const int BAT_ADC  = 35;   // battery divider on the board
const int BTN      = 32;   // external button between this pin and GND
const int LED      = 5;    // onboard blue LED (has a weak pull-up at boot, so drive it low)
const int ENC_A    = 18;   // rotary encoder A (CLK)
const int ENC_B    = 19;   // rotary encoder B (DT)
const int EXT_TRIG = 13;   // external trigger input, 3.3 V logic only
const int PC_SYNC  = 14;   // camera sync contact: 10k pull-up to 3V3 + 1k in series (see the schematic)
const int FIRE_BTN = 4;    // manual fire button between this pin and GND (GPIO0 = the onboard BOOT button also works)
const int SDA_PIN  = 21, SCL_PIN = 22;
const int NCH = 4;
const int FLASH_PIN[NCH] = {26, 25, 27, 33};   // opto 1..4

// ---- Fixed settings (not on the web page) ----
const float BAT_DIV = 2.0f;                    // T-Energy: two 100k resistors
const unsigned long BAT_PERIOD_MS   = 60000;   // battery check interval
const unsigned long LONG_PRESS_MS   = 800;     // long press: back / cancel / leave config mode
const unsigned long DEBOUNCE_MS     = 40;
const float SYNC_MARGIN_MS = 5.0f;             // curtain travel (~4 ms at 1/250 s sync) plus margin, for the shutter hint
const unsigned long MENU_TIMEOUT_MS = 30000;   // menu closes itself after this much inactivity
const char* const FW_VERSION = "V3.0";
const char* const FW_NAME = "FlashBang V3.0";
const char* const AP_SSID = "FlashBang";             // open network, no password (config and OTA modes)
const char* const OTA_HOST = "flashbang";
const int MAX_PRESETS = 8;
const int MAX_SEQ     = 50;                    // entries in a sequence
const int MAX_EVENTS  = 100;                   // flashes in one sequence, all channels together
const int MAX_DELAY_US = 300000000;            // longest delay: 300 s = 5 minutes
const bool ACTIVE_LOW   = true;                // sound module DO goes LOW on sound
const int  PULSE_US     = 2000;                // opto pulse; buildSchedule() shortens it for fast repeats
const int  POT_PERIOD_MS = 200;                // sensitivity pot: ms between reads
const int  POT_SAMPLES   = 4;                  // ADC samples averaged per read
const int  POT_DEADBAND  = 6;                  // ADC counts before the % display changes
const bool SENS_INVERT   = false;
const int  ENC_DIV       = 4;                  // encoder pulses per click
const bool ENC_INVERT    = true;               // reversed direction

// ---- Flash modes ----
// Single: the flashes you switch on fire together, once, after one delay.
// Multi:  every channel has its own count, interval and delay (count 0 = channel off).
enum { MODE_SINGLE = 0, MODE_MULTI = 1, MODE_SEQ = 2, NUM_MODES = 3 };
const char* const MODE_NAME[NUM_MODES] = {"Single", "Multi", "Sequence"};

// What starts a flash sequence: any combination of three inputs. trig_src holds the combination
// as (bit mask - 1): 0 = sound, 1 = external, 2 = sound + external, 3 = PC sync, and so on.
enum { TRIGB_SOUND = 1, TRIGB_EXT = 2, TRIGB_PC = 4 };
enum { TRIG_SOUND = 0, TRIG_EXT = 1, TRIG_PC = 3, NUM_TRIG = 7 };
// What started the last flash sequence.
enum { SRC_NONE = 0, SRC_SOUND, SRC_EXT, SRC_PC, SRC_MANUAL };
const char* const SRC_NAME[] = {"-", "Sound", "EXT", "PC", "Manual"};
const char* const TRIG_NAME[NUM_TRIG] = {"Sound", "External", "Snd+Ext", "PC sync", "Snd+PC", "Ext+PC", "All"};
inline int trigMask(int trig_src) { return trig_src + 1; }
const char* const POL_NAME[2]  = {"Low", "High"};      // external input: level that means "event"

// What gets fired and when. This is what a preset stores. Both modes keep their own values,
// so switching the mode never loses the other one.
// One flash of a sequence: when (us after the trigger) and on which channel (0..3).
struct SeqEntry { int t_us; int ch; };

struct Setup {
  int mode;           // MODE_SINGLE, MODE_MULTI or MODE_SEQ
  int chmask;         // single mode: bit 0..3 = channel 1..4 on
  int single_us;      // single mode: delay after the trigger, the same for every channel
  int count[4];       // multi mode: flashes per channel, 0 = channel off
  int interval_us[4]; // multi mode: time between the flashes of one channel
  int delay_us[4];    // multi mode: delay of the first flash of each channel
  int trig_src;       // which inputs start the sequence, see TRIG_NAME
  int ext_high;       // external input: 0 = event pulls the pin LOW, 1 = event pulls it HIGH
  int seq_n;          // sequence mode: number of entries
  SeqEntry seq[MAX_SEQ]; // sequence mode: kept sorted by time, unused entries are zero
};
const Setup DEF_SETUP = { MODE_SINGLE, 0x01, 0, {5, 0, 0, 0},
                          {100000, 100000, 100000, 100000}, {0, 0, 0, 0}, TRIG_SOUND, 0 };

struct Preset {
  char name[16];
  Setup s;
  bool used;
};

// Hardware and system settings.
struct Config {
  int   holdoff_ms;     // ignore sound after a shot
  int   cpu_mhz;        // 80, 160 or 240 (WiFi needs at least 80)
  float bat_cal;        // battery voltage trim factor
  int   batt_mah;       // cell capacity
  float avg_ma;         // average current draw, for the runtime estimate
  int   disp_sec;       // seconds the display stays lit (always on in menu and config mode)
};
const Config DEF = { 3000, 80, 1.00f, 3000, 60.0f, 10 };

// I2C address of the display, 7-bit as the Wire library wants it. A board that says 0x78 on its
// back (the 8-bit form) is 0x3C here; one that says 0x7A is 0x3D.
const uint8_t OLED_ADDR = 0x3C;
