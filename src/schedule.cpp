// FlashBang V3.0 - turns the setup into a timed list of flash events and plays it
#include "app.h"

// ================= flash schedule =================

// Turns the setup into a time-ordered list of flash events (times in us after the trigger).
//   Single: the enabled channels fire together, once, after the single delay.
//   Multi:  every channel fires `count` times, `interval` apart, starting at its own delay.
//   Sequence: a free list of (time, channel) flashes; flashes at the same time fire together.
void buildSchedule() {
  evN = 0;
  evPw = PULSE_US;

  if (cur.mode == MODE_SINGLE) {
    if (cur.chmask & 0x0F) {
      evT[0] = (uint32_t)cur.single_us;
      evM[0] = (uint8_t)(cur.chmask & 0x0F);
      evN = 1;
    }
    return;
  }

  // Multi and Sequence: collect every flash, sort by time, merge flashes that share a time.
  static uint32_t tT[MAX_EVENTS];
  static uint8_t  tM[MAX_EVENTS];
  int n = 0;
  if (cur.mode == MODE_SEQ) {
    for (int i = 0; i < cur.seq_n && n < MAX_EVENTS; i++) {
      tT[n] = (uint32_t)cur.seq[i].t_us;
      tM[n] = (uint8_t)(1 << cur.seq[i].ch);
      n++;
    }
  } else for (int c = 0; c < NCH; c++) {
    for (int k = 0; k < cur.count[c] && n < MAX_EVENTS; k++) {
      tT[n] = (uint32_t)cur.delay_us[c] + (uint32_t)k * (uint32_t)cur.interval_us[c];
      tM[n] = (uint8_t)(1 << c);
      n++;
    }
    if (cur.count[c] >= 2) {                       // a channel that repeats must be able to re-fire
      uint32_t half = (uint32_t)cur.interval_us[c] / 2;
      if (half < 50) half = 50;
      if (evPw > half) evPw = half;
    }
  }
  for (int i = 1; i < n; i++) {                    // insertion sort by time
    uint32_t t = tT[i];
    uint8_t m = tM[i];
    int j = i - 1;
    while (j >= 0 && tT[j] > t) { tT[j + 1] = tT[j]; tM[j + 1] = tM[j]; j--; }
    tT[j + 1] = t;
    tM[j + 1] = m;
  }
  if (cur.mode == MODE_SEQ) {                      // a channel that flashes again soon must be able to re-fire
    uint32_t last[NCH];
    bool seen[NCH] = {false, false, false, false};
    for (int i = 0; i < n; i++) {
      int c = 0;
      while (c < NCH - 1 && !(tM[i] & (1 << c))) c++;
      if (seen[c] && tT[i] > last[c]) {
        uint32_t half = (tT[i] - last[c]) / 2;
        if (half < 50) half = 50;
        if (evPw > half) evPw = half;
      }
      seen[c] = true;
      last[c] = tT[i];
    }
  }
  for (int i = 0; i < n; i++) {
    if (evN > 0 && evT[evN - 1] == tT[i]) {
      evM[evN - 1] |= tM[i];
    } else {
      evT[evN] = tT[i];
      evM[evN] = tM[i];
      evN++;
    }
  }
}

inline void writeMask(uint8_t m, int level) {
  for (int c = 0; c < NCH; c++) if (m & (1 << c)) digitalWrite(FLASH_PIN[c], level);
}

void allOff() {
  for (int c = 0; c < NCH; c++) digitalWrite(FLASH_PIN[c], LOW);
}

// Sorts a sequence by time (stable: entries with the same time keep their order). `track` is the
// index of an entry the caller wants to keep following; its new index is returned.
int seqSort(Setup& s, int track) {
  for (int i = 1; i < s.seq_n; i++) {
    SeqEntry e = s.seq[i];
    int j = i - 1;
    while (j >= 0 && s.seq[j].t_us > e.t_us) { s.seq[j + 1] = s.seq[j]; j--; }
    s.seq[j + 1] = e;
    if (track == i) track = j + 1;
    else if (track > j && track < i) track++;
  }
  return track;
}

// Number of sequence entries on a channel.
int seqCount(const Setup& s, int ch) {
  int n = 0;
  for (int i = 0; i < s.seq_n; i++) if (s.seq[i].ch == ch) n++;
  return n;
}

// Plays the schedule, timed from t0 (the moment of the bang). Pressing the button aborts it.
void fireSchedule(uint32_t t0) {
  int i = 0, j = 0;                                 // i = next ON event, j = next OFF event
  while (j < evN) {
    uint32_t now = micros() - t0;
    bool did = false;
    if (i < evN && now >= evT[i]) { writeMask(evM[i], HIGH); i++; did = true; }
    if (j < i && now >= evT[j] + evPw) { writeMask(evM[j], LOW); j++; did = true; }
    if (!did) {
      uint32_t nextOn  = (i < evN) ? evT[i] : 0xFFFFFFFFu;
      uint32_t nextOff = (j < i) ? evT[j] + evPw : 0xFFFFFFFFu;
      uint32_t next = (nextOn < nextOff) ? nextOn : nextOff;
      if (next - now > 3000) {                      // far away: yield instead of spinning
        vTaskDelay(1);
        if (digitalRead(BTN) == LOW) { allOff(); return; }
      }
    }
  }
}
