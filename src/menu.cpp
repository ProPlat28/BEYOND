// The screens: main menu, worlds, controls, dialogs, loading and pause.
#include "game.h"

// ================================================================ CONTROLS SCREEN (shared by the main menu and the pause menu)
static void uiSliderDraw(float x, float y, float w, float h, float f, bool hot) {
  float u = uiU, tr = 6 * u, cy = y + h * .5f, kn = 18 * u; uiBox(x, cy - tr * .5f, w, tr, tr * .5f, {1, 1, 1, .14f}, {1, 1, 1, .14f}); uiBox(x, cy - tr * .5f, std::max(tr, w * f), tr, tr * .5f, {1, .82f, .46f, 1}, {1, .6f, .3f, 1});
  uiBox(x + w * f - kn * .5f, cy - kn * .5f, kn, kn, kn * .5f, {1, 1, 1, 1}, {.78f, .84f, .94f, 1}, {1, .82f, .46f, hot ? 1.f : .6f}, std::max(1.f, 1.4f * u), {1, .7f, .35f, hot ? .45f : .15f}, 7 * u);
}
static void uiSlider(int id, float x, float y, float w, float h, float& v, float lo, float hi, float step) {
  if (uiInput) uiRects.push_back({x, y, w, h, id, true}); if (uiInput && padNudge && uiOver(x - 8 * uiU, y, w + 16 * uiU, h)) { v = clampf(v + padNudge * step * std::max(1.f, std::round((hi - lo) / step / 20)), lo, hi); saveSettings(); }
  if (uiOver(x - 8 * uiU, y, w + 16 * uiU, h) && uiPressed) uiActive = id; bool drag = uiActive == id;
  if (drag) { float f = clampf((uiMx - x) / std::max(1.f, w), 0, 1); v = clampf(std::round((lo + (hi - lo) * f) / step) * step, lo, hi); if (uiReleased) saveSettings(); }
  uiSliderDraw(x, y, w, h, (v - lo) / (hi - lo), drag || uiOver(x - 8 * uiU, y, w + 16 * uiU, h));
}
static void captureDialog() {
  float u = uiU, w = 540 * u, h = 232 * u, x = (FW - w) * .5f, y = (FH - h) * .5f, pad = 28 * u, bw = (w - pad * 2 - 12 * u) * .5f, bh = 46 * u, by = y + h - pad - bh; int a = captureAct, s = captureSlot;
  { Col dim{.005f, .008f, .02f, .66f}; uiBox(0, 0, (float)FW, (float)FH, 0, dim, dim); } uiBox(x, y, w, h, 14 * u, {.05f, .075f, .15f, .99f}, {.022f, .032f, .075f, .99f}, {1, .82f, .46f, .55f}, std::max(1.f, 1.2f * u), {1, .7f, .35f, .16f}, 26 * u);
  if (capturePad) {   // controller button: the press is read in pollGamepad
    uiTextC("PRESS A CONTROLLER BUTTON", x + w * .5f, y + 30 * u, uiLarge(), UI_GOLD); uiTextC(ACT_NAME[a], x + w * .5f, y + 74 * u, uiS, UI_TXT); uiTextC(fmt("Now: %s   -   %s or Esc cancels", padName(padBind[a][captureSlot]).c_str(), padName(GLFW_GAMEPAD_BUTTON_START).c_str()), x + w * .5f, y + 104 * u, uiSmall(), UI_DIM);
    if (uiButton(262, x + pad, by, bw, bh, "NO BUTTON", BTN_NORMAL, padBind[a][s] >= 0)) { padBind[a][s] = -1; captureAct = -1; capturePad = false; saveSettings(); }
    if (uiButton(263, x + pad + bw + 12 * u, by, bw, bh, "CANCEL")) { captureAct = -1; capturePad = false; } return; }
  uiTextC("PRESS A KEY", x + w * .5f, y + 30 * u, uiLarge(), UI_GOLD); uiTextC(ACT_NAME[a], x + w * .5f, y + 74 * u, uiS, UI_TXT); uiTextC(fmt("Now: %s   -   Esc cancels", keyName(keyBind[a][s]).c_str()), x + w * .5f, y + 104 * u, uiSmall(), UI_DIM);
  // an action can only lose its last key by giving it to another action, never by CLEAR
  if (uiButton(260, x + pad, by, bw, bh, "CLEAR THIS KEY", BTN_NORMAL, keyBind[a][s] != 0 && keyBind[a][1 - s] != 0)) { keyBind[a][s] = 0; captureAct = -1; saveSettings(); }
  if (uiButton(261, x + pad + bw + 12 * u, by, bw, bh, "CANCEL")) captureAct = -1;
}
// SETTINGS (main menu and pause menu): sound and music volume, fullscreen, and the controls. Returns 1 for BACK, 2 to open CONTROLS.
static int settingsPanel() {
  float u = uiU, pw = std::min(FW - 48 * u, 560 * u), ph = 380 * u, px = (FW - pw) * .5f, py = (FH - ph) * .5f, pad = 26 * u, rowH = 58 * u, bh = 40 * u, sx = px + pad + 170 * u, sw = pw - pad * 2 - 170 * u - 70 * u; int r = 0;
  uiPanel(px, py, pw, ph); uiText("SETTINGS", px + pad, py + 22 * u, uiLarge(), UI_TXT); float y = py + 78 * u;
  auto vol = [&](int id, const char* name, float& v) { uiText(name, px + pad, y + (bh - 7 * uiS) * .5f, uiS, UI_TXT); float pct = std::round(v * 100); uiSlider(id, sx, y, sw, bh, pct, 0, 100, 1); v = pct / 100.f;
    std::string t = fmt("%d%%", (int)pct); uiText(t, px + pw - pad - textW(t, uiS), y + (bh - 7 * uiS) * .5f, uiS, UI_GOLD); y += rowH; };
  vol(290, "SOUND", sfxVol); vol(291, "MUSIC", musVol);
  uiText("FULLSCREEN", px + pad, y + (bh - 7 * uiS) * .5f, uiS, UI_TXT); if (uiButton(292, sx, y, pw - pad - sx + px, bh, fs ? "ON" : "OFF")) { toggleFS(); } y += rowH;
  if (uiButton(293, px + pad, y + 6 * u, pw - pad * 2, bh, "CONTROLS")) r = 2;
  float fy = py + ph - 58 * u; if (uiButton(294, px + (pw - 160 * u) * .5f, fy, 160 * u, 40 * u, "BACK")) r = 1;   // centred
  return r;
}
// returns true when BACK is clicked. Two tabs: KEYBOARD (two keys per action, mouse options) and CONTROLLER (one button per action, stick options)
static bool controlsPanel() {
  static int tab = 0;
  float u = uiU, pw = std::min(FW - 48 * u, 620 * u), ph = std::min(FH - 20 * u, 508 * u), px = (FW - pw) * .5f, py = (FH - ph) * .5f, pad = 22 * u, slotW = 128 * u, gap = 8 * u, s2 = px + pw - pad - slotW, s1 = s2 - gap - slotW, rowH = 25 * u, bh = 22 * u; bool base = uiInput, back = false, anyMissing = false; uiInput = base && captureAct < 0;
  uiPanel(px, py, pw, ph); uiText("CONTROLS", px + pad, py + 22 * u, uiLarge(), UI_TXT);
  if (captureAct < 0 && uiTabStep(tab, 2, 270)) ctrlMsg.clear();
  { float tw = 130 * u; static const char* TN[2] = {"KEYBOARD", "CONTROLLER"}; for (int i = 0; i < 2; i++) if (uiButton(270 + i, px + pw - pad - (2 - i) * (tw + 8 * u) + 8 * u, py + 16 * u, tw, 30 * u, TN[i], i == tab ? BTN_PRIMARY : BTN_NORMAL)) { tab = i; ctrlMsg.clear(); } }
  float y = py + 58 * u; auto label = [&](const std::string& t, V3 c) { float ts = uiS, lw = s1 - px - pad * 2; while (ts > 1 && textW(t, ts) > lw) ts -= 1; uiText(fitText(t, lw, ts), px + pad, y + (bh - 7 * ts) * .5f, ts, c); };
  if (tab == 0) {
    uiTextC("KEY 1", s1 + slotW * .5f, y, uiSmall(), UI_DIM); uiTextC("KEY 2", s2 + slotW * .5f, y, uiSmall(), UI_DIM); y += 20 * u;
    for (int a = 0; a < ACT_COUNT; a++, y += rowH) { bool none = !keyBind[a][0] && !keyBind[a][1]; anyMissing = anyMissing || none; label(ACT_NAME[a], none ? UI_RED : UI_TXT);
      for (int s = 0; s < 2; s++) if (uiButton(200 + a * 2 + s, s ? s2 : s1, y, slotW, bh, keyName(keyBind[a][s]))) { captureAct = a; captureSlot = s; capturePad = false; ctrlMsg.clear(); } }
    y += 4 * u; label("Mouse sensitivity", UI_TXT); std::string pct = fmt("%d%%", (int)std::round(mouseSens * 100)); uiText(pct, s2 + slotW - textW(pct, uiS), y + (bh - 7 * uiS) * .5f, uiS, UI_TXT);
    uiSlider(250, s1 + 8 * u, y, slotW * 2 + gap - 84 * u, bh, mouseSens, .2f, 3.f, .05f); y += rowH;
    label("Invert mouse up/down", UI_TXT); if (uiButton(251, s1, y, slotW * 2 + gap, bh, mouseInvert ? "ON" : "OFF")) { mouseInvert = !mouseInvert; saveSettings(); } y += rowH;
    label("Invert mouse left/right", UI_TXT); if (uiButton(254, s1, y, slotW * 2 + gap, bh, mouseInvertX ? "ON" : "OFF")) { mouseInvertX = !mouseInvertX; saveSettings(); } y += rowH;
    if (anyMissing || !ctrlMsg.empty()) uiTextFitC(anyMissing ? "An action has no key - give it one to use it" : ctrlMsg, px + pw * .5f, y + 2 * u, pw - pad * 2, uiSmall(), anyMissing ? UI_RED : UI_GOLD);
  } else {
    std::string st = padConnected ? "Connected: " + padLabel : "No controller detected - plug one in"; uiTextFit(st, px + pad, y, s1 - px - pad * 2, uiSmall(), padConnected ? V3{.6f, 1, .7f} : UI_GOLD);   // same rows as the keyboard tab
    uiTextC("KEY 1", s1 + slotW * .5f, y, uiSmall(), UI_DIM); uiTextC("KEY 2", s2 + slotW * .5f, y, uiSmall(), UI_DIM); y += 20 * u;
    for (int a = 0; a < ACT_COUNT; a++, y += rowH) { bool mv = a <= ACT_RIGHT; label(ACT_NAME[a], padBind[a][0] < 0 && padBind[a][1] < 0 && !mv ? UI_DIM : UI_TXT);
      for (int s = 0; s < 2; s++) if (uiButton(280 + a * 2 + s, s ? s2 : s1, y, slotW, bh, padBind[a][s] >= 0 ? padName(padBind[a][s]) : "-")) { captureAct = a; captureSlot = s; capturePad = true; ctrlMsg.clear(); } }
    y += 4 * u; label("Look sensitivity", UI_TXT); std::string pct = fmt("%d%%", (int)std::round(padSens * 100)); uiText(pct, s2 + slotW - textW(pct, uiS), y + (bh - 7 * uiS) * .5f, uiS, UI_TXT);
    uiSlider(255, s1 + 8 * u, y, slotW * 2 + gap - 84 * u, bh, padSens, .2f, 3.f, .05f); y += rowH;
    label("Invert look up/down", UI_TXT); if (uiButton(256, s1, y, slotW * 2 + gap, bh, padInvert ? "ON" : "OFF")) { padInvert = !padInvert; saveSettings(); } y += rowH;
    label("Invert look left/right", UI_TXT); if (uiButton(257, s1, y, slotW * 2 + gap, bh, padInvertX ? "ON" : "OFF")) { padInvertX = !padInvertX; saveSettings(); } y += rowH;
    if (!ctrlMsg.empty()) uiTextFitC(ctrlMsg, px + pw * .5f, y + 2 * u, pw - pad * 2, uiSmall(), UI_GOLD);
  }
  float fy = py + ph - 46 * u, fh = 34 * u; if (uiButton(252, px + pad, fy, 130 * u, fh, "BACK")) back = true; if (uiButton(253, px + pw - pad - 230 * u, fy, 230 * u, fh, "RESET TO DEFAULTS")) { resetControls(); ctrlMsg = "Controls reset to defaults"; saveSettings(); }
  if (captureAct >= 0) { uiInput = base; captureDialog(); } uiInput = base; if (back) { captureAct = -1; capturePad = false; ctrlMsg.clear(); } return back;
}
// ================================================================ MAIN MENU (home, world list, dialogs)
static void openDialog(Dialog d, int world, const std::string& txt) { dlgDiff = 1; dlg = d; dlgWorld = world; dlgText = txt; dlgTime = 0; menuMsg.clear(); }
void goScreen(MenuScreen s) { menuScreen = s; menuFade = 0; menuMsg.clear(); worldScroll = 0; captureAct = -1; ctrlMsg.clear(); }
static void dialogConfirm() {
  std::string name = cleanName(dlgText); if (dlg == DLG_NEW || dlg == DLG_RENAME) { if (name.empty()) return; }
  if (dlg == DLG_NEW) { int id = createWorld(name, dlgDiff); if (id >= 0) { scanWorlds(); pendingOpen = id; } else menuMsg = "Could not create the world - is the disk full or read-only?"; }
  else if (dlg == DLG_RENAME) { if (!renameWorld(dlgWorld, name)) menuMsg = "Could not rename the world."; }
  else if (dlg == DLG_DELETE) { if (!deleteWorld(dlgWorld)) menuMsg = "Could not fully delete the world's folder."; }
  dlg = DLG_NONE; audioEvent(S_CLICK);
}
void menuKey(int key, int act) {
  if (dlg == DLG_NONE && key == GLFW_KEY_ESCAPE && act == GLFW_PRESS && captureAct < 0) { if (menuScreen == SC_CONTROLS) goScreen(SC_SETTINGS); else if (menuScreen != SC_HOME) { saveSettings(); goScreen(SC_HOME); } return; }   // Esc goes back one screen
  if (dlg == DLG_NONE || act == GLFW_RELEASE) return;
  if (key == GLFW_KEY_ESCAPE && act == GLFW_PRESS) dlg = DLG_NONE;
  if (dlg != DLG_NEW && dlg != DLG_RENAME) return; // Enter never confirms a delete
  if (key == GLFW_KEY_BACKSPACE && !dlgText.empty()) dlgText.pop_back();
  if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && act == GLFW_PRESS) dialogConfirm();
}
void menuChar(unsigned cp) {
  if (dlg != DLG_NEW && dlg != DLG_RENAME) return;
  // the font is ASCII only, so accented letters typed on a French keyboard are stored without the accent
  char c = cp >= 32 && cp < 127 ? (char)cp : cp >= 0xC0 && cp <= 0xFF ? LATIN_FOLD[cp - 0xC0] : 0; if (c && dlgText.size() < WORLD_NAME_LEN) dlgText += c;
}
// opens a fixed web page in the default browser (the address is never built from user input)
static void openUrl(const char* url) {
#ifdef _WIN32
  std::string c = std::string("start \"\" \"") + url + "\""; 
#elif defined(__APPLE__)
  std::string c = std::string("open '") + url + "' &";
#else
  std::string c = std::string("xdg-open '") + url + "' >/dev/null 2>&1 &";
#endif
  if (std::system(c.c_str()) != 0) {}
}
static void menuHome() {
  float u = uiU, x = std::max(60 * u, FW * .075f), y = 104 * u, bw = 340 * u, bh = 46 * u, gap = 11 * u; uiLogo(x, y, 62 * u, uiA);
  { // top left: version, and the credit, which opens the source code in the browser
    float tx = 18 * u, ty = 16 * u, ts = uiSmall(); uiText("Version 1.0.0", tx, ty, ts, UI_DIM, uiA * .85f); ty += 7 * ts + 8 * u;
    const std::string cr = "Made by ProPlat28"; float cw = textW(cr, ts), ch = 7 * ts;
    if (uiInput) uiRects.push_back({tx - 4 * u, ty - 4 * u, cw + 8 * u, ch + 8 * u, 9, false});   // the controller can select it too (D-pad / stick, then A)
    bool hov = uiOver(tx - 4 * u, ty - 4 * u, cw + 8 * u, ch + 8 * u);
    uiText(cr, tx, ty, ts, hov ? UI_GOLD : UI_TXT, uiA * (hov ? 1.f : .85f)); if (hov) uiBox(tx, ty + ch + 2 * u, cw, std::max(1.f, u), 0, {1, .82f, .46f, 1}, {1, .82f, .46f, 1});
    if (hov && uiPressed) { openUrl("https://github.com/ProPlat28/BEYOND"); } }
  y += 62 * u + 22 * u; uiText("Build the rocket. Leave Earth. Discover an universe.", x + 2 * u, y, uiS, {.84f, .90f, 1}); y += 54 * u;
  if (!worlds.empty()) { if (uiButton(1, x, y, bw, bh, "CONTINUE  -  " + worlds[0].name, BTN_PRIMARY)) pendingOpen = worlds[0].id; }
  else if (uiButton(1, x, y, bw, bh, "CREATE YOUR FIRST WORLD", BTN_PRIMARY)) openDialog(DLG_NEW, -1, defaultWorldName());
  y += bh + gap; if (uiButton(2, x, y, bw, bh, worlds.empty() ? "WORLDS" : fmt("WORLDS  (%d)", (int)worlds.size()))) goScreen(SC_WORLDS);
  y += bh + gap; if (uiButton(6, x, y, bw, bh, "SETTINGS")) goScreen(SC_SETTINGS);
  y += bh + gap; if (uiButton(5, x, y, bw, bh, "QUIT")) dlg = DLG_QUIT;   // asks first
}
static void menuWorlds() {
  float u = uiU, pw = std::min(FW - 48 * u, 1000 * u), ph = 590 * u, px = (FW - pw) * .5f, py = (FH - ph) * .5f, pad = 26 * u; uiPanel(px, py, pw, ph);
  uiText("YOUR WORLDS", px + pad, py + 26 * u, uiS * 2, UI_TXT);
  std::string count = worlds.size() == 1 ? "1 world" : fmt("%d worlds", (int)worlds.size()); uiText(count, px + pw - pad - textW(count, uiS), py + 26 * u + 4 * uiS, uiS, UI_DIM);
  float ly = py + 74 * u, rowH = 86 * u, listH = ph - 74 * u - 86 * u; int vis = std::max(1, (int)(listH / rowH)), n = (int)worlds.size(), maxScroll = std::max(0, n - vis);
  if (uiInput) { int steps = (int)uiWheel; worldScroll -= steps; uiWheel -= (float)steps; } worldScroll = std::min(std::max(worldScroll, 0), maxScroll);
  bool bar = n > vis; float sbW = bar ? 16 * u : 0, rowW = pw - pad * 2 - sbW;
  if (n == 0) { uiTextC("No worlds yet.", px + pw * .5f, ly + listH * .36f, uiLarge(), UI_TXT); uiTextC("Create one below and give it a name - then start building your rocket.", px + pw * .5f, ly + listH * .36f + 28 * u, uiS, UI_DIM); }
  for (int i = worldScroll; i < std::min(n, worldScroll + vis); i++) {
    const World& w = worlds[i]; float ry = ly + (i - worldScroll) * rowH, rh = rowH - 8 * u, bh = 38 * u, by = ry + (rh - bh) * .5f; int base = 100 + w.id * 4;
    bool over = uiOver(px + pad, ry, rowW, rh); uiBox(px + pad, ry, rowW, rh, 9 * u, over ? Col{.13f, .19f, .32f, .72f} : Col{.075f, .105f, .185f, .55f}, over ? Col{.08f, .12f, .21f, .72f} : Col{.04f, .055f, .105f, .55f}, {.5f, .72f, 1, over ? .36f : .12f}, std::max(1.f, u));
    // a little planet for each world, coloured by its universe seed
    float ic = 40 * u, ix = px + pad + 16 * u, iy = ry + (rh - ic) * .5f; C4 a = hsl(.6f + w.seed * .0618f, .62f, .62f), b = hsl(.62f + w.seed * .0618f, .7f, .24f);
    uiBox(ix, iy, ic, ic, ic * .5f, {a[0] / 255, a[1] / 255, a[2] / 255, 1}, {b[0] / 255, b[1] / 255, b[2] / 255, 1}, {1, 1, 1, .18f}, std::max(1.f, u), {a[0] / 255, a[1] / 255, a[2] / 255, .3f}, 7 * u);
    float btnPlay = 80 * u, btnRen = 100 * u, btnCopy = 80 * u, btnDel = 92 * u, g = 8 * u, bx = px + pad + rowW - 14 * u - btnDel, tx = ix + ic + 16 * u, tw = bx - g * 3 - btnRen - btnCopy - btnPlay - tx - 14 * u;
    float nameS = uiLarge(), infoS = uiSmall(), blockH = 7 * nameS + 9 * u + 7 * infoS, ty = ry + (rh - blockH) * .5f;
    uiTextFit(w.name, tx, ty, tw, nameS, UI_TXT); uiTextFit(worldInfo(w), tx, ty + 7 * nameS + 9 * u, tw, infoS, UI_DIM);
    if (uiButton(base + 2, bx, by, btnDel, bh, "DELETE", BTN_DANGER)) openDialog(DLG_DELETE, w.id, w.name); bx -= g + btnRen;
    if (uiButton(base + 1, bx, by, btnRen, bh, "RENAME")) openDialog(DLG_RENAME, w.id, w.name); bx -= g + btnCopy;
    if (uiButton(base + 3, bx, by, btnCopy, bh, "COPY")) { if (copyWorld(w.id) >= 0) { worldScroll = 0; audioEvent(S_CHIME); menuMsg.clear(); } else menuMsg = "Could not copy the world."; break; } bx -= g + btnPlay;
    if (uiButton(base, bx, by, btnPlay, bh, "PLAY", BTN_PRIMARY)) pendingOpen = w.id;
  }
  if (bar) { // scrollbar: drag the thumb, click the track, or use the mouse wheel
    float tx = px + pad + rowW + 6 * u, th = vis * rowH - 8 * u, thumb = std::max(30 * u, th * vis / n), ty = ly + (th - thumb) * worldScroll / std::max(1, maxScroll), tw = 8 * u; const int SB = 90;
    if (uiOver(tx - 4 * u, ly, tw + 8 * u, th) && uiPressed) uiActive = SB;
    if (uiActive == SB && uiDown) { float f = clampf((uiMy - ly - thumb * .5f) / std::max(1.f, th - thumb), 0, 1); worldScroll = (int)std::round(f * maxScroll); ty = ly + (th - thumb) * worldScroll / std::max(1, maxScroll); }
    uiBox(tx, ly, tw, th, 4 * u, {1, 1, 1, .06f}, {1, 1, 1, .06f}); float ta = uiActive == SB ? .85f : uiOver(tx - 4 * u, ly, tw + 8 * u, th) ? .62f : .40f; uiBox(tx, ty, tw, thumb, 4 * u, {.6f, .8f, 1, ta}, {.45f, .66f, 1, ta});
  }
  float fy = py + ph - 64 * u, fh = 44 * u; if (uiButton(10, px + pad, fy, 130 * u, fh, "BACK")) goScreen(SC_HOME);
  if (uiButton(11, px + pw - pad - 230 * u, fy, 230 * u, fh, "+  NEW WORLD", BTN_PRIMARY)) openDialog(DLG_NEW, -1, defaultWorldName());
  if (!menuMsg.empty()) uiTextC(menuMsg, px + pw * .5f, fy + (fh - 7 * uiS) * .5f, uiS, UI_RED);
}
static void menuDialog() {
  float u = uiU, w = 560 * u, h = dlg == DLG_QUIT ? 210 * u : dlg == DLG_DELETE ? 236 * u : dlg == DLG_NEW ? 392 * u : 256 * u, x = (FW - w) * .5f, y = (FH - h) * .5f, pad = 28 * u, bw = (w - pad * 2 - 12 * u) * .5f, bh = 46 * u, by = y + h - pad - bh;
  float d0 = uiA; uiA = 1; { Col dim{.005f, .008f, .02f, .66f}; uiBox(0, 0, (float)FW, (float)FH, 0, dim, dim); } uiA = d0; uiBox(x, y, w, h, 14 * u, {.05f, .075f, .15f, .99f}, {.022f, .032f, .075f, .99f}, dlg == DLG_DELETE ? Col{1, .42f, .38f, .55f} : Col{.5f, .72f, 1, .5f}, std::max(1.f, 1.2f * u), dlg == DLG_DELETE ? Col{1, .25f, .22f, .16f} : Col{.25f, .5f, 1, .16f}, 26 * u);
  if (dlg == DLG_QUIT) {
    uiTextC("QUIT BEYOND?", x + w * .5f, y + 34 * u, uiLarge(), UI_GOLD); uiTextC("Your worlds are saved.", x + w * .5f, y + 84 * u, uiS, UI_DIM);
    if (uiButton(34, x + pad, by, bw, bh, "CANCEL")) dlg = DLG_NONE; if (uiButton(35, x + pad + bw + 12 * u, by, bw, bh, "QUIT", BTN_PRIMARY)) { dlg = DLG_NONE; glfwSetWindowShouldClose(win, 1); } return;
  }
  if (dlg == DLG_DELETE) {
    uiTextC("DELETE THIS WORLD?", x + w * .5f, y + 32 * u, uiLarge(), UI_RED); uiTextFitC("\"" + dlgText + "\"", x + w * .5f, y + 74 * u, w - pad * 2, uiLarge(), UI_TXT);
    uiTextC("All of its progress will be erased for good.", x + w * .5f, y + 122 * u, uiS, UI_DIM);
    if (uiButton(30, x + pad, by, bw, bh, "CANCEL")) dlg = DLG_NONE; if (uiButton(31, x + pad + bw + 12 * u, by, bw, bh, "DELETE WORLD", BTN_DANGER_STRONG)) dialogConfirm(); return;
  }
  dlgTime += uiDt; uiTextC(dlg == DLG_NEW ? "NEW WORLD" : "RENAME WORLD", x + w * .5f, y + 28 * u, uiLarge(), UI_GOLD); uiText("World name", x + pad, y + 70 * u, uiS, UI_DIM);
  float fy = y + 96 * u, fh = 54 * u, fw = w - pad * 2; uiBox(x + pad, fy, fw, fh, 8 * u, {.012f, .018f, .045f, 1}, {.03f, .045f, .09f, 1}, {.55f, .78f, 1, .75f}, std::max(1.f, 1.3f * u), {.3f, .6f, 1, .22f}, 8 * u);
  float ts = uiLarge(); if (textW(dlgText, ts) > fw - 44 * u) ts = uiS; float tx = x + pad + 14 * u, ty = fy + (fh - 7 * ts) * .5f; uiText(dlgText, tx, ty, ts, UI_TXT); if (std::fmod(dlgTime, 1.f) < .55f) uiBox(std::floor(tx + textW(dlgText, ts) + 2 * u), ty - 2 * u, std::max(2.f, 2 * u), 7 * ts + 4 * u, 0, {1, .82f, .46f, 1}, {1, .82f, .46f, 1});
  std::string left = fmt("%d/%d", (int)dlgText.size(), (int)WORLD_NAME_LEN); uiText(left, x + pad + fw - textW(left, uiS), y + 70 * u, uiS, UI_DIM);
  if (dlg == DLG_NEW) {   // the difficulty is picked once, here: it cannot be changed after the world exists
    static const char* DD[3] = {"No hunger. The wicked hit for 1-2 lives.", "Hunger only slows you down. The wicked hit for 1-3 lives.", "Starving and long falls cost lives. Faster wicked hit for 3-5."};
    float dy = fy + fh + 22 * u, dw = (fw - 2 * 10 * u) / 3; uiText("Difficulty", x + pad, dy, uiS, UI_DIM); std::string lk = "cannot be changed later"; uiText(lk, x + pad + fw - textW(lk, uiSmall()), dy + 2 * u, uiSmall(), UI_GOLD); dy += 7 * uiS + 12 * u;
    for (int d = 0; d < 3; d++) if (uiButton(34 + d, x + pad + d * (dw + 10 * u), dy, dw, 42 * u, DIFF_NAME[d], d == dlgDiff ? BTN_PRIMARY : BTN_NORMAL)) dlgDiff = d;
    uiTextFitC(DD[dlgDiff], x + w * .5f, dy + 42 * u + 14 * u, fw, uiSmall(), UI_DIM); }
  bool ok = !cleanName(dlgText).empty(); if (uiButton(32, x + pad, by, bw, bh, "CANCEL")) dlg = DLG_NONE; if (uiButton(33, x + pad + bw + 12 * u, by, bw, bh, dlg == DLG_NEW ? "CREATE WORLD" : "SAVE NAME", BTN_PRIMARY, ok)) dialogConfirm();
}
void menuFrame(float dt) {
  glfwGetFramebufferSize(win, &FW, &FH); if (FW < 1 || FH < 1) return;
  audioSet(0, 0, 100, .11f); drawMenuBackground(); uiBegin(dt);
  float tx = clampf(uiMx / FW * 2 - 1, -1, 1), ty = clampf(1 - uiMy / FH * 2, -1, 1), k = std::min(1.f, dt * 1.6f); menuMX += (tx - menuMX) * k; menuMY += (ty - menuMY) * k; // the scene drifts gently with the mouse
  menuFade = std::min(1.f, menuFade + dt * 5); uiA = menuFade; uiInput = dlg == DLG_NONE;
  if (menuScreen == SC_HOME) menuHome(); else if (menuScreen == SC_WORLDS) menuWorlds();
  else if (menuScreen == SC_SETTINGS) { int r = settingsPanel(); if (r == 1) { saveSettings(); goScreen(SC_HOME); } else if (r == 2) goScreen(SC_CONTROLS); }
  else if (controlsPanel()) goScreen(SC_SETTINGS);
  uiA = 1; if (dlg != DLG_NONE) { uiInput = true; menuDialog(); }
  if (menuScreen != SC_WORLDS || dlg != DLG_NONE) uiWheel = 0;
  if (!menuMsg.empty() && menuScreen == SC_HOME) uiTextC(menuMsg, FW * .5f, FH - 40 * uiU, uiS, UI_RED);
  uiEnd();
}
// shown while the universe is generated: the game can take a few seconds to build 128 planets
void loadingFrame(const std::string& title, float progress) {
  glfwPollEvents(); glfwGetFramebufferSize(win, &FW, &FH); if (FW < 1 || FH < 1) return; drawMenuBackground(); uiBegin(0); uiInput = false;
  float u = uiU, bw = 420 * u, bh = 8 * u, x = (FW - bw) * .5f, y = FH * .5f; uiLogo((FW - 62 * u * 6.52f) * .5f, y - 150 * u, 62 * u, 1); uiTextC(title, FW * .5f, y - 32 * u, uiLarge(), UI_TXT);
  uiBox(x, y, bw, bh, 4 * u, {1, 1, 1, .12f}, {1, 1, 1, .12f}); uiBox(x, y, std::max(bh, bw * clampf(progress, 0, 1)), bh, 4 * u, {1, .82f, .46f, 1}, {1, .6f, .3f, 1}, {0, 0, 0, 0}, 0, {1, .6f, .3f, .35f}, 7 * u);
  uiEnd(); glfwSwapBuffers(win);
}
// ================================================================ PAUSE MENU
void pauseFrame(float dt) {
  if (devPanel) { devFrame(dt); return; }
  if (FW < 1 || FH < 1) return; audioSet(0, 0, 100, 0); uiBegin(dt); if (!pauseEncy) uiWheel = 0; float u = uiU; { Col dim{.004f, .008f, .025f, .72f}; uiBox(0, 0, (float)FW, (float)FH, 0, dim, dim); }
  if (pauseControls) { if (controlsPanel()) { pauseControls = false; pauseSettings = true; } uiEnd(); return; }
  if (pauseSettings) { int r = settingsPanel(); if (r == 1) { saveSettings(); pauseSettings = false; } else if (r == 2) { pauseSettings = false; pauseControls = true; } uiEnd(); return; }
  if (pauseEncy) { if (encyclopediaPanel()) pauseEncy = false; uiEnd(); return; }
  float bw = 380 * u, bh = 46 * u, gap = 11 * u, w = bw + 72 * u, h = 118 * u + (bh + gap) * 4 + 58 * u, x = (FW - w) * .5f, y = (FH - h) * .5f, bx = x + 36 * u, by = y + 118 * u; uiPanel(x, y, w, h);
  uiTextC("PAUSED", FW * .5f, y + 30 * u, uiS * 3, UI_TXT); if (World* cw = findWorld(curWorld)) uiTextFitC(cw->name, FW * .5f, y + 30 * u + 7 * uiS * 3 + 14 * u, bw, uiS, UI_GOLD);
  if (uiButton(1, bx, by, bw, bh, "RESUME", BTN_PRIMARY)) setPaused(false);
  by += bh + gap; if (uiButton(7, bx, by, bw, bh, "ENCYCLOPEDIA")) pauseEncy = true;
  by += bh + gap; if (uiButton(2, bx, by, bw, bh, "SETTINGS")) pauseSettings = true;
  by += bh + gap; if (uiButton(5, bx, by, bw, bh, "SAVE AND GO TO MAIN MENU")) pendingMenu = true;
  by += bh + gap; uiTextC("Progress is saved automatically", FW * .5f, by, uiSmall(), {.6f, 1, .7f});
  uiEnd();
}
