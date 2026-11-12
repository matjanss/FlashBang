// FlashBang V3.0 - on-device menu: pages, editing, quick adjust
#include "app.h"

// ================= menu =================

enum ItemType { IT_SUB, IT_BACK, IT_VAL, IT_TOGGLE, IT_ENUM, IT_ACTION };
// Value ids. Everything up to V_SEQT belongs to the setup (and so to presets).
enum { V_SDELAY,
       V_COUNT0, V_COUNT1, V_COUNT2, V_COUNT3,
       V_INT0, V_INT1, V_INT2, V_INT3,
       V_DLY0, V_DLY1, V_DLY2, V_DLY3,
       V_SEQT,                                   // time of the selected sequence entry
       V_HOLDOFF, V_DISP };
enum { E_MODE, E_TRIG, E_EXTPOL, E_SEQCH };                 // enum items
enum { A_WIFI, A_OTA, A_EXIT, A_SEQ_DEL, A_SEQ_ADD };
enum { PG_MAIN, PG_MODE, PG_CH1, PG_CH2, PG_CH3, PG_CH4, PG_PRESETS, PG_ENTRY, PG_EDIT, PG_COUNT };

struct Item {
  const char* label;
  uint8_t type;
  uint8_t arg;       // page, value id, channel or action, depending on type
  int8_t  ch;        // value items on a channel page: the channel they belong to (-1 = none)
  uint8_t modes;     // bitmask of flash modes the item is shown in (1 single, 2 multi, 4 sequence)
  uint8_t minc;      // with ch >= 0: shown only if that channel's count is at least this
};

const Item MAIN_ITEMS[] = {
  {"Preset",     IT_SUB,    PG_PRESETS, -1, 7, 0},
  {"Mode",       IT_SUB,    PG_MODE,    -1, 7, 0},
  {"Trigger",    IT_ENUM,   E_TRIG,     -1, 7, 0},
  {"Ext active", IT_ENUM,   E_EXTPOL,   -1, 7, 0},   // hidden when the external input is not used
  {"Holdoff",    IT_VAL,    V_HOLDOFF,  -1, 7, 0},
  {"Display",    IT_VAL,    V_DISP,     -1, 7, 0},
  {"WiFi setup", IT_ACTION, A_WIFI,     -1, 7, 0},
  {"OTA Update", IT_ACTION, A_OTA,      -1, 7, 0},
  {"Exit",       IT_ACTION, A_EXIT,     -1, 7, 0},
};
// The Mode page: the mode itself, then what applies to it.
const Item MODE_ITEMS[] = {
  {"Mode",      IT_ENUM,   E_MODE,   -1, 7, 0},
  {"Delay",     IT_VAL,    V_SDELAY, -1, 1, 0},       // single
  {"Channel 1", IT_TOGGLE, 0,        -1, 1, 0},
  {"Channel 2", IT_TOGGLE, 1,        -1, 1, 0},
  {"Channel 3", IT_TOGGLE, 2,        -1, 1, 0},
  {"Channel 4", IT_TOGGLE, 3,        -1, 1, 0},
  {"CH1",       IT_SUB,    PG_CH1,   -1, 2, 0},       // multi
  {"CH2",       IT_SUB,    PG_CH2,   -1, 2, 0},
  {"CH3",       IT_SUB,    PG_CH3,   -1, 2, 0},
  {"CH4",       IT_SUB,    PG_CH4,   -1, 2, 0},
  {"Back",      IT_BACK,   0,        -1, 7, 0},
};
// One page per channel in multi mode. Interval only matters from 2 flashes, delay from 1.
#define CH_LIST(n) {"Count",    IT_VAL, V_COUNT0 + (n), (n), 7, 0}, \
                   {"Interval", IT_VAL, V_INT0 + (n),   (n), 7, 2}, \
                   {"Delay",    IT_VAL, V_DLY0 + (n),   (n), 7, 1}, \
                   {"Back",     IT_BACK, 0,             -1,  7, 0}
const Item CH1_ITEMS[] = { CH_LIST(0) };
const Item CH2_ITEMS[] = { CH_LIST(1) };
const Item CH3_ITEMS[] = { CH_LIST(2) };
const Item CH4_ITEMS[] = { CH_LIST(3) };

