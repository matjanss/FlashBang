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
  ArduinoOTA.onStart([]() { showUpdateStart(); });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    showUpdateProgress(total ? (int)((uint64_t)done * 100 / total) : 0);
  });
  ArduinoOTA.onEnd([]() { showUpdateDone(); });
  ArduinoOTA.onError([](ota_error_t e) {
    char b[16];
    snprintf(b, sizeof b, "error %d", (int)e);
    showUpdateFailed(b);
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
