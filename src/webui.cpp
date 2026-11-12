// FlashBang V3.0 - WiFi config page and its form handlers
#include "app.h"
#include <Update.h>

// ================= web page =================

const char PAGE_HEAD[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>FlashBang V3.0</title>
<style>
:root{--bg:#f4f5f7;--card:#fff;--ink:#14161a;--mute:#6b7280;--line:#e5e7eb;--acc:#4f46e5;--accink:#fff;--ok:#047857;--okbg:#ecfdf5;--bad:#b91c1c;--badbg:#fef2f2}
@media(prefers-color-scheme:dark){:root{--bg:#0e1013;--card:#171a1f;--ink:#e8eaee;--mute:#8b93a1;--line:#272c34;--acc:#818cf8;--accink:#0e1013;--ok:#6ee7b7;--okbg:#10231c;--bad:#fca5a5;--badbg:#2a1416}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
main{max-width:560px;margin:0 auto;padding:20px 16px 48px}
h1{font-size:1.6rem;margin:8px 0 2px;letter-spacing:-.01em}
.sub{color:var(--mute);margin:0 0 20px}
.card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:4px 16px 8px;margin:0 0 16px}
.card h2{font-size:.72rem;letter-spacing:.09em;text-transform:uppercase;color:var(--mute);margin:14px 0 2px}
.f{display:block;padding:12px 0}
.f+.f{border-top:1px solid var(--line)}
.l{font-weight:600}
.in{display:flex;align-items:center;gap:10px;margin-top:6px}
input,select{flex:1;min-width:0;height:44px;padding:0 12px;border:1px solid var(--line);border-radius:10px;background:var(--bg);color:var(--ink);font:inherit}
input:focus,select:focus{outline:2px solid var(--acc);outline-offset:1px}
input[type=checkbox]{flex:none;width:18px;height:18px;padding:0;accent-color:var(--acc)}
.in em{color:var(--mute);font-style:normal;min-width:3.2em}
small{display:block;color:var(--mute);margin-top:6px;font-size:.82rem}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:6px}
.g .l{font-size:.85rem;font-weight:500;color:var(--mute)}
.chips{display:flex;gap:8px;margin-top:6px}
.chips label{flex:1;display:flex;align-items:center;justify-content:center;gap:8px;height:44px;border:1px solid var(--line);border-radius:10px;background:var(--bg);cursor:pointer}
.pre{display:flex;align-items:center;gap:8px;padding:10px 0}
.pre+.pre{border-top:1px solid var(--line)}
.pi{flex:1;min-width:0}
.pi small{margin-top:2px}
.tag{font-style:normal;font-size:.7rem;background:var(--okbg);color:var(--ok);padding:2px 8px;border-radius:99px;margin-left:8px}
.hint{color:var(--mute);font-size:.9rem;margin:10px 0}
.actions{display:flex;flex-wrap:wrap;gap:10px;margin-top:4px}
button{height:48px;border-radius:12px;border:1px solid var(--line);font:inherit;font-weight:600;cursor:pointer}
.primary{flex:1;background:var(--acc);color:var(--accink);border:0}
.ghost{background:transparent;color:var(--ink);padding:0 18px}
.sm{height:36px;padding:0 14px;border-radius:10px;background:var(--acc);color:var(--accink);border:0}
.sm.ghost{background:transparent;color:var(--ink);border:1px solid var(--line)}
.ok{background:var(--okbg);color:var(--ok);border-radius:12px;padding:12px 14px;margin:0 0 16px;font-weight:600}
.bad{background:var(--badbg);color:var(--bad);border-radius:12px;padding:12px 14px;margin:0 0 16px;font-weight:600}
.foot{color:var(--mute);font-size:.82rem;margin:18px 2px 0}
.grp.dim{opacity:.45}
.grid1{margin-top:6px}
.mt{display:grid;grid-template-columns:2.6rem 1fr 1fr 1fr;gap:6px;align-items:center;margin-top:8px}
.mt input{height:40px;padding:0 8px;min-width:0}
.mh{font-size:.72rem;color:var(--mute)}
.mc{font-weight:600}
.sr{display:grid;grid-template-columns:1fr 5.5rem 44px;gap:6px;margin-top:6px;align-items:center}
.sr input,.sr select{height:44px;padding:0 8px;min-width:0}
.sr .sm{height:44px;padding:0}
input[type=file]{height:auto;padding:9px 12px}
.prog{height:10px;background:var(--line);border-radius:99px;overflow:hidden;margin:4px 0 10px}
.prog div{height:100%;width:0;background:var(--acc);transition:width .2s}
.fwm{margin:0 0 10px}
</style></head><body><main>
)HTML";

const char PAGE_SCRIPT[] PROGMEM = R"JS(<script>
(function(){
 var f=document.forms.cfg;
 function $(i){return document.getElementById(i)}
 function sync(){var m=f.mode.value;
  $('gs').className='f grp'+(m==='0'?'':' dim');
  $('gm').className='f grp'+(m==='1'?'':' dim');
  $('gq').className='f grp'+(m==='2'?'':' dim');}
 var q=$('sq');
 function row(t,c){var d=document.createElement('div');d.className='sr';
  d.innerHTML='<input type=number class=st min=0 max=300000 step=any inputmode=decimal value="'+t+'">'+
   '<select class=sc><option value=1>CH1<option value=2>CH2<option value=3>CH3<option value=4>CH4</select>'+
   '<button type=button class="sm ghost" onclick="seqDel(this)">&times;</button>';
  d.querySelector('.sc').value=c;q.appendChild(d);}
 window.seqAdd=function(){var r=q.children,t=0,c=1;if(r.length>=50)return;
  if(r.length){var l=r[r.length-1];t=l.querySelector('.st').value;c=(parseInt(l.querySelector('.sc').value)||1)%4+1;}
  row(t,c);};
 window.seqDel=function(b){var r=b.parentNode;r.parentNode.removeChild(r);};
 f.addEventListener('submit',function(){var o=[],r=q.children,i,t;
  for(i=0;i<r.length;i++){t=parseFloat(r[i].querySelector('.st').value);
   if(!isNaN(t))o.push(t+':'+r[i].querySelector('.sc').value);}
  $('seqv').value=o.join(';');});
 f.mode.addEventListener('change',sync);
 $('sg').style.display='';$('sgh').style.display='';
 window.stagger=function(){
  var x=parseFloat($('stag').value)||0,act=[],i;
  for(i=1;i<=4;i++){if((parseInt(f['n'+i].value)||0)>0)act.push(i);}
  if(!act.length)return;
  var base=parseFloat(f['md'+act[0]].value)||0;
  for(i=0;i<act.length;i++){
   f['md'+act[i]].value=(base+i*x).toFixed(1);
   f['iv'+act[i]].value=(x*act.length).toFixed(1);}
 };
 // Firmware upload: sent with XHR so the page can show the upload progress.
 var w=$('fw'),bar=$('fwb'),btn=$('fwbtn');
 function msg(t,c){var e=$('fwm');e.textContent=t;e.className='fwm '+(c||'hint');e.hidden=false;}
 w.addEventListener('submit',function(ev){ev.preventDefault();
  var fi=w.firmware.files[0];if(!fi)return;
  if(/factory/i.test(fi.name)){msg('That is the factory image. Choose firmware.bin instead.','bad');return;}
  if(!confirm('Install '+fi.name+'? FlashBang restarts when it is done.'))return;
  var x=new XMLHttpRequest(),d=new FormData();d.append('firmware',fi,fi.name);
  $('fwp').hidden=false;bar.style.width='0';btn.disabled=true;msg('Uploading...');
  x.upload.onprogress=function(e){if(!e.lengthComputable)return;var p=Math.round(e.loaded*100/e.total);
   bar.style.width=p+'%';msg(p<100?'Uploading... '+p+' %':'Installing...');};
  x.onload=function(){if(x.status===200){bar.style.width='100%';
    msg('Installed. FlashBang is restarting in normal mode, so this WiFi network goes away.','ok');}
   else{msg('Update failed: '+x.responseText,'bad');btn.disabled=false;}};
  x.onerror=function(){msg('The connection was lost during the upload. Nothing was installed.','bad');btn.disabled=false;};
  x.open('POST','/update');x.send(d);});
})();
</script>
)JS";

