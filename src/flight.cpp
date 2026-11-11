// Walking on Earth, the launch, flying in space, and the rocket model.
#include "game.h"

void emit(V3 p, V3 dir, float pw, float dt) {
  int n = (int)std::ceil(pw * 90 * dt); if (parts.empty()) parts.resize(240, Part{{}, {}, 0, 1});
  for (int i = 0; i < n; i++) { Part& f = parts[partI++ % parts.size()]; auto rv = [] { return (float)std::rand() / RAND_MAX - .5f; };
    f.l = 1; f.p = p + V3{rv(), rv(), rv()} * 1.6f; f.v = dir * (18 + pw * 45) + V3{rv(), rv(), rv()} * 8.f; f.sc = 2.5f + pw * 3.5f; f.tint = flameTint; }
}
void updParts(float dt) { for (auto& f : parts) if (f.l > 0) { f.l -= dt * 1.5f; f.p = f.p + f.v * dt; } }
// ---------------------------------------------------------------- update
// what the ground sounds like: 0 grass, 1 wood floor, 2 sand, 3 snow, 4 metal/glass, 5 stone
static int groundType() {
  if (inRoom) return 1; if (st != SURFACE || curP < 0) return 0; const Planet& p = planets[curP];
  switch (p.kind) { case 2: case 14: return 2; case 3: return 3; case 8: case 9: case 12: return 4; case 4: case 5: case 6: case 15: return 5; default: return 0; }
}
static int stepKind() { if (inRoom) return -1; if (st != SURFACE || curP < 0) return 0; return planets[curP].kind; }   // recorded footsteps per planet kind (Earth is temperate); -1: house floors keep the synthesized wood
// shared first-person walking: returns horizontal speed
float walk(float dt, float (*gh)(float, float), float padR, float bound, float grav, float sp0) {
  auto held = [](int a) { return !modal && ::held(a); };   // a window is open: no walking, running or jumping
  if (devFly(dt, gh, bound)) return 0;   // developer fly / noclip
  float fz = clampf((float)(held(ACT_FORWARD) - held(ACT_BACK)) + padMove[1], -1, 1), fx = clampf((float)(held(ACT_RIGHT) - held(ACT_LEFT)) + padMove[0], -1, 1), sp = (held(ACT_RUN) ? 9.5f : 5.2f) * sp0;
  P.yaw -= (float)mdx * .0022f; P.pitch = clampf(P.pitch - (float)mdy * .0022f, -1.5f, 1.5f); mdx = mdy = 0;
  float s = std::sin(P.yaw), c = std::cos(P.yaw), tx = -s * fz + c * fx, tz = -c * fz - s * fx, l = std::hypot(tx, tz); if (l > 0) { float m = std::min(1.f, std::hypot(fx, fz)); tx = tx / l * sp * m; tz = tz / l * sp * m; }   // a half-tilted stick walks slowly
  float k = 1 - std::exp(-dt * 10); P.vx += (tx - P.vx) * k; P.vz += (tz - P.vz) * k; P.x += P.vx * dt; P.z += P.vz * dt; if (!(devWorld && devNoclip)) resolveHits();   // developer noclip: walk through everything
  float d = std::hypot(P.x, P.z); if (d > bound) { P.x *= bound / d; P.z *= bound / d; d = bound; }
  float gy = std::max(gh(P.x, P.z) + (d < padR ? .9f : 0), devWorld && devNoclip ? -1e9f : hitFloor(P.x, P.z, P.y)); bool grounded = P.y <= gy + .02f; static bool wasAir = false; static float fallV = 0;   // low hitboxes (a wonder's base, a rock) can be stood on
  if (grounded && wasAir && fallV < -3.f) { if (!audioStep(stepKind(), false, true)) audioEvent(120 + groundType());
    float safe = 6.8f * perkJump + 6.f; if (difficulty == 2 && st == SURFACE && !inRoom && !cine && -fallV > safe) hurtPlayer(1 + (int)((-fallV - safe) / 4.f), "a long fall"); }   // hard: fall damage (a jump on flat ground never hurts)
  wasAir = !grounded; fallV = P.vy;
  if (grounded && held(ACT_JUMP)) { P.vy = 6.8f * perkJump; audioEvent(140); } P.vy -= 18 * grav * dt; P.y += P.vy * dt; if (P.y < gy) { P.y = gy; P.vy = 0; }
  float spd = std::hypot(P.vx, P.vz); if (grounded && spd > 1) { P.step += spd * dt; P.bob += spd * dt * 2.2f; float stride = held(ACT_RUN) ? 2.9f : 2.1f; if (P.step > stride) { P.step = 0; if (!audioStep(stepKind(), held(ACT_RUN), false)) audioEvent(100 + groundType() + (held(ACT_RUN) ? 10 : 0)); } }
  camPos = {P.x, P.y + 1.75f + std::sin(P.bob) * .05f * (spd > 1), P.z}; basis(P.yaw, P.pitch, 0, cR, cU, cF); fov = 72; nearZ = .3f; return spd;
}
void updEarth(float dt) {
  float spd = walk(dt, hAt, 17, 215, 1, 1), d = std::hypot(P.x, P.z);
  for (int i = 0; i < 5; i++) { Pick& p = picks[i]; if (p.got) continue; if (std::hypot(P.x - p.pos.x, P.z - p.pos.z) < 3.2f && std::fabs(P.y - p.pos.y) < 6) { p.got = true; gotN++; audioEvent(S_CHIME);
    toast(std::string(p.name) + "\nRocket part found (" + std::to_string(gotN) + "/5)" + (gotN == 5 ? " - bring them to the launch pad" : ""), 3.5f); } }
  if (d < 20 && gotN > instN) { for (auto& p : picks) if (p.got && !p.inst) { p.inst = true; instN++; } audioEvent(S_CLANG); toast("INSTALLED\n" + std::to_string(instN) + "/5 parts on the rocket" + (instN == 5 ? " - press {E} to launch" : ""), 4); }
  audioSet(0, 0, 100, 0); (void)spd;   // no wind hiss on Earth (it sounded like the sea)
}
void updLaunch(float dt) {
  V3 base{0, rocketY + .5f, 0};
  if (st == COUNT) { float prev = cd; cd += dt; int n = 6 - (int)cd; if ((int)cd != (int)prev && n > 0) { audioEvent(n == 1 ? S_BEEP_HI : S_BEEP_LO); toast("T-" + std::to_string(n), .9f); }
    float pw = clampf((cd - 2.5f) / 3.5f, 0, 1); if (cd > 2.5f) emit(base, {0, -1, 0}, pw * .6f, dt); audioSet(0, .08f + pw * .25f, 110 + pw * 50, 0);   // engine fire only (no wind hiss)
    auto rv = [] { return (float)std::rand() / RAND_MAX - .5f; }; camPos = {42 + rv() * pw * .6f, 9 + rv() * pw * .6f, 42}; lookAt(camPos, {0, 11, 0}); fov = 60; nearZ = .3f;
    if (cd >= 6) { st = LAUNCH; vyL = 0; toast("LIFTOFF", 3); } return; }
  vyL += (16 + rocketY * .012f) * dt; rocketY += vyL * dt; emit({0, rocketY + .5f, 0}, {0, -1, 0}, 1, dt);
  V3 want{38, std::max(10.f, rocketY * .8f), 38}; camPos = camPos + (want - camPos) * (1 - std::exp(-dt * 3)); lookAt(camPos, {0, rocketY + 8, 0}); fov = 60;
  float t = clampf(rocketY / 800, 0, 1); fogDen = .0035f * (1 - t); ambient = {lerpf(.3f, .01f, t), lerpf(.36f, .01f, t), lerpf(.45f, .015f, t)};
  audioSet(0, .35f * (1 - t * .5f), 160, 0);   // engine fire only (no wind hiss)
  if (rocketY > 900) { st = SPACE; flashA = 1; fogDen = 0; ambient = {.012f, .012f, .018f}; S.pos = {0, 0, 0}; S.pitch = S.cpitch = .3f; S.yaw = S.cyaw = 0; S.speed = 200; nearZ = 2;
    V3 f; basis(0, .3f, 0, cR, cU, f); camPos = f * -26.f + cU * 5.f; cF = f;
    toast("BEYOND\n{F}/{B} throttle  |  mouse steer  |  {RUN} boost  |  fly to the worlds around you\nNear a planet: press {E} to descend and meet its people", 9); }
}
void updSpace(float dt) {
  if (endT >= 0) { mdx = mdy = 0; updEnding(dt); return; }
  if (!endSeen && totalHere() > 0 && discHere() == totalHere()) { startEnding(0); return; }   // the last planet was just discovered
  if (!moonEndSeen && moonsTotalHere() > 0 && moonsDiscHere() == moonsTotalHere()) { startEnding(1); return; }   // ... or the last moon
  S.yaw -= (float)mdx * .0018f; S.pitch = clampf(S.pitch - (float)mdy * .0018f, -1.45f, 1.45f); S.roll += (clampf(-(float)mdx * .02f, -.7f, .7f) - S.roll) * std::min(1.f, dt * 3); mdx = mdy = 0;
  S.thr = clampf(S.thr + clampf((float)(held(ACT_FORWARD) - held(ACT_BACK)) * .7f + padMove[1] * 1.8f, -1.8f, 1.8f) * dt, 0, 1);   // the stick ramps the throttle faster than the keys
  bool boost = held(ACT_RUN) || padDown[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];   // L3 (click the left stick) also boosts
  float tg = S.thr * 900 * ROCKET[rocketSel].speed * (boost ? 6 : 1) * (devFast ? 10 : 1); flameTint = ROCKET[rocketSel].flame; S.speed += (tg - S.speed) * std::min(1.f, dt * (padActive ? 2.5f : 1.5f));
  V3 r, u, f; basis(S.yaw, S.pitch, S.roll, r, u, f); V3 prevPos = S.pos; S.pos = S.pos + f * (S.speed * dt);
  // asteroids: the path of this frame is tested against every rock, so even a boosted rocket cannot fly through one. A hit pushes you out and slows you down.
  { static float bumpCd = 0; bumpCd = std::max(0.f, bumpCd - dt); V3 seg = S.pos - prevPos; float sl2 = dot(seg, seg), reach = std::sqrt(sl2) + 14.f;
    for (auto& k : srocks) { V3 dc = k.pos - prevPos; float lim = reach + k.r; if (dot(dc, dc) > lim * lim) continue;
      float t = sl2 > 1e-6f ? clampf(dot(dc, seg) / sl2, 0, 1) : 0.f; V3 cp = prevPos + seg * t, off = cp - k.pos; float dist = len(off), rr = k.r + 14.f; if (dist >= rr) continue;
      V3 nrm = dist > 1e-3f ? off * (1.f / dist) : f * -1.f; S.pos = k.pos + nrm * (rr + 1.f); S.speed *= .35f; S.thr = std::max(0.f, S.thr - .08f);
      (void)bumpCd; } }   // a silent bump: no sound, no message
  nearEnter = -1; float bd = 1e9f;
  for (int i = 0; i < (int)planets.size(); i++) { Planet& p = planets[i]; if (p.isMoon) { p.orbA += dt * p.orbS; const V3& pp = planets[p.parent].pos; p.pos = pp + V3{std::cos(p.orbA) * p.orbD, std::sin(p.orbA) * p.orbD * .12f, std::sin(p.orbA) * p.orbD}; } V3 dv = S.pos - p.pos; float d = len(dv); if (d < p.R * 1.04f) { S.pos = p.pos + norm(dv) * (p.R * 1.04f); S.speed *= .6f; }
    p.spin += dt * p.spinRate;
    float sd = d - p.R; if (p.enterable && sd < p.R * 2.2f + 400 && sd < bd) { bd = sd; nearEnter = i; } }
  float k = 1 - std::exp(-dt * 8); S.cyaw += (S.yaw - S.cyaw) * k; S.cpitch += (S.pitch - S.cpitch) * k; S.croll += (S.roll - S.croll) * k;
  V3 r2, u2, f2; basis(S.cyaw, S.cpitch, S.croll, r2, u2, f2); V3 want = S.pos + f * -26.f + u * 5.f; camPos = camPos + (want - camPos) * (1 - std::exp(-dt * 7)); cR = r2; cU = u2; cF = f2;
  fov = 72 + clampf(S.speed / 6000, 0, .12f) * 220; nearZ = 2; emit(S.pos, f * -1.f, .35f + S.thr * .65f, dt);
  audioSet(0, .15f + S.thr * .35f + (boost ? .2f : 0), 90 + S.speed * .1f, .1f);
}
// ---------------------------------------------------------------- asteroids
// Every star system has its own rocks, scattered one by one over the whole system (never in heaps): a minimum gap between any two,
// and a wide clear zone around every planet and the orbits of its moons, plus the start point.
void buildSpaceRocks(int id) {
  srocks.clear(); Rng r{((unsigned)id + worldSeed * 6151u) * 2246822519u + 311u}; r.f(); r.f();
  struct Zone { V3 c; float r; }; std::vector<Zone> zones; float far = 8000.f;
  for (auto& p : planets) { if (p.isMoon) continue; float orb = 0; for (auto& m : p.moons) orb = std::max(orb, m.d + std::max(34.f, m.r) * 2.f); zones.push_back({p.pos, std::max(p.R * 3.2f, orb + 900.f) + 1500.f}); far = std::max(far, len(V3{p.pos.x, 0, p.pos.z}) + 6000.f); }
  std::unordered_map<long long, std::vector<int>> grid; const float CELL = 2400.f, GAP = 1800.f;   // spatial hash so the spacing test stays fast
  auto cellOf = [&](float v) { return (long long)std::floor(v / CELL); };
  auto keyOf = [&](long long x, long long y, long long z) { return (x * 73856093LL) ^ (y * 19349663LL) ^ (z * 83492791LL); };
  int want = 3200, tries = 0;
  while ((int)srocks.size() < want && tries++ < want * 6) {
    V3 c; if (r.f() < .45f && !zones.empty()) { const Zone& z = zones[r.i((int)zones.size())]; float t = r.r(.05f, 1.15f); c = z.c * t + V3{r.r(-1, 1), r.r(-.4f, .4f), r.r(-1, 1)} * 2500.f; }   // somewhere along the way to a world
    else { float a = r.r(0, 6.283f), d = std::sqrt(r.f()) * far; c = {std::cos(a) * d, r.r(-4200, 4200), std::sin(a) * d}; }                                   // anywhere in the system
    if (len(c) < 3000.f) continue; bool ok = true; for (auto& z : zones) if (len(c - z.c) < z.r) { ok = false; break; } if (!ok) continue;
    long long cx = cellOf(c.x), cy = cellOf(c.y), cz = cellOf(c.z);
    for (long long i = -1; i <= 1 && ok; i++) for (long long j = -1; j <= 1 && ok; j++) for (long long k = -1; k <= 1 && ok; k++) { auto it = grid.find(keyOf(cx + i, cy + j, cz + k)); if (it == grid.end()) continue; for (int q : it->second) if (len(srocks[q].pos - c) < GAP) { ok = false; break; } }
    if (!ok) continue;
    SRock k; k.pos = c; k.r = 30 + r.f() * r.f() * 230; k.sx = r.r(.75f, 1.f); k.sy = r.r(.6f, .95f); k.sz = r.r(.75f, 1.f); k.yaw = r.r(0, 6.283f); float g = r.r(.5f, .95f); k.tint = {g, g * .95f, g * .88f};
    grid[keyOf(cx, cy, cz)].push_back((int)srocks.size()); srocks.push_back(k);
  }
}
// ---------------------------------------------------------------- render
void useMain() { glUseProgram(pMain); glUniformMatrix4fv(uVP, 1, GL_FALSE, VP); glUniform3f(uSunDir, sunDir.x, sunDir.y, sunDir.z); glUniform3f(uSunCol, sunCol.x, sunCol.y, sunCol.z);
  glUniform3f(uAmb, ambient.x, ambient.y, ambient.z); glUniform3f(uFogCol, fogCol.x, fogCol.y, fogCol.z); glUniform1f(uFog, fogDen); glUniform1i(uTex, 0); }
