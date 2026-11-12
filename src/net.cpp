// FlashBang V3.0 - config AP with captive portal, and ArduinoOTA mode
#include "app.h"
#include <ArduinoOTA.h>

// ================= WiFi config mode =================

void startAp() {
  detachTriggers();                              // no triggering while configuring
  hit = false;
  fireReq = false;
  holdoff = false;
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, nullptr, 1, 0, 2);        // open network, max 2 clients
  server.begin();
  dns.start(53, "*", WiFi.softAPIP());           // captive portal: all names resolve to us
  apActive = true;
  showConfig();
  displayOn();                                   // stays lit for as long as config mode lasts
}

void startOta() {
  detachTriggers();                              // no triggering while updating
  hit = false;
  fireReq = false;
  holdoff = false;
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, nullptr, 1, 0, 2);        // open network, max 2 clients

  ArduinoOTA.setHostname(OTA_HOST);
  ArduinoOTA.onStart([]() {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SH110X_WHITE);
    oled.setCursor(0, 0);  oled.print("Receiving firmware");
    oled.setCursor(0, 56); oled.print("Do not power off");
    oled.display();
  });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    static int lastPct = -1;
    int pct = total ? (int)((uint64_t)done * 100 / total) : 0;
    if (pct == lastPct || (pct % 5 != 0 && pct != 100)) return;   // a redraw takes ~25 ms: not on every packet
    lastPct = pct;
    oled.fillRect(0, 16, 128, 32, SH110X_BLACK);
    oled.drawRect(0, 20, 128, 12, SH110X_WHITE);
    oled.fillRect(2, 22, (124 * pct) / 100, 8, SH110X_WHITE);
    oled.setCursor(0, 38); oled.print(pct); oled.print(" %");
    oled.display();
  });
  ArduinoOTA.onEnd([]() {
    oled.clearDisplay();
    oled.setCursor(0, 24); oled.print("Done. Restarting...");
    oled.display();
  });
  ArduinoOTA.onError([](ota_error_t e) {
    oled.clearDisplay();
    oled.setCursor(0, 16); oled.print("Update failed");
    oled.setCursor(0, 32); oled.print("error "); oled.print((int)e);
    oled.display();
    delay(2500);
    showOtaScreen();
  });
  ArduinoOTA.begin();

  otaActive = true;
  apActive = true;
  showOtaScreen();
  displayOn();                                   // stays lit for as long as OTA mode lasts
}

void stopAp() {
  if (otaActive) { ArduinoOTA.end(); otaActive = false; }
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  apActive = false;

  // Apply the (possibly changed) settings.
  setCpuFrequencyMhz(cfg.cpu_mhz);
  buildSchedule();
  sensRaw = readSensRaw();
  sensPct = rawToPct(sensRaw);
  readBattery();
  armTriggers();
  show(stateText());
  displayWake();                                 // lit for disp_sec, then off
}

// Called from loop() while config or OTA mode runs.
void netService() {
  if (otaActive) ArduinoOTA.handle();
  else { dns.processNextRequest(); server.handleClient(); }
}
