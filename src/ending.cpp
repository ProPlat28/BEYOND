// The endings: played once each, in space, when every planet (mode 0) or every moon (mode 1) of the universe is discovered
// (or with --test-ending / --test-moons).
// The camera pulls far back over the whole system, the 128 planets light up one by one along their spiral,
// then the title, the player's stats and the credits roll, and the player is handed back the rocket.
#include "game.h"

static int mode = 0;                                 // 0 every planet, 1 every moon
static float END_T = 36.f;                           // total length (s): the moons version has no credits
static const float PULL0 = 0, PULL1 = 8;             // camera pull-back
static const float LIT0 = 2, LIT1 = 14;              // the planets light up
static const float TITLE = 14.5f, STATS = 17, CRED0 = 21, CRED1 = 33;
static V3 camFrom, camTo, lookFrom, lookTo;
static std::vector<int> order;                        // planets (not moons) in the order they light up: the spiral, inside out
static int litN = 0;

static float smooth(float x) { x = clampf(x, 0, 1); return x * x * (3 - 2 * x); }
static V3 planetGlow(const Planet& p) { return lin(hsl((p.uniq * 37 % 128) / 128.f, .65f, .62f)); }

void startEnding(int m) {
  mode = m; END_T = m ? 24.f : 36.f; endT = 0; litN = 0; order.clear(); V3 c{0, 0, 0}; int n = 0; float far = 0;
  for (int i = 0; i < (int)planets.size(); i++) if (planets[i].enterable && planets[i].isMoon == (m == 1)) { order.push_back(i); c = c + planets[i].pos; n++; }
  if (n) c = c * (1.f / n);
  std::sort(order.begin(), order.end(), [&](int a, int b) { return len(planets[a].pos - c) < len(planets[b].pos - c); });
  for (int i : order) far = std::max(far, len(planets[i].pos - c));
  camFrom = camPos; lookFrom = S.pos; lookTo = c;
  V3 side = S.pos - c; side.y = 0; if (len(side) < 1) side = {0, 0, 1}; side = norm(side);
  camTo = c + side * (far * 1.05f) + V3{0, far * 1.25f, 0};   // high above the disc of the system, a little to the side, so it reads as a spiral
  S.speed = 0; S.thr = 0; talkL = 0; toastL = 0; prompt.clear(); audioSet(0, 0, 100, 0); audioEvent(S_WHOOSH);
}
bool endingOn() { return endT >= 0; }
void skipEnding() { if (endT > 2 && endT < END_T - 2) endT = END_T - 2; }

void updEnding(float dt) {
  float prev = endT; endT += dt; audioSet(0, 0, 100, 0);
  for (auto& p : planets) p.spin += dt * p.spinRate;
  float k = smooth((endT - PULL0) / (PULL1 - PULL0)), ke = k * k * (3 - 2 * k);
  float orbit = std::max(0.f, endT - PULL1) * .012f;   // then a slow drift around the system
  V3 to = camTo - lookTo, rot{to.x * std::cos(orbit) - to.z * std::sin(orbit), to.y, to.x * std::sin(orbit) + to.z * std::cos(orbit)};
  camPos = camFrom + (lookTo + rot - camFrom) * ke; V3 look = lookFrom + (lookTo - lookFrom) * smooth((endT - PULL0) / (PULL1 * .6f));
  lookAt(camPos, look); fov = 60; nearZ = 40.f + 260.f * k; farZ = 80000.f + 2.2e6f * k;
  int want = (int)clampf((endT - LIT0) / (LIT1 - LIT0) * (float)order.size(), 0, (float)order.size());
  for (; litN < want; litN++) if (litN % 6 == 0) audioEvent(S_CHIME);
  if (prev < TITLE && endT >= TITLE) { flashA = .45f; audioEvent(S_DISCOVER); }
  if (endT > END_T - 1.2f) flashA = std::max(flashA, clampf((endT - (END_T - 1.2f)) / 1.2f, 0, 1));
  if (endT >= END_T) {   // back to the rocket
    endT = -1; (mode ? moonEndSeen : endSeen) = true; saveReq = true; nearZ = 2; farZ = 80000.f; fov = 72; flashA = 1;
    V3 r, u, f; basis(S.yaw, S.pitch, S.roll, r, u, f); camPos = S.pos + f * -26.f + u * 5.f; cR = r; cU = u; cF = f; S.cyaw = S.yaw; S.cpitch = S.pitch; S.croll = S.roll;
    toast(mode ? "EVERY MOON IS YOURS\nNot one rock left unvisited - fly on, star traveler" : "THE UNIVERSE IS YOURS\nEvery world remembers you - fly on, visit old friends, finish what you like", 8); }
}

// beacons over every lit planet, drawn as point sprites (negative size = pixels, so they stay visible from far away)
void endingPoints(std::vector<PT>& pts) {
  if (endT < 0) return;
  for (int j = 0; j < litN && j < (int)order.size(); j++) { const Planet& p = planets[order[j]]; V3 q = p.pos - camPos, c = planetGlow(p);
    float age = endT - (LIT0 + (j + .5f) / order.size() * (LIT1 - LIT0)), pop = std::exp(-std::max(0.f, age) * 2.2f), tw = .8f + .2f * std::sin(endT * 2.3f + j);
    const float ms = mode ? .7f : 1.f;   // moons are smaller beacons
    pts.push_back({q.x, q.y, q.z, -(5.f + 18.f * pop) * ms * FH / 720.f, c.x * (1 + 2 * pop) * tw, c.y * (1 + 2 * pop) * tw, c.z * (1 + 2 * pop) * tw, 1});
    pts.push_back({q.x, q.y, q.z, -(22.f + 60.f * pop) * ms * FH / 720.f, c.x * .18f, c.y * .18f, c.z * .18f, 1}); }
}