// Sequence mode: the entry page (Edit / Delete / Add / Back) and the page that edits one entry.
const Item ENTRY_ITEMS[] = {
  {"Edit",   IT_SUB,    PG_EDIT,   -1, 7, 0},
  {"Delete", IT_ACTION, A_SEQ_DEL, -1, 7, 0},
  {"Add",    IT_ACTION, A_SEQ_ADD, -1, 7, 0},
  {"Back",   IT_BACK,   0,         -1, 7, 0},
};
const Item EDIT_ITEMS[] = {
  {"Time",    IT_VAL,  V_SEQT,  -1, 7, 0},
  {"Channel", IT_ENUM, E_SEQCH, -1, 7, 0},
  {"Back",    IT_BACK, 0,       -1, 7, 0},
};
const char* const CH_NAME[NCH] = {"CH1", "CH2", "CH3", "CH4"};

const int MENU_ROWS = 7;     // items visible at once (the title bar takes the first text row)

struct PageDef { const char* title; const Item* items; int n; };
#define NITEMS(a) ((int)(sizeof(a) / sizeof(Item)))
const PageDef PAGES[PG_COUNT] = {
  {"Menu",    MAIN_ITEMS, NITEMS(MAIN_ITEMS)},
  {"Mode",    MODE_ITEMS, NITEMS(MODE_ITEMS)},
  {"CH1",     CH1_ITEMS,  NITEMS(CH1_ITEMS)},
  {"CH2",     CH2_ITEMS,  NITEMS(CH2_ITEMS)},
  {"CH3",     CH3_ITEMS,  NITEMS(CH3_ITEMS)},
  {"CH4",     CH4_ITEMS,  NITEMS(CH4_ITEMS)},
  {"Presets", nullptr,    0},                          // built on the fly from the stored presets
  {"Entry",   ENTRY_ITEMS, NITEMS(ENTRY_ITEMS)},           // title is replaced by the entry number
  {"Edit",    EDIT_ITEMS,  NITEMS(EDIT_ITEMS)},
};

UiState ui = UI_STATUS;
unsigned long lastInput = 0;
int menuPage = PG_MAIN, menuSel = 0, menuTop = 0;
int stkPage[4], stkSel[4], stkTop[4], menuDepth = 0;   // where Back goes
// Entries of the Mode page in sequence mode are listed as VIS_SEQ + entry index; "Add Entry" is VIS_ADD.
const int VIS_ADD = 900, VIS_SEQ = 1000;
int seqSel = 0;              // sequence entry the Entry / Edit pages work on
int vis[MAX_SEQ + 8];        // visible entries of the current page (item index, preset slot, VIS_* code, or -1 = Back)
int nvis = 0;
const char* editLabel = "";
int editId = 0, editBackup = 0;
bool editIsEnum = false;

// accel: 0 = fixed step, 1 = faster turning gives x5 / x20, 2 = a delay: steps grow with the value
struct ValInfo { int* p; int mn; int mx; int step; int accel; };

bool getValInfo(int id, ValInfo& v) {
  if (id == V_SDELAY) { v = { &cur.single_us, 0, MAX_DELAY_US, 100, 2 }; return true; }
  if (id >= V_COUNT0 && id <= V_COUNT3) {
    int c = id - V_COUNT0;
    int room = MAX_EVENTS - (totalCount() - cur.count[c]);        // 100 flashes in all
    v = { &cur.count[c], 0, room < 99 ? room : 99, 1, 0 };
    return true;
  }
  if (id >= V_INT0 && id <= V_INT3) { v = { &cur.interval_us[id - V_INT0], 1000, 2000000, 100, 1 }; return true; }
  if (id >= V_DLY0 && id <= V_DLY3) { v = { &cur.delay_us[id - V_DLY0], 0, MAX_DELAY_US, 100, 2 }; return true; }
  if (id == V_SEQT) {
    if (seqSel < 0 || seqSel >= cur.seq_n) return false;
    v = { &cur.seq[seqSel].t_us, 0, MAX_DELAY_US, 100, 2 };
    return true;
  }
  if (id == V_HOLDOFF) { v = { &cfg.holdoff_ms, 0, 60000, 100, 1 }; return true; }
  if (id == V_DISP)    { v = { &cfg.disp_sec,   1, 600,   1,   0 }; return true; }
  return false;
}

struct EnumInfo { int* p; int n; const char* const* names; bool setup; };

