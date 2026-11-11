// Menu background and widgets: panels, buttons, text.
#include "game.h"
#include "menu_shaders.h"

// ================================================================ MENU DRAWING
static GLint uNebT, uNebAsp, uSpRes, uSpMouse, uSpT, uSpNeb, uWR, uWScr, uWPad, uWTop, uWBot, uWEdge, uWGlow, uWRad, uWEW, uWGR, uLR, uLScr, uLPad, uLT, uLA;
void setupMenuGfx() {
  pNebula = compile(VS_QUAD, FS_NEBULA); pSpace = compile(VS_QUAD, FS_SPACE); pWidget = compile(VS_WIDGET, FS_WIDGET); pLogo = compile(VS_WIDGET, FS_LOGO);
  auto U = [](GLuint p, const char* n) { return glGetUniformLocation(p, n); };
  uNebT = U(pNebula, "uT"); uNebAsp = U(pNebula, "uAsp"); uSpRes = U(pSpace, "uRes"); uSpMouse = U(pSpace, "uMouse"); uSpT = U(pSpace, "uT"); uSpNeb = U(pSpace, "uNeb");
  uWR = U(pWidget, "uR"); uWScr = U(pWidget, "uScr"); uWPad = U(pWidget, "uPad"); uWTop = U(pWidget, "uTop"); uWBot = U(pWidget, "uBot"); uWEdge = U(pWidget, "uEdge"); uWGlow = U(pWidget, "uGlow"); uWRad = U(pWidget, "uRad"); uWEW = U(pWidget, "uEW"); uWGR = U(pWidget, "uGR");
  uLR = U(pLogo, "uR"); uLScr = U(pLogo, "uScr"); uLPad = U(pLogo, "uPad"); uLT = U(pLogo, "uT"); uLA = U(pLogo, "uA");
  glGenVertexArrays(1, &emptyVAO);
  glGenTextures(1, &nebTex); glBindTexture(GL_TEXTURE_2D, nebTex); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, NEB_W, NEB_H, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glGenFramebuffers(1, &nebFBO); glBindFramebuffer(GL_FRAMEBUFFER, nebFBO); glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nebTex, 0); glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