// title, stats and credits, over the 3D view
void endingHud(float sc) {
  if (endT < 0) return;
  V3 w{1, 1, 1}, gold{1, .88f, .45f}, dim{.7f, .82f, 1};
  { std::string sk = keysIn("[{E}] skip"); if (endT > 2 && endT < END_T - 2) text(sk, FW - stb_easy_font_width((char*)sk.c_str()) * sc - 22, FH - 26.f * sc - 10, sc, dim, .6f); }
  if (endT > LIT0 && endT < TITLE + 1) { std::string c = std::to_string(litN) + " / " + std::to_string(order.size()); float a = clampf((endT - LIT0) / .6f, 0, 1) * clampf((TITLE + 1 - endT) / .8f, 0, 1);
    textC(mode ? "MOONS DISCOVERED" : "WORLDS DISCOVERED", FW / 2.f, FH * .08f, sc * 1.3f, dim, a); textC(c, FW / 2.f, FH * .08f + 16 * sc * 1.3f, sc * 2.2f, gold, a); }
  float fadeOut = clampf((END_T - 1.5f - endT) / 1.5f, 0, 1);
  if (endT >= TITLE) { float a = clampf((endT - TITLE) / 1.5f, 0, 1) * fadeOut, up = mode == 0 && endT > CRED0 ? std::min(1.f, (endT - CRED0) / 2.f) : 0.f;
    float y = FH * (.34f - .2f * smooth(up)); textC(mode ? "EVERY MOON DISCOVERED" : "UNIVERSE DISCOVERED", FW / 2.f, y, sc * (mode ? 3.6f : 4.2f), w, a);
    if (endT >= STATS) { float b = clampf((endT - STATS) / 1.2f, 0, 1) * fadeOut; int nr = 0; for (int i = 0; i < 6; i++) nr += (rocketMask >> i) & 1;
      std::string s1 = mode ? fmt("%d moons around %d of %d planets   -   %d rockets built", moonsDiscHere(), discHere(), totalHere(), nr) : fmt("%d planets   -   %d of %d moons   -   %d rockets built", discHere(), moonsDiscHere(), moonsTotalHere(), nr), s2 = deaths == 0 ? std::string("You never fell once.") : fmt("You fell %d %s, and got back up every time.", deaths, deaths == 1 ? "time" : "times");
      textC(s1, FW / 2.f, y + 34 * sc * 1.4f, sc * 1.4f, gold, b); textC(s2, FW / 2.f, y + 34 * sc * 1.4f + 16 * sc * 1.3f, sc * 1.3f, dim, b); } }
  if (mode == 0 && endT >= CRED0) {   // credits scroll up from the bottom (planets ending only)
    static const char* CR[] = {"BEYOND", "", "A game made by ProPlat28", "", "Built in C++ with GLFW, miniaudio and stb", "Music, footsteps and alien voices made with ElevenLabs", "", "Every world, every people and every wonder", "was generated just for you", "", "Thank you for playing", nullptr};
    float t = (endT - CRED0) / (CRED1 - CRED0), y0 = FH * (1.05f - .75f * t), lh = 15 * sc * 1.5f;
    for (int i = 0; CR[i]; i++) { float y = y0 + i * lh; if (y < FH * .42f || y > FH) continue; float a = clampf((y - FH * .42f) / (FH * .08f), 0, 1) * clampf((FH - y) / (FH * .06f), 0, 1) * fadeOut;
      textC(CR[i], FW / 2.f, y, i == 0 ? sc * 2.6f : sc * 1.5f, i == 0 ? gold : w, a); } }
}

// --test-ending / --test-moons: a throwaway universe (nothing is saved) with everything discovered, straight into that ending
void testEnding(int m) {
  curWorld = -1; worldSeed = 0; difficulty = 0; resetWorldState(); buildSystem(0);
  for (auto& p : planets) if (p.enterable) { p.found = true; p.qi = nqOf(p); }
  foundN = totalHere(); rocketMask = 1 | 2 | 4; deaths = 3;
  st = SPACE; ambient = {.012f, .012f, .018f}; fogDen = 0; nearZ = 2; S = Ship();
  const Planet& last = planets[planets.size() - 2]; V3 dv = norm(last.pos); S.pos = last.pos + dv * (last.R * 3.f); S.yaw = std::atan2(dv.x, dv.z); S.pitch = 0; S.cyaw = S.yaw;
  V3 r, u, f; basis(S.yaw, S.pitch, 0, r, u, f); camPos = S.pos + f * -26.f + u * 5.f; cR = r; cU = u; cF = f;
  if (m) endSeen = true; else moonEndSeen = true;   // only the ending being tested plays
  app = A_GAME; paused = false; syncModes(); startEnding(m);
}