String esc(const char* s) {
  String o;
  for (; *s; s++) {
    switch (*s) {
      case '&': o += F("&amp;"); break;
      case '<': o += F("&lt;"); break;
      case '>': o += F("&gt;"); break;
      case '"': o += F("&quot;"); break;
      default:  o += *s;
    }
  }
  return o;
}

String msStr(int us) {
  char b[16];
  if (us % 100) snprintf(b, sizeof b, "%.3f", us / 1000.0);
  else          snprintf(b, sizeof b, "%.1f", us / 1000.0);
  return String(b);
}

String summary(const Setup& s) {
  String o = MODE_NAME[s.mode];
  o += F(" | ");
  if (s.mode == MODE_SINGLE) {
    o += F("CH");
    bool first = true;
    for (int c = 0; c < NCH; c++) {
      if (!(s.chmask & (1 << c))) continue;
      if (!first) o += '+';
      o += String(c + 1);
      first = false;
    }
    if (first) o += F("none");
    o += F(" after ");
    o += msStr(s.single_us);
    o += F(" ms");
  } else if (s.mode == MODE_SEQ) {
    if (s.seq_n == 0) {
      o += F("no flashes");
    } else {
      o += String(s.seq_n);
      o += F(" flashes, ");
      o += msStr(s.seq[0].t_us);
      if (s.seq_n > 1) { o += F(" to "); o += msStr(s.seq[s.seq_n - 1].t_us); }
      o += F(" ms");
    }
  } else {
    bool any = false;
    for (int c = 0; c < NCH; c++) {
      if (s.count[c] <= 0) continue;
      if (any) o += F(", ");
      o += F("CH");
      o += String(c + 1);
      o += ' ';
      o += String(s.count[c]);
      o += F("x");
      if (s.count[c] >= 2) { o += F("/"); o += msStr(s.interval_us[c]); o += F("ms"); }
      o += F(" @");
      o += msStr(s.delay_us[c]);
      o += F("ms");
      any = true;
    }
    if (!any) o += F("no flashes");
  }
  o += F(" | ");
  o += TRIG_NAME[s.trig_src];
  if (trigMask(s.trig_src) & TRIGB_EXT) { o += F(" "); o += POL_NAME[s.ext_high]; }
  return o;
}