bool getEnumInfo(int id, EnumInfo& e) {
  switch (id) {
    case E_MODE:   e = { &cur.mode,      NUM_MODES, MODE_NAME, true };  return true;
    case E_TRIG:   e = { &cur.trig_src,  NUM_TRIG,  TRIG_NAME, true };  return true;
    case E_EXTPOL: e = { &cur.ext_high,  2,         POL_NAME,  true };  return true;
    case E_SEQCH:
      if (seqSel < 0 || seqSel >= cur.seq_n) return false;
      e = { &cur.seq[seqSel].ch, NCH, CH_NAME, true };
      return true;
  }
  return false;
}

void fmtVal(int id, char* buf, size_t n) {
  ValInfo v;
  buf[0] = 0;
  if (!getValInfo(id, v)) return;
  int x = *v.p;
  if (id == V_SDELAY || (id >= V_INT0 && id <= V_SEQT)) fmtTime(x, buf, n);
  else if (id >= V_COUNT0 && id <= V_COUNT3) {
    if (x == 0) snprintf(buf, n, "off");
    else        snprintf(buf, n, "%d", x);
  }
  else if (id == V_HOLDOFF) snprintf(buf, n, "%.1fs", x / 1000.0);
  else if (id == V_DISP)    snprintf(buf, n, "%ds", x);
}

void buildVisible() {
  nvis = 0;
  if (menuPage == PG_PRESETS) {
    for (int i = 0; i < MAX_PRESETS; i++) if (presets[i].used) vis[nvis++] = i;
    vis[nvis++] = -1;
    return;
  }
  if (menuPage == PG_MODE && cur.mode == MODE_SEQ) {
    vis[nvis++] = 0;                                         // the Mode item
    for (int i = 0; i < cur.seq_n; i++) vis[nvis++] = VIS_SEQ + i;
    if (cur.seq_n < MAX_SEQ) vis[nvis++] = VIS_ADD;
    vis[nvis++] = NITEMS(MODE_ITEMS) - 1;                    // Back is the last item of the page
    return;
  }
  const PageDef& pg = PAGES[menuPage];
  for (int i = 0; i < pg.n; i++) {
    const Item& it = pg.items[i];
    if (!(it.modes & (1 << cur.mode))) continue;
    if (it.type == IT_ENUM && it.arg == E_EXTPOL && !(trigMask(cur.trig_src) & TRIGB_EXT)) continue;
    if (it.type == IT_VAL && it.ch >= 0 && cur.count[it.ch] < it.minc) continue;
    vis[nvis++] = i;
  }
}

void fixScroll() {
  if (nvis > 0 && menuSel >= nvis) menuSel = nvis - 1;
  if (menuSel < menuTop) menuTop = menuSel;
  if (menuSel > menuTop + MENU_ROWS - 1) menuTop = menuSel - (MENU_ROWS - 1);
  if (menuTop < 0) menuTop = 0;
}

void itemStrings(int idx, char* label, size_t ln, char* value, size_t vn) {
  label[0] = 0;
  value[0] = 0;
  if (menuPage == PG_PRESETS) {
    if (idx < 0) { snprintf(label, ln, "Back"); return; }
    snprintf(label, ln, "%s", presets[idx].name);
    if (idx == curPreset) snprintf(value, vn, "*");
    return;
  }
  if (menuPage == PG_MODE && cur.mode == MODE_SEQ && idx >= VIS_ADD) {
    if (idx == VIS_ADD) { snprintf(label, ln, "Add Entry"); snprintf(value, vn, "+"); return; }
    int i = idx - VIS_SEQ;
    fmtTime(cur.seq[i].t_us, label, ln);
    snprintf(value, vn, "%s", CH_NAME[cur.seq[i].ch]);
    return;
  }
  const Item& it = PAGES[menuPage].items[idx];
  snprintf(label, ln, "%s", it.label);
  switch (it.type) {
    case IT_SUB:
      if (it.arg == PG_MODE) {
        snprintf(value, vn, "%s", MODE_NAME[cur.mode]);              // shows the current mode
      } else if (it.arg >= PG_CH1 && it.arg <= PG_CH4) {
        int n = cur.count[it.arg - PG_CH1];                           // shows the channel's flash count
        if (n == 0) snprintf(value, vn, "off");
        else        snprintf(value, vn, "%dx", n);
      } else {
        snprintf(value, vn, ">");
      }
      break;
    case IT_TOGGLE: snprintf(value, vn, "%s", (cur.chmask >> it.arg) & 1 ? "ON" : "off"); break;
    case IT_ENUM: {
      EnumInfo e;
      if (getEnumInfo(it.arg, e)) snprintf(value, vn, "%s", e.names[*e.p]);
      break;
    }
    case IT_VAL:    fmtVal(it.arg, value, vn); break;
  }
}

