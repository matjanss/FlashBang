// FlashBang V3.0 - settings in flash (NVS), clamping
#include "app.h"

// ================= storage =================

void clampSetup(Setup& s) {
  s.mode   = constrain(s.mode, 0, NUM_MODES - 1);
  s.chmask &= 0x0F;
  s.single_us = constrain(s.single_us, 0, MAX_DELAY_US);
  int total = 0;
  for (int i = 0; i < NCH; i++) {
    s.interval_us[i] = constrain(s.interval_us[i], 1000, 2000000);
    s.delay_us[i]    = constrain(s.delay_us[i], 0, MAX_DELAY_US);
    s.count[i]       = constrain(s.count[i], 0, 99);
    if (total + s.count[i] > MAX_EVENTS) s.count[i] = MAX_EVENTS - total;   // 100 flashes in all
    total += s.count[i];
  }
  s.trig_src = constrain(s.trig_src, 0, NUM_TRIG - 1);
  s.ext_high = constrain(s.ext_high, 0, 1);
  s.seq_n = constrain(s.seq_n, 0, MAX_SEQ);
  for (int i = 0; i < MAX_SEQ; i++) {
    if (i < s.seq_n) {
      s.seq[i].t_us = constrain(s.seq[i].t_us, 0, MAX_DELAY_US);
      s.seq[i].ch   = constrain(s.seq[i].ch, 0, NCH - 1);
    } else {
      s.seq[i].t_us = 0;                         // unused entries stay zero, so setups compare equal
      s.seq[i].ch = 0;
    }
  }
}

int totalCount() {
  int t = 0;
  for (int c = 0; c < NCH; c++) t += cur.count[c];
  return t;
}

void clampConfig() {
  cfg.holdoff_ms   = constrain(cfg.holdoff_ms, 0, 60000);
  if (cfg.cpu_mhz != 80 && cfg.cpu_mhz != 160 && cfg.cpu_mhz != 240) cfg.cpu_mhz = 80;
  cfg.bat_cal      = constrain(cfg.bat_cal, 0.5f, 1.5f);
  cfg.batt_mah     = constrain(cfg.batt_mah, 100, 30000);
  cfg.avg_ma       = constrain(cfg.avg_ma, 1.0f, 1000.0f);
  cfg.disp_sec     = constrain(cfg.disp_sec, 1, 600);
}

void loadAll() {
  prefs.begin("cfg", false);

  cfg.holdoff_ms   = prefs.getInt("holdoff_ms", DEF.holdoff_ms);
  cfg.cpu_mhz      = prefs.getInt("cpu_mhz", DEF.cpu_mhz);
  cfg.bat_cal      = prefs.getFloat("bat_cal", DEF.bat_cal);
  cfg.batt_mah     = prefs.getInt("batt_mah", DEF.batt_mah);
  cfg.avg_ma       = prefs.getFloat("avg_ma", DEF.avg_ma);
  cfg.disp_sec     = prefs.getInt("disp_sec", DEF.disp_sec);

  cur.mode      = prefs.getInt("mode", DEF_SETUP.mode);
  cur.chmask    = prefs.getInt("chmask", DEF_SETUP.chmask);
  cur.single_us = prefs.getInt("single_us", DEF_SETUP.single_us);
  for (int i = 0; i < NCH; i++) {
    char k[8];
    snprintf(k, sizeof k, "n%d", i);   cur.count[i]       = prefs.getInt(k, DEF_SETUP.count[i]);
    snprintf(k, sizeof k, "iv%d", i);  cur.interval_us[i] = prefs.getInt(k, DEF_SETUP.interval_us[i]);
    snprintf(k, sizeof k, "md%d", i);  cur.delay_us[i]    = prefs.getInt(k, DEF_SETUP.delay_us[i]);
  }
  cur.trig_src = prefs.getInt("trig_src", DEF_SETUP.trig_src);
  cur.ext_high = prefs.getInt("ext_high", DEF_SETUP.ext_high);
  cur.seq_n    = prefs.getInt("seq_n", 0);
  if (cur.seq_n < 0 || cur.seq_n > MAX_SEQ) cur.seq_n = 0;
  if (cur.seq_n > 0 && prefs.getBytes("seq", cur.seq, sizeof(SeqEntry) * cur.seq_n) != sizeof(SeqEntry) * cur.seq_n) cur.seq_n = 0;
  curPreset    = prefs.getInt("cur_preset", -1);

  for (int i = 0; i < MAX_PRESETS; i++) {
    char k[8]; snprintf(k, sizeof k, "p%d", i);
    memset(&presets[i], 0, sizeof(Preset));
    size_t n = prefs.getBytes(k, &presets[i], sizeof(Preset));
    if (n != sizeof(Preset)) { memset(&presets[i], 0, sizeof(Preset)); }
    presets[i].name[15] = 0;
    clampSetup(presets[i].s);
  }
  prefs.end();

  clampConfig();
  clampSetup(cur);
  seqSort(cur, -1);
  for (int i = 0; i < MAX_PRESETS; i++) seqSort(presets[i].s, -1);
  if (curPreset < 0 || curPreset >= MAX_PRESETS || !presets[curPreset].used) curPreset = -1;
}

void saveConfig() {
  prefs.begin("cfg", false);
  prefs.putInt("holdoff_ms", cfg.holdoff_ms);
  prefs.putInt("cpu_mhz", cfg.cpu_mhz);
  prefs.putFloat("bat_cal", cfg.bat_cal);
  prefs.putInt("batt_mah", cfg.batt_mah);
  prefs.putFloat("avg_ma", cfg.avg_ma);
  prefs.putInt("disp_sec", cfg.disp_sec);
  prefs.end();
}

void saveSetup() {
  prefs.begin("cfg", false);
  prefs.putInt("mode", cur.mode);
  prefs.putInt("chmask", cur.chmask);
  prefs.putInt("single_us", cur.single_us);
  for (int i = 0; i < NCH; i++) {
    char k[8];
    snprintf(k, sizeof k, "n%d", i);   prefs.putInt(k, cur.count[i]);
    snprintf(k, sizeof k, "iv%d", i);  prefs.putInt(k, cur.interval_us[i]);
    snprintf(k, sizeof k, "md%d", i);  prefs.putInt(k, cur.delay_us[i]);
  }
  prefs.putInt("trig_src", cur.trig_src);
  prefs.putInt("ext_high", cur.ext_high);
  prefs.putInt("seq_n", cur.seq_n);
  if (cur.seq_n > 0) prefs.putBytes("seq", cur.seq, sizeof(SeqEntry) * cur.seq_n);
  prefs.putInt("cur_preset", curPreset);
  prefs.end();
}

void savePreset(int i) {
  char k[8]; snprintf(k, sizeof k, "p%d", i);
  prefs.begin("cfg", false);
  prefs.putBytes(k, &presets[i], sizeof(Preset));
  prefs.end();
}

void saveAllIfDirty() {
  if (!dirty) return;
  saveSetup();
  saveConfig();
  dirty = false;
  saveAt = 0;
}
