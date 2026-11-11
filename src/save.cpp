// Saving and loading the open world.
#include "game.h"

// ================================================================ SAVE SYSTEM (text file in the user's data folder)
std::string dataDir() {
  namespace sfs = std::filesystem; std::string dir = ".";
#ifdef _WIN32
  if (const char* a = std::getenv("APPDATA")) dir = std::string(a) + "/Beyond";
#elif defined(__APPLE__)
  if (const char* h = std::getenv("HOME")) dir = std::string(h) + "/Library/Application Support/Beyond";
#else
  const char* x = std::getenv("XDG_DATA_HOME"); const char* h = std::getenv("HOME"); if (x) dir = std::string(x) + "/beyond"; else if (h) dir = std::string(h) + "/.local/share/beyond";
#endif
  std::error_code ec; sfs::create_directories(dir, ec); return dir;
}
std::string worldDir(int id) { return dataDir() + "/worlds/w" + std::to_string(id); }
std::string savePath() { return worldDir(curWorld) + "/save.txt"; }
void syncPSaves() {
  for (auto& p : planets) { if (!p.enterable) continue; if (!(p.qi || p.qph || p.prog || p.mask || p.found || p.vmask)) continue; PSave& q = psaves[p.seed]; q.qi = p.qi; q.qph = p.qph; q.prog = p.prog; q.mask = p.mask; q.vmask = p.vmask; q.found = p.found ? 1 : 0; q.sys = sysId; q.uniq = p.uniq; q.kind = p.kind; q.moon = p.isMoon; q.name = p.name; }
}
void applyPSaves() {
  for (auto& p : planets) { auto it = psaves.find(p.seed); if (it == psaves.end()) continue; const PSave& e = it->second; p.qi = e.qi; p.qph = e.qph; p.prog = e.prog; p.mask = e.mask; p.vmask = e.vmask; p.found = e.found != 0; if (p.found && p.enterable) p.qi = nqOf(p); }
}
std::string checkpointPath() { return worldDir(curWorld) + "/checkpoint.txt"; }
bool hasCheckpoint() { std::error_code ec; return curWorld >= 0 && std::filesystem::exists(checkpointPath(), ec); }
// One writer for both kinds of save. cp == nullptr: the normal save of what is happening now.
// cp != nullptr: the hidden checkpoint - the same save, but with the rocket parked in space in front of that world (cpDir = direction from the world to the rocket).
static bool writeSave(const std::string& path, const Planet* cp, V3 cpDir) {
  std::string tmp = path + ".tmp"; FILE* f = std::fopen(tmp.c_str(), "w"); if (!f) return false;
  int stOut = cp ? (int)SPACE : (int)st; V3 sp = S.pos, entry = spaceEntry; float syaw = S.yaw, spitch = S.pitch, sthr = S.thr;
  if (cp) { sp = cp->pos + cpDir * (cp->R * 1.5f); entry = sp; syaw = std::atan2(cpDir.x, cpDir.z); spitch = std::asin(clampf(-cpDir.y, -1, 1)); sthr = .2f; }
  std::fprintf(f, "BEYOND_SAVE 2\nstate %d\nsys %d\nfound %d\n", stOut, sysId, foundN);
  if (stOut == EARTH) std::fprintf(f, "earth %.3f %.3f %.4f\n", P.x, P.z, P.yaw);
  std::fprintf(f, "parts"); for (auto& p : picks) std::fprintf(f, " %d %d", p.got ? 1 : 0, p.inst ? 1 : 0); std::fprintf(f, "\n");
  if (stOut != EARTH) std::fprintf(f, "ship %.2f %.2f %.2f %.4f %.4f %.3f\nentry %.2f %.2f %.2f\n", sp.x, sp.y, sp.z, syaw, spitch, sthr, entry.x, entry.y, entry.z);
  if (cp) std::fprintf(f, "cpplanet %d %.4f %.4f %.4f\n", cp->seed, cpDir.x, cpDir.y, cpDir.z);
  if (stOut == SURFACE && curP >= 0) std::fprintf(f, "cur %d\nsurf %.3f %.3f %.4f %d\n", planets[curP].seed, P.x, P.z, P.yaw, hp);
  syncPSaves(); visitedSys.insert(sysId);
  for (auto& kv : psaves) { const PSave& q = kv.second; std::fprintf(f, "planet %d %d %d %d %u %d\n", kv.first, q.qi, q.qph, q.prog, q.mask, q.found); if (q.vmask) std::fprintf(f, "poi %d %u\n", kv.first, q.vmask);
    std::string nm = cleanName(q.name); std::fprintf(f, "pinfo %d %d %d %d %d %s\n", kv.first, q.sys, q.uniq, q.kind, q.moon ? 1 : 0, nm.c_str()); }
  std::fprintf(f, "surv %d %.2f 0\nending %d %d\n", difficulty, food, endSeen ? 1 : 0, moonEndSeen ? 1 : 0);
  // foods, rare finds, tools and chest contents: "<tag> key count ... -1" (a list that ends with -1)
  std::fprintf(f, "foods"); for (auto& kv : foods) if (kv.second > 0) std::fprintf(f, " %d %d", kv.first, kv.second); std::fprintf(f, " -1\n");
  std::fprintf(f, "rares"); for (auto& kv : rares) if (kv.second > 0) std::fprintf(f, " %d %d", kv.first, kv.second); std::fprintf(f, " -1\n");
  std::fprintf(f, "swords %d", swordSel); for (int k : swords) std::fprintf(f, " %d", k); std::fprintf(f, " -1\npickaxes"); for (int k : pickaxes) std::fprintf(f, " %d", k); std::fprintf(f, " -1\n");
  for (auto& kv : chests) { bool any = false; for (auto& it : kv.second) any = any || it.second > 0; if (!any) continue; std::fprintf(f, "chest %lld", kv.first); for (auto& it : kv.second) if (it.second > 0) std::fprintf(f, " %d %d", it.first, it.second); std::fprintf(f, " -1\n"); }
  std::fprintf(f, "inv"); for (int i = 0; i < NRES; i++) std::fprintf(f, " %d", inv[i]); std::fprintf(f, "\nrock %d %d %d %d\nsystems", rocketMask, rocketSel, meds, deaths); for (int s : visitedSys) std::fprintf(f, " %d", s); std::fprintf(f, " -1\n");
  bool ok = std::fclose(f) == 0; std::error_code ec; if (ok) std::filesystem::rename(tmp, path, ec); return ok && !ec;
}
void saveGame() {
  if (curWorld < 0 || st == COUNT || st == LAUNCH || modal == M_DEATH) return;   // on the death screen the save is already the checkpoint: leave it alone
  writeSave(savePath(), nullptr, {}); savedL = 2.5f;
}
// automatic and silent: no sound, no message, no "saved" text - the player never learns it exists
void makeCheckpoint(const Planet& p) {
  if (curWorld < 0) return; V3 dv = norm(spaceEntry - p.pos); if (len(dv) < .5f) dv = {0, 0, 1};
  writeSave(checkpointPath(), &p, dv);
}
void commitCheckpoint() {
  if (!hasCheckpoint()) return; namespace sfs = std::filesystem; std::error_code ec; std::string tmp = savePath() + ".tmp";
  sfs::copy_file(checkpointPath(), tmp, sfs::copy_options::overwrite_existing, ec); if (!ec) sfs::rename(tmp, savePath(), ec);
}
static int gCpSeed = 0; static V3 gCpDir;
static bool loadFile(const std::string& path) {
  FILE* f = std::fopen(path.c_str(), "r"); if (!f) return false; char tag[32]; int ver = 0; gCpSeed = 0;
  if (std::fscanf(f, "%31s %d", tag, &ver) != 2 || std::strcmp(tag, "BEYOND_SAVE") != 0) { std::fclose(f); return false; }
  psaves.clear(); visitedSys.clear(); std::fill(inv, inv + NRES, 0); rocketMask = 1; rocketSel = 0; meds = 0; deaths = 0; food = 20; foodT = starveT = regenT = 0; starving = hungryWarned = false;
  foods.clear(); rares.clear(); swords.clear(); pickaxes.clear(); swordSel = -1; chests.clear(); endSeen = moonEndSeen = false;
  int state = 0, sys = 0, found = 0, cur = 0, g[5] = {0, 0, 0, 0, 0}, in[5] = {0, 0, 0, 0, 0}, hp_ = MAXHP; float ep[3] = {30, 50, .54f}, sh[6] = {0, 0, 0, 0, .3f, .35f}, en[3] = {0, 0, 0}, sf[3] = {7, 7, 0};
#define RD(...) (void)std::fscanf(f, __VA_ARGS__)
  while (std::fscanf(f, "%31s", tag) == 1) {
    if (!std::strcmp(tag, "state")) RD("%d", &state); else if (!std::strcmp(tag, "sys")) RD("%d", &sys); else if (!std::strcmp(tag, "found")) RD("%d", &found);
    else if (!std::strcmp(tag, "earth")) RD("%f %f %f", &ep[0], &ep[1], &ep[2]); else if (!std::strcmp(tag, "parts")) { for (int i = 0; i < 5; i++) RD("%d %d", &g[i], &in[i]); }
    else if (!std::strcmp(tag, "ship")) RD("%f %f %f %f %f %f", &sh[0], &sh[1], &sh[2], &sh[3], &sh[4], &sh[5]); else if (!std::strcmp(tag, "entry")) RD("%f %f %f", &en[0], &en[1], &en[2]);
    else if (!std::strcmp(tag, "cpplanet")) RD("%d %f %f %f", &gCpSeed, &gCpDir.x, &gCpDir.y, &gCpDir.z);
    else if (!std::strcmp(tag, "cur")) RD("%d", &cur); else if (!std::strcmp(tag, "surf")) RD("%f %f %f %d", &sf[0], &sf[1], &sf[2], &hp_);
    else if (!std::strcmp(tag, "planet")) { int sd = 0; PSave e; RD("%d %d %d %d %u %d", &sd, &e.qi, &e.qph, &e.prog, &e.mask, &e.found); PSave& q = psaves[sd]; q.qi = e.qi; q.qph = e.qph; q.prog = e.prog; q.mask = e.mask; q.found = e.found; }
    else if (!std::strcmp(tag, "poi")) { int sd = 0; unsigned m = 0; RD("%d %u", &sd, &m); psaves[sd].vmask = m; }
    else if (!std::strcmp(tag, "pinfo")) { int sd = 0, sy = 0, un = 0, kd = 0, mo = 0; char nm[64] = {0}; if (std::fscanf(f, "%d %d %d %d %d", &sd, &sy, &un, &kd, &mo) == 5 && std::fgets(nm, sizeof nm, f)) { std::string n = nm; while (!n.empty() && (n.back() == '\n' || n.back() == '\r')) n.pop_back(); while (!n.empty() && n[0] == ' ') n.erase(0, 1);   /* read to the end of THIS line only: an empty name must not swallow the next line */ PSave& q = psaves[sd]; q.sys = sy; q.uniq = un; q.kind = kd; q.moon = mo != 0; q.name = n; } }
    else if (!std::strcmp(tag, "surv")) { int d = 1; float fd = 20; int fr = 0; RD("%d %f %d", &d, &fd, &fr); (void)d; food = fd; if (fr > 0) foods[0] += fr; }   // fruit of older saves becomes plain wild food
    else if (!std::strcmp(tag, "foods") || !std::strcmp(tag, "rares")) { auto& m = tag[0] == 'f' ? foods : rares; int k = 0, n = 0; while (std::fscanf(f, "%d", &k) == 1 && k >= 0 && std::fscanf(f, "%d", &n) == 1) if (n > 0) m[k] = n; }
    else if (!std::strcmp(tag, "swords")) { int k = 0; RD("%d", &swordSel); while (std::fscanf(f, "%d", &k) == 1 && k >= 0) swords.insert(k); if (!swords.count(swordSel)) swordSel = swords.empty() ? -1 : *swords.begin(); }
    else if (!std::strcmp(tag, "pickaxes")) { int k = 0; while (std::fscanf(f, "%d", &k) == 1 && k >= 0) pickaxes.insert(k); }
    else if (!std::strcmp(tag, "chest")) { long long key = 0; int k = 0, n = 0; RD("%lld", &key); auto& m = chests[key]; while (std::fscanf(f, "%d", &k) == 1 && k >= 0 && std::fscanf(f, "%d", &n) == 1) if (n > 0) m[k] = n; }
    else if (!std::strcmp(tag, "cp")) { int sd = 0; float x = 0, z = 0; RD("%d %f %f", &sd, &x, &z); (void)sd; (void)x; (void)z; }   // visible checkpoint pillars of older saves: gone
    else if (!std::strcmp(tag, "ending")) { int e = 0, m = 0; RD("%d %d", &e, &m); endSeen = e != 0; moonEndSeen = m != 0; }
    else if (!std::strcmp(tag, "inv")) { for (int i = 0; i < NRES; i++) RD("%d", &inv[i]); }
    else if (!std::strcmp(tag, "rock")) RD("%d %d %d %d", &rocketMask, &rocketSel, &meds, &deaths);
    else if (!std::strcmp(tag, "systems")) { int v = 0; while (std::fscanf(f, "%d", &v) == 1 && v >= 0) visitedSys.insert(v); }
  }
#undef RD
  std::fclose(f); if (state < 0 || state > 4 || state == COUNT || state == LAUNCH) state = EARTH; if (sys < 0 || sys > 100000) sys = 0;
  sysId = sys; foundN = std::max(0, found); gotN = instN = 0; for (int i = 0; i < 5; i++) { picks[i].got = g[i] != 0; picks[i].inst = in[i] != 0; gotN += g[i] ? 1 : 0; instN += in[i] ? 1 : 0; }
  if (rocketSel < 0 || rocketSel > 5 || !((rocketMask >> rocketSel) & 1)) rocketSel = 0; rocketMask |= 1; visitedSys.insert(sysId);
  buildSystem(sysId);   // applies the remembered progress of every planet and moon in this system
  if (gCpSeed && state != EARTH) for (auto& p : planets) if (p.seed == gCpSeed) {   // checkpoint: park in front of that world (moons orbit, so this is worked out from where the world is now)
    V3 dv = norm(gCpDir); if (len(dv) < .5f) dv = {0, 0, 1}; V3 q = p.pos + dv * (p.R * 1.5f); sh[0] = en[0] = q.x; sh[1] = en[1] = q.y; sh[2] = en[2] = q.z; sh[3] = std::atan2(dv.x, dv.z); sh[4] = std::asin(clampf(-dv.y, -1, 1)); sh[5] = .2f; state = SPACE; break; }
  for (auto& p : planets) { if (p.found && p.enterable) p.qi = nqOf(p); else if (ver < 2) { p.qph = 0; p.prog = 0; p.mask = 0; } }
  if (state == EARTH || state == COUNT) { P = Player(); P.x = ep[0]; P.z = ep[1]; P.yaw = ep[2]; P.y = hAt(P.x, P.z); st = EARTH; return true; }
  st = SPACE; ambient = {.012f, .012f, .018f}; fogDen = 0; nearZ = 2; S = Ship(); S.pos = {sh[0], sh[1], sh[2]}; S.yaw = sh[3]; S.pitch = sh[4]; S.thr = clampf(sh[5], 0, 1); S.cyaw = S.yaw; S.cpitch = S.pitch; S.speed = S.thr * 900;
  V3 r, u, fw; basis(S.yaw, S.pitch, 0, r, u, fw); camPos = S.pos + fw * -26.f + u * 5.f; cR = r; cU = u; cF = fw;
  if (state == SURFACE && cur) for (int i = 0; i < (int)planets.size(); i++) if (planets[i].seed == cur) { S.pos = {en[0], en[1], en[2]}; enterPlanet(i, false); P.x = sf[0]; P.z = sf[1]; P.yaw = sf[2]; P.y = shAt(P.x, P.z) + .5f; hp = hp_ > 0 ? std::min(hp_, MAXHP) : MAXHP; break; }
  return true;
}
bool loadGame() { return loadFile(savePath()); }
// Dying: wake up in space in front of the last world you discovered, with the save from the moment you discovered it
bool respawnFromCheckpoint() {
  if (!hasCheckpoint()) return false;
  { FILE* f = std::fopen(checkpointPath().c_str(), "r"); if (!f) return false; char tag[32]; int ver = 0; bool ok = std::fscanf(f, "%31s %d", tag, &ver) == 2 && !std::strcmp(tag, "BEYOND_SAVE"); std::fclose(f); if (!ok) return false; }
  int keepDeaths = deaths; std::string title = "You wake up..."; double lastDraw = -1; loadingFrame(title, 0);
  buildProgress = [&](float f) { double now = glfwGetTime(); if (now - lastDraw > .033) { lastDraw = now; loadingFrame(title, f); } };
  resetWorldState(); bool ok = loadFile(checkpointPath()); buildProgress = nullptr;
  if (!ok) ok = loadFile(savePath());
  deaths = keepDeaths; hp = MAXHP; food = std::max(food, 14.f); starving = false; invul = 3.f; flashA = 1; mdx = mdy = 0; mouseFresh = true; modal = 0; paused = false; syncModes(); uiFlushMouse();
  std::string where; for (auto& p : planets) if (p.seed == gCpSeed) where = p.name;
  toast(where.empty() ? std::string("YOU WAKE UP") : "YOU WAKE UP\nBack in space, in front of " + where, 4);
  saveGame(); savedL = 0; sinceSave = 0; lastSig = sigOf(); return ok;
}
// back to a blank slate: called whenever a world is opened or closed, so nothing leaks from one world into another
void resetWorldState() {
  freeMesh(mSurf); if (tSurf) { glDeleteTextures(1, &tSurf); tSurf = 0; } npcs.clear(); objs.clear(); pois.clear(); flora.clear(); boulders.clear(); perkJump = perkSpeed = perkGrav = 1; curP = -1; hp = MAXHP; invul = 0; raceT = 0; nearEnter = -1;
  st = EARTH; gotN = instN = 0; psaves.clear(); visitedSys.clear(); std::fill(inv, inv + NRES, 0); rocketMask = 1; rocketSel = 0; meds = 0; deaths = 0; food = 20; foodT = starveT = regenT = 0; starving = hungryWarned = false; foods.clear(); rares.clear(); swords.clear(); pickaxes.clear(); swordSel = -1; chests.clear(); chestOpen = 0; flats.clear(); swingT = mineT = 0; inRoom = 0; roomNpcs.clear(); hits.clear(); hitsSaved.clear(); nodes.clear(); cine = 0; surfRocketY = .9f; modal = 0; for (auto& p : picks) { p.got = false; p.inst = false; } P = Player(); P.y = hAt(P.x, P.z); S = Ship(); sysId = 0; foundN = 0; rocketY = .9f; cd = 0; vyL = 0; parts.clear(); partI = 0; flashA = 0; talkL = 0; toastL = 0; savedL = 0; prompt.clear(); saveReq = false; paused = false; endT = -1; endSeen = moonEndSeen = false; farZ = 80000.f;
  fogDen = .0035f; ambient = {.3f, .36f, .45f}; fogCol = {.28f, .55f, 1.f}; fov = 72; nearZ = .3f; mdx = mdy = 0; buildEarthHits();
}
int sigOf() { long long s = (long long)st * 1000003LL + sysId * 131 + foundN * 37 + rocketMask * 53 + meds * 59 + gotN * 41 + instN * 43 + curP * 101; for (int i = 0; i < NRES; i++) s += inv[i] * (i + 3); for (auto& kv : foods) s += kv.second * (kv.first % 97 + 7); for (auto& kv : rares) s += kv.second * 131; s += (long long)swords.size() * 991 + (long long)pickaxes.size() * 577 + swordSel; for (auto& p : planets) s += p.qi * 13 + p.qph * 7 + p.prog * 5 + (long long)p.mask * 3 + (p.found ? 17 : 0) + (long long)p.vmask * 11; return (int)(s ^ (s >> 32)); }