// Menu: title bar plus seven rows. The selected row is inverted; arrows in the
// title bar show that there is more above or below.
void drawMenu() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.fillRect(0, 0, 128, 8, SH110X_WHITE);
  oled.setTextColor(SH110X_BLACK);
  oled.setCursor(2, 0);
  char title[24];
  if (menuPage == PG_ENTRY || menuPage == PG_EDIT) snprintf(title, sizeof title, "Entry %d of %d", seqSel + 1, cur.seq_n);
  else snprintf(title, sizeof title, "%s", PAGES[menuPage].title);
  oled.print(title);
  if (menuTop > 0)                { oled.setCursor(110, 0); oled.print((char)0x18); }   // up arrow
  if (menuTop + MENU_ROWS < nvis) { oled.setCursor(118, 0); oled.print((char)0x19); }   // down arrow

  for (int row = 0; row < MENU_ROWS; row++) {
    int k = menuTop + row;
    if (k >= nvis) break;
    char label[24], value[16];
    itemStrings(vis[k], label, sizeof label, value, sizeof value);
    int y = 8 + row * 8;
    bool sel = (k == menuSel);
    if (sel) oled.fillRect(0, y, 128, 8, SH110X_WHITE);
    oled.setTextColor(sel ? SH110X_BLACK : SH110X_WHITE);
    int vlen = (int)strlen(value);
    int maxLabel = 20 - vlen - (vlen ? 1 : 0);
    if ((int)strlen(label) > maxLabel) label[maxLabel] = 0;
    oled.setCursor(2, y);
    oled.print(label);
    oled.setCursor(126 - 6 * vlen, y);
    oled.print(value);
  }
  oled.display();
}

// Edit screen: the value in big letters, centred, with a helpful line below.
void drawEdit() {
  char b[16];
  EnumInfo e;
  if (editIsEnum && getEnumInfo(editId, e)) snprintf(b, sizeof b, "%s", e.names[*e.p]);
  else fmtVal(editId, b, sizeof b);

  char hint[24];
  snprintf(hint, sizeof hint, "turn: change value");
  if (!editIsEnum) {
    ValInfo v;
    if (editId >= V_INT0 && editId <= V_INT3 && getValInfo(editId, v)) snprintf(hint, sizeof hint, "= %.1f Hz", 1e6 / *v.p);
    else if (editId >= V_COUNT0 && editId <= V_COUNT3)                 snprintf(hint, sizeof hint, "all channels: %d/%d", totalCount(), MAX_EVENTS);
  }
  if ((!editIsEnum && editId == V_SEQT) || (editIsEnum && editId == E_SEQCH))
    snprintf(hint, sizeof hint, "entry %d of %d", seqSel + 1, cur.seq_n);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.fillRect(0, 0, 128, 8, SH110X_WHITE);
  oled.setTextColor(SH110X_BLACK);
  oled.setCursor(2, 0);
  oled.print(editLabel);
  oled.setTextColor(SH110X_WHITE);

  int len = (int)strlen(b);
  int size = (len <= 6) ? 3 : 2;                         // 3x text for short values, 2x for long ones
  oled.setTextSize(size);
  oled.setCursor((128 - len * 6 * size) / 2, size == 3 ? 16 : 20);
  oled.print(b);

  oled.setTextSize(1);
  oled.setCursor(0, 48);
  oled.print(hint);
  oled.setCursor(0, 56);
  oled.print("click=OK  hold=cancel");
  oled.display();
}

// A setup or config value changed in the menu.
void changed(bool setupChanged) {
  if (setupChanged) {
    curPreset = -1;
    clampSetup(cur);
    buildSchedule();
  }
  dirty = true;
}

void scheduleSave() {
  dirty = true;
  saveAt = millis() + 3000;
}

void openMenu() {
  ui = UI_MENU;
  menuPage = PG_MAIN;
  menuSel = 0;
  menuTop = 0;
  menuDepth = 0;
  buildVisible();
  displayOn();
  drawMenu();
}

void closeMenu() {
  ui = UI_STATUS;
  menuDepth = 0;
  saveAllIfDirty();
  armTriggers();                                 // also picks up a loaded preset's trigger source
  show(stateText());
  displayWake();
}

