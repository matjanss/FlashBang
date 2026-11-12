// FlashBang V3.0 - the global variables (declared extern in state.h).
#include "app.h"

Config cfg = DEF;
Setup cur = DEF_SETUP;
int curPreset = -1;                  // slot the current setup came from, -1 = custom
Preset presets[MAX_PRESETS];
Preferences prefs;

Adafruit_SH1106G oled(128, 64, &Wire, -1);
WebServer server(80);
DNSServer dns;

TaskHandle_t mainTask;
volatile bool hit = false;
volatile uint32_t hitUs = 0;
volatile uint8_t hitSrc = SRC_NONE;
uint8_t lastSrc = SRC_NONE;
volatile bool fireReq = false;                   // manual fire button pressed
volatile uint32_t fireUs = 0;
unsigned long fireLock = 0;                      // ignores contact bounce right after a manual fire

bool holdoff = false;
unsigned long holdoffEnd = 0;
bool apActive = false;              // config or OTA mode is running
bool otaActive = false;             // ... and it is OTA mode
bool btnDown = false;

bool dispOn = true;                  // the SH1106 is on after begin()
unsigned long dispOffAt = 0;

bool dirty = false;                  // settings changed, not yet saved
unsigned long saveAt = 0;            // delayed save after quick adjust (0 = none)

int shots = 0;
int sensRaw = 0, sensPct = 0;
float battV = 0;
int battPct = 0;
float battHours = 0;

// Flash schedule, rebuilt whenever the setup changes. Times are in us after the bang.
uint32_t evT[MAX_EVENTS];
uint8_t  evM[MAX_EVENTS];            // channel bitmask fired at that time
int      evN = 0;
uint32_t evPw = 2000;                // pulse width actually used