// the scene runs with a long offset so the rocket is already on its way when the menu first appears
static float menuTime() { return (float)glfwGetTime() + 18.f; }
void drawMenuBackground() {
  float t = menuTime(); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDepthMask(GL_FALSE); glDisable(GL_FRAMEBUFFER_SRGB); glDisable(GL_MULTISAMPLE); glBindVertexArray(emptyVAO); glActiveTexture(GL_TEXTURE0);
  glBindFramebuffer(GL_FRAMEBUFFER, nebFBO); glViewport(0, 0, NEB_W, NEB_H); glUseProgram(pNebula); glUniform1f(uNebT, t); glUniform1f(uNebAsp, (float)FW / FH); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindFramebuffer(GL_FRAMEBUFFER, 0); glViewport(0, 0, FW, FH); glUseProgram(pSpace); glUniform2f(uSpRes, (float)FW, (float)FH); glUniform2f(uSpMouse, menuMX, menuMY); glUniform1f(uSpT, t); glUniform1i(uSpNeb, 0);
  glBindTexture(GL_TEXTURE_2D, nebTex); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); glEnable(GL_MULTISAMPLE);
}
// menu colours are plain display colours, so sRGB output is switched off while widgets are drawn
void uiBegin(float dt) {
  double cx, cy; int ww_, wh_; glfwGetCursorPos(win, &cx, &cy); glfwGetWindowSize(win, &ww_, &wh_); // the cursor is in window units, drawing is in framebuffer pixels (they differ on HiDPI screens)
  uiMx = (float)cx * FW / std::max(ww_, 1); uiMy = (float)cy * FH / std::max(wh_, 1); uiDt = dt; uiU = FH / 720.f; uiS = std::max(1.f, std::round(FH / 360.f)); uiA = 1; uiInput = true; uiRectsPrev.swap(uiRects); uiRects.clear();
  if (padNav) { const UiRect* f = nullptr; for (auto& r : uiRectsPrev) if (r.id == uiFocus) { f = &r; break; }   // controller selection: the virtual pointer sits on it
    if (!f && !uiRectsPrev.empty()) { f = &uiRectsPrev[0]; uiFocus = f->id; } if (f) { uiMx = f->x + f->w * .5f; uiMy = f->y + f->h * .5f; } }
  glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE); glDisable(GL_FRAMEBUFFER_SRGB); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glUseProgram(pHud); glUniform2f(uHScr, (float)FW, (float)FH);
}
void uiEnd() {
  padNudge = 0; padTab = 0; if (uiReleased) uiActive = 0; uiPressed = uiReleased = false;
  glEnable(GL_FRAMEBUFFER_SRGB); glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE);
}
void uiBox(float x, float y, float w, float h, float rad, Col top, Col bot, Col edge, float ew, Col glow, float gr) {
  glUseProgram(pWidget); glUniform4f(uWR, x, y, w, h); glUniform2f(uWScr, (float)FW, (float)FH); glUniform1f(uWPad, gr * 4 + 1); glUniform1f(uWRad, std::min(rad, std::min(w, h) * .5f)); glUniform1f(uWEW, ew); glUniform1f(uWGR, gr);
  glUniform4f(uWTop, top.r, top.g, top.b, top.a * uiA); glUniform4f(uWBot, bot.r, bot.g, bot.b, bot.a * uiA); glUniform4f(uWEdge, edge.r, edge.g, edge.b, edge.a * uiA); glUniform4f(uWGlow, glow.r, glow.g, glow.b, glow.a * uiA);
  glBindVertexArray(emptyVAO); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); glUseProgram(pHud);
}
float uiSmall() { return std::max(1.f, std::round(uiS * .75f)); }
float uiLarge() { return std::round(uiS * 1.5f); }
float textW(const std::string& s, float sc) { return stb_easy_font_width((char*)s.c_str()) * sc; }
void uiText(const std::string& s, float x, float y, float sc, V3 c, float a) { text(s, std::floor(x), std::floor(y), sc, c, a * uiA); }
void uiTextC(const std::string& s, float cx, float y, float sc, V3 c, float a) { uiText(s, cx - textW(s, sc) * .5f, y, sc, c, a); }
// text that must fit a width: smaller letters first, "..." only if even the smallest size is too wide
float fitScale(const std::string& s, float maxW, float sc) { float ts = sc; while (ts > 1 && textW(s, ts) > maxW) ts -= 1; return ts; }
void uiTextFit(const std::string& s, float x, float y, float maxW, float sc, V3 c, float a) { float ts = fitScale(s, maxW, sc); uiText(fitText(s, maxW, ts), x, y + (7 * sc - 7 * ts) * .5f, ts, c, a); }
void uiTextFitC(const std::string& s, float cx, float y, float maxW, float sc, V3 c, float a) { float ts = fitScale(s, maxW, sc); uiTextC(fitText(s, maxW, ts), cx, y + (7 * sc - 7 * ts) * .5f, ts, c, a); }
std::string fitText(std::string s, float maxW, float sc) { if (textW(s, sc) <= maxW) return s; while (!s.empty() && textW(s + "...", sc) > maxW) s.pop_back(); return s + "..."; }
bool uiOver(float x, float y, float w, float h) { return uiInput && uiMx >= x && uiMx < x + w && uiMy >= y && uiMy < y + h; }
bool uiButton(int id, float x, float y, float w, float h, const std::string& label, int style, bool enabled) {
  if (enabled && uiInput) uiRects.push_back({x, y, w, h, id, false});
  bool over = enabled && uiOver(x, y, w, h), click = false; if (over && uiPressed) uiActive = id; if (uiReleased && uiActive == id && over) click = true;
  float& hv = uiHov[id]; hv += ((over ? 1.f : 0.f) - hv) * std::min(1.f, uiDt * 14); bool held = uiActive == id && over && uiDown; float k = enabled ? hv : 0, pr = held ? .6f : 1.f;
  Col acc = style == BTN_PRIMARY ? Col{1, .72f, .34f, 1} : style == BTN_DANGER || style == BTN_DANGER_STRONG ? Col{1, .36f, .33f, 1} : Col{.45f, .74f, 1, 1};
  Col top = {lerpf(.075f, acc.r * .36f, k) * pr, lerpf(.115f, acc.g * .36f, k) * pr, lerpf(.205f, acc.b * .36f, k) * pr, .92f}, bot = {lerpf(.035f, acc.r * .22f, k) * pr, lerpf(.055f, acc.g * .22f, k) * pr, lerpf(.115f, acc.b * .22f, k) * pr, .95f};
  if (style == BTN_PRIMARY) { top = {lerpf(.66f, .86f, k) * pr, lerpf(.42f, .58f, k) * pr, lerpf(.15f, .22f, k) * pr, .95f}; bot = {lerpf(.40f, .58f, k) * pr, lerpf(.22f, .33f, k) * pr, lerpf(.07f, .10f, k) * pr, .97f}; }
  if (style == BTN_DANGER_STRONG) { top = {lerpf(.50f, .78f, k) * pr, lerpf(.11f, .18f, k) * pr, lerpf(.11f, .16f, k) * pr, .95f}; bot = {lerpf(.30f, .50f, k) * pr, lerpf(.05f, .08f, k) * pr, lerpf(.06f, .08f, k) * pr, .96f}; }
  float ea = enabled ? (style == BTN_NORMAL || style == BTN_DANGER ? .24f + .62f * k : .55f + .4f * k) : .1f; uiBox(x, y, w, h, 7 * uiU, top, bot, {acc.r, acc.g, acc.b, ea}, std::max(1.f, 1.2f * uiU), {acc.r, acc.g, acc.b, .28f * k}, 9 * uiU);
  V3 tc = !enabled ? V3{.45f, .5f, .58f} : (style == BTN_DANGER && k > .4f) ? V3{1, .78f, .75f} : UI_TXT; float ts = uiS, avail = w - 12 * uiU; while (ts > 1 && textW(label, ts) > avail) ts -= 1;   // long labels get smaller text instead of being cut with "..."
  uiTextC(fitText(label, avail, ts), x + w * .5f, y + (h - 7 * ts) * .5f, ts, tc);
  // a click belongs to one button only: without this, a quick tap that opens a dialog could also press whatever appears under the cursor
  if (click) { uiActive = 0; uiPressed = uiReleased = false; audioEvent(S_CLICK); }
  return click;
}
void uiPanel(float x, float y, float w, float h) { uiBox(x, y, w, h, 14 * uiU, {.035f, .055f, .115f, .93f}, {.015f, .022f, .055f, .96f}, {.5f, .72f, 1, .26f}, std::max(1.f, 1.1f * uiU), {.2f, .45f, 1, .10f}, 22 * uiU); }
void uiLogo(float x, float y, float h, float a) {
  float pad = h * .9f; glBlendFunc(GL_ONE, GL_ONE); glUseProgram(pLogo); glUniform4f(uLR, x, y, h * 6.52f, h); glUniform2f(uLScr, (float)FW, (float)FH); glUniform1f(uLPad, pad); glUniform1f(uLT, menuTime()); glUniform1f(uLA, a);
  glBindVertexArray(emptyVAO); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); glUseProgram(pHud); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
// LB/LT and RB/RT flip through a row of tabs (buttons baseId .. baseId+n-1); 'skip' is a hidden tab. The selection follows if it was on a tab.
bool uiTabStep(int& tab, int n, int baseId, int skip) {
  if (!padTab) return false; int t = tab; do t = (t + padTab + n) % n; while (t == skip && t != tab);
  if (t == tab) return false; if (padNav && uiFocus == baseId + tab) uiFocus = baseId + t; tab = t; padTab = 0; audioEvent(S_CLICK); return true;
}