// Opens a sub page and remembers where to come back to.
void pushPage(int page) {
  stkPage[menuDepth] = menuPage;
  stkSel[menuDepth]  = menuSel;
  stkTop[menuDepth]  = menuTop;
  menuDepth++;
  menuPage = page;
  menuSel = 0;
  menuTop = 0;
}

void menuBack() {
  if (menuDepth == 0) { closeMenu(); return; }
  int from = menuPage;
  menuDepth--;
  menuPage = stkPage[menuDepth];
  menuSel  = stkSel[menuDepth];
  menuTop  = stkTop[menuDepth];
  buildVisible();
  if (menuPage == PG_MODE && cur.mode == MODE_SEQ && (from == PG_ENTRY || from == PG_EDIT))
    menuSel = 1 + (seqSel < 0 ? 0 : seqSel);                 // back on the entry we just worked on
  fixScroll();
  drawMenu();
}

void loadPreset(int slot) {
  cur = presets[slot].s;
  clampSetup(cur);
  curPreset = slot;
  buildSchedule();
  saveSetup();
}

void startEdit(const char* label, int id, bool isEnum) {
  editLabel = label;
  editId = id;
  editIsEnum = isEnum;
  if (isEnum) { EnumInfo e; getEnumInfo(id, e); editBackup = *e.p; }
  else { ValInfo v; getValInfo(id, v); editBackup = *v.p; }
  ui = UI_EDIT;
  drawEdit();
}

void editAdjust(int steps) {
  if (editIsEnum) {
    EnumInfo e;
    if (!getEnumInfo(editId, e)) return;
    *e.p = ((*e.p + steps) % e.n + e.n) % e.n;
  } else {
    ValInfo v;
    if (!getValInfo(editId, v)) return;
    long step = v.step;
    if (v.accel == 2)      step = timeStep(*v.p);              // delays: fine when slow, grows with the value when fast
    else if (v.accel == 1) step = (long)v.step * accelMult();
    long nv = (long)*v.p + (long)steps * step;
    if (nv < v.mn) nv = v.mn;
    if (nv > v.mx) nv = v.mx;
    *v.p = (int)nv;
  }
  buildSchedule();                               // the schedule follows the edit, so a test fire uses it
  drawEdit();
}

void leaveEdit() {
  ui = UI_MENU;
  buildVisible();
  fixScroll();
  drawMenu();
}

void editConfirm() {
  if (editIsEnum) {
    EnumInfo e;
    if (getEnumInfo(editId, e) && e.setup) changed(true);
    else dirty = true;
    if (editId == E_TRIG || editId == E_EXTPOL) armTriggers();   // reconnect the inputs
  } else {
    changed(editId < V_HOLDOFF);
    if (editId == V_SEQT) seqSel = seqSort(cur, seqSel);       // the entry may have moved in the list
  }
  leaveEdit();
}

void editCancel() {
  if (editIsEnum) { EnumInfo e; if (getEnumInfo(editId, e)) *e.p = editBackup; }
  else { ValInfo v; if (getValInfo(editId, v)) *v.p = editBackup; }
  buildSchedule();
  leaveEdit();
}


// Inserts an entry before index `at` and makes it the selected one.
void seqInsert(int at, int t_us, int ch) {
  if (cur.seq_n >= MAX_SEQ) return;
  for (int i = cur.seq_n; i > at; i--) cur.seq[i] = cur.seq[i - 1];
  cur.seq[at].t_us = t_us;
  cur.seq[at].ch = ch;
  cur.seq_n++;
  seqSel = at;
  changed(true);
}

void seqDeleteSelected() {
  if (seqSel < 0 || seqSel >= cur.seq_n) return;
  for (int i = seqSel; i < cur.seq_n - 1; i++) cur.seq[i] = cur.seq[i + 1];
  cur.seq_n--;
  cur.seq[cur.seq_n].t_us = 0;
  cur.seq[cur.seq_n].ch = 0;
  if (seqSel >= cur.seq_n) seqSel = cur.seq_n - 1;
  if (seqSel < 0) seqSel = 0;
  changed(true);
}

