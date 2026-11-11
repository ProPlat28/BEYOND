// Hitboxes, houses you can walk into (villagers' huts and the wicked house), the market stall, the workbench and the ore nodes.
#include "game.h"

// ---------------------------------------------------------------- hitboxes (circles and oriented boxes; people have none for the player, but they collide with them)
Cir boxHit(V3 c, float hx, float hz, float yaw, float y, float h) { Cir k{c.x, c.z, std::sqrt(hx * hx + hz * hz), y, h}; k.hx = hx; k.hz = hz; k.ca = std::cos(yaw); k.sa = -std::sin(yaw); return k; }   // yaw: the model's Z axis is (sin yaw, cos yaw)
bool insideHit(const Cir& c, float x, float z, float rad) {
  float dx = x - c.x, dz = z - c.z; if (dx * dx + dz * dz > (c.r + rad) * (c.r + rad)) return false; if (c.hx <= 0) return true;
  float lx = dx * c.ca + dz * c.sa, lz = -dx * c.sa + dz * c.ca; return std::fabs(lx) < c.hx + rad && std::fabs(lz) < c.hz + rad;
}
bool pushOut(const Cir& c, float& x, float& z, float rad) {
  float dx = x - c.x, dz = z - c.z;
  if (c.hx <= 0) { float d = std::sqrt(dx * dx + dz * dz), m = c.r + rad; if (d >= m) return false; if (d < 1e-4f) { dx = 1; dz = 0; d = 1; } x = c.x + dx / d * m; z = c.z + dz / d * m; return true; }
  float lx = dx * c.ca + dz * c.sa, lz = -dx * c.sa + dz * c.ca, ex = c.hx + rad, ez = c.hz + rad; if (std::fabs(lx) >= ex || std::fabs(lz) >= ez) return false;
  if (ex - std::fabs(lx) < ez - std::fabs(lz)) lx = lx < 0 ? -ex : ex; else lz = lz < 0 ? -ez : ez;   // out through the nearest side
  x = c.x + lx * c.ca - lz * c.sa; z = c.z + lx * c.sa + lz * c.ca; return true;
}
static void push(const Cir& c) { if (P.y < c.y + c.h - STEP_UP) pushOut(c, P.x, P.z, .5f); }   // what is lower than a step is walked onto instead (see hitFloor)
float hitFloor(float x, float z, float y) { float f = -1e9f; for (auto& c : hits) { float top = c.y + c.h; if (top <= y + STEP_UP && top > f && insideHit(c, x, z, .1f)) f = top; } return f; }
bool npcBlocked(float x, float z, float y, float rad) { for (auto& c : hits) if (y < c.y + c.h - .6f && insideHit(c, x, z, rad)) return true; return false; }
void npcCollide(Npc& n, float rad) { for (int it = 0; it < 2; it++) for (auto& c : hits) if (n.pos.y < c.y + c.h - .6f) pushOut(c, n.pos.x, n.pos.z, rad); }
void resolveHits() {
  for (auto& c : hits) push(c);
  if (st == SURFACE && !inRoom && curP >= 0) {
    for (auto& nd : nodes) push(Cir{nd.pos.x, nd.pos.z, .8f, nd.pos.y, 1.5f});
    for (auto& o : objs) if (o.type == 1 || o.type == 5) push(Cir{o.pos.x, o.pos.z, .75f, o.pos.y - 1.4f, 4.f});
    if (!planets[curP].isMoon) push(Cir{relicPos.x, relicPos.z, .9f, relicPos.y - 1.6f, 1.2f});
  }
}
static const float HUT_R[8] = {2.4f, 2.25f, 3.6f, 2.6f, 2.f, 3.1f, 2.2f, 2.2f}, HUT_D[8] = {1.72f, 2.2f, 3.8f, 2.55f, 1.45f, 2.4f, .75f, 1.4f};
float hutRadius(int ar) { return HUT_R[((ar % 8) + 8) % 8]; }
float hutDoorDist(int ar) { return HUT_D[((ar % 8) + 8) % 8]; }
V3 hutDoor(int i) { float ang = i * 1.3f; V3 Z{std::sin(ang), 0, std::cos(ang)}; return hutPos[i] + Z * hutDoorDist(surfArch); }
V3 wDoor() { return wHouse + V3{-3.6f, 0, 0}; }
void buildEarthHits() {
  hits.clear(); hits.push_back({0, 0, 3.4f, 0, 16}); for (int sx = -1; sx <= 1; sx += 2) hits.push_back({sx * 9.f, 0, 1.1f, 0, 28});
  for (auto& t : trees) hits.push_back({t.x, t.z, .55f * t.s, hAt(t.x, t.z), 6.f}); for (auto& r : rocks) hits.push_back({r.x, r.z, std::max(r.sx, r.sz) * .85f, hAt(r.x, r.z), r.sy * 1.3f});
}
void buildHits(const Planet& p) {
  hits.clear(); static const float FR[8] = {.45f, .5f, .5f, .35f, .45f, .85f, .4f, .65f};
  hits.push_back({0, 0, 3.4f, 0, 16});
  // huts: the hitbox follows the drawn shape (boxes for box houses, the ground-level radius for domes and cones)
  for (size_t i = 0; i < hutPos.size(); i++) { V3 h = hutPos[i]; float ang = i * 1.3f, y = shAt(h.x, h.z); int ar = ((surfArch % 8) + 8) % 8;
    if (ar == 0) hits.push_back(boxHit(h, 1.7f, 1.7f, ang, y, 4.f)); else if (ar == 4) hits.push_back(boxHit(h, 1.4f, 1.4f, ang, y, 4.6f));
    else if (ar == 6) hits.push_back(boxHit(h, 2.1f, .7f, ang, y, 2.2f)); else hits.push_back({h.x, h.z, hutRadius(ar), y, 4.f}); }
  hits.push_back(boxHit(marketPos, 1.7f, .55f, marketYaw, shAt(marketPos.x, marketPos.z), 1.1f));   // only the counter: you trade from the front
  hits.push_back(boxHit(benchPos, 1.3f, .6f, 0, shAt(benchPos.x, benchPos.z), 1.2f));
  if (!p.isMoon) { hits.push_back(boxHit(wHouse, 3.5f, 3.5f, 0, shAt(wHouse.x, wHouse.z), 5.f)); if (!p.hideCamp) for (int i = 0; i < 8; i++) { float a = i * PI / 4; float x = campC.x + std::cos(a) * 11, z = campC.z + std::sin(a) * 11; hits.push_back({x, z, .7f, shAt(x, z), 5.f}); } }
  for (auto& b : boulders) hits.push_back({b.x, b.z, std::max(b.sx, b.sz) * .8f, shAt(b.x, b.z), b.sy * 1.15f});   // small ones can be stepped onto
  for (auto& f : flora) hits.push_back({f.x, f.z, std::max(.3f, FR[f.type & 7] * f.s), shAt(f.x, f.z), 3.f * f.s});
  for (auto& q : pois) { V3 b = q.pos; float y = shAt(b.x, b.z); V3 Z{std::sin(q.yaw), 0, std::cos(q.yaw)}, X{Z.z, 0, -Z.x}; auto L = [&](float lx, float lz) { return b + X * lx + Z * lz; };
    switch (q.type) {
      case 0: hits.push_back(boxHit(L(0, 2), 3.f, 8.f, q.yaw, y, 5.f)); break;                                   // crashed freighter lying on its side
      case 1: for (int k = 0; k < 6; k++) { float a = k * 1.047f; V3 c = L(std::cos(a) * 7, std::sin(a) * 7); hits.push_back({c.x, c.z, .8f, y, k % 3 == 0 ? 3.f : 6.f}); } hits.push_back(boxHit(b, 2.5f, 2.5f, q.yaw, y, 1.f)); break;   // ruins: walk between the pillars
      case 2: hits.push_back(boxHit(b, 5.f, 3.5f, q.yaw, y, 5.f)); break;
      case 3: for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) { V3 c = L(sx * 2.f, sz * 2.f); hits.push_back({c.x, c.z, .35f, y, 10.f}); } break;   // you can stand under the watchtower
      case 4: for (int k = 0; k < 7; k++) { float a = k * .897f; V3 c = L(std::cos(a) * 6.5f, std::sin(a) * 6.5f); hits.push_back({c.x, c.z, 1.1f, y, .9f}); } break;
      case 5: hits.push_back(boxHit(b, 2.5f, 2.5f, q.yaw, y, 3.f)); hits.push_back({b.x, b.z, 1.8f, y, 12.f}); break;
      case 6: for (int k = 0; k < 4; k++) { V3 c = L(k * 4.f - 6, std::sin(k * 3.f) * 3); hits.push_back({c.x, c.z, 1.2f, y, 6.f}); } break;
      case 7: hits.push_back({b.x, b.z, 3.f, y, 3.f}); break;
      case 8: hits.push_back({b.x, b.z, .6f, y, 12.f}); break;
      default: for (int k = -1; k <= 1; k++) hits.push_back(boxHit(L(k * 6.f, 0), 2.f, 1.f, q.yaw, y, 1.2f)); break;   // night market: three low stalls
    } }
  { V3 b = wonderPos; float y = shAt(b.x, b.z), ang = (wNoun == 1 || wNoun == 3) ? 0.f : wAdj * .39f; V3 X{std::cos(ang), 0, -std::sin(ang)};
    hits.push_back({b.x, b.z, 6.f, y, .8f});   // the round base: low enough to step onto
    switch (wNoun) { case 0: hits.push_back({b.x, b.z, 1.6f, y, 16.f}); break; case 1: hits.push_back({b.x, b.z, .9f, y, 9.f}); break;
      case 2: for (int s = -1; s <= 1; s += 2) { V3 c = b + X * (s * 5.f); hits.push_back({c.x, c.z, 1.3f, y, 14.f}); } break; case 3: hits.push_back({b.x, b.z, 2.4f, y, 12.f}); break;
      case 4: hits.push_back(boxHit(b, 2.5f, 2.5f, ang, y, 7.f)); break; case 5: hits.push_back({b.x, b.z, 2.f, y, 11.f}); break;
      case 6: for (int s = -1; s <= 1; s += 2) { V3 c = b + X * (s * 5.f); hits.push_back({c.x, c.z, .45f, y, 11.f}); } break; default: hits.push_back({b.x, b.z, 2.6f, y, 14.f}); }
  }
}
// ---------------------------------------------------------------- inside a house
namespace {
struct Prop { int m; V3 p, s, c; float em; };
std::vector<Prop> props; V3 chestPos; int chestBit = 24;
void add(int m, float x, float y, float z, float sx, float sy, float sz, V3 c, float em = 0, bool solid = false) { props.push_back({m, {x, y, z}, {sx, sy, sz}, c, em}); if (solid) hits.push_back({x, z, std::max(sx, sz) * (m == 0 ? .55f : .5f), ROOMY, sy + (m == 0 ? 0.f : 0.f)}); }
float roomH(float, float) { return ROOMY; }
V3 wallC, floorC;
const char* HOME_SAY[6] = {"Oh! A visitor! Mind the rug. It bites. Only a little.", "Welcome to my home. Please admire the furniture. It took me an entire weekend.", "I was just about to make tea. Out of what, I'm not sure. Want some?",
  "You walked right in! That is allowed, right? I never checked.", "Careful with the chest. It is not locked, it is just very shy.", "Shh. The bed is sleeping. Do you hear it? That is the mattress snoring."};
const char* WICK_SAY[4] = {"Hm? A skyling in my house? This is the dullest wicked den you'll ever see. Please wipe your boots.", "I retired from wickedness. Now I just collect spikes. And stew.", "The chief says this house is 'the heart of evil'. It is mostly a kitchen.", "Don't touch the throne. It's mine. Well, the chief's. Well, nobody's, it is broken."};
}
void enterRoom(int kind, int idx) {
  Planet& p = planets[curP]; inRoom = kind; roomIdx = idx; outX = P.x; outZ = P.z; outYaw = P.yaw; hitsSaved = hits; hits.clear(); props.clear(); roomNpcs.clear();
  float m = (idx & 1) ? -1.f : 1.f; V3 acc = lin(p.pal[3]) * 1.2f, wood{.33f, .2f, .11f};
  if (kind == 1) { wallC = hutCol * 1.15f + V3{.06f, .05f, .04f}; floorC = roofCol * 2.f + V3{.12f, .08f, .06f}; V3 cl = lin(hsl(.04f + (idx * 3 + p.uniq) % 12 * .07f, .5f, .5f));
    add(1, 0, .02f, 0, 2.4f, .04f, 2.4f, acc * .5f); add(0, -2.8f * m, .45f, -2.2f, 1.9f, .9f, 1.1f, wood, 0, true); add(1, -2.8f * m, 0, -.8f, .35f, .5f, .35f, wood * 1.2f, 0, true); add(1, -1.3f * m, 0, -2.2f, .35f, .5f, .35f, wood * 1.2f, 0, true);
    add(0, 3.4f * m, .3f, -3.9f, 2.2f, .6f, 1.2f, cl, 0, true); add(0, 3.9f * m, .7f, -3.9f, .7f, .16f, .5f, {.9f, .9f, .85f}); add(0, 0, 1.2f, -RS + .4f, 2.6f, 2.4f, .6f, wood * .9f, 0, true);
    for (int k = -1; k <= 1; k++) add(0, k * .8f, 1.45f + (k == 0) * .5f, -RS + .75f, .4f, .4f, .3f, lin(hsl(.1f * (k + 2) + idx * .05f, .7f, .55f)) * 1.4f, 1.2f);
    add(1, 4.6f * m, 0, 1.4f, .08f, 1.6f, .08f, wood); add(2, 4.6f * m, 1.85f, 1.4f, .28f, .28f, .28f, {1, .85f, .5f}, 3.f); add(0, 2.8f * m, 2.6f, -RS + .22f, 1.f, 1.6f, .06f, acc, .6f);
    add(0, -4.2f * m, .35f, 3.2f, 1.1f, .7f, .7f, wood * 1.1f, 0, true); add(0, -4.2f * m, .76f, 3.2f, 1.15f, .12f, .75f, {1, .8f, .3f}, 1.2f); chestPos = {-4.2f * m, 0, 3.2f}; chestBit = 24 + std::min(idx, 5);
    Npc r; r.dweller = true; r.name = std::string("Villager ") + (char)('A' + (p.uniq + idx * 5) % 26) + "ax"; r.col = cl; r.home = r.pos = r.dest = {.9f * m, ROOMY, -.7f}; r.look = idx + 3; r.voice = .8f + idx * .1f; r.wait = .4f; r.wphase = idx * 1.4f; roomNpcs.push_back(r); }
  else { wallC = {.11f, .08f, .1f}; floorC = {.24f, .07f, .08f}; chestBit = 30;
    add(1, 0, .02f, 0, 2.6f, .04f, 2.6f, {.05f, .02f, .03f}); add(0, 0, .5f, -4.6f, 1.7f, 1.f, 1.4f, {.06f, .04f, .05f}, 0, true); add(0, 0, 1.9f, -5.35f, 1.9f, 3.2f, .3f, {.05f, .03f, .04f}, 0, true);
    add(2, -.8f, 3.6f, -5.2f, .16f, .16f, .16f, {1, .1f, .1f}, 3.f); add(2, .8f, 3.6f, -5.2f, .16f, .16f, .16f, {1, .1f, .1f}, 3.f); add(3, 0, 3.4f, -5.3f, .3f, .9f, .3f, {.1f, .08f, .08f});
    add(1, -3.8f, 0, -2.8f, 1.f, .9f, 1.f, {.03f, .03f, .03f}, 0, true); add(1, -3.8f, .9f, -2.8f, .85f, .06f, .85f, {.3f, 1.f, .3f}, 2.2f); add(3, 3.8f, 0, -3.f, .12f, 2.2f, .12f, {.6f, .6f, .65f}); add(3, 4.4f, 0, -2.4f, .12f, 2.f, .12f, {.6f, .6f, .65f});
    add(2, -4.6f, .45f, 1.f, .32f, .32f, .32f, {.9f, .88f, .8f}); add(2, -4.6f, 2.6f, -1.f, .26f, .26f, .26f, {1, .1f, .08f}, 3.f); add(2, 4.6f, 2.6f, -1.f, .26f, .26f, .26f, {1, .1f, .08f}, 3.f);
    for (int k = 0; k < 6; k++) add(3, -4.8f + k * 1.9f, 0, -RS + .6f, .25f, .8f + (k % 3) * .3f, .25f, {.08f, .05f, .06f});
    add(0, 4.2f, .35f, 3.2f, 1.1f, .7f, .7f, {.1f, .06f, .06f}, 0, true); add(0, 4.2f, .76f, 3.2f, 1.15f, .12f, .75f, {1, .2f, .1f}, 1.5f); chestPos = {4.2f, 0, 3.2f};
    Npc r; r.dweller = true; r.wicked = true; r.name = "Old Grumbo"; r.col = {.1f, .02f, .03f}; r.home = r.pos = r.dest = {-1.2f, ROOMY, -2.4f}; r.look = 2; r.voice = .9f; r.wait = .5f; r.wphase = 2.2f; roomNpcs.push_back(r); }
  // walls with windows (the door is drawn in drawRoom)
  for (int s = -1; s <= 1; s += 2) { add(0, s * (RS - .15f), 2.3f, -1.5f, .1f, 1.1f, 1.5f, kind == 2 ? V3{1, .15f, .08f} : surfSky * 2.5f + V3{.3f, .3f, .35f}, 1.4f); add(0, s * (RS - .1f), 2.3f, 2.f, .06f, 1.2f, .08f, {.1f, .08f, .06f}); }
  for (auto& n : roomNpcs) n.pos.y = ROOMY;
  P.x = 0; P.z = RS - 1.8f; P.y = ROOMY; P.vx = P.vz = P.vy = 0; P.yaw = 0; P.pitch = 0; flashA = .8f; audioEvent(S_CLANG); talkL = 0; (void)p;
}
void exitRoom() {
  hits = hitsSaved; hitsSaved.clear(); roomNpcs.clear(); props.clear(); int ar = surfArch; V3 door = inRoom == 2 ? wDoor() : hutDoor(roomIdx); V3 out = inRoom == 2 ? V3{-1, 0, 0} : V3{std::sin(roomIdx * 1.3f), 0, std::cos(roomIdx * 1.3f)}; (void)ar;
  P.x = door.x + out.x * 3.1f; P.z = door.z + out.z * 3.1f; P.yaw = PI + std::atan2(out.x, out.z); P.pitch = 0; P.y = shAt(P.x, P.z); P.vx = P.vz = P.vy = 0; inRoom = 0; flashA = .8f; audioEvent(S_CLANG); talkL = 0; mdx = mdy = 0;
}
bool roomInteract(Planet& p) {
  V3 me{P.x, 0, P.z};
  for (auto& n : roomNpcs) if (len(V3{n.pos.x, 0, n.pos.z} - me) < 3.6f) { setVoice(&n); say(n.name, inRoom == 2 ? WICK_SAY[(p.uniq + (int)(tAcc * .2f)) % 4] : HOME_SAY[(roomIdx * 2 + p.uniq + (int)(tAcc * .2f)) % 6]); return true; }
  if (len(V3{chestPos.x, 0, chestPos.z} - me) < 2.6f) {   // a chest is a storage box: its first opening fills it once, then it keeps whatever you leave in it
    long long key = (long long)p.seed * 64 + chestBit; std::map<int, int>& ch = chests[key];
    if (!(p.vmask & (1u << chestBit))) { p.vmask |= 1u << chestBit; int a = resOf(p, 0), b = resOf(p, 1), na = 2 + (p.uniq + roomIdx) % 3 + (inRoom == 2 ? 2 : 0), nb = 1 + (p.uniq + roomIdx) % 2 + (inRoom == 2 ? 1 : 0);
      ch[a] += na; ch[b] += nb; if (difficulty > 0) ch[IT_FOOD + pkey(p) * 4 + 2 + (roomIdx & 1)] += 1 + (inRoom == 2 ? 1 : 0); if (inRoom == 2 && hasRare(p)) ch[IT_RARE + pkey(p)] += 1; audioEvent(S_DISCOVER); saveReq = true; }
    chestOpen = key; openModal(M_CHEST); return true; }
  if (P.z > RS - 2.6f && std::fabs(P.x) < 1.9f) { exitRoom(); return true; }
  return false;
}
void updRoom(float dt) {
  Planet& p = planets[curP]; walk(dt, roomH, 0, 1e9f, p.grav * perkGrav, clampf(1 / std::sqrt(std::max(.3f, p.grav)), .7f, 1.5f) * perkSpeed * (starving ? .65f : 1.f)); invul = std::max(0.f, invul - dt); updSurvival(dt); if (!inRoom) return;
  P.x = clampf(P.x, -RS + .7f, RS - .7f); P.z = clampf(P.z, -RS + .7f, RS - .7f); if (P.y > ROOMY + 2.3f) { P.y = ROOMY + 2.3f; if (P.vy > 0) P.vy = 0; }
  camPos.y = P.y + 1.75f; updTalk(dt); prompt.clear(); V3 me{P.x, 0, P.z};
  for (auto& n : roomNpcs) {
    n.ph += dt; V3 np{n.pos.x, 0, n.pos.z}, dv = me - np; float dist = len(dv), spd = 0; V3 mv{};
    if (n.wait > 0) n.wait -= dt;
    else {
      V3 t = n.dest - np; float td = len(t);
      if (td > .7f) { mv = norm(t); spd = 1.15f; }
      else { float a = n.ph * 1.1f + n.wphase + n.look; n.dest = {std::sin(a) * 2.6f, ROOMY, std::cos(a * .85f) * 2.2f}; n.dest.x = clampf(n.dest.x, -RS + 1.4f, RS - 1.4f); n.dest.z = clampf(n.dest.z, -RS + 1.4f, RS - 1.4f); n.wait = .8f + std::fmod(std::fabs(a), 1.8f); }
    }
    { float nx = n.pos.x + mv.x * spd * dt - me.x, nz = n.pos.z + mv.z * spd * dt - me.z; if (spd > 0 && nx * nx + nz * nz < 1.f && nx * nx + nz * nz < dist * dist) spd = 0; }   // do not walk into the player
    if (n.pushed) { V3 t = n.back - np; float td = len(t); if (n.wait > 0) n.wait += dt; if (td > .2f) { mv = norm(t); spd = 1.2f; } else { n.pos.x = n.back.x; n.pos.z = n.back.z; n.pushed = false; mv = {}; spd = 0; } }
    n.pos.x += mv.x * spd * dt; n.pos.z += mv.z * spd * dt; n.pos.y = ROOMY;
    { float px = n.pos.x - P.x, pz = n.pos.z - P.z, pd = std::sqrt(px * px + pz * pz); if (pd < .9f) { if (!n.pushed) { n.pushed = true; n.back = n.pos; } if (pd < 1e-3f) { px = 0; pz = -1; pd = 1; } n.pos.x = clampf(P.x + px / pd * .9f, -RS + .7f, RS - .7f); n.pos.z = clampf(P.z + pz / pd * .9f, -RS + .7f, RS - .7f); } } n.sp += (spd - n.sp) * std::min(1.f, dt * 8);
    V3 face = dist < 4.f ? dv : spd > .1f ? mv : dv; if (len(face) > .2f) n.yaw = std::atan2(face.x, face.z);
    if (prompt.empty() && dist < 3.4f) prompt = "[{E}] Talk to " + n.name;
  }
  if (prompt.empty() && len(V3{chestPos.x, 0, chestPos.z} - me) < 2.4f) prompt = "[{E}] Open the chest";
  if (prompt.empty() && P.z > RS - 2.6f && std::fabs(P.x) < 1.9f) prompt = "[{E}] Leave the house";
  audioSet(0, 0, 100, 0);
}
void drawRoom(const Planet& p) {
  glUniform3f(uAmb, .62f, .57f, .52f); glUniform3f(uSunCol, .22f, .2f, .18f); glUniform1f(uFog, 0);
  auto B = [&](int m, V3 pos, V3 s, V3 col, float em, GLuint tex = 0, float uvs = 1) { DP d; d.p = pos; d.s = s; d.t = col; d.emis = em; d.tex = tex ? tex : tWhite; d.uvs = uvs; draw(m == 0 ? mBox : m == 1 ? mCylN : m == 2 ? mSphereLo : mCone, d); };
  float W = RS * 2 + .8f; B(0, {0, ROOMY - .1f, 0}, {W, .2f, W}, floorC, 0, tConc, 4); B(0, {0, ROOMY + 4.5f, 0}, {W, .2f, W}, wallC * .7f, 0, tConc, 3);
  B(0, {-(RS + .2f), ROOMY + 2.2f, 0}, {.4f, 4.6f, W}, wallC, 0, tConc, 3); B(0, {RS + .2f, ROOMY + 2.2f, 0}, {.4f, 4.6f, W}, wallC, 0, tConc, 3);
  B(0, {0, ROOMY + 2.2f, -(RS + .2f)}, {W, 4.6f, .4f}, wallC, 0, tConc, 3); B(0, {0, ROOMY + 2.2f, RS + .2f}, {W, 4.6f, .4f}, wallC, 0, tConc, 3);
  V3 woodD{.25f, .15f, .08f}; B(0, {0, ROOMY + 1.05f, RS - .02f}, {1.5f, 2.1f, .12f}, woodD, 0); B(0, {0, ROOMY + 2.2f, RS - .05f}, {1.9f, .16f, .1f}, p.pal[3][0] > 0 ? lin(p.pal[3]) * 1.5f : V3{1, 1, 1}, 1.4f);   // the way out
  B(0, {-.85f, ROOMY + 1.05f, RS - .05f}, {.1f, 2.1f, .1f}, {1, .9f, .6f}, 1.2f); B(0, {.85f, ROOMY + 1.05f, RS - .05f}, {.1f, 2.1f, .1f}, {1, .9f, .6f}, 1.2f); B(0, {0, ROOMY + .03f, RS - 1.2f}, {1.6f, .05f, 1.1f}, inRoom == 2 ? V3{.4f, .03f, .03f} : V3{.5f, .35f, .2f}, 0);
  for (auto& q : props) { V3 pos = q.p; pos.y += ROOMY; if (q.m == 0) B(0, pos, q.s, q.c, q.em, q.em > 0 ? tConc : (tWood ? tWood : tConc), 1); else if (q.m == 1) B(1, pos, q.s, q.c, q.em); else if (q.m == 2) B(2, pos, q.s, q.c, q.em); else B(3, pos, q.s, q.c, q.em); }
  useMain();
}
// ---------------------------------------------------------------- props outside: workbench, market, wicked house, ore nodes
void drawProps(const Planet& p) {
  V3 acc = lin(p.pal[3]) * 1.3f; auto B = [&](const Mesh& m, V3 pos, V3 s, V3 col, float em, V3 X = {1, 0, 0}, V3 Z = {0, 0, 1}, GLuint tex = 0) { DP d; d.p = pos; d.s = s; d.X = X; d.Z = Z; d.t = col; d.emis = em; d.tex = tex ? tex : tWhite; draw(m, d); };
  { V3 b = benchPos; b.y = shAt(b.x, b.z); B(mBox, b + V3{0, .5f, 0}, {2.6f, 1.f, 1.2f}, {.35f, .37f, .42f}, 0, {1, 0, 0}, {0, 0, 1}, tHull); B(mBox, b + V3{-.7f, 1.15f, 0}, {.6f, .3f, .5f}, {.1f, .1f, .12f}, 0); B(mBox, b + V3{.6f, 1.05f, .1f}, {.7f, .1f, .5f}, {.2f, .8f, 1}, 1.6f);
    B(mBox, b + V3{0, 1.9f, -.55f}, {2.f, 1.f, .08f}, {.2f, .7f, 1}, 1.2f); B(mCylN, b + V3{-1.2f, 0, -.5f}, {.07f, 2.4f, .07f}, {.3f, .3f, .35f}, 0); B(mCylN, b + V3{1.2f, 0, -.5f}, {.07f, 2.4f, .07f}, {.3f, .3f, .35f}, 0); }
  { V3 b = marketPos; b.y = shAt(b.x, b.z); V3 Z{std::sin(marketYaw), 0, std::cos(marketYaw)}, X{Z.z, 0, -Z.x}; Offer o[NOFFER]; marketOffers(p, o);
    B(mBox, b + V3{0, .55f, 0}, {3.4f, 1.1f, 1.1f}, {.4f, .26f, .14f}, 0, X, Z, tWood ? tWood : tConc);
    if (difficulty > 0) for (int k = 0; k < 2; k++) { V3 fc = foodColor(pkey(p) * 4 + 2 + k), c = b + X * (k ? 1.25f : -1.25f) + Z * .15f + V3{0, 1.1f, 0}; B(mCylN, c, {.32f, .06f, .32f}, {.85f, .82f, .75f}, 0); B(mSphereLo, c + V3{0, .12f, 0}, {.22f, .14f, .22f}, fc, .35f); }   // the dishes for sale for (int s = -1; s <= 1; s += 2) B(mCylN, b + X * (s * 1.9f) - Z * .9f, {.09f, 3.2f, .09f}, {.35f, .22f, .12f}, 0);
    for (int k = 0; k < 5; k++) B(mBox, b + X * ((k - 2) * .88f) + V3{0, 3.2f, 0} - Z * .1f, {.88f, .16f, 2.7f}, (k & 1) ? acc : V3{.95f, .92f, .85f}, .3f, X, Z);
    int ids[4] = {o[0].rr, o[1].rr, o[0].gr, o[1].gr}; for (int k = 0; k < 4; k++) B(mBox, b + X * ((k - 1.5f) * .75f) + V3{0, 1.28f, .1f}, {.4f, .36f, .4f}, RES_COL[ids[k]] * 1.3f, 1.2f, X, Z); }
  if (!p.isMoon) { V3 b = wHouse; b.y = shAt(b.x, b.z); V3 dk{.07f, .05f, .06f};
    B(mBox, b + V3{0, 2.2f, 0}, {7.f, 4.4f, 7.f}, dk, 0, {1, 0, 0}, {0, 0, 1}, tConc); B(mBox, b + V3{0, 4.6f, 0}, {7.8f, .4f, 7.8f}, {.14f, .02f, .03f}, 0, {1, 0, 0}, {0, 0, 1}, tRock);
    B(mCone, b + V3{0, 4.8f, 0}, {5.6f, 3.4f, 5.6f}, {.12f, .02f, .03f}, 0, {1, 0, 0}, {0, 0, 1}, tRock);
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) B(mCone, b + V3{sx * 3.7f, 4.7f, sz * 3.7f}, {.45f, 2.f, .45f}, {.04f, .03f, .03f}, .3f);
    B(mBox, b + V3{-3.53f, 1.05f, 0}, {.16f, 2.1f, 1.6f}, {.02f, .01f, .01f}, 0); B(mBox, b + V3{-3.6f, 2.2f, 0}, {.1f, .18f, 2.2f}, {1, .1f, .05f}, 2.f); B(mSphereLo, b + V3{-3.6f, 3.3f, 0}, {.5f, .55f, .5f}, {.9f, .88f, .8f}, .1f);
    for (int s = -1; s <= 1; s += 2) { B(mBox, b + V3{-3.52f, 2.6f, s * 2.4f}, {.1f, .8f, .8f}, {1, .15f, .05f}, 2.f); B(mBox, b + V3{s * 2.4f, 2.6f, 3.52f}, {.8f, .8f, .1f}, {1, .15f, .05f}, 2.f); } }
  for (auto& nd : nodes) {
    if (nd.res == 100 || nd.res == 101) {   // wild food: every people has its own two, and they grow in different ways
      int id = pkey(p) * 4 + (nd.res - 100), sh = foodShape(id); V3 fc = foodColor(id), g = lin(p.pal[2]) * .9f + V3{.05f, .12f, .04f}, q = nd.pos; GLuint lf = tLeafW ? tLeafW : tWhite, bk = tBarkW ? tBarkW : tWhite;
      if (nd.cool > 0) { B(mSphereLo, q + V3{0, .1f, 0}, {.7f, .3f, .7f}, g * .6f, 0, {1, 0, 0}, {0, 0, 1}, lf); continue; }   // picked: only the leaves are left
      switch (sh) {
        case 0: B(mSphereLo, q + V3{0, .5f, 0}, {.95f, .6f, .95f}, g, 0, {1, 0, 0}, {0, 0, 1}, lf); for (int k = 0; k < 7; k++) { float a = k * .9f; B(mSphereLo, q + V3{std::cos(a) * .62f, .62f + (k & 1) * .25f, std::sin(a) * .62f}, {.15f, .15f, .15f}, fc, .9f); } break;   // berry bush
        case 1: for (int k = 0; k < 4; k++) { float a = k * 1.6f, r = k ? .45f : 0.f, s = k ? .65f : 1.f; V3 c = q + V3{std::cos(a) * r, 0, std::sin(a) * r}; B(mCylN, c, {.09f * s, .55f * s, .09f * s}, {.9f, .88f, .8f}, .1f); B(mSphereLo, c + V3{0, .55f * s, 0}, {.38f * s, .2f * s, .38f * s}, fc, .7f); } break;   // mushrooms
        case 2: B(mSphereLo, q + V3{0, .08f, 0}, {.35f, .25f, .35f}, fc * .8f, .3f); for (int k = 0; k < 5; k++) { float a = k * 1.25f; B(mCone, q + V3{std::cos(a) * .1f, .15f, std::sin(a) * .1f}, {.12f, .9f, .12f}, g * 1.2f, 0, {std::cos(a), 0, -std::sin(a)}, {std::sin(a), 0, std::cos(a)}, lf); } break;   // root with a leaf crown
        case 3: B(mCylN, q, {.6f, .25f, .6f}, {.35f, .25f, .14f}, 0, {1, 0, 0}, {0, 0, 1}, bk); for (int k = 0; k < 3; k++) { float a = k * 2.1f; B(mSphereLo, q + V3{std::cos(a) * .22f, .32f, std::sin(a) * .22f}, {.16f, .21f, .16f}, fc, .5f); } break;   // nest with eggs
        case 4: B(mCylN, q, {.1f, 1.3f, .1f}, {.3f, .2f, .12f}, 0, {1, 0, 0}, {0, 0, 1}, bk); B(mSphereLo, q + V3{0, 1.45f, 0}, {.6f, .45f, .6f}, g, 0, {1, 0, 0}, {0, 0, 1}, lf); for (int k = 0; k < 5; k++) { float a = k * 1.25f; B(mSphereLo, q + V3{std::cos(a) * .45f, 1.15f, std::sin(a) * .45f}, {.11f, .13f, .11f}, fc, .6f); } break;   // nut shrub
        case 5: B(mSphereLo, q + V3{0, .32f, 0}, {.5f, .38f, .45f}, fc, .45f); for (int k = 0; k < 4; k++) { float a = k * 1.57f; B(mSphereLo, q + V3{std::cos(a) * .7f, .06f, std::sin(a) * .7f}, {.32f, .06f, .2f}, g, 0, {1, 0, 0}, {0, 0, 1}, lf); } break;   // melon on its vine
        case 6: for (int k = 0; k < 3; k++) { float a = k * 2.1f + .4f; V3 c = q + V3{std::cos(a) * .3f, 0, std::sin(a) * .3f}; B(mCylN, c, {.05f, 1.1f + k * .2f, .05f}, g, 0); B(mSphereLo, c + V3{0, 1.1f + k * .2f, 0}, {.13f, .26f, .13f}, fc, .9f); } break;   // pod stalks
        default: B(mCylN, q, {.42f, .5f, .42f}, fc, .5f); B(mCylN, q + V3{0, .5f, 0}, {.3f, .08f, .3f}, fc * 1.2f, .9f); break;   // honeycomb
      }
      continue; }
    if (nd.res == 200) { V3 c = rareColor(pkey(p)); if (nd.cool > 0) { B(mSphereLo, nd.pos + V3{0, .2f, 0}, {.7f, .35f, .7f}, {.2f, .2f, .22f}, 0, {1, 0, 0}, {0, 0, 1}, tRock); continue; }   // the rare find: a crystal growing out of dark stone
      B(mSphereLo, nd.pos + V3{0, .25f, 0}, {1.1f, .55f, 1.1f}, {.12f, .1f, .13f}, 0, {1, 0, 0}, {0, 0, 1}, tRock);
      for (int k = 0; k < 4; k++) { float a = k * 1.57f, r = k ? .35f : 0.f; B(mCone, nd.pos + V3{std::cos(a) * r, .3f, std::sin(a) * r}, {.22f, k ? 1.1f : 2.f, .22f}, c, 2.2f + std::sin(tAcc * 3 + k) * .6f); } continue; }
    V3 c = RES_COL[nd.res]; if (nd.cool > 0) { B(mSphereLo, nd.pos + V3{0, .2f, 0}, {.6f, .3f, .6f}, {.2f, .2f, .2f}, 0, {1, 0, 0}, {0, 0, 1}, tRock); continue; }
    bool hard = nd.need != 0; B(mSphereLo, nd.pos + V3{0, .2f, 0}, hard ? V3{1.15f, .65f, 1.15f} : V3{.9f, .45f, .9f}, hard ? V3{.24f, .24f, .27f} : V3{.35f, .35f, .38f}, 0, {1, 0, 0}, {0, 0, 1}, tRock);   // pickaxe-only ore sits in a bigger, darker boulder
    B(mCone, nd.pos + V3{0, .2f, 0}, {.38f, 1.9f, .38f}, c * 1.5f, 1.3f); B(mCone, nd.pos + V3{.45f, .15f, .1f}, {.28f, 1.3f, .28f}, c * 1.3f, 1.1f); B(mCone, nd.pos + V3{-.4f, .15f, -.2f}, {.3f, 1.1f, .3f}, c * 1.4f, 1.1f);
    if (hard) for (int k = 0; k < 3; k++) { float a = k * 2.1f + .5f; B(mBox, nd.pos + V3{std::cos(a) * .75f, .5f, std::sin(a) * .75f}, {.35f, .5f, .3f}, {.2f, .2f, .23f}, 0, {std::cos(a), 0, -std::sin(a)}, {std::sin(a), 0, std::cos(a)}, tRock); } }
}
