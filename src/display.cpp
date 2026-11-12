// FlashBang V3.0 - OLED screens: status, splash, config and OTA; display on/off
#include "app.h"

// ================= display: status screen =================

const char* stateText() {
  if (evN == 0) return "NO CH";
  return holdoff ? "FIRED" : "ARMED";
}

// Delay in ms in at most 4 characters: "0.0", "8.5", "12.5", "250", "2000".
void fmtDelayShort(int us, char* buf, size_t n) {
  double ms = us / 1000.0;
  if (ms < 100.0) snprintf(buf, n, "%.1f", ms);
  else            snprintf(buf, n, "%.0f", ms);
}

// Text centred in a field of `width` characters.
void centerText(const char* s, int width, char* out, size_t n) {
  int len = (int)strlen(s);
  int left = (width - len) / 2;
  if (left < 0) left = 0;
  int right = width - len - left;
  if (right < 0) right = 0;
  snprintf(out, n, "%*s%s%*s", left, "", s, right, "");
}

// Times on the small display: milliseconds below 10 s ("8.0ms", "2000.0ms"), seconds from there ("12.500s").
void fmtTime(int us, char* buf, size_t n) {
  if (us < 10000000) snprintf(buf, n, "%.1fms", us / 1000.0);
  else               snprintf(buf, n, "%.3fs", us / 1000000.0);
}

// Status screen. The 128x64 panel is eight text rows of 8 px, 21 characters wide.
//   row 0    title bar: preset name (or "Custom") and battery
//   rows 1-2 state in big letters, trigger and sensitivity on the right
//   rows 4-6 one column per channel with its flash count, and a timing summary
//   row 7    source of the last sequence (Sound, EXT, PC, Manual) and shot counter
void show(const char* state) {
  char b[28];
  oled.clearDisplay();
  oled.setTextSize(1);

  // Row 0: title bar.
  oled.fillRect(0, 0, 128, 8, SH110X_WHITE);
  oled.setTextColor(SH110X_BLACK);
  char pct[8];
  snprintf(pct, sizeof pct, "%d%%", battPct);
  int pw = 6 * (int)strlen(pct);
  char name[16];
  snprintf(name, sizeof name, "%s", curPreset >= 0 ? presets[curPreset].name : "Custom");
  int maxName = (126 - pw - 2) / 6 - 1;
  if ((int)strlen(name) > maxName) name[maxName] = 0;
  oled.setCursor(2, 0);
  oled.print(name);
  oled.setCursor(126 - pw, 0);
  oled.print(pct);
  oled.setTextColor(SH110X_WHITE);

  // Rows 1-2: state. Anything but ARMED (FIRED, NO CH) is drawn inverted.
  if (strcmp(state, "ARMED") != 0) {
    oled.fillRect(0, 9, 12 * (int)strlen(state) + 4, 18, SH110X_WHITE);
    oled.setTextColor(SH110X_BLACK);
  }
  oled.setTextSize(2);
  oled.setCursor(2, 10);
  oled.print(state);
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);

  // Right of the state: trigger source and sensitivity.
  char l1[16], l2[16];
  int tm = trigMask(cur.trig_src);
  snprintf(l1, sizeof l1, "%s", TRIG_NAME[cur.trig_src]);
  if (tm & TRIGB_SOUND)    snprintf(l2, sizeof l2, "Sens %d%%", sensPct);
  else if (tm & TRIGB_EXT) snprintf(l2, sizeof l2, "Act %s", POL_NAME[cur.ext_high]);
  else                     snprintf(l2, sizeof l2, "X-sync");
  oled.setCursor(72, 10);  oled.print(l1);
  oled.setCursor(72, 18);  oled.print(l2);

  // Rows 4-6: one 5-character column per channel. Active channels get an inverted label with
  // their number of flashes below it. The last row summarizes the timing.
  int total = 0;
  for (int c = 0; c < NCH; c++) {
    int n;
    if (cur.mode == MODE_SINGLE)   n = ((cur.chmask >> c) & 1) ? 1 : 0;
    else if (cur.mode == MODE_SEQ) n = seqCount(cur, c);
    else                           n = cur.count[c];
    total += n;
    int x = c * 30;
    char lab[4], val[16], cell[12];
    snprintf(lab, sizeof lab, "CH%d", c + 1);
    oled.setCursor(x + 6, 32);
    if (n > 0) oled.setTextColor(SH110X_BLACK, SH110X_WHITE);
    oled.print(lab);
    oled.setTextColor(SH110X_WHITE);
    if (n > 0) snprintf(val, sizeof val, "%dx", n);
    else       snprintf(val, sizeof val, "-");
    centerText(val, 5, cell, sizeof cell);
    oled.setCursor(x, 40);
    oled.print(cell);
  }
  char tb[16];
  if (evN == 0) {
    snprintf(b, sizeof b, "no flashes enabled");
  } else if (cur.mode == MODE_SINGLE) {
    fmtTime(cur.single_us, tb, sizeof tb);
    snprintf(b, sizeof b, "all after %s", tb);
  } else {
    fmtTime((int)evT[evN - 1], tb, sizeof tb);
    snprintf(b, sizeof b, "%dx, last at %s", total, tb);
  }
  b[21] = 0;
  oled.setCursor(0, 48);
  oled.print(b);

  // Row 7: what started the last sequence on the left, shot counter on the right.
  snprintf(b, sizeof b, "Last: %s", SRC_NAME[lastSrc]);
  oled.setCursor(0, 56);
  oled.print(b);
  snprintf(b, sizeof b, "#%d", shots);
  oled.setCursor(126 - 6 * (int)strlen(b), 56);
  oled.print(b);

  oled.display();
}