void activateItem() {
  int v = vis[menuSel];
  if (menuPage == PG_MODE && cur.mode == MODE_SEQ && v >= VIS_ADD) {
    if (v == VIS_ADD) {                                        // new entry at the end, then straight into editing it
      seqInsert(cur.seq_n, cur.seq_n ? cur.seq[cur.seq_n - 1].t_us : 0, 0);
      pushPage(PG_EDIT);
    } else {                                                   // an existing entry
      seqSel = v - VIS_SEQ;
      pushPage(PG_ENTRY);
    }
    buildVisible();
    drawMenu();
    return;
  }
  if (menuPage == PG_PRESETS) {
    if (v < 0) menuBack();
    else { loadPreset(v); closeMenu(); }
    return;
  }
  const Item& it = PAGES[menuPage].items[v];
  switch (it.type) {
    case IT_SUB:
      pushPage(it.arg);
      buildVisible();
      drawMenu();
      break;
    case IT_BACK:
      menuBack();
      break;
    case IT_TOGGLE:
      cur.chmask ^= (1 << it.arg);
      changed(true);
      buildVisible();
      fixScroll();
      drawMenu();
      break;
    case IT_ENUM:
      startEdit(it.label, it.arg, true);
      break;
    case IT_VAL:
      startEdit(it.label, it.arg, false);
      break;
    case IT_ACTION:
      if (it.arg == A_SEQ_DEL) { seqDeleteSelected(); menuBack(); break; }
      if (it.arg == A_SEQ_ADD) {                               // next to this entry: same time, next channel
        if (cur.seq_n >= MAX_SEQ) break;
        seqInsert(seqSel + 1, cur.seq[seqSel].t_us, (cur.seq[seqSel].ch + 1) % NCH);
        pushPage(PG_EDIT);
        buildVisible();
        drawMenu();
        break;
      }
      closeMenu();
      if (it.arg == A_WIFI) startAp();
      else if (it.arg == A_OTA) startOta();
      break;
  }
}

// Rotating the encoder on the status screen shifts the timing: the single delay in single mode,
// the delays of all active channels together (offsets are kept) in multi mode, the whole
// sequence in sequence mode.
void quickAdjust(int steps) {
  if (cur.mode == MODE_SINGLE) {
    long nv = (long)cur.single_us + (long)steps * timeStep(cur.single_us);
    if (nv < 0) nv = 0;
    if (nv > MAX_DELAY_US) nv = MAX_DELAY_US;
    cur.single_us = (int)nv;
  } else if (cur.mode == MODE_SEQ) {                   // shift the whole sequence, spacing is kept
    if (cur.seq_n == 0) return;
    int mn = MAX_DELAY_US, mx = 0;
    for (int i = 0; i < cur.seq_n; i++) {
      if (cur.seq[i].t_us < mn) mn = cur.seq[i].t_us;
      if (cur.seq[i].t_us > mx) mx = cur.seq[i].t_us;
    }
    long delta = (long)steps * timeStep(mn);
    if (delta < -mn) delta = -mn;
    if (delta > MAX_DELAY_US - mx) delta = MAX_DELAY_US - mx;
    for (int i = 0; i < cur.seq_n; i++) cur.seq[i].t_us += (int)delta;
  } else {
    int mn = MAX_DELAY_US, mx = 0;
    bool any = false;
    for (int c = 0; c < NCH; c++) {
      if (cur.count[c] <= 0) continue;
      if (cur.delay_us[c] < mn) mn = cur.delay_us[c];
      if (cur.delay_us[c] > mx) mx = cur.delay_us[c];
      any = true;
    }
    if (!any) return;
    long delta = (long)steps * timeStep(mn);          // sized by the earliest delay, applied to all
    if (delta < -mn) delta = -mn;
    if (delta > MAX_DELAY_US - mx) delta = MAX_DELAY_US - mx;
    for (int c = 0; c < NCH; c++) if (cur.count[c] > 0) cur.delay_us[c] += (int)delta;
  }
  curPreset = -1;
  buildSchedule();
  scheduleSave();
}

void handleUi(BtnEvent be, int steps) {
  if (ui == UI_STATUS) {
    if (be == BTN_SHORT) { openMenu(); return; }
    if (steps) quickAdjust(steps);
    show(stateText());                              // any input lights the display
    displayWake();
    return;
  }
  if (ui == UI_MENU) {
    if (steps && nvis > 0) {
      menuSel = ((menuSel + steps) % nvis + nvis) % nvis;
      fixScroll();
    }
    if (be == BTN_SHORT) { activateItem(); return; }
    if (be == BTN_LONG)  { menuBack(); return; }
    drawMenu();
    return;
  }
  // UI_EDIT
  if (steps) editAdjust(steps);
  if (be == BTN_SHORT) editConfirm();
  else if (be == BTN_LONG) editCancel();
}