void drawRocket(V3 b, V3 X, V3 Y, V3 Z) {
  bool full = st == SPACE || st == SURFACE; int rid = full ? rocketSel : 0; const RocketDef& R = ROCKET[rid]; if (st == SURFACE) flameTint = R.flame;
  auto L = [&](const Mesh& m, float lx, float ly, float lz, V3 sc, V3 t, float em, GLuint tex, V3 ax, V3 az) { DP d; d.p = b + X * lx + Y * ly + Z * lz; d.s = sc; d.X = ax; d.Y = Y; d.Z = az; d.t = t; d.emis = em; d.tex = tex; d.spec = .25f; draw(m, d); };
  auto Wg = [&](float lx, float ly, float lz, V3 sc, V3 t, float em, float tilt) { V3 ax = X * std::cos(tilt) + Y * std::sin(tilt), ay = Y * std::cos(tilt) - X * std::sin(tilt); DP d; d.p = b + X * lx + Y * ly + Z * lz; d.s = sc; d.X = ax; d.Y = ay; d.Z = Z; d.t = t; d.emis = em; d.tex = tHull; draw(mBox, d); };   // wings tilt around the Z axis
  float em = rid == 5 ? .35f : 0.f;
  if (picks[0].inst || full) { L(mCylT, 0, 2, 0, {2.5f, 13, 2.5f}, R.hull, em, tHull, X, Z); L(mCylN, 0, 12.4f, 0, {2.57f, 1.2f, 2.57f}, {.02f, .03f, .05f}, 0, tHull, X, Z); }
  if (picks[1].inst || full) for (int s = -1; s <= 1; s += 2) { L(mCylN, s * 3.6f, 2, 0, {.9f, 9, .9f}, R.boost, em, tHull, X, Z); L(mCone, s * 3.6f, 11, 0, {.9f, 2, .9f}, {.7f, .7f, .7f}, 0, tHull, X, Z); }
  if (picks[2].inst || full) { L(mCylT, 0, 0, 0, {2.3f, 3, 2.3f}, {.08f, .09f, .1f}, 0, tHull, X, Z); L(mCylN, 0, .9f, 0, {1.3f, .1f, 1.3f}, R.flame * .8f + V3{.2f, .2f, .2f}, 2.5f, 0, X, Z); }
  if (picks[3].inst || full) { L(mCone, 0, 15, 0, {2.3f, 6, 2.3f}, R.nose, em, tHull, X, Z); L(mSphereLo, 0, 11, 2.2f, {.75f, .75f, .75f}, {.2f, .5f, 1}, 2.f, 0, X, Z); }
  if (picks[4].inst || full) for (int i = 0; i < 4; i++) { float a = i * PI / 2, ca = std::cos(a), sa = std::sin(a); L(mBox, ca * 3, 4, sa * 3, {.3f, 4.5f, 2.8f}, R.fin, em, tHull, X * -sa + Z * ca, X * ca + Z * sa); }
  if (!full) return;
  if (rid == 1) for (int s = -1; s <= 1; s += 2) Wg(s * 3.4f, 2.5f, 0, {2.4f, .3f, 3.2f}, {1, .6f, .15f}, 0, s * .25f);
  if (rid == 2) { for (int s = -1; s <= 1; s += 2) { L(mCylN, 0, 2, s * 3.6f, {.9f, 8, .9f}, R.boost, 0, tHull, X, Z); L(mCone, 0, 10, s * 3.6f, {.9f, 2, .9f}, {.8f, .85f, 1}, 0, tHull, X, Z); } L(mCylN, 0, 8, 0, {2.6f, .5f, 2.6f}, {1, 1, 1}, .3f, 0, X, Z); }
  if (rid == 3) { L(mCone, 0, 20, 0, {1.2f, 8, 1.2f}, R.nose, 0, tHull, X, Z); for (int s = -1; s <= 1; s += 2) Wg(s * 5.2f, 3, 0, {5.5f, .28f, 3.4f}, R.fin, .2f, s * -.35f); }
  if (rid == 4) { for (int s = -1; s <= 1; s += 2) { Wg(s * 6.f, 4, 0, {7.5f, .3f, 4.5f}, R.fin, .3f, s * -.5f); L(mSphereLo, s * 9.5f, 6, 0, {.5f, .5f, .5f}, R.boost, 3.f, 0, X, Z); } L(mCylN, 0, .2f, 0, {3.4f, .3f, 3.4f}, R.boost, 2.2f, 0, X, Z); }
  if (rid == 5) { for (int s = -1; s <= 1; s += 2) { Wg(s * 6.5f, 3.5f, 0, {8.f, .3f, 5.f}, {1, .78f, .25f}, .8f, s * -.55f); Wg(s * 4.4f, 9, 0, {4.f, .22f, 2.6f}, {1, .9f, .5f}, .8f, s * -.4f); }
    for (int k = 0; k < 3; k++) { float a = tAcc * 1.3f + k * 2.0944f; L(mSphereLo, std::cos(a) * 5.f, 7 + std::sin(a * 1.7f) * 3.f, std::sin(a) * 5.f, {.7f, .7f, .7f}, {.8f, 1, 1}, 3.5f, 0, X, Z); } L(mCone, 0, 20, 0, {1.4f, 7, 1.4f}, {1, 1, 1}, 1.5f, tHull, X, Z); }
}
