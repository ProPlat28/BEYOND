// Developer tools (F1), for one world only: "Developer Mode", made with --make-developer-world.
//
// How a world is allowed to have them: its world.txt carries a "dev" tag, a keyed hash of the world's folder number, seed and
// creation time. The game checks the tag every time the world list is read; the world's NAME plays no part, so a world someone
// creates and calls "Developer Mode" (or anything else) gets nothing. A copied world gets a new folder number and creation time,
// so its copied tag no longer matches. Renaming the real one keeps its tools.
// Release builds (BEYOND_PORTABLE) leave all of this out: no F1, no tag check, no --make-developer-world.
#include "game.h"

#ifdef BEYOND_DEVTOOLS
static const unsigned long long DEV_SECRET[4] = {0x9e3779b97f4a7c15ull, 0xd1b54a32d192ed03ull, 0x8cb92ba72f3d8dd7ull, 0x5be0cd19137e2179ull};
static unsigned long long mix64(unsigned long long z) { z += 0x9e3779b97f4a7c15ull; z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull; z = (z ^ (z >> 27)) * 0x94d049bb133111ebull; return z ^ (z >> 31); }
std::string devTagFor(int id, unsigned seed, long long created) {
  unsigned long long h = DEV_SECRET[0];
  for (unsigned long long v : {(unsigned long long)id, (unsigned long long)seed, (unsigned long long)created, DEV_SECRET[1]}) for (int r = 0; r < 4; r++) h = mix64(h ^ v ^ DEV_SECRET[(r + 2) & 3]);
  char b[40]; std::snprintf(b, sizeof b, "%016llx%016llx", h, mix64(h ^ DEV_SECRET[3])); return b;
}
bool devAllowed(const World& w) { return !w.devTag.empty() && w.devTag == devTagFor(w.id, w.seed, w.created); }
#else
std::string devTagFor(int, unsigned, long long) { return ""; }
bool devAllowed(const World&) { return false; }
#endif

void devReset() { devWorld = devPanel = devNoclip = devFlyOn = devGod = devFast = false; }
void toggleDevPanel() {
  if (!devWorld || app != A_GAME || modal || cine || endT >= 0) return;
  if (devPanel) { devPanel = false; setPaused(false); } else { setPaused(true); devPanel = true; }
}

// fly: move where you look with no gravity (Space up, Ctrl/C down, Shift fast). FLY still bumps into walls, boulders and the ground,
// NOCLIP goes through everything. The camera follows, like walking.
bool devFly(float dt, float (*gh)(float, float), float bound) {
  if (!devWorld || !devFlyOn || (st != EARTH && st != SURFACE)) return false;   // NOCLIP alone walks (see walk()); with FLY it flies through everything
  P.yaw -= (float)mdx * .0022f; P.pitch = clampf(P.pitch - (float)mdy * .0022f, -1.5f, 1.5f); mdx = mdy = 0;
  float fz = clampf((float)(held(ACT_FORWARD) - held(ACT_BACK)) + padMove[1], -1, 1), fx = clampf((float)(held(ACT_RIGHT) - held(ACT_LEFT)) + padMove[0], -1, 1);
  float up = (float)held(ACT_JUMP) - (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS ? 1.f : 0.f), sp = held(ACT_RUN) ? 60.f : 16.f;
  float cy = std::cos(P.pitch), sy = std::sin(P.pitch), s = std::sin(P.yaw), c = std::cos(P.yaw);
  V3 f{-s * cy, sy, -c * cy}, r{c, 0, -s}, v = f * fz + r * fx + V3{0, up, 0}; float l = len(v); if (l > 1) v = v * (1 / l);
  P.x += v.x * sp * dt; P.y += v.y * sp * dt; P.z += v.z * sp * dt; P.vx = P.vz = P.vy = 0;
  if (!devNoclip) {   // fly: solid world
    resolveHits(); float d = std::hypot(P.x, P.z); if (d > bound) { P.x *= bound / d; P.z *= bound / d; }
    float gy = std::max(gh(P.x, P.z), hitFloor(P.x, P.z, P.y)); if (P.y < gy) P.y = gy; }
  camPos = {P.x, P.y + 1.75f, P.z}; basis(P.yaw, P.pitch, 0, cR, cU, cF); fov = 72; nearZ = .3f; return true;
}

