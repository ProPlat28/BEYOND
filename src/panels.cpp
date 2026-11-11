// Windows that freeze the game: market, workbench, death screen, and the encyclopedia of the pause menu.
#include "game.h"

void openModal(int m) { modal = m; modalT = 0; uiFlushMouse(); syncModes(); if (m) audioEvent(S_CLICK); }
void closeModal() { modal = 0; uiFlushMouse(); syncModes(); }
static void dim(float a) { Col d{.004f, .008f, .025f, a}; float s = uiA; uiA = 1; uiBox(0, 0, (float)FW, (float)FH, 0, d, d); uiA = s; }
static void cargo(float x, float y, float w) {
  float u = uiU, cx = x; uiText("CARGO", x, y, uiSmall(), UI_DIM); y += 20 * u; bool any = false;
  for (int i = 0; i < NRES; i++) if (inv[i]) { std::string t = fmt("%s %d", RES_NAME[i], inv[i]); float tw = textW(t, uiSmall()) + 16 * u; if (cx + tw > x + w) { cx = x; y += 20 * u; } uiText(t, cx, y, uiSmall(), RES_COL[i] * .5f + V3{.5f, .5f, .5f}); cx += tw; any = true; }
  if (!any) uiText("nothing yet - mine glowing ore nodes with E", x, y, uiSmall(), UI_DIM);
}
// ---------------------------------------------------------------- market: GOODS (ore swaps, rare finds), FOOD (dishes, not on easy), TOOLS (pickaxe, sword, medpack)
static bool payable(const std::vector<Cost>& c) { for (auto& k : c) if (inv[k.res] < k.n) return false; return true; }
static std::string priceText(const std::vector<Cost>& c) { std::string s = "Needs: "; for (size_t i = 0; i < c.size(); i++) s += fmt("%s%d %s (You have %d)", i ? " + " : "", c[i].n, RES_NAME[c[i].res], inv[c[i].res]); return s; }
static void pay(const std::vector<Cost>& c) { for (auto& k : c) inv[k.res] -= k.n; audioEvent(S_CHIME); saveReq = true; }
static void marketPanel() {
  static int tab = 0; Planet& p = planets[curP]; Offer o[NOFFER]; marketOffers(p, o); int key = pkey(p), r0 = resOf(p, 0), r1 = resOf(p, 1); if (difficulty == 0 && tab == 1) tab = 0;
  float u = uiU, pw = std::min(FW - 40 * u, 1040.f * u), ph = std::min(FH - 20 * u, 640.f * u), px = (FW - pw) * .5f, py = (FH - ph) * .5f, pad = 26 * u;
  uiPanel(px, py, pw, ph); uiText("MARKET", px + pad, py + 22 * u, uiS * 2, UI_TXT); uiTextFit(modalWho + "  -  " + p.name, px + pad, py + 22 * u + 7 * uiS * 2 + 10 * u, pw * .45f, uiS, UI_GOLD);
  uiTabStep(tab, 3, 530, difficulty == 0 ? 1 : -1);
  { static const char* TN[3] = {"GOODS", "FOOD", "TOOLS"}; float tw = 130 * u, tx = px + pw - pad - 3 * (tw + 8 * u) + 8 * u; for (int i = 0; i < 3; i++) { if (i == 1 && difficulty == 0) continue; if (uiButton(530 + i, tx + i * (tw + 8 * u), py + 22 * u, tw, 38 * u, TN[i], i == tab ? BTN_PRIMARY : BTN_NORMAL)) tab = i; } }
  float y = py + 100 * u, rh = 68 * u; int row = 0;
  auto line = [&](const std::string& title, V3 tc, const std::string& sub, bool subOk, const std::string& btn, bool en) {
    float by = y + (rh - 8 * u - 38 * u) * .5f; uiBox(px + pad, y, pw - pad * 2, rh - 8 * u, 9 * u, {.075f, .105f, .185f, .55f}, {.04f, .055f, .105f, .55f}, {.5f, .72f, 1, .12f}, std::max(1.f, u));
    float mw = pw - pad * 2 - 200 * u; uiTextFit(title, px + pad + 16 * u, y + 10 * u, mw, uiS, tc); uiTextFit(sub, px + pad + 16 * u, y + 10 * u + 7 * uiS + 8 * u, mw, uiS, subOk ? V3{.6f, 1, .7f} : UI_RED);
    bool c = uiButton(540 + row, px + pw - pad - 16 * u - 160 * u, by, 160 * u, 38 * u, btn, BTN_PRIMARY, en); y += rh; row++; return c; };
  if (tab == 0) {
    uiText("Swap ore with the merchant: your local ore buys goods from other worlds.", px + pad, y - 18 * u, uiSmall(), UI_DIM);
    for (int k = 0; k < NOFFER; k++) { const Offer& f = o[k]; std::vector<Cost> c{{f.gr, f.gn}}; bool ok = payable(c);
      if (line(RES_NAME[f.rr], RES_COL[f.rr] * .55f + V3{.45f, .45f, .45f}, priceText(c), ok, "TRADE", ok)) { pay(c); inv[f.rr] += f.rn; } }
    int foreign = -1; for (auto& kv : rares) if (kv.second > 0 && kv.first != key) { foreign = kv.first; break; }   // they trade for rare finds from other worlds, never their own
    std::string rn = foreign >= 0 ? rareName(foreign) : "a rare find from another world";
    if (line(RES_NAME[r0], {1, .85f, .5f}, "for 1 x " + rn + fmt(" (You have %d)", foreign >= 0 ? rares[foreign] : 0), foreign >= 0, "TRADE", foreign >= 0)) { itemAdd(IT_RARE + foreign, -1); inv[r0] += 1; audioEvent(S_CHIME); saveReq = true; }
  } else if (tab == 1) {
    uiText("Market dishes fill you far more than anything you can pick in the wild.", px + pad, y - 18 * u, uiSmall(), UI_DIM);
    for (int k = 0; k < 2; k++) { int id = key * 4 + 2 + k; std::vector<Cost> c = k ? std::vector<Cost>{{r0, 1}, {r1, 2}} : std::vector<Cost>{{r0, 2}}; bool ok = payable(c);
      if (line(fmt("%s (+%d food)", foodName(id).c_str(), foodValue(id)), foodColor(id) * .5f + V3{.5f, .5f, .5f}, priceText(c), ok, "BUY", ok)) { pay(c); itemAdd(IT_FOOD + id, 1); } }
    uiText(fmt("Food %d/20   -   you carry %d food   -   press %s to eat", (int)std::ceil(food), foodTotal(), keysIn("{EAT}").c_str()), px + pad, y + 6 * u, uiSmall(), UI_DIM);
  } else {
    { int t = pickTier(key); std::vector<Cost> c{{r0, 2 + 2 * t}, {r1, 1 + t}}; bool own = pickaxes.count(key) > 0, ok = payable(c);
      if (line(fmt("PICKAXE - %s (tier %d)", pickName(key).c_str(), t), pickColor(key) * .5f + V3{.5f, .5f, .5f}, (own ? std::string("You own it. Pickaxes mine tougher ore and dig out more.") : priceText(c)), own || ok, own ? "OWNED" : "BUY", !own && ok)) { pay(c); pickaxes.insert(key); toast("NEW PICKAXE\n" + pickName(key) + fmt("\nTier %d - your best is now tier %d", t, bestPickTier()), 4); } }
    { int d = swordDamage(key); std::vector<Cost> c{{r0, 3 + d}, {r1, 2 + d}}; bool own = swords.count(key) > 0, ok = payable(c), eq = swordSel == key;
      if (line("SWORD - " + swordName(key), swordColor(key) * .5f + V3{.5f, .5f, .5f}, own ? (eq ? std::string("Equipped. Strike the wicked with ") + keysIn("{ATK}") + " or left click." : std::string("You own it.")) : priceText(c), own || ok, own ? (eq ? "EQUIPPED" : "EQUIP") : "BUY", own ? !eq : ok)) {
        if (!own) { pay(c); swords.insert(key); toast("NEW SWORD\n" + swordName(key) + std::string("\nEach hit takes 1-2 lives - strike with {ATK} or left click"), 4); } swordSel = key; saveReq = true; } }
    { std::vector<Cost> c{{r0, 8}, {r1, 6}}; bool ok = payable(c);
      if (line(std::string("MEDPACK - gets you back up instead of dying"), {1, .6f, .6f}, priceText(c), ok, "BUY", ok)) { pay(c); meds++; } }
    std::string tl = toolsLine(); uiTextFit(tl.empty() ? "You carry no tools yet." : tl, px + pad, y + 6 * u, pw - pad * 2, uiSmall(), UI_DIM);
  }
  cargo(px + pad, py + ph - 108 * u, pw - pad * 2 - 170 * u); if (uiButton(549, px + pw - pad - 150 * u, py + ph - 64 * u, 150 * u, 44 * u, "CLOSE")) closeModal();
}
// ---------------------------------------------------------------- chest: two grids of item slots, like a storage box. Click moves the stack, SHIFT+click moves one.
static bool slotWidget(int id, float x, float y, float s, int it, int n, std::string& hover) {
  if (uiInput) uiRects.push_back({x, y, s, s, id, false});   // the controller can select slots like buttons
  float u = uiU; bool over = uiOver(x, y, s, s), click = false; if (over && uiPressed) uiActive = id; if (uiReleased && uiActive == id && over) click = true;
  uiBox(x, y, s, s, 8 * u, over ? Col{.16f, .22f, .36f, .9f} : Col{.075f, .105f, .185f, .75f}, over ? Col{.1f, .14f, .25f, .9f} : Col{.04f, .055f, .105f, .75f}, {.5f, .72f, 1, over ? .5f : .14f}, std::max(1.f, u));
  if (it >= 0) { V3 c = itemColor(it); float ic = s * .42f; bool round = it >= IT_FOOD; uiBox(x + (s - ic) * .5f, y + s * .14f, ic, ic, round ? ic * .5f : 4 * u, {c.x, c.y, c.z, 1}, {c.x * .5f, c.y * .5f, c.z * .5f, 1}, {1, 1, 1, .2f}, std::max(1.f, u));
    std::string cn = fmt("%d", n); uiText(cn, x + s - 6 * u - textW(cn, uiSmall()), y + 5 * u, uiSmall(), UI_TXT); uiTextFitC(itemLabel(it), x + s * .5f, y + s - 7 * uiSmall() - 6 * u, s - 6 * u, uiSmall(), UI_DIM);
    if (over) hover = itemLabel(it) + fmt("  x%d", n); }
  if (click) { uiActive = 0; uiPressed = uiReleased = false; audioEvent(S_CLICK); }
  return click && it >= 0;
}
static void chestPanel() {
  static int scrollL = 0, scrollR = 0; std::map<int, int>& ch = chests[chestOpen]; for (auto it = ch.begin(); it != ch.end();) { if (it->second <= 0) it = ch.erase(it); else ++it; }
  float u = uiU, pw = std::min(FW - 30 * u, 1000.f * u), ph = std::min(FH - 20 * u, 620.f * u), px = (FW - pw) * .5f, py = (FH - ph) * .5f, pad = 24 * u; uiPanel(px, py, pw, ph);
  uiText("CHEST", px + pad, py + 20 * u, uiS * 2, UI_TXT); uiText(padActive ? padName(GLFW_GAMEPAD_BUTTON_A) + " moves the whole stack   -   hold " + padName(GLFW_GAMEPAD_BUTTON_X) + " to move just one" : std::string( "Click an item to move the whole stack   -   hold SHIFT to move just one"), px + pad, py + 20 * u + 7 * uiS * 2 + 10 * u, uiSmall(), UI_DIM);
  bool one = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS || padDown[GLFW_GAMEPAD_BUTTON_X]; std::string hover;
  float gx = px + pad, gy = py + 96 * u, gw = (pw - pad * 3) * .5f, gh = ph - 96 * u - 84 * u, sl = 86 * u, g = 8 * u; int cols = std::max(1, (int)((gw + g) / (sl + g))), rows = std::max(1, (int)((gh - 26 * u + g) / (sl + g)));
  std::vector<std::pair<int, int>> L(ch.begin(), ch.end()), R = playerItems(); int moveL = -1, moveR = -1;
  auto grid = [&](float x0, const char* title, std::vector<std::pair<int, int>>& v, int& scroll, int idBase, int& moved) {
    uiText(fmt("%s  (%d)", title, (int)v.size()), x0, gy, uiS, UI_GOLD); float y0 = gy + 26 * u; int total = (int)((v.size() + cols - 1) / cols) + 1, mx = std::max(0, total - rows);
    if (uiOver(x0, y0, gw, gh) && uiInput) { int st2 = (int)uiWheel; scroll -= st2; uiWheel -= (float)st2; } scroll = std::min(std::max(scroll, 0), mx);
    for (int r = 0; r < rows; r++) for (int c = 0; c < cols; c++) { int i = (r + scroll) * cols + c; bool has = i < (int)v.size();
      if (slotWidget(idBase + r * cols + c, x0 + c * (sl + g), y0 + r * (sl + g), sl, has ? v[i].first : -1, has ? v[i].second : 0, hover)) moved = i; }
  };
  grid(gx, "IN THE CHEST", L, scrollL, 1000, moveL); grid(gx + gw + pad, "YOUR BAG", R, scrollR, 1400, moveR);
  if (moveL >= 0) { int it = L[moveL].first, n = one ? 1 : L[moveL].second; ch[it] -= n; if (ch[it] <= 0) ch.erase(it); itemAdd(it, n); saveReq = true; }
  if (moveR >= 0) { int it = R[moveR].first, n = one ? 1 : R[moveR].second; itemAdd(it, -n); ch[it] += n; saveReq = true; }
  float fy = py + ph - 62 * u, fh = 44 * u; uiTextFit(hover.empty() ? std::string("Tools (swords, pickaxes) stay with you.") : hover, px + pad, fy + (fh - 7 * uiS) * .5f, pw - pad * 2 - 520 * u, uiS, hover.empty() ? UI_DIM : UI_TXT);
  if (uiButton(1900, px + pw - pad - 150 * u - 10 * u - 170 * u - 10 * u - 170 * u, fy, 170 * u, fh, "TAKE ALL", BTN_NORMAL, !ch.empty())) { for (auto& kv : ch) itemAdd(kv.first, kv.second); ch.clear(); audioEvent(S_CHIME); saveReq = true; }
  if (uiButton(1901, px + pw - pad - 150 * u - 10 * u - 170 * u, fy, 170 * u, fh, "STORE ALL", BTN_NORMAL, !R.empty())) { for (auto& kv : R) { itemAdd(kv.first, -kv.second); ch[kv.first] += kv.second; } audioEvent(S_CHIME); saveReq = true; }
  if (uiButton(1902, px + pw - pad - 150 * u, fy, 150 * u, fh, "CLOSE")) closeModal();
}
// ---------------------------------------------------------------- workbench
static void costLine(float x, float y, const Cost* c, int n, int prizes) {
  float u = uiU, x0 = x, lim = x0 + std::min(FW - 40 * u, 900.f * u) - 260 * u; for (int i = 0; i < n; i++) { std::string t = fmt("%d %s", c[i].n, RES_NAME[c[i].res]); bool ok = inv[c[i].res] >= c[i].n; if (x + textW(t, uiSmall()) > lim) { x = x0; y += 15 * u; } uiText(t, x, y, uiSmall(), ok ? V3{.6f, 1, .7f} : UI_RED); x += textW(t, uiSmall()) + 14 * u; }
  if (prizes) { bool ok = prizesCount() >= prizes; std::string t = fmt("%d elder prizes", prizes); if (x + textW(t, uiSmall()) > lim) { x = x0; y += 15 * u; } uiText(t, x, y, uiSmall(), ok ? V3{.6f, 1, .7f} : UI_RED); }
}
static void craftPanel() {
  float u = uiU, pw = std::min(FW - 40 * u, 900.f * u), ph = 620 * u, px = (FW - pw) * .5f, py = (FH - ph) * .5f, pad = 26 * u; uiPanel(px, py, pw, ph);
  uiText("WORKBENCH", px + pad, py + 20 * u, uiS * 2, UI_TXT); uiText(fmt("Equipped: %s   (speed x%.2f)", ROCKET[rocketSel].name, ROCKET[rocketSel].speed / ROCKET[0].speed), px + pad, py + 20 * u + 7 * uiS * 2 + 8 * u, uiS, UI_GOLD);
  float y = py + 92 * u, rh = 72 * u;
  for (int id = 1; id < 6; id++, y += rh) {
    uiBox(px + pad, y, pw - pad * 2, rh - 6 * u, 9 * u, {.075f, .105f, .185f, .55f}, {.04f, .055f, .105f, .55f}, {.5f, .72f, 1, .12f}, std::max(1.f, u)); float tx = px + pad + 14 * u, by = y + (rh - 6 * u - 38 * u) * .5f;
    const RocketDef& r = ROCKET[id]; bool own = (rocketMask >> id) & 1;
    uiText(fmt("%s   speed x%.2f", r.name, r.speed / ROCKET[0].speed), tx, y + 9 * u, uiS, id == 5 ? V3{1, .85f, .4f} : UI_TXT); uiTextFit(r.desc, tx, y + 9 * u + 7 * uiS + 6 * u, pw - 260 * u, uiSmall(), UI_DIM);
    if (!own) costLine(tx, y + 9 * u + 7 * uiS + 6 * u + 7 * uiSmall() + 5 * u, r.cost, r.nc, r.needPrizes);
    float bx = px + pw - pad - 14 * u - 150 * u; if (!own) { if (uiButton(560 + id, bx, by, 150 * u, 38 * u, "CRAFT", BTN_PRIMARY, canCraft(id))) craftRocket(id); }
    else if (rocketSel == id) uiButton(560 + id, bx, by, 150 * u, 38 * u, "EQUIPPED", BTN_NORMAL, false); else if (uiButton(560 + id, bx, by, 150 * u, 38 * u, "EQUIP")) { rocketSel = id; audioEvent(S_CLANG); saveReq = true; } }
  cargo(px + pad, py + ph - 100 * u, pw - pad * 2 - 170 * u); if (uiButton(559, px + pw - pad - 150 * u, py + ph - 66 * u, 150 * u, 44 * u, "CLOSE")) closeModal();
}
// ---------------------------------------------------------------- death screen
static void deathPanel() {
  float u = uiU, a = clampf(modalT * .9f, 0, 1); Col red{.28f, .01f, .02f, .86f * a}; { float s = uiA; uiA = 1; uiBox(0, 0, (float)FW, (float)FH, 0, red, {.03f, 0, 0, .95f * a}); uiA = s; }
  uiA = a; uiTextC("YOU DIED", FW * .5f, FH * .26f, uiS * 5, {1, .25f, .22f}); uiTextC(modalWho.empty() ? "You were overwhelmed." : modalWho == "starvation" ? "You starved." : modalWho == "a long fall" ? "You fell too far." : "Struck down by " + modalWho, FW * .5f, FH * .26f + 7 * uiS * 5 + 18 * u, uiLarge(), UI_TXT);
  uiTextC(fmt("Times fallen: %d   (medpacks stand you back up on your own)", deaths), FW * .5f, FH * .26f + 7 * uiS * 5 + 70 * u, uiSmall(), UI_DIM);
  float bw = 380 * u, bh = 50 * u, bx = (FW - bw) * .5f, by = FH * .56f;
  if (modalT > .8f) { if (uiButton(600, bx, by, bw, bh, "RESPAWN", BTN_PRIMARY)) { closeModal(); pendingRespawn = true; }
    if (uiButton(601, bx, by + bh + 12 * u, bw, bh, "SAVE AND GO TO MAIN MENU")) { closeModal(); pendingRespawn = true; pendingMenu = true; } }
  uiA = 1;
}
void modalFrame(float dt) {
  if (FW < 1 || FH < 1) return; modalT += dt; uiBegin(dt); 
  if (modal != M_DEATH) dim(.66f);
  switch (modal) { case M_MARKET: marketPanel(); break; case M_CRAFT: craftPanel(); break; case M_DEATH: deathPanel(); break; case M_CHEST: chestPanel(); break; }
  uiWheel = 0;   // after the panels: they use the scroll of this frame
  uiEnd();
}
// ---------------------------------------------------------------- encyclopedia (pause menu): rockets, prizes, planets, moons, species, resources
namespace { struct ERow { std::string t, s, tag; V3 col, tagCol; }; }
bool encyclopediaPanel() {
  syncPSaves(); static int tab = 0, scroll = 0; static const char* TN[6] = {"ROCKETS", "PRIZES", "PLANETS", "MOONS", "SPECIES", "ITEMS"};
  std::vector<ERow> rows; V3 ok{.6f, 1, .7f};
  if (tab == 0) for (int i = 0; i < 6; i++) { bool own = (rocketMask >> i) & 1; rows.push_back({std::string(ROCKET[i].name) + fmt("   speed x%.2f", ROCKET[i].speed / ROCKET[0].speed), own ? std::string(ROCKET[i].desc) : "Needs: " + costText(i), own ? (rocketSel == i ? "EQUIPPED" : "OWNED") : "NOT BUILT", ROCKET[i].hull, own ? ok : UI_DIM}); }
  else if (tab == 1) { for (auto& kv : psaves) if (kv.second.vmask & VM_PRIZE) rows.push_back({prizeName(kv.second.uniq, kv.second.moon), prizeDesc(kv.second.uniq) + "  From " + kv.second.name + ", gift of the " + cultureOf(kv.second.uniq) + " elder", "COLLECTED", {1, .85f, .4f}, ok}); }
  else if (tab == 2 || tab == 3) { bool mn = tab == 3; for (auto& kv : psaves) { const PSave& q = kv.second; if (q.moon != mn || !(q.found || (q.vmask & VM_VISIT))) continue;
      rows.push_back({q.name, std::string(KL[((q.kind % 16) + 16) % 16]) + "  |  " + sysInfo(q.sys).name + "  |  " + (q.found ? "all quests done" : fmt("quests %d/%d", q.qi, mn ? 3 : 5)), q.found ? "DISCOVERED" : "VISITED", lin(hsl(q.uniq * .0618f, .5f, .55f)), q.found ? ok : UI_GOLD}); } }
  else if (tab == 4) { for (auto& kv : psaves) if (kv.second.vmask & VM_MET) rows.push_back({"The " + cultureOf(kv.second.uniq) + " people", alienDesc(kv.second.uniq, kv.second.moon) + "  |  home: " + kv.second.name, "MET", alienOf(kv.second.uniq, kv.second.moon).skin * 1.5f, ok}); }
  else { for (int i = 0; i < NRES; i++) rows.push_back({RES_NAME[i], i < 16 ? std::string("mined on ") + KL[i] + "s, or traded for" : "mined on moons, or traded for", fmt("x %d", inv[i]), RES_COL[i], inv[i] ? ok : UI_DIM});
    for (auto& kv : rares) rows.push_back({rareName(kv.first), "rare find - only its own people's pickaxe can mine it; any other market trades for it", fmt("x %d", kv.second), rareColor(kv.first), ok});
    for (int k : swords) rows.push_back({swordName(k), std::string("sword, 1-2 damage per hit"), swordSel == k ? "EQUIPPED" : "OWNED", swordColor(k), ok});
    for (int k : pickaxes) rows.push_back({pickName(k), fmt("pickaxe, tier %d", pickTier(k)), "OWNED", pickColor(k), ok});
    for (auto& kv : foods) rows.push_back({foodName(kv.first), fmt("food, +%d when eaten", foodValue(kv.first)), fmt("x %d", kv.second), foodColor(kv.first), ok}); }
  float u = uiU, pw = std::min(FW - 40 * u, 940.f * u), ph = std::min(FH - 30.f * u, 660.f * u), px = (FW - pw) * .5f, py = (FH - ph) * .5f, pad = 24 * u; bool back = false; uiPanel(px, py, pw, ph);
  uiText("ENCYCLOPEDIA", px + pad, py + 18 * u, uiS * 2, UI_TXT); int np = 0, nm = 0; for (auto& kv : psaves) if (kv.second.found) (kv.second.moon ? nm : np)++;
  std::string hd = fmt("%d planets   %d moons   %d prizes   %d fallen", np, nm, prizesCount(), deaths); uiText(hd, px + pw - pad - textW(hd, uiS), py + 22 * u, uiS, UI_GOLD);
  if (uiTabStep(tab, 6, 700)) scroll = 0;
  float tw = (pw - pad * 2) / 6; for (int i = 0; i < 6; i++) if (uiButton(700 + i, px + pad + i * tw, py + 62 * u, tw - 6 * u, 36 * u, TN[i], i == tab ? BTN_PRIMARY : BTN_NORMAL)) { tab = i; scroll = 0; }
  float ly = py + 112 * u, rh = 58 * u, listH = ph - 112 * u - 74 * u; int vis = std::max(1, (int)(listH / rh)), n = (int)rows.size(), mx = std::max(0, n - vis);
  if (uiInput) { int st2 = (int)uiWheel; scroll -= st2; uiWheel -= (float)st2; } scroll = std::min(std::max(scroll, 0), mx);
  if (n == 0) uiTextC("Nothing here yet. Explore, finish quests and come back.", px + pw * .5f, ly + listH * .4f, uiS, UI_DIM);
  for (int i = scroll; i < std::min(n, scroll + vis); i++) { const ERow& r = rows[i]; float ry = ly + (i - scroll) * rh;
    uiBox(px + pad, ry, pw - pad * 2, rh - 6 * u, 8 * u, {.075f, .105f, .185f, .5f}, {.04f, .055f, .105f, .5f}, {.5f, .72f, 1, .1f}, std::max(1.f, u)); float ic = 28 * u;
    uiBox(px + pad + 12 * u, ry + (rh - 6 * u - ic) * .5f, ic, ic, ic * .5f, {r.col.x, r.col.y, r.col.z, 1}, {r.col.x * .5f, r.col.y * .5f, r.col.z * .5f, 1}, {1, 1, 1, .15f}, std::max(1.f, u));
    float tx = px + pad + 54 * u, maxw = pw - pad * 2 - 54 * u - 150 * u; uiTextFit(r.t, tx, ry + 8 * u, maxw, uiS, UI_TXT); uiTextFit(r.s, tx, ry + 8 * u + 7 * uiS + 6 * u, maxw, uiSmall(), UI_DIM);
    uiText(r.tag, px + pw - pad - 14 * u - textW(r.tag, uiS), ry + (rh - 6 * u - 7 * uiS) * .5f, uiS, r.tagCol); }
  if (n > vis) uiText(fmt("%d-%d of %d  (mouse wheel)", scroll + 1, std::min(n, scroll + vis), n), px + pad, py + ph - 62 * u, uiSmall(), UI_DIM);
  if (uiButton(799, px + pw - pad - 150 * u, py + ph - 62 * u, 150 * u, 44 * u, "BACK")) back = true; return back;
}
