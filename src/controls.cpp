// Shared game state, key bindings and input callbacks.
#include "game.h"

// ---------------------------------------------------------------- game state
void toast(const std::string& s, float sec) { toastT = s; toastL = toastMax = sec; }
static bool K(int k) { return glfwGetKey(win, k) == GLFW_PRESS; }
// ---------------------------------------------------------------- controls (key bindings, saved in settings.txt)
void resetControls() {
  static const int D[ACT_COUNT][2] = {{GLFW_KEY_W, GLFW_KEY_UP}, {GLFW_KEY_S, GLFW_KEY_DOWN}, {GLFW_KEY_A, GLFW_KEY_LEFT}, {GLFW_KEY_D, GLFW_KEY_RIGHT}, {GLFW_KEY_LEFT_SHIFT, 0}, {GLFW_KEY_SPACE, 0}, {GLFW_KEY_E, 0}, {GLFW_KEY_F, 0}, {GLFW_KEY_R, 0}};
  // controller: D-pad walks (the left stick always does too), A jump, LB interact, Y eat, X sprint, RT attack
  static const int PD[ACT_COUNT] = {GLFW_GAMEPAD_BUTTON_DPAD_UP, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, GLFW_GAMEPAD_BUTTON_DPAD_LEFT, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, GLFW_GAMEPAD_BUTTON_X, GLFW_GAMEPAD_BUTTON_A, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, GLFW_GAMEPAD_BUTTON_Y, PAD_RT};
  for (int a = 0; a < ACT_COUNT; a++) { keyBind[a][0] = D[a][0]; keyBind[a][1] = D[a][1]; padBind[a][0] = PD[a]; padBind[a][1] = -1; } mouseSens = 1; mouseInvert = false; mouseInvertX = false; padSens = 1; padInvert = false; padInvertX = false;
}
static bool padHeld(int a) { for (int b : padBind[a]) if (b >= 0 && b < PAD_NB && padDown[b]) return true; return false; }
bool held(int a) { return (keyBind[a][0] && K(keyBind[a][0])) || (keyBind[a][1] && K(keyBind[a][1])) || padHeld(a); }
static bool isAct(int key, int a) { return key > 0 && (key == keyBind[a][0] || key == keyBind[a][1]); }
// The name printed on the key for the current keyboard layout (GLFW_KEY_W shows Z on AZERTY). Always plain ASCII: the HUD font has nothing else.
std::string keyName(int k) {
  if (k <= 0) return "-";
  static const std::unordered_map<int, const char*> N = {{GLFW_KEY_SPACE, "SPACE"}, {GLFW_KEY_LEFT_SHIFT, "LEFT SHIFT"}, {GLFW_KEY_RIGHT_SHIFT, "RIGHT SHIFT"}, {GLFW_KEY_LEFT_CONTROL, "LEFT CTRL"}, {GLFW_KEY_RIGHT_CONTROL, "RIGHT CTRL"}, {GLFW_KEY_LEFT_ALT, "LEFT ALT"}, {GLFW_KEY_RIGHT_ALT, "RIGHT ALT"}, {GLFW_KEY_LEFT_SUPER, "LEFT SUPER"}, {GLFW_KEY_RIGHT_SUPER, "RIGHT SUPER"}, {GLFW_KEY_MENU, "MENU"},
    {GLFW_KEY_TAB, "TAB"}, {GLFW_KEY_ENTER, "ENTER"}, {GLFW_KEY_BACKSPACE, "BACKSPACE"}, {GLFW_KEY_CAPS_LOCK, "CAPS LOCK"}, {GLFW_KEY_NUM_LOCK, "NUM LOCK"}, {GLFW_KEY_SCROLL_LOCK, "SCROLL LOCK"}, {GLFW_KEY_PRINT_SCREEN, "PRINT SCREEN"}, {GLFW_KEY_PAUSE, "PAUSE"}, {GLFW_KEY_UP, "UP"}, {GLFW_KEY_DOWN, "DOWN"}, {GLFW_KEY_LEFT, "LEFT"}, {GLFW_KEY_RIGHT, "RIGHT"},
    {GLFW_KEY_INSERT, "INSERT"}, {GLFW_KEY_DELETE, "DELETE"}, {GLFW_KEY_HOME, "HOME"}, {GLFW_KEY_END, "END"}, {GLFW_KEY_PAGE_UP, "PAGE UP"}, {GLFW_KEY_PAGE_DOWN, "PAGE DOWN"}, {GLFW_KEY_KP_DECIMAL, "NUM ."}, {GLFW_KEY_KP_DIVIDE, "NUM /"}, {GLFW_KEY_KP_MULTIPLY, "NUM *"}, {GLFW_KEY_KP_SUBTRACT, "NUM -"}, {GLFW_KEY_KP_ADD, "NUM +"}, {GLFW_KEY_KP_ENTER, "NUM ENTER"}, {GLFW_KEY_KP_EQUAL, "NUM ="}};
  auto it = N.find(k); if (it != N.end()) return it->second;
  if (k >= GLFW_KEY_F1 && k <= GLFW_KEY_F25) return "F" + std::to_string(k - GLFW_KEY_F1 + 1);
  if (k >= GLFW_KEY_KP_0 && k <= GLFW_KEY_KP_9) return "NUM " + std::to_string(k - GLFW_KEY_KP_0);
  bool digit = k >= GLFW_KEY_0 && k <= GLFW_KEY_9;
  if (const char* n = glfwGetKeyName(k, 0)) { unsigned char c = (unsigned char)n[0], c2 = c ? (unsigned char)n[1] : 0;
    if (c > 32 && c < 127 && !c2) return std::string(1, (char)std::toupper(c));
    if (!digit && (c & 0xE0) == 0xC0 && (c2 & 0xC0) == 0x80) { unsigned cp = ((c & 0x1Fu) << 6) | (c2 & 0x3Fu); if (cp >= 0xC0 && cp <= 0xFF) return std::string(1, (char)std::toupper((unsigned char)LATIN_FOLD[cp - 0xC0])) + "'"; } } // an accented key such as the French u-grave: U' so it is not mistaken for U
  return digit ? std::to_string(k - GLFW_KEY_0) : "KEY " + std::to_string(k);
}
// Buttons work by position (the bottom face button is always "A" for the game), but they are NAMED like the controller in your hands:
// a Nintendo pad has B at the bottom and A on the right, a PlayStation pad has Cross / Circle / Square / Triangle.
static int padStyle = 0;   // 0 Xbox and others, 1 Nintendo, 2 PlayStation
std::string padName(int b) {
  static const char* N[3][PAD_NB] = {
    {"A", "B", "X", "Y", "LB", "RB", "BACK", "START", "GUIDE", "L3", "R3", "D-PAD UP", "D-PAD RIGHT", "D-PAD DOWN", "D-PAD LEFT", "LT", "RT"},
    {"B", "A", "Y", "X", "L", "R", "-", "+", "HOME", "L STICK", "R STICK", "D-PAD UP", "D-PAD RIGHT", "D-PAD DOWN", "D-PAD LEFT", "ZL", "ZR"},
    {"CROSS", "CIRCLE", "SQUARE", "TRIANGLE", "L1", "R1", "SHARE", "OPTIONS", "PS", "L3", "R3", "D-PAD UP", "D-PAD RIGHT", "D-PAD DOWN", "D-PAD LEFT", "L2", "R2"}};
  return b >= 0 && b < PAD_NB ? N[padStyle][b] : "-";
}
// with a controller in hand, every on-screen hint names its buttons instead of keys
static std::string actKey(int a) { if (padActive && padConnected && (padBind[a][0] >= 0 || padBind[a][1] >= 0)) return padName(padBind[a][0] >= 0 ? padBind[a][0] : padBind[a][1]); int k = keyBind[a][0] ? keyBind[a][0] : keyBind[a][1]; return k ? keyName(k) : "(no key)"; }
// On-screen text writes {E}, {J}... instead of a fixed key, and shows whatever the player has bound right now.
std::string keysIn(std::string s) {
  static const struct { const char* tok; int act; } T[] = {{"{F}", ACT_FORWARD}, {"{B}", ACT_BACK}, {"{L}", ACT_LEFT}, {"{R}", ACT_RIGHT}, {"{RUN}", ACT_RUN}, {"{JUMP}", ACT_JUMP}, {"{E}", ACT_INTERACT}, {"{EAT}", ACT_EAT}, {"{ATK}", ACT_ATTACK}};
  if (s.find('{') == std::string::npos) return s;
  for (auto& t : T) { size_t n = std::strlen(t.tok), p = 0; while ((p = s.find(t.tok, p)) != std::string::npos) { std::string v = actKey(t.act); s.replace(p, n, v); p += v.size(); } }
  return s;
}
void toggleFS() {
  if (!fs) { glfwGetWindowPos(win, &wx, &wy); glfwGetWindowSize(win, &ww, &wh); const GLFWvidmode* m = glfwGetVideoMode(mon); glfwSetWindowMonitor(win, mon, 0, 0, m->width, m->height, m->refreshRate); }
  else { const GLFWvidmode* m = glfwGetVideoMode(mon); wx = std::max(0, (m->width - ww) / 2); wy = std::max(60, (m->height - wh) / 2);   // back to a centred window, clear of the top bar
    glfwSetWindowMonitor(win, nullptr, wx, wy, ww, wh, 0); fsFix = .25f; }   // the desktop settles the window a moment later (title bar): its size is applied again then, so no black band is left
  fs = !fs;
}
// the mouse is free in every menu; all sound stops while the game is paused
void syncModes() {
  glfwSetInputMode(win, GLFW_CURSOR, app == A_GAME && !paused && !modal ? GLFW_CURSOR_DISABLED : padNav && padConnected ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);   // controller in use: no pointer on menus or loading screens
  audioMute(muted); mdx = mdy = 0; mouseFresh = true;
}
void uiFlushMouse() { uiPressed = uiReleased = uiDown = false; uiWheel = 0; uiActive = 0; }
static bool uiLive() { return app == A_MENU || (app == A_GAME && (paused || modal)); } // clicks made while playing must not press a menu button later
void setPaused(bool on) { paused = on; if (!on) devPanel = false; uiFlushMouse(); pauseControls = false; pauseSettings = false; pauseEncy = false; captureAct = -1; ctrlMsg.clear(); if (on) saveReq = true; syncModes(); }
// a new key is swapped with whichever action already had it, so one key never does two things
static void captureKey(int key, int act) {
  if (act != GLFW_PRESS || key <= 0) return; // key < 0: a key GLFW has no name for
  if (key == GLFW_KEY_ESCAPE) { captureAct = -1; capturePad = false; return; }
  if (capturePad) return;   // waiting for a controller button: keys other than Esc do nothing
  int a = captureAct, s = captureSlot, old = keyBind[a][s]; ctrlMsg.clear();
  for (int b = 0; b < ACT_COUNT; b++) for (int t = 0; t < 2; t++) if (keyBind[b][t] == key && !(b == a && t == s)) { keyBind[b][t] = old; if (b != a) ctrlMsg = keyName(key) + " was used by " + ACT_NAME[b] + (old ? " - keys swapped" : ""); }
  keyBind[a][s] = key; captureAct = -1; saveSettings();
}
// Keyboard = gameplay only. Menus are driven by the mouse; there the keyboard only types world names and chooses keys to bind.
void doAction(int a) {
  if (app != A_GAME || paused || modal || cine) return;
  if (endT >= 0) { if (a == ACT_INTERACT) skipEnding(); return; }   // during the ending, interact skips it
  if (a == ACT_INTERACT) interact(); else if (a == ACT_EAT && st == SURFACE) eatFood(); else if (a == ACT_ATTACK) attack();
}
void onKey(GLFWwindow* w, int key, int, int act, int) {
  if (w && act == GLFW_PRESS) padActive = false;   // a real key press: hints go back to keyboard names (w is null when the controller fakes Esc)
  if (captureAct >= 0) { captureKey(key, act); return; }
  if (app == A_MENU) { menuKey(key, act); return; }
  if (app != A_GAME || act != GLFW_PRESS) return;
  if (key == GLFW_KEY_F1) { toggleDevPanel(); return; }   // does nothing outside the developer world
  if (key == GLFW_KEY_ESCAPE && devPanel) { toggleDevPanel(); return; }
  if (key == GLFW_KEY_ESCAPE) { if (modal) { if (modal != M_DEATH) closeModal(); } else if (paused && pauseControls) { pauseControls = false; pauseSettings = true; ctrlMsg.clear(); } else if (paused && pauseSettings) { pauseSettings = false; saveSettings(); } else if (paused && pauseEncy) pauseEncy = false; else setPaused(!paused); }
  else if (paused) return;
  else if (modal) return;
  else if (cine) return;
  else for (int a : {ACT_INTERACT, ACT_EAT, ACT_ATTACK}) if (isAct(key, a)) { doAction(a); break; }
}
void onChar(GLFWwindow*, unsigned cp) { if (app == A_MENU) menuChar(cp); }
void onCursor(GLFWwindow*, double x, double y) {
  if (mouseFresh) { mouseFresh = false; mlx = x; mly = y; mdx = mdy = 0; return; }   // GLFW teleports the cursor when it is captured: that jump is not a mouse movement
  if (std::fabs(x - mlx) > 350 || std::fabs(y - mly) > 350) { mlx = x; mly = y; return; }   // a teleporting cursor, not a hand
  if (padNav && (std::fabs(x - mlx) > 2 || std::fabs(y - mly) > 2)) padNav = false;   // the real mouse moved: it takes over from the controller selection
  if (app == A_GAME && !paused && !modal) { mdx += (x - mlx) * mouseSens * (mouseInvertX ? -1 : 1); mdy += (y - mly) * mouseSens * (mouseInvert ? -1 : 1); } mlx = x; mly = y;
}
void onMouseButton(GLFWwindow*, int button, int act, int) {
  if (act == GLFW_PRESS) padActive = false;
  if (button == GLFW_MOUSE_BUTTON_LEFT && act == GLFW_PRESS && app == A_GAME && !paused && !modal && st == SURFACE) { doAction(ACT_ATTACK); return; }   // left click swings the sword
  if (button != GLFW_MOUSE_BUTTON_LEFT || !uiLive()) return; if (act == GLFW_PRESS) { uiPressed = uiDown = true; } else if (act == GLFW_RELEASE) { uiReleased = true; uiDown = false; } }