static void devToast(const std::string& s) { toast("DEV\n" + s, 2.5f); audioEvent(S_CLICK); }
static int nearestUnexplored() { int b = -1; float bd = 1e30f; for (int i = 0; i < (int)planets.size(); i++) { const Planet& p = planets[i]; if (!p.enterable || p.found) continue; float d = len(p.pos - S.pos); if (d < bd) { bd = d; b = i; } } return b; }

// the panel, drawn over the paused game
void devFrame(float dt) {
  if (FW < 1 || FH < 1) return; audioSet(0, 0, 100, 0); uiBegin(dt); uiWheel = 0; float u = uiU; { Col dim{.004f, .008f, .025f, .72f}; uiBox(0, 0, (float)FW, (float)FH, 0, dim, dim); }
  const int ROWS = 9; float bw = 300 * u, bh = 42 * u, gap = 10 * u, w = bw * 2 + gap + 72 * u, h = 118 * u + (bh + gap) * ROWS + 50 * u, x = (FW - w) * .5f, y = (FH - h) * .5f, c0 = x + 36 * u, c1 = c0 + bw + gap, by = y + 112 * u;
  uiPanel(x, y, w, h); uiTextC("DEVELOPER TOOLS", FW * .5f, y + 28 * u, uiS * 2.6f, UI_GOLD); uiTextC("F1 or Esc closes   -   only this world has them", FW * .5f, y + 28 * u + 7 * uiS * 2.6f + 12 * u, uiSmall(), UI_DIM);
  int endReq = -1; bool closeReq = false;   // acted on after uiEnd(): leaving early would skip it and leave the colors wrong (sRGB off)
  bool ground = st == EARTH || st == SURFACE, surf = st == SURFACE && curP >= 0 && !inRoom, space = st == SPACE;
  auto onoff = [](const char* n, bool v) { return std::string(n) + (v ? ": ON" : ": OFF"); };
  float yy = by;
  // left column: toggles and player
  if (uiButton(900, c0, yy, bw, bh, onoff("FLY", devFlyOn), devFlyOn ? BTN_PRIMARY : BTN_NORMAL, ground)) { devFlyOn = !devFlyOn; P.vy = 0; devToast(devFlyOn ? (devNoclip ? "Fly on - through everything (noclip is on)" : "Fly on - Space up, Ctrl down, Shift fast (walls stay solid)") : "Fly off"); } yy += bh + gap;
  if (uiButton(908, c0, yy, bw, bh, onoff("NOCLIP", devNoclip), devNoclip ? BTN_PRIMARY : BTN_NORMAL, ground)) { devNoclip = !devNoclip; devToast(devNoclip ? (devFlyOn ? "Noclip on - flying through everything" : "Noclip on - walk through walls, trees and houses") : "Noclip off"); } yy += bh + gap;
  if (uiButton(901, c0, yy, bw, bh, onoff("GOD MODE", devGod), devGod ? BTN_PRIMARY : BTN_NORMAL)) { devGod = !devGod; devToast(devGod ? "God mode on - no damage, no hunger" : "God mode off"); } yy += bh + gap;
  if (uiButton(902, c0, yy, bw, bh, onoff("ROCKET x10 SPEED", devFast), devFast ? BTN_PRIMARY : BTN_NORMAL)) { devFast = !devFast; devToast(devFast ? "Rocket speed x10" : "Rocket speed normal"); } yy += bh + gap;
  if (uiButton(903, c0, yy, bw, bh, "HEAL AND FEED")) { hp = MAXHP; food = 20; starving = false; devToast("Lives and food full"); } yy += bh + gap;
  if (uiButton(904, c0, yy, bw, bh, "+99 OF EVERY RESOURCE")) { for (int r = 0; r < NRES; r++) inv[r] += 99; meds += 5; saveReq = true; devToast("+99 of every resource, +5 meds"); } yy += bh + gap;
  if (uiButton(905, c0, yy, bw, bh, "ALL ROCKETS")) { rocketMask = 63; saveReq = true; devToast("Every rocket unlocked - equip them at a workbench"); } yy += bh + gap;
  if (uiButton(906, c0, yy, bw, bh, "BEST SWORD AND PICKAXE")) { int sk = -1, pk = -1; for (int k = 0; k < 256; k++) { if (sk < 0 && swordDamage(k) == 3 && swordShape(k) == 3) sk = k; if (pk < 0 && pickTier(k) == 3) pk = k; }
    if (sk >= 0) { swords.insert(sk); swordSel = sk; } if (pk >= 0) pickaxes.insert(pk); saveReq = true; devToast("Best sword equipped, best pickaxe in the bag"); } yy += bh + gap;
  if (uiButton(907, c0, yy, bw, bh, "EARTH: ALL ROCKET PARTS", BTN_NORMAL, st == EARTH)) { for (auto& p : picks) p.got = p.inst = true; gotN = instN = 5; saveReq = true; devToast("Rocket built - stand on the pad and press E"); }
  // right column: the world you are on
  yy = by;
  if (uiButton(910, c1, yy, bw, bh, "FINISH THIS QUEST STEP", BTN_NORMAL, surf)) { Planet& p = planets[curP]; if (p.qi < nqOf(p) && p.qph == 1) { p.prog = std::max(p.prog, nObjFor(p, qtype(p, p.qi))); p.qph = 2; for (auto& o : objs) o.done = true; saveReq = true; devToast("Step done - go back to the elder"); } else devToast(p.qph == 0 ? "Talk to the elder first to start a quest" : "Nothing to finish - talk to the elder"); } yy += bh + gap;
  if (uiButton(911, c1, yy, bw, bh, "DISCOVER THIS WORLD", BTN_NORMAL, surf)) { Planet& p = planets[curP]; if (!p.found) { p.found = true; p.qi = nqOf(p); p.qph = 0; if (!p.isMoon) foundN++; saveReq = true; devToast(p.name + " discovered"); } else devToast("Already discovered"); } yy += bh + gap;
  if (uiButton(912, c1, yy, bw, bh, "DEFEAT NEARBY WICKED", BTN_NORMAL, surf)) { Planet& p = planets[curP]; int n = 0; for (auto& q : npcs) if (q.wicked && !q.dead && len(q.pos - V3{P.x, q.pos.y, P.z}) < 80) { q.dead = true; n++; if (q.widx >= 0 && q.widx < 8) p.vmask |= 1u << (5 + q.widx); } saveReq = true; devToast(fmt("%d wicked defeated", n)); } yy += bh + gap;
  if (uiButton(913, c1, yy, bw, bh, "TELEPORT TO NEXT UNEXPLORED", BTN_NORMAL, space)) { int i = nearestUnexplored(); if (i >= 0) { const Planet& p = planets[i]; V3 dv = norm(S.pos - p.pos); if (len(dv) < .5f) dv = {0, 0, 1}; S.pos = p.pos + dv * (p.R * 2.6f + 200.f); S.yaw = std::atan2(dv.x, dv.z); S.pitch = std::asin(clampf(-dv.y, -1, 1)); S.speed = 0; S.thr = 0; devToast("At " + p.name); } else devToast("Nothing left to explore"); } yy += bh + gap;
  if (uiButton(914, c1, yy, bw, bh, "DISCOVER ALL BUT ONE", BTN_NORMAL, space)) { int keep = nearestUnexplored(); for (int i = 0; i < (int)planets.size(); i++) { Planet& p = planets[i]; if (!p.enterable || p.found || i == keep) continue; p.found = true; p.qi = nqOf(p); if (!p.isMoon) foundN++; } saveReq = true; devToast("Everything discovered except the nearest world"); } yy += bh + gap;
  if (uiButton(915, c1, yy, bw, bh, "PLAY PLANETS ENDING", BTN_NORMAL, space)) { endReq = 0; } yy += bh + gap;
  if (uiButton(916, c1, yy, bw, bh, "PLAY MOONS ENDING", BTN_NORMAL, space)) { endReq = 1; } yy += bh + gap;
  if (uiButton(917, c1, yy, bw, bh, "CLOSE", BTN_PRIMARY)) closeReq = true;
  uiTextC(fmt("%s%s   -   X %.0f  Y %.0f  Z %.0f", st == SPACE ? "SPACE" : st == SURFACE ? (curP >= 0 ? planets[curP].name.c_str() : "SURFACE") : "EARTH", st == SURFACE && curP >= 0 ? (classicVoice(planets[curP]) ? "  (classic voices)" : "  (ElevenLabs voices)") : "", st == SPACE ? S.pos.x : P.x, st == SPACE ? S.pos.y : P.y, st == SPACE ? S.pos.z : P.z), FW * .5f, y + h - 32 * u, uiSmall(), UI_DIM);
  uiEnd();
  if (closeReq || endReq >= 0) { devPanel = false; setPaused(false); if (endReq >= 0) startEnding(endReq); }
}

// --make-developer-world: creates the one world with developer tools (or re-signs it if it already exists)
bool makeDeveloperWorld() {
#ifdef BEYOND_DEVTOOLS
  return signDeveloperWorld("Developer Mode");
#else
  return false;
#endif
}