String opt(const char* v, const char* text, bool sel) {
  String s = F("<option value=");
  s += v;
  if (sel) s += F(" selected");
  s += '>';
  s += text;
  s += F("</option>");
  return s;
}

String num(const char* label, const char* name, const String& val, const char* unit,
           const String& hint, const char* attrs) {
  String s = F("<label class=f><span class=l>");
  s += label;
  s += F("</span><span class=in><input type=number name=");
  s += name;
  s += F(" value=\"");
  s += val;
  s += F("\" ");
  s += attrs;
  s += F(" required>");
  if (unit[0]) { s += F("<em>"); s += unit; s += F("</em>"); }
  s += F("</span>");
  if (hint.length()) { s += F("<small>"); s += hint; s += F("</small>"); }
  s += F("</label>");
  return s;
}

String cell(const String& label, const char* name, const String& val, const char* unit) {
  String s = F("<label class=g><span class=l>");
  s += label;
  s += F("</span><span class=in><input type=number name=");
  s += name;
  s += F(" value=\"");
  s += val;
  s += F("\" min=0 max=2000 step=any inputmode=decimal required><em>");
  s += unit;
  s += F("</em></span></label>");
  return s;
}

String boolSelect(const char* label, const char* name, bool v, const char* textTrue, const char* textFalse,
                  const char* hint) {
  String s = F("<label class=f><span class=l>");
  s += label;
  s += F("</span><span class=in><select name=");
  s += name;
  s += F(">");
  s += opt("1", textTrue, v);
  s += opt("0", textFalse, !v);
  s += F("</select></span><small>");
  s += hint;
  s += F("</small></label>");
  return s;
}

// Shutter times in 1/3-stop steps, for the PC sync hint.
const float SHUTTER_SEC[] = {1/250.0f, 1/200.0f, 1/160.0f, 1/125.0f, 1/100.0f, 1/80.0f, 1/60.0f, 1/50.0f, 1/40.0f,
                             1/30.0f, 1/25.0f, 1/20.0f, 1/15.0f, 1/13.0f, 1/10.0f, 1/8.0f, 1/6.0f, 1/5.0f, 1/4.0f,
                             0.3f, 0.4f, 0.5f, 0.6f, 0.8f, 1.0f, 1.3f, 1.6f, 2.0f, 2.5f, 3.2f, 4.0f, 5.0f, 6.0f,
                             8.0f, 10.0f, 13.0f, 15.0f, 20.0f, 25.0f, 30.0f};