void showConfig() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.fillRect(0, 0, 128, 8, SH110X_WHITE);            // inverted title bar
  oled.setTextColor(SH110X_BLACK);
  oled.setCursor(2, 0);   oled.print("WiFi Config Mode");
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 16);  oled.print("Join the network:");
  oled.setCursor(12, 24); oled.print(AP_SSID);
  oled.setCursor(0, 40);  oled.print("Then browse to:");
  oled.setCursor(12, 48); oled.print(WiFi.softAPIP());
  oled.setCursor(0, 56);  oled.print("Hold button = exit");
  oled.display();
}

void showOtaScreen() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.fillRect(0, 0, 128, 8, SH110X_WHITE);
  oled.setTextColor(SH110X_BLACK);
  oled.setCursor(2, 0);   oled.print("OTA Update Mode");
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 16);  oled.print("Join the network:");
  oled.setCursor(12, 24); oled.print(AP_SSID);
  oled.setCursor(0, 40);  oled.print("Upload to:");
  oled.setCursor(12, 48); oled.print(WiFi.softAPIP());
  oled.setCursor(0, 56);  oled.print("Hold button = exit");
  oled.display();
}

// ================= display: firmware update =================
// Shared by both update methods: ArduinoOTA (OTA mode) and the upload on the config page.

static int updLastPct = -1;

void showUpdateStart() {
  updLastPct = -1;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 0);  oled.print("Receiving firmware");
  oled.setCursor(0, 56); oled.print("Do not power off");
  oled.display();
}

// Progress bar. A redraw takes ~25 ms, so it only redraws in 5 % steps, not on every packet.
void showUpdateProgress(int pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  if (pct == updLastPct || (pct % 5 != 0 && pct != 100)) return;
  updLastPct = pct;
  oled.fillRect(0, 16, 128, 32, SH110X_BLACK);
  oled.drawRect(0, 20, 128, 12, SH110X_WHITE);
  oled.fillRect(2, 22, (124 * pct) / 100, 8, SH110X_WHITE);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 38); oled.print(pct); oled.print(" %");
  oled.display();
}

void showUpdateDone() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 24); oled.print("Done. Restarting...");
  oled.display();
}

// `why` is cut to one 21-character line.
void showUpdateFailed(const char* why) {
  char b[22];
  snprintf(b, sizeof b, "%s", why);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 16); oled.print("Update failed");
  oled.setCursor(0, 32); oled.print(b);
  oled.display();
}

// Switch the panel off (sleep mode, almost no current). The RAM keeps its content.
void displaySleep() {
  oled.oled_command(SH110X_DISPLAYOFF);
  dispOn = false;
}

// Panel on, no timeout (menu and config mode).
void displayOn() {
  if (!dispOn) {
    oled.oled_command(SH110X_DISPLAYON);
    dispOn = true;
  }
}

// Panel on and (re)start the timeout. Call after drawing, so the old screen never flashes up.
void displayWake() {
  displayOn();
  dispOffAt = millis() + (unsigned long)cfg.disp_sec * 1000UL;
}


// Text of `size` centered on the 128 px wide screen at row y.
void centerText(const char* t, int y, int size) {
  int w = (int)strlen(t) * 6 * size - size;      // 6 px per character cell, minus the trailing gap
  oled.setTextSize(size);
  oled.setCursor((128 - w) / 2, y);
  oled.print(t);
}

// Startup splash: two big lines (3 x 24 px each) and the version below, 64 px in all.
void showSplash() {
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  centerText("FLASH!", 0, 3);
  centerText("BANG!", 24, 3);
  char v[24];
  snprintf(v, sizeof v, "Firmware %s", FW_VERSION);
  centerText(v, 56, 1);
  oled.display();
}
