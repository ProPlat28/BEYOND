// The world list on disk, settings, and opening or closing a world.
#include "game.h"

// ================================================================ WORLDS
// the HUD font only has printable ASCII, so stored names are checked before they are ever drawn
std::string cleanName(std::string s) {
  for (auto& c : s) if ((unsigned char)c < 32 || (unsigned char)c > 126) c = '?';
  size_t a = s.find_first_not_of(' '); if (a == std::string::npos) return ""; size_t b = s.find_last_not_of(' '); s = s.substr(a, b - a + 1);
  if (s.size() > WORLD_NAME_LEN) { s.resize(WORLD_NAME_LEN); while (!s.empty() && s.back() == ' ') s.pop_back(); }
  return s;
}
static bool writeWorldMeta(const World& w) {
  std::string path = worldDir(w.id) + "/world.txt", tmp = path + ".tmp"; FILE* f = std::fopen(tmp.c_str(), "w"); if (!f) return false;
  std::fprintf(f, "BEYOND_WORLD 1\nseed %u\ncreated %lld\nplayed %lld\ndifficulty %d\nname %s\n", w.seed, w.created, w.played, w.difficulty, w.name.c_str());
  if (!w.devTag.empty()) std::fprintf(f, "dev %s\n", w.devTag.c_str());   // checked against the world itself (see devtools.cpp), so copying it to another world does nothing
  bool ok = std::fclose(f) == 0; std::error_code ec; if (ok) std::filesystem::rename(tmp, path, ec); return ok && !ec;
}
static bool readWorld(int id, World& w) {
  FILE* f = std::fopen((worldDir(id) + "/world.txt").c_str(), "r"); if (!f) return false; char line[256]; w = World(); w.id = id;
  while (std::fgets(line, sizeof line, f)) { std::string s = line; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    if (s.rfind("name ", 0) == 0) w.name = cleanName(s.substr(5)); else if (s.rfind("seed ", 0) == 0) w.seed = (unsigned)std::strtoul(s.c_str() + 5, nullptr, 10) % 1000;
    else if (s.rfind("difficulty ", 0) == 0) w.difficulty = std::max(0, std::min(2, std::atoi(s.c_str() + 11)));   // worlds made before this existed stay NORMAL
    else if (s.rfind("dev ", 0) == 0) w.devTag = s.substr(4, 64);
    else if (s.rfind("created ", 0) == 0) w.created = std::atoll(s.c_str() + 8); else if (s.rfind("played ", 0) == 0) w.played = std::atoll(s.c_str() + 7); }
  std::fclose(f); if (w.name.empty()) w.name = "Unnamed world";
  // read just enough of the save to describe the world in the list
  if (FILE* g = std::fopen((worldDir(id) + "/save.txt").c_str(), "r")) { char tag[32];
    while (std::fscanf(g, "%31s", tag) == 1) {
      if (!std::strcmp(tag, "state")) (void)std::fscanf(g, "%d", &w.state); else if (!std::strcmp(tag, "sys")) (void)std::fscanf(g, "%d", &w.sys); else if (!std::strcmp(tag, "found")) (void)std::fscanf(g, "%d", &w.found);
      else if (!std::strcmp(tag, "parts")) { for (int i = 0; i < 5; i++) { int a = 0, b = 0; if (std::fscanf(g, "%d %d", &a, &b) == 2 && a) w.got++; } } }
    std::fclose(g); }
  return true;
}
void scanWorlds() {
  namespace sfs = std::filesystem; worlds.clear(); std::error_code ec;
  for (sfs::directory_iterator it(dataDir() + "/worlds", ec), end; !ec && it != end; it.increment(ec)) {
    std::string n = it->path().filename().string(); if (n.size() < 2 || n[0] != 'w' || n.find_first_not_of("0123456789", 1) != std::string::npos || n.size() > 9) continue;
    World w; if (readWorld(std::atoi(n.c_str() + 1), w)) worlds.push_back(w); }
  std::sort(worlds.begin(), worlds.end(), [](const World& a, const World& b) { return a.played != b.played ? a.played > b.played : a.id > b.id; });
}
World* findWorld(int id) { for (auto& w : worlds) if (w.id == id) return &w; return nullptr; }
// returns the new world's id, or -1 if it could not be written to disk
static int makeWorld(const std::string& name, unsigned seed, int diff = 1) {
  namespace sfs = std::filesystem; std::error_code ec; int id = 1; while (sfs::exists(worldDir(id), ec)) id++;
  sfs::create_directories(worldDir(id), ec); if (ec) return -1;
  World w; w.id = id; w.seed = seed; w.name = name; w.difficulty = std::max(0, std::min(2, diff)); w.created = w.played = (long long)std::time(nullptr); return writeWorldMeta(w) ? id : -1;
}
int createWorld(const std::string& name, int diff) { std::random_device rd; return makeWorld(name, 1 + rd() % 999, diff); }
// copy = the whole world folder (save, hidden checkpoint, seed, difficulty) under a new name: "World 1" -> "World 1 (1)", then "World 1 (2)"...
int copyWorld(int id) {
  namespace sfs = std::filesystem; World* src = findWorld(id); if (!src) return -1; World w = *src; std::error_code ec; int nid = 1; while (sfs::exists(worldDir(nid), ec)) nid++;
  sfs::copy(worldDir(id), worldDir(nid), sfs::copy_options::recursive, ec); if (ec) { std::error_code e2; sfs::remove_all(worldDir(nid), e2); return -1; }
  std::string base = w.name; size_t op = base.rfind(" ("); if (op != std::string::npos && base.size() > op + 3 && base.back() == ')' && base.find_first_not_of("0123456789", op + 2) == base.size() - 1) base.resize(op);   // copying "X (1)" gives "X (2)", not "X (1) (1)"
  std::string name; for (int n = 1;; n++) { std::string suf = " (" + std::to_string(n) + ")"; std::string b = base; if (b.size() + suf.size() > WORLD_NAME_LEN) b.resize(WORLD_NAME_LEN - suf.size()); while (!b.empty() && b.back() == ' ') b.pop_back();
    name = b + suf; bool used = false; for (auto& o : worlds) if (o.name == name) used = true; if (!used) break; }
  w.id = nid; w.name = name; w.created = w.played = (long long)std::time(nullptr); if (!writeWorldMeta(w)) { std::error_code e2; sfs::remove_all(worldDir(nid), e2); return -1; }
  scanWorlds(); return nid;
}
bool renameWorld(int id, const std::string& name) { World* w = findWorld(id); if (!w) return false; World copy = *w; copy.name = name; if (!writeWorldMeta(copy)) return false; scanWorlds(); return true; }
bool deleteWorld(int id) { std::error_code ec; std::filesystem::remove_all(worldDir(id), ec); scanWorlds(); return !ec; }
std::string defaultWorldName() { for (int i = 1;; i++) { std::string n = "World " + std::to_string(i); bool used = false; for (auto& w : worlds) if (w.name == n) used = true; if (!used) return n; } }
static std::string whenText(long long t) {
  long long d = (long long)std::time(nullptr) - t; if (t <= 0) return "never played"; if (d < 90) return "just now"; if (d < 3600) return std::to_string(d / 60) + " min ago";
  if (d < 86400) return std::to_string(d / 3600) + " h ago"; if (d < 172800) return "yesterday"; return std::to_string(d / 86400) + " days ago";
}
std::string worldInfo(const World& w) {
  std::string where = w.state >= SPACE ? fmt("System %d - %d planets", w.sys, w.found) : fmt("Earth - %d/5 rocket parts", w.got); return std::string("[") + DIFF_NAME[w.difficulty] + "] " + where + " - " + whenText(w.played);
}
void saveSettings() {
  std::string path = dataDir() + "/settings.txt", tmp = path + ".tmp"; FILE* f = std::fopen(tmp.c_str(), "w"); if (!f) return;   // written aside then swapped in, so a crash never leaves a half file
  std::fprintf(f, "sfxvol %.2f\nmusvol %.2f\nsens %.2f\ninvert %d\ninvertx %d\n", sfxVol, musVol, mouseSens, mouseInvert ? 1 : 0, mouseInvertX ? 1 : 0); for (int a = 0; a < ACT_COUNT; a++) std::fprintf(f, "bind %s %d %d\n", ACT_ID[a], keyBind[a][0], keyBind[a][1]);
  std::fprintf(f, "padsens %.2f\npadinvert %d\npadinvertx %d\n", padSens, padInvert ? 1 : 0, padInvertX ? 1 : 0); for (int a = 0; a < ACT_COUNT; a++) std::fprintf(f, "pbind2 %s %d %d\n", ACT_ID[a], padBind[a][0], padBind[a][1]);
  if (std::fclose(f) == 0) { std::error_code ec; std::filesystem::rename(tmp, path, ec); }
}
// anything missing (older settings files) or invalid (hand-edited ones) keeps its default
void loadSettings() {
  resetControls(); FILE* f = std::fopen((dataDir() + "/settings.txt").c_str(), "r"); if (!f) return; char tag[32], id[32];
  auto validKey = [](int k) { return k == 0 || (k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST && k != GLFW_KEY_ESCAPE); };
  while (std::fscanf(f, "%31s", tag) == 1) { int a = 0, b = 0; float v = 0;
    if (!std::strcmp(tag, "muted") && std::fscanf(f, "%d", &a) == 1) { if (a) sfxVol = musVol = 0; muted = false; }   // older settings: SOUND OFF becomes 0% volumes
    else if (!std::strcmp(tag, "sfxvol") && std::fscanf(f, "%f", &v) == 1) sfxVol = clampf(v, 0, 1);
    else if (!std::strcmp(tag, "musvol") && std::fscanf(f, "%f", &v) == 1) musVol = clampf(v, 0, 1);
    else if (!std::strcmp(tag, "invert") && std::fscanf(f, "%d", &a) == 1) mouseInvert = a != 0;
    else if (!std::strcmp(tag, "invertx") && std::fscanf(f, "%d", &a) == 1) mouseInvertX = a != 0;
    else if (!std::strcmp(tag, "voices") && std::fscanf(f, "%d", &a) == 1) {}   // old setting, no longer used: voices are always on
    else if (!std::strcmp(tag, "sens") && std::fscanf(f, "%f", &v) == 1) { if (v >= .2f && v <= 3.f) mouseSens = v; }
    else if (!std::strcmp(tag, "padsens") && std::fscanf(f, "%f", &v) == 1) { if (v >= .2f && v <= 3.f) padSens = v; }
    else if (!std::strcmp(tag, "padinvertx") && std::fscanf(f, "%d", &a) == 1) padInvertX = a != 0;
    else if (!std::strcmp(tag, "padinvert") && std::fscanf(f, "%d", &a) == 1) padInvert = a != 0;
    else if (!std::strcmp(tag, "pbind2") && std::fscanf(f, "%31s %d %d", id, &a, &b) == 3) { auto ok = [](int v) { return v >= -1 && v < PAD_NB && v != GLFW_GAMEPAD_BUTTON_START; };
      if (ok(a) && ok(b) && (a != b || a < 0)) for (int i = 0; i < ACT_COUNT; i++) if (!std::strcmp(id, ACT_ID[i])) { padBind[i][0] = a; padBind[i][1] = b; } }
    else if (!std::strcmp(tag, "pbind") && std::fscanf(f, "%31s %d", id, &a) == 2 && a >= -1 && a < PAD_NB && a != GLFW_GAMEPAD_BUTTON_START) { for (int i = 0; i < ACT_COUNT; i++) if (!std::strcmp(id, ACT_ID[i])) { padBind[i][0] = a; padBind[i][1] = -1; } }   // older single-button setting
    else if (!std::strcmp(tag, "bind") && std::fscanf(f, "%31s %d %d", id, &a, &b) == 3 && validKey(a) && validKey(b)) { for (int i = 0; i < ACT_COUNT; i++) if (!std::strcmp(id, ACT_ID[i])) { keyBind[i][0] = a; keyBind[i][1] = b; } } }
  std::fclose(f);
}
void setMuted(bool m) { muted = m; saveSettings(); syncModes(); }
// a save from before worlds existed becomes a world called "My World", keeping its original universe and mute choice
void migrateOldSave() {
  namespace sfs = std::filesystem; std::error_code ec; std::string old = dataDir() + "/save.txt"; if (!sfs::exists(old, ec)) return;
  bool hadSettings = sfs::exists(dataDir() + "/settings.txt", ec); int id = makeWorld("My World", 0); if (id < 0) return;
  std::string moved = worldDir(id) + "/save.txt"; ec.clear(); sfs::rename(old, moved, ec); if (ec) { std::error_code e2; sfs::remove_all(worldDir(id), e2); return; } // try again next launch
  if (FILE* f = hadSettings ? nullptr : std::fopen(moved.c_str(), "r")) { char tag[32]; int m = 0; while (std::fscanf(f, "%31s", tag) == 1) if (!std::strcmp(tag, "muted") && std::fscanf(f, "%d", &m) == 1) { setMuted(m != 0); break; } std::fclose(f); }
}
// ================================================================ OPEN / CLOSE A WORLD
static void setupPicks() { for (int i = 0; i < 5; i++) { Rng r{(unsigned)(77 + i) + worldSeed * 131u}; float a = i / 5.f * 6.283f + r.f() * .9f, d = 70 + r.f() * 95, x = std::cos(a) * d, z = std::sin(a) * d; picks[i].pos = {x, hAt(x, z) + 1.5f, z}; } }
void openWorld(int id) {
  World* found = findWorld(id); if (!found) return; World w = *found; app = A_LOADING; syncModes(); audioSet(0, 0, 100, .11f); dlg = DLG_NONE; menuMsg.clear();
  std::string title = "Entering " + w.name; double lastDraw = -1; loadingFrame(title, 0);
  buildProgress = [&](float f) { double now = glfwGetTime(); if (now - lastDraw > .033) { lastDraw = now; loadingFrame(title, f); } };
  curWorld = id; worldSeed = w.seed; difficulty = w.difficulty; resetWorldState(); devReset(); devWorld = devAllowed(w); setupPicks(); std::error_code ec; bool hasSave = std::filesystem::exists(savePath(), ec), loaded = loadGame(); buildProgress = nullptr;
  if (!loaded && hasSave) { curWorld = -1; app = A_MENU; syncModes(); uiFlushMouse(); menuMsg = "Could not read \"" + w.name + "\". Its save was left untouched."; return; }
  if (!loaded) { buildProgress = [&](float f) { double now = glfwGetTime(); if (now - lastDraw > .033) { lastDraw = now; loadingFrame(title, f); } }; buildSystem(0); buildProgress = nullptr; }
  w.played = (long long)std::time(nullptr); writeWorldMeta(w); if (!loaded) saveGame(); savedL = 0;
  app = A_GAME; paused = false; syncModes(); sinceSave = 0; lastSig = sigOf(); uiFlushMouse();
  if (loaded) toast(w.name + "\nWelcome back - " + std::to_string(discHere()) + "/" + std::to_string(totalHere()) + " planets discovered", 6);
  else toast(w.name + "\nBuild the rocket. Leave Earth. Discover an universe that has never existed.\n{F}/{L}/{B}/{R} move  |  mouse look  |  {RUN} sprint  |  {JUMP} jump  |  {E} interact  |  Esc pause menu", 9);
}
// --make-ending-outro: (re)builds the world "Ending Outro" for trying the endings in a real save: every planet and moon is discovered
// except the easiest planet and its easiest moon, and on those two only the last quest is left. You start in space in front of them,
// with the best sword and the best pickaxe. An existing "Ending Outro" is rebuilt in place (same folder), not duplicated.
static float easeScore(const Planet& p) { return std::fabs(p.grav - 1.f) * 2.f + p.wicked * 3.f + (p.hideVil ? 10.f : 0.f); }   // lower = easier
bool makeEndingOutro() {
  namespace sfs = std::filesystem; std::error_code ec; scanWorlds(); int id = -1; for (auto& o : worlds) if (o.name == "Ending Outro") id = o.id;
  unsigned seed = 1 + (unsigned)std::time(nullptr) % 999;
  if (id < 0) id = makeWorld("Ending Outro", seed, 1);
  else { World o = *findWorld(id); o.seed = seed; o.difficulty = 1; o.played = (long long)std::time(nullptr); writeWorldMeta(o); curWorld = id; sfs::remove(checkpointPath(), ec); }
  scanWorlds(); World* w = id >= 0 ? findWorld(id) : nullptr; if (!w) return false;
  curWorld = id; worldSeed = w->seed; difficulty = w->difficulty; resetWorldState(); setupPicks(); buildSystem(0);
  for (auto& p : picks) { p.got = p.inst = true; } gotN = instN = 5;
  int pi = -1, mi = -1; float best = 1e30f;   // the easiest planet, together with its easiest moon
  for (int i = 0; i < (int)planets.size(); i++) { const Planet& p = planets[i]; if (p.isMoon || !p.enterable || p.moons.empty()) continue;
    int bm = -1; float ms = 1e30f; for (int j = 0; j < (int)planets.size(); j++) if (planets[j].isMoon && planets[j].parent == i) { float s2 = std::fabs(planets[j].grav - .25f); if (s2 < ms) { ms = s2; bm = j; } }   // moons: the least floaty
    float sc = easeScore(p) + ms; if (bm >= 0 && sc < best) { best = sc; pi = i; mi = bm; } }
  if (pi < 0 || mi < 0) return false;
  for (int i = 0; i < (int)planets.size(); i++) { Planet& p = planets[i]; if (!p.enterable) continue;
    if (i == pi || i == mi) { p.found = false; p.qi = nqOf(p) - 1; p.qph = 0; p.prog = 0; p.mask = 0; } else { p.found = true; p.qi = nqOf(p); } }
  foundN = discHere(); rocketMask = 1 | 2 | 4; rocketSel = 2; for (int r = 0; r < NRES; r++) inv[r] = 12; meds = 5;
  int sk = -1, pk = -1; for (int k = 0; k < 256; k++) { if (sk < 0 && swordDamage(k) == 3 && swordShape(k) == 3) sk = k; if (pk < 0 && pickTier(k) == 3) pk = k; }   // top-tier glowing crystal sword, Starmetal pickaxe
  if (sk >= 0) { swords.insert(sk); swordSel = sk; } if (pk >= 0) pickaxes.insert(pk);
  const Planet& P = planets[pi]; const Planet& M = planets[mi]; V3 side = norm(cross(norm(M.pos - P.pos), {0, 1, 0})); if (len(side) < .5f) side = {1, 0, 0};
  V3 dv = norm(side + norm(M.pos - P.pos) * .6f + V3{0, .25f, 0});   // beside the planet, turned so its moon is in view too
  st = SPACE; ambient = {.012f, .012f, .018f}; fogDen = 0; nearZ = 2; S = Ship(); S.pos = P.pos + dv * (P.R * 2.6f + 200.f); S.yaw = std::atan2(dv.x, dv.z); S.pitch = std::asin(clampf(-dv.y, -1, 1)); S.cyaw = S.yaw; S.cpitch = S.pitch; spaceEntry = S.pos;
  saveGame(); std::fprintf(stderr, "Ending Outro: world %d, planet %s (gravity %.2f, wicked %.2f), moon %s (gravity %.2f), sword %s, pickaxe %s\n", id, P.name.c_str(), P.grav, P.wicked, M.name.c_str(), M.grav,
    sk >= 0 ? swordName(sk).c_str() : "-", pk >= 0 ? pickName(pk).c_str() : "-");
  World m = *w; m.played = (long long)std::time(nullptr); writeWorldMeta(m); return true;
}
// the one world with developer tools: an existing signed one is kept, otherwise a fresh world is made and signed
bool signDeveloperWorld(const std::string& name) {
  scanWorlds(); for (auto& o : worlds) if (devAllowed(o)) { std::fprintf(stderr, "Developer world already exists: %s (folder w%d)\n", o.name.c_str(), o.id); return true; }
  int id = makeWorld(name, 1 + (unsigned)std::time(nullptr) % 999, 1); if (id < 0) return false; scanWorlds(); World* w = findWorld(id); if (!w) return false;
  World m = *w; m.devTag = devTagFor(m.id, m.seed, m.created); if (!writeWorldMeta(m)) return false; scanWorlds();
  std::fprintf(stderr, "Developer world created: %s (folder w%d)\n", m.name.c_str(), id); return true;
}
void touchWorld() { if (World* f = findWorld(curWorld)) { World w = *f; w.played = (long long)std::time(nullptr); writeWorldMeta(w); } }
void closeWorld() {
  saveGame(); touchWorld();
  audioCut(); devReset(); resetWorldState(); freePlanets(); curWorld = -1; app = A_MENU; pendingMenu = false; scanWorlds(); goScreen(SC_HOME); syncModes(); uiFlushMouse();
}