const char* const SHUTTER_NAME[] = {"1/250 s", "1/200 s", "1/160 s", "1/125 s", "1/100 s", "1/80 s", "1/60 s", "1/50 s",
                                    "1/40 s", "1/30 s", "1/25 s", "1/20 s", "1/15 s", "1/13 s", "1/10 s", "1/8 s",
                                    "1/6 s", "1/5 s", "1/4 s", "0.3 s", "0.4 s", "0.5 s", "0.6 s", "0.8 s", "1 s",
                                    "1.3 s", "1.6 s", "2 s", "2.5 s", "3.2 s", "4 s", "5 s", "6 s", "8 s", "10 s",
                                    "13 s", "15 s", "20 s", "25 s", "30 s"};
const int NUM_SHUTTER = sizeof(SHUTTER_SEC) / sizeof(SHUTTER_SEC[0]);
static_assert(sizeof(SHUTTER_SEC) / sizeof(SHUTTER_SEC[0]) == sizeof(SHUTTER_NAME) / sizeof(SHUTTER_NAME[0]),
              "shutter tables must match");

// With the camera's sync contact as the trigger, the whole sequence has to fit in the time the shutter is
// fully open: the shutter time minus about 4 ms of curtain travel. We ask for the last flash plus 5 ms.
String shutterHint() {
  String s = F("<div class=f><span class=l>Camera shutter (PC sync)</span><small>");
  if (evN == 0) {
    s += F("No channel is enabled.");
  } else {
    float lastMs = evT[evN - 1] / 1000.0f;
    float needMs = lastMs + SYNC_MARGIN_MS;
    int k = -1;
    for (int i = 0; i < NUM_SHUTTER; i++) {
      if (SHUTTER_SEC[i] * 1000.0f >= needMs) { k = i; break; }
    }
    if (k >= 0) { s += SHUTTER_NAME[k]; s += F(" or slower. "); }
    else        { s += F("Bulb (more than 30 s). "); }
    s += F("The last flash fires ");
    s += msStr((int)evT[evN - 1]);
    s += F(" ms after the trigger, and the curtains need about 5 ms on top. Based on the saved setup.");
  }
  s += F("</small></div>");
  return s;
}