void onScroll(GLFWwindow*, double, double dy) { if (uiLive()) uiWheel += (float)dy; }
// ---------------------------------------------------------------- game controller
// In the game: sticks move and look, buttons do their actions, Start pauses. In every menu: the left stick moves the mouse pointer,
// A clicks, the right stick scrolls, B goes back (like Esc). Polled once per frame from main().
// controller browsing: a selected widget (uiFocus) that the D-pad / left stick moves between, like a console menu. The mouse pointer is never touched:
// while padNav is on, uiBegin places the UI's virtual pointer on the selected widget, so it gets the normal hover look. Moving the mouse turns it off.
static void padNavigate(int dx, int dy) {
  const auto& R = uiRects; if (R.empty()) return;
  int cur = -1; for (int i = 0; i < (int)R.size(); i++) if (R[i].id == uiFocus) { cur = i; break; }
  if (!padNav || cur < 0) {   // first press: select the widget under the pointer, or the first one on screen
    int pick = 0; for (int i = 0; i < (int)R.size(); i++) if (uiMx >= R[i].x && uiMx < R[i].x + R[i].w && uiMy >= R[i].y && uiMy < R[i].y + R[i].h) { pick = i; break; }
    uiFocus = R[pick].id; padNav = true; return; }
  padNav = true;
  if (R[cur].slider && dx) { padNudge = dx; return; }
  float px = R[cur].x + R[cur].w * .5f, py = R[cur].y + R[cur].h * .5f; int best = -1; float bs = 1e30f;
  for (int i = 0; i < (int)R.size(); i++) { if (i == cur) continue; float vx = R[i].x + R[i].w * .5f - px, vy = R[i].y + R[i].h * .5f - py;
    float along = dx ? vx * dx : vy * dy; if (along < 4) continue;   // must lie in that direction
    // widgets in the same row (or column) always win: score by the gap between edges plus how far off that row/column it sits
    const UiRect &a = R[cur], &b = R[i];
    float gap = dx ? (dx > 0 ? b.x - (a.x + a.w) : a.x - (b.x + b.w)) : (dy > 0 ? b.y - (a.y + a.h) : a.y - (b.y + b.h));
    float off = dx ? std::max(0.f, std::max(a.y, b.y) - std::min(a.y + a.h, b.y + b.h)) : std::max(0.f, std::max(a.x, b.x) - std::min(a.x + a.w, b.x + b.w));
    float sc = std::max(0.f, gap) + off * 6.f + std::fabs(dx ? vy : vx) * .2f; if (sc < bs) { bs = sc; best = i; } }
  if (best >= 0) { uiFocus = R[best].id; audioEvent(S_CLICK); }
}
// hide the pointer while the controller browses a menu; moving the mouse (or unplugging the pad) brings it back
static void padCursor() {
  int m = glfwGetInputMode(win, GLFW_CURSOR); if (m == GLFW_CURSOR_DISABLED) return;   // playing: the game owns the pointer
  int want = padNav && padConnected ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL; if (m != want) glfwSetInputMode(win, GLFW_CURSOR, want);
}
void pollGamepad(float dt) {
  static bool prev[PAD_NB]; GLFWgamepadstate gs; int jid = -1;
  static bool rawPad = false; rawPad = false;
  for (int j = GLFW_JOYSTICK_1; j <= GLFW_JOYSTICK_LAST; j++) if (glfwJoystickIsGamepad(j) && glfwGetGamepadState(j, &gs)) { jid = j; break; }
  if (jid < 0) {   // no SDL mapping known for this device (e.g. Xbox Wireless over Bluetooth): read the raw joystick with the standard Linux Xbox layout
    for (int j = GLFW_JOYSTICK_1; j <= GLFW_JOYSTICK_LAST && jid < 0; j++) if (glfwJoystickPresent(j)) {
      int na = 0, nb = 0; const float* ax = glfwGetJoystickAxes(j, &na); const unsigned char* bt = glfwGetJoystickButtons(j, &nb); 
      if (!ax || !bt || na < 4 || nb < 4) continue;
      std::memset(&gs, 0, sizeof gs); for (float& a : gs.axes) a = 0; gs.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = gs.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = -1;
      gs.axes[GLFW_GAMEPAD_AXIS_LEFT_X] = ax[0]; gs.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] = ax[1];
      gs.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] = ax[2]; gs.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] = ax[3];
      bool wide = nb >= 15;   // 15 buttons = Linux generic-HID Xbox layout (A B C X Y Z LB RB LT RT Back Start Guide L3 R3, axes X Y Z RZ GAS BRAKE); fewer = classic xpad layout
      if (na >= 6) { gs.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = ax[wide ? 5 : 4]; gs.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = ax[wide ? 4 : 5]; }
      static const int MAPW[15] = {GLFW_GAMEPAD_BUTTON_A, GLFW_GAMEPAD_BUTTON_B, -1, GLFW_GAMEPAD_BUTTON_X, GLFW_GAMEPAD_BUTTON_Y, -1, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, -1, -1,
        GLFW_GAMEPAD_BUTTON_BACK, GLFW_GAMEPAD_BUTTON_START, GLFW_GAMEPAD_BUTTON_GUIDE, GLFW_GAMEPAD_BUTTON_LEFT_THUMB, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB};
      static const int MAPB[11] = {GLFW_GAMEPAD_BUTTON_A, GLFW_GAMEPAD_BUTTON_B, GLFW_GAMEPAD_BUTTON_X, GLFW_GAMEPAD_BUTTON_Y, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
        GLFW_GAMEPAD_BUTTON_BACK, GLFW_GAMEPAD_BUTTON_START, GLFW_GAMEPAD_BUTTON_GUIDE, GLFW_GAMEPAD_BUTTON_LEFT_THUMB, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB};
      for (int b = 0; b < nb && b < (wide ? 15 : 11); b++) { int m = wide ? MAPW[b] : MAPB[b]; if (m >= 0) gs.buttons[m] = bt[b]; }
      // D-pad: filled in below from the hat (hat bits are 1/2/4/8, so they cannot be stored here as-is)
      jid = j; rawPad = true; }
  }
  if (jid >= 0) {   // the D-pad is also read straight from the device's hat (and hat axes): some mappings leave it out or put it elsewhere
    int nh = 0, na = 0; const unsigned char* ht = glfwGetJoystickHats(jid, &nh); const float* ax = glfwGetJoystickAxes(jid, &na);
    unsigned char h = nh > 0 && ht ? ht[0] : 0;
    if (!nh && ax && na >= 8) h = (ax[6] < -.5f ? GLFW_HAT_LEFT : ax[6] > .5f ? GLFW_HAT_RIGHT : 0) | (ax[7] < -.5f ? GLFW_HAT_UP : ax[7] > .5f ? GLFW_HAT_DOWN : 0);
    if (h & GLFW_HAT_UP) gs.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] = GLFW_PRESS; if (h & GLFW_HAT_DOWN) gs.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] = GLFW_PRESS;
    if (h & GLFW_HAT_LEFT) gs.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] = GLFW_PRESS; if (h & GLFW_HAT_RIGHT) gs.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] = GLFW_PRESS; }
  if (jid < 0) { padConnected = false; padActive = false; padNav = false; padCursor(); padLabel.clear(); padMove[0] = padMove[1] = 0; for (int b = 0; b < PAD_NB; b++) padDown[b] = prev[b] = false; return; }
  if (!padConnected) { const char* n = rawPad ? glfwGetJoystickName(jid) : glfwGetGamepadName(jid); padLabel = n ? n : "Controller"; { std::string l = padLabel; for (auto& ch : l) ch = (char)std::tolower((unsigned char)ch);
      padStyle = l.find("nintendo") != std::string::npos || l.find("switch") != std::string::npos || l.find("joy-con") != std::string::npos || l.find("pro controller") != std::string::npos ? 1
               : l.find("playstation") != std::string::npos || l.find("dualshock") != std::string::npos || l.find("dualsense") != std::string::npos || l.find("ps4") != std::string::npos || l.find("ps5") != std::string::npos || l.find("sony") != std::string::npos ? 2 : 0; } for (auto& c : padLabel) if ((unsigned char)c < 32 || (unsigned char)c > 126) c = '?'; }
  padConnected = true;
  bool now[PAD_NB]; for (int b = 0; b <= GLFW_GAMEPAD_BUTTON_LAST; b++) now[b] = gs.buttons[b] == GLFW_PRESS;
  now[PAD_LT] = gs.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > .2f; now[PAD_RT] = gs.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > .2f;   // triggers rest at -1
  auto dz = [](float v) { float a = std::fabs(v); return a < .18f ? 0.f : std::copysign(std::min(1.f, (a - .18f) / .8f), v); };
  float lx = dz(gs.axes[GLFW_GAMEPAD_AXIS_LEFT_X]), ly = dz(gs.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]), rx = dz(gs.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]), ry = dz(gs.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);
  bool any = lx != 0 || ly != 0 || rx != 0 || ry != 0; for (int b = 0; b < PAD_NB; b++) any = any || now[b]; if (any) padActive = true;
  auto edge = [&](int b) { return now[b] && !prev[b]; };
  auto fakeEsc = [] { onKey(nullptr, GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0); padActive = true; };
  padMove[0] = padMove[1] = 0;
  if (captureAct >= 0 && capturePad) {   // binding a button: the next press is taken (Start cancels), and swapped away from any action that had it
    for (int b = 0; b < PAD_NB; b++) if (edge(b)) { if (b != GLFW_GAMEPAD_BUTTON_START) { int a = captureAct, s = captureSlot, old = padBind[a][s]; ctrlMsg.clear();
        for (int o = 0; o < ACT_COUNT; o++) for (int t = 0; t < 2; t++) if ((o != a || t != s) && padBind[o][t] == b) { padBind[o][t] = old; if (o != a) ctrlMsg = padName(b) + " was used by " + ACT_NAME[o] + " - buttons swapped"; }
        padBind[a][s] = b; saveSettings(); }
      captureAct = -1; capturePad = false; break; }
  } else if (uiLive() || app == A_LOADING) {
    static float rep = 0; static int hdx = 0, hdy = 0;
    int dx = 0, dy = 0;
    if (now[GLFW_GAMEPAD_BUTTON_DPAD_LEFT]) dx = -1; else if (now[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT]) dx = 1; else if (now[GLFW_GAMEPAD_BUTTON_DPAD_UP]) dy = -1; else if (now[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]) dy = 1;
    else if (std::fabs(lx) > .5f || std::fabs(ly) > .5f) { if (std::fabs(lx) > std::fabs(ly)) dx = lx < 0 ? -1 : 1; else dy = ly < 0 ? -1 : 1; }
    if (uiRects.empty()) {   // a screen without buttons: the stick drives the pointer directly
      padNav = false; if (lx != 0 || ly != 0) { double cx, cy; int w, h; glfwGetCursorPos(win, &cx, &cy); glfwGetWindowSize(win, &w, &h); float sp = 820.f * (h / 720.f) * padSens * dt;
        cx = clampf((float)(cx + lx * std::fabs(lx) * sp * 1.4f), 0, (float)w - 1); cy = clampf((float)(cy + ly * std::fabs(ly) * sp * 1.4f), 0, (float)h - 1); glfwSetCursorPos(win, cx, cy); }
    } else if (dx || dy) {   // first step at once, then repeats while held (slower start, like a console menu)
      bool fresh = dx != hdx || dy != hdy; rep -= dt; if (fresh) { hdx = dx; hdy = dy; }
      if (fresh || rep <= 0) { padNavigate(dx, dy); rep = fresh ? .38f : .12f; padActive = true; } }
    else { hdx = hdy = 0; rep = 0; }
    if (edge(GLFW_GAMEPAD_BUTTON_A) && !padNav && !uiRects.empty()) padNavigate(0, 0);   // A with nothing selected yet: select first, the next A clicks
    else if (edge(GLFW_GAMEPAD_BUTTON_A)) { uiPressed = uiDown = true; } else if (!now[GLFW_GAMEPAD_BUTTON_A] && prev[GLFW_GAMEPAD_BUTTON_A]) { uiReleased = true; uiDown = false; }
    if (ry != 0) uiWheel -= ry * dt * 9.f;
    if (edge(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER) || edge(PAD_LT)) padTab = -1; else if (edge(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER) || edge(PAD_RT)) padTab = 1;   // shoulder buttons / triggers flip tabs
    if (edge(GLFW_GAMEPAD_BUTTON_B) || (app == A_GAME && edge(GLFW_GAMEPAD_BUTTON_START))) {
      if (app == A_GAME) fakeEsc();
      else if (app == A_MENU) { if (dlg != DLG_NONE) dlg = DLG_NONE; else if (menuScreen == SC_CONTROLS) goScreen(SC_SETTINGS); else if (menuScreen != SC_HOME) { saveSettings(); goScreen(SC_HOME); } } }
  } else if (app == A_GAME) {
    float look = 900.f * padSens * dt;
    if (st == SPACE) {   // flying: both sticks steer (up/down = climb/dive, left/right = turn), RT speeds up and LT slows down (D-pad too)
      float rt = (gs.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1) * .5f, lt = (gs.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1) * .5f;   // triggers rest at -1
      padMove[0] = 0; padMove[1] = (rt > .1f ? rt : 0) - (lt > .1f ? lt : 0); float fl = look;   // same turn rate as a steady mouse sweep (the Look sensitivity slider tunes it)
      float sx = clampf(lx * std::fabs(lx) + rx * std::fabs(rx), -1, 1), sy = clampf(ly * std::fabs(ly) + ry * std::fabs(ry), -1, 1);
      mdx += sx * fl * (padInvertX ? -1 : 1); mdy += sy * fl * .75f * (padInvert ? -1 : 1); }
    else { padMove[0] = lx; padMove[1] = -ly; mdx += rx * std::fabs(rx) * look * (padInvertX ? -1 : 1); mdy += ry * std::fabs(ry) * look * .75f * (padInvert ? -1 : 1); }
    if (edge(GLFW_GAMEPAD_BUTTON_START)) fakeEsc();
    else for (int a : {ACT_INTERACT, ACT_EAT, ACT_ATTACK}) for (int b : padBind[a]) if (b >= 0 && edge(b) && !(st == SPACE && (b == PAD_LT || b == PAD_RT))) { doAction(a); break; }   // in space the triggers are the throttle
  }
  if (any && !(uiLive() && uiRects.empty())) padNav = true;   // any controller input switches to controller mode (also while playing, so pausing or entering a world keeps the pointer hidden)   // any controller input on a menu switches to controller mode (selection shown, pointer hidden)
  padCursor();
  for (int b = 0; b < PAD_NB; b++) { padDown[b] = now[b]; prev[b] = now[b]; }
}