String buildPage(const String& saved, const String& err) {
  String s;
  s.reserve(16000);
  s += FPSTR(PAGE_HEAD);
  s += F("<h1>FlashBang V3.0</h1><p class=sub>Settings</p>");
  if (err == "full") s += F("<div class=bad>All 8 preset slots are in use. Delete one first.</div>");
  else if (saved.length()) s += F("<div class=ok>&#10003; Saved. Takes effect when you leave config mode.</div>");

  // --- Presets (own small forms, so they sit outside the main form) ---
  s += F("<section class=card><h2>Presets</h2>");
  int np = 0;
  for (int i = 0; i < MAX_PRESETS; i++) {
    if (!presets[i].used) continue;
    np++;
    s += F("<div class=pre><div class=pi><span class=l>");
    s += esc(presets[i].name);
    s += F("</span>");
    if (i == curPreset) s += F("<em class=tag>active</em>");
    s += F("<small>");
    s += summary(presets[i].s);
    s += F("</small></div><form method=post action=/preset_load><input type=hidden name=i value=");
    s += String(i);
    s += F("><button class=sm>Load</button></form>");
    s += F("<form method=post action=/preset_delete onsubmit=\"return confirm('Delete this preset?')\">"
           "<input type=hidden name=i value=");
    s += String(i);
    s += F("><button class=\"sm ghost\">Delete</button></form></div>");
  }
  if (!np) s += F("<p class=hint>No presets yet. Set up the flashes below, enter a name and press &ldquo;Save as preset&rdquo; in the Flash setup panel.</p>");
  s += F("</section>");

  s += F("<form method=post action=/save name=cfg>");

  // --- Flash setup ---
  s += F("<section class=card><h2>Flash setup</h2>");
  s += F("<label class=f><span class=l>Mode</span><span class=in><select name=mode>");
  s += opt("0", "Single: the enabled flashes fire together", cur.mode == MODE_SINGLE);
  s += opt("1", "Multi: count, interval and delay per channel", cur.mode == MODE_MULTI);
  s += opt("2", "Sequence: your own list of flashes", cur.mode == MODE_SEQ);
  s += F("</select></span><small>Single is one delay for the flashes you switch on. Multi sets how many times, how often and when each channel fires. "
         "Sequence is a free list of times and channels. Each mode keeps its own values.</small></label>");

  // Single mode: one delay, four on/off switches.
  s += F("<div class=\"f grp");
  if (cur.mode != MODE_SINGLE) s += F(" dim");
  s += F("\" id=gs><span class=l>Single mode</span>"
         "<div class=grid1><label class=g><span class=l>Delay</span><span class=in>"
         "<input type=number name=sdelay value=\"");
  s += msStr(cur.single_us);
  s += F("\" min=0 max=300000 step=any inputmode=decimal required><em>ms</em></span></label></div>"
         "<div class=chips>");
  for (int c = 0; c < NCH; c++) {
    s += F("<label><input type=checkbox name=ch");
    s += String(c + 1);
    s += F(" value=1");
    if (cur.chmask & (1 << c)) s += F(" checked");
    s += F("> ");
    s += String(c + 1);
    s += F("</label>");
  }
  s += F("</div><small>Time from the trigger to the flashes (up to 300 s, entered in ms), and which channels fire.</small></div>");

  // Multi mode: count, interval and delay for every channel.
  s += F("<div class=\"f grp");
  if (cur.mode != MODE_MULTI) s += F(" dim");
  s += F("\" id=gm><span class=l>Multi mode</span><div class=mt>"
         "<span></span><span class=mh>Count</span><span class=mh>Interval ms</span><span class=mh>Delay ms</span>");
  for (int c = 0; c < NCH; c++) {
    s += F("<span class=mc>CH");
    s += String(c + 1);
    s += F("</span><input type=number name=n");
    s += String(c + 1);
    s += F(" value=\"");
    s += String(cur.count[c]);
    s += F("\" min=0 max=99 step=1 inputmode=numeric required><input type=number name=iv");
    s += String(c + 1);
    s += F(" value=\"");
    s += msStr(cur.interval_us[c]);
    s += F("\" min=1 max=2000 step=any inputmode=decimal required><input type=number name=md");
    s += String(c + 1);
    s += F(" value=\"");
    s += msStr(cur.delay_us[c]);
    s += F("\" min=0 max=300000 step=any inputmode=decimal required>");
  }
  s += F("</div><small>Count 0 switches a channel off. Interval is the time between the flashes of one channel "
         "(50 ms is 20 Hz, up to 2 s). Delays can be up to 300 s (300000 ms). The total over all channels is limited to 100 flashes.</small>"
         "<div class=in id=sg style=\"display:none\"><input type=number id=stag value=50 min=0 step=any inputmode=decimal>"
         "<em>ms</em><button type=button class=\"sm ghost\" onclick=\"stagger()\">Stagger</button></div>"
         "<small id=sgh style=\"display:none\">Stagger spreads the channels that have a count over time, one after another, "
         "and sets their interval so the pattern repeats.</small></div>");

  // Sequence mode: a list of (time, channel) flashes.
  s += F("<div class=\"f grp");
  if (cur.mode != MODE_SEQ) s += F(" dim");
  s += F("\" id=gq><span class=l>Sequence</span><div id=sq>");
  String packed;
  for (int i = 0; i < cur.seq_n; i++) {
    s += F("<div class=sr><input type=number class=st min=0 max=300000 step=any inputmode=decimal value=\"");
    s += msStr(cur.seq[i].t_us);
    s += F("\"><select class=sc>");
    for (int c = 0; c < NCH; c++) {
      s += F("<option value=");
      s += String(c + 1);
      if (cur.seq[i].ch == c) s += F(" selected");
      s += F(">CH");
      s += String(c + 1);
      s += F("</option>");
    }
    s += F("</select><button type=button class=\"sm ghost\" onclick=\"seqDel(this)\">&times;</button></div>");
    if (i) packed += ';';
    packed += msStr(cur.seq[i].t_us);
    packed += ':';
    packed += String(cur.seq[i].ch + 1);
  }
  s += F("</div><input type=hidden id=seqv name=seq value=\"");
  s += packed;
  s += F("\"><div class=actions><button type=button class=\"sm ghost\" onclick=\"seqAdd()\">+ Add flash</button></div>"
         "<small>Time in ms after the trigger (up to 300 s = 300000 ms), and the channel. Up to 50 flashes; several "
         "flashes may share a time. The list is sorted by time when you save.</small></div>");

  s += F("<label class=f><span class=l>Trigger source</span><span class=in><select name=trig_src>");
  s += opt("0", "Sound module", cur.trig_src == 0);
  s += opt("1", "External input", cur.trig_src == 1);
  s += opt("2", "Sound + external", cur.trig_src == 2);
  s += opt("3", "PC sync (camera)", cur.trig_src == 3);
  s += opt("4", "Sound + PC sync", cur.trig_src == 4);
  s += opt("5", "External + PC sync", cur.trig_src == 5);
  s += opt("6", "All three", cur.trig_src == 6);
  s += F("</select></span><small>What starts the flash sequence. With several inputs, the first one wins. "
         "PC sync is the camera's sync contact on GPIO");
  s += String(PC_SYNC);
  s += F(": it closes when the shutter is fully open. Saved in presets.</small></label>");
  s += F("<label class=f><span class=l>External input level</span><span class=in><select name=ext_high>");
  s += opt("0", "Pulled LOW on event", cur.ext_high == 0);
  s += opt("1", "Pulled HIGH on event", cur.ext_high == 1);
  s += F("</select></span><small>GPIO");
  s += String(EXT_TRIG);
  s += F(", 3.3 V logic only. Low uses the internal pull-up, so a contact or open-collector output to GND works. "
         "High uses the internal pull-down.</small></label>");

  s += num("Holdoff after shot", "holdoff_ms", String(cfg.holdoff_ms), "ms",
           "Triggers are ignored after a shot (reloading, echoes).",
           "min=0 max=60000 step=100 inputmode=numeric");

  s += shutterHint();

  s += F("<label class=f><span class=l>Preset name</span><span class=in>"
         "<input type=text name=pname maxlength=14 placeholder=\"e.g. Flintlock\" autocomplete=off></span>"
         "<small>An existing name is overwritten.</small></label>"
         "<div class=actions><button class=ghost type=submit formaction=/preset_save>Save as preset</button></div>");
  s += F("</section>");

  // --- Battery ---
  s += F("<section class=card><h2>Battery</h2>");
  s += num("Calibration", "bat_cal", String(cfg.bat_cal, 3), "x",
           "Correction factor. Compare with a multimeter and adjust.",
           "min=0.5 max=1.5 step=0.001 inputmode=decimal");
  s += num("Capacity", "batt_mah", String(cfg.batt_mah), "mAh",
           "Rated capacity of your 18650.",
           "min=100 max=30000 step=50 inputmode=numeric");
  s += num("Average current", "avg_ma", String(cfg.avg_ma, 1), "mA",
           "Measured current draw of the circuit. Used for the remaining-time estimate.",
           "min=1 max=1000 step=0.5 inputmode=decimal");
  s += F("</section>");

  // --- System ---
  s += F("<section class=card><h2>System</h2>");
  s += F("<label class=f><span class=l>CPU frequency</span><span class=in><select name=cpu_mhz>");
  s += opt("80", "80 MHz (lowest power)", cfg.cpu_mhz == 80);
  s += opt("160", "160 MHz", cfg.cpu_mhz == 160);
  s += opt("240", "240 MHz", cfg.cpu_mhz == 240);
  s += F("</select></span><small>Lower saves power. WiFi needs at least 80 MHz.</small></label>");
  s += num("Display on-time", "disp_sec", String(cfg.disp_sec), "s",
           "How long the display stays lit after startup, a shot or a control being used. Always on in the menu and config mode.",
           "min=1 max=600 step=1 inputmode=numeric");
  s += F("</section>");

  s += F("<div class=actions><button class=primary type=submit>Save</button>"
         "<button class=ghost type=submit formaction=/reset formnovalidate "
         "onclick=\"return confirm('Restore default values?')\">Defaults</button></div>");
  s += F("</form>");

  // --- Firmware update (its own form: a file upload, not part of the settings) ---
  s += F("<section class=card><h2>Firmware update</h2>"
         "<form id=fw method=post action=/update enctype=multipart/form-data>"
         "<label class=f><span class=l>Firmware file</span><span class=in>"
         "<input type=file name=firmware accept=.bin required></span><small>Installed: ");
  s += FW_NAME;
  s += F(". Choose <b>firmware.bin</b> from <code>.pio/build/ttgo-t-energy/</code> (not firmware.factory.bin). "
         "Settings and presets are kept. FlashBang restarts when the update is installed.</small></label>"
         "<div class=prog id=fwp hidden><div id=fwb></div></div><p id=fwm hidden></p>"
         "<div class=actions><button class=primary id=fwbtn>Upload &amp; install</button></div></form></section>");

  s += F("<p class=foot>Hold the button to leave config mode. Changes take effect after that.</p></main>");
  s += FPSTR(PAGE_SCRIPT);
  s += F("</body></html>");
  return s;
}

void redirectTo(const char* where) {
  server.sendHeader("Location", where);
  server.send(303, "text/plain", "");
}

// Any URL we don't know (phones probe things like /generate_204 or /hotspot-detect.html)
// gets sent to the config page. An absolute address is what the OS checks expect.
void redirectToPortal() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
  server.sendHeader("Cache-Control", "no-store");
  server.send(302, "text/plain", "");
}

void argInt(const char* n, int& v)     { if (server.hasArg(n)) v = server.arg(n).toInt(); }
void argFloat(const char* n, float& v) { if (server.hasArg(n)) v = server.arg(n).toFloat(); }
int msToUs(const String& s) { return (int)lroundf(s.toFloat() * 1000.0f); }

// Preset names: plain printable ASCII, no markup characters, max 14 chars.
void sanitizeName(char* dst, String src) {
  src.trim();
  int n = 0;
  for (unsigned i = 0; i < src.length() && n < 14; i++) {
    char c = src[i];
    if (c >= 32 && c < 127 && c != '<' && c != '>' && c != '&' && c != '"' && c != '\'') dst[n++] = c;
  }
  dst[n] = 0;
}

// Takes every field of the settings form and stores it.
void applyForm() {
  Setup old = cur;

  if (server.hasArg("mode")) {
    cur.mode = server.arg("mode").toInt();
    cur.chmask = 0;
    if (server.hasArg("sdelay")) cur.single_us = msToUs(server.arg("sdelay"));
    for (int c = 0; c < NCH; c++) {
      char k[8];
      snprintf(k, sizeof k, "ch%d", c + 1);
      if (server.hasArg(k)) cur.chmask |= (1 << c);        // unchecked boxes are not sent
      snprintf(k, sizeof k, "n%d", c + 1);
      if (server.hasArg(k)) cur.count[c] = server.arg(k).toInt();
      snprintf(k, sizeof k, "iv%d", c + 1);
      if (server.hasArg(k)) cur.interval_us[c] = msToUs(server.arg(k));
      snprintf(k, sizeof k, "md%d", c + 1);
      if (server.hasArg(k)) cur.delay_us[c] = msToUs(server.arg(k));
    }
    argInt("trig_src", cur.trig_src);
    argInt("ext_high", cur.ext_high);
    if (server.hasArg("seq")) {                              // "ms:channel;ms:channel;..." packed by the page script
      String q = server.arg("seq");
      int n = 0, pos = 0, len = (int)q.length();
      while (pos < len && n < MAX_SEQ) {
        int end = q.indexOf(';', pos);
        if (end < 0) end = len;
        String item = q.substring(pos, end);
        int colon = item.indexOf(':');
        if (colon > 0) {
          cur.seq[n].t_us = msToUs(item.substring(0, colon));
          cur.seq[n].ch = item.substring(colon + 1).toInt() - 1;
          n++;
        }
        pos = end + 1;
      }
      cur.seq_n = n;
    }
  }

  argInt("holdoff_ms", cfg.holdoff_ms);
  argInt("cpu_mhz", cfg.cpu_mhz);
  argFloat("bat_cal", cfg.bat_cal);
  argInt("batt_mah", cfg.batt_mah);
  argFloat("avg_ma", cfg.avg_ma);
  argInt("disp_sec", cfg.disp_sec);

  clampSetup(cur);
  seqSort(cur, -1);
  clampConfig();
  if (memcmp(&old, &cur, sizeof(Setup)) != 0) curPreset = -1;
  buildSchedule();
  saveConfig();
  saveSetup();
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html; charset=utf-8", buildPage(server.arg("saved"), server.arg("err")));
}

void handleSave() {
  applyForm();
  redirectTo("/?saved=1");
}

void handlePresetSave() {
  applyForm();
  char name[16];
  sanitizeName(name, server.arg("pname"));

  int slot = -1;
  for (int i = 0; i < MAX_PRESETS; i++) {
    if (presets[i].used && name[0] && strcasecmp(presets[i].name, name) == 0) { slot = i; break; }
  }
  if (slot < 0) {
    for (int i = 0; i < MAX_PRESETS; i++) if (!presets[i].used) { slot = i; break; }
  }
  if (slot < 0) { redirectTo("/?err=full"); return; }

  if (name[0] == 0) snprintf(name, sizeof name, "Preset %d", slot + 1);
  memset(&presets[slot], 0, sizeof(Preset));
  snprintf(presets[slot].name, sizeof presets[slot].name, "%s", name);
  presets[slot].s = cur;
  presets[slot].used = true;
  curPreset = slot;
  savePreset(slot);
  saveSetup();
  redirectTo("/?saved=1");
}

void handlePresetLoad() {
  int i = server.arg("i").toInt();
  if (i >= 0 && i < MAX_PRESETS && presets[i].used) loadPreset(i);
  redirectTo("/?saved=1");
}

void handlePresetDelete() {
  int i = server.arg("i").toInt();
  if (i >= 0 && i < MAX_PRESETS && presets[i].used) {
    memset(&presets[i], 0, sizeof(Preset));
    if (curPreset == i) curPreset = -1;
    savePreset(i);
    saveSetup();
  }
  redirectTo("/?saved=1");
}

void handleReset() {
  cfg = DEF;
  cur = DEF_SETUP;
  curPreset = -1;
  buildSchedule();
  saveConfig();
  saveSetup();
  redirectTo("/?saved=1");
}

// ================= firmware upload =================
// The upload handler is called for each chunk of the file and writes it straight into the free
// OTA partition. When the request is complete, handleUpdateDone() answers and restarts.
// Update checks the image (magic byte, checksum), so a wrong file is rejected, and the running
// firmware stays in place until the new one has been written and checked completely.

static String updErr;                            // empty = no error so far

void handleUpdateUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    updErr = "";
    showUpdateStart();
    if (up.filename.indexOf("factory") >= 0) updErr = "use firmware.bin, not the factory image";
    else if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) updErr = Update.errorString();
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (updErr.length()) return;                 // already failed: let the rest of the upload pass
    if (Update.write(up.buf, up.currentSize) != up.currentSize) {
      updErr = Update.errorString();
      Update.abort();
      return;
    }
    // The request length includes a few hundred bytes of form overhead, so this is close but
    // never quite 100 % until the end.
    int total = server.clientContentLength();
    if (total > 0) showUpdateProgress((int)((uint64_t)up.totalSize * 100 / total));
  } else if (up.status == UPLOAD_FILE_END) {
    if (updErr.length()) return;
    showUpdateProgress(100);
    if (!Update.end(true)) updErr = Update.errorString();
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    updErr = "upload aborted";
  }
}

void handleUpdateDone() {
  server.sendHeader("Connection", "close");
  if (updErr.length() || !Update.isFinished()) {
    if (!updErr.length()) updErr = "no firmware received";
    server.send(500, "text/plain", updErr);
    showUpdateFailed(updErr.c_str());
    delay(2500);
    showConfig();                                // still in config mode, nothing changed
    return;
  }
  server.send(200, "text/plain", "OK");
  showUpdateDone();
  saveAllIfDirty();
  delay(1000);                                   // let the answer reach the browser
  ESP.restart();
}


// Registers the web pages. Called once from setup().
void webSetupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/preset_save", HTTP_POST, handlePresetSave);
  server.on("/preset_load", HTTP_POST, handlePresetLoad);
  server.on("/preset_delete", HTTP_POST, handlePresetDelete);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  server.onNotFound(redirectToPortal);
}
