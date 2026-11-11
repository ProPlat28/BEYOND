// Resources, markets (barter), crafting rockets, the elder's unique prizes.
#include "game.h"

int resOf(const Planet& p, int slot) {
  if (p.isMoon) return slot == 0 ? 16 : (p.parent >= 0 && p.parent < (int)planets.size() ? planets[p.parent].kind : 9);
  return slot == 0 ? p.kind : (p.kind + 1 + (p.uniq / 16) % 8) % 16;
}
void giveRes(int r, int n) { if (r >= 0 && r < NRES) inv[r] += n; }
std::string invLine() { std::string s; for (int i = 0; i < NRES; i++) if (inv[i]) s += fmt("%s %d  ", RES_NAME[i], inv[i]); return s.empty() ? "nothing yet" : s; }
// every market offers 4 swaps: your own local ore for goods from other worlds, or what they crave for those same goods
void marketOffers(const Planet& p, Offer o[NOFFER]) {
  int l1 = resOf(p, 0), l2 = resOf(p, 1);
  auto pick = [&](int seed, std::vector<int> ex, int mod) { int r = ((seed % mod) + mod) % mod; for (int t = 0; t < mod; t++) { bool bad = false; for (int e : ex) if (e == r) bad = true; if (!bad) return r; r = (r + 1) % mod; } return r; };
  int w1 = pick(p.uniq * 3 + 4, {l1, l2}, 16), w2 = pick(p.uniq * 5 + 9, {l1, l2, w1}, 16), i1 = pick(p.uniq * 7 + 2, {l1, l2, w1, w2}, NRES), i2 = pick(p.uniq * 11 + 6, {l1, l2, w1, w2, i1}, NRES);
  o[0] = {l1, 1, i1, 1}; o[1] = {l2, 1, i2, 1}; o[2] = {w1, 2, i1, 1}; o[3] = {w2, 2, i2, 1};
}
// ---------------------------------------------------------------- prizes
static const char* PZA[16] = {"Radiant", "Humble", "Singing", "Starlit", "Ancient", "Cheerful", "Hollow", "Gilded", "Wandering", "Velvet", "Thundering", "Quiet", "Mischievous", "Crystal", "Ember", "Lucky"};
static const char* PZN[8] = {"Medallion", "Lantern", "Music Box", "Compass", "Crown", "Feather", "Locket", "Seed Pod"};
static const char* PZD[8] = {"It hums when nobody is looking.", "It is warm, and nobody knows why.", "It plays one note, always slightly off.", "It points to the best snack within a mile.", "Too big. Still worn with pride.", "It never falls, only floats off in a huff.", "Inside is a tiny portrait of someone smug.", "Plant it and something will probably happen."};
std::string prizeName(int u, bool moon) { return std::string(moon ? "Lunar " : "") + PZA[((u * 7 + 3) % 16 + 16) % 16] + " " + PZN[((u / 16) % 8 + 8) % 8]; }
std::string prizeDesc(int u) { return PZD[((u / 16 + u) % 8 + 8) % 8]; }
std::string grantPrize(Planet& p) {
  p.vmask |= VM_PRIZE; int a = resOf(p, 0), b = resOf(p, 1), na = 3 + p.uniq % 3; giveRes(a, na); giveRes(b, 2); audioEvent(S_DISCOVER); saveReq = true;
  return prizeName(p.uniq, p.isMoon) + " (+" + std::to_string(na) + " " + RES_NAME[a] + ", +2 " + RES_NAME[b] + ")";
}
int prizesCount() { syncPSaves(); int n = 0; for (auto& kv : psaves) if (kv.second.vmask & VM_PRIZE) n++; return n; }
// ---------------------------------------------------------------- crafting
bool canCraft(int id) {
  if (id <= 0 || id >= 6 || (rocketMask >> id) & 1) return false; const RocketDef& r = ROCKET[id];
  for (int i = 0; i < r.nc; i++) if (inv[r.cost[i].res] < r.cost[i].n) return false;
  return prizesCount() >= r.needPrizes;
}
void craftRocket(int id) {
  if (!canCraft(id)) return; const RocketDef& r = ROCKET[id]; for (int i = 0; i < r.nc; i++) inv[r.cost[i].res] -= r.cost[i].n;
  rocketMask |= 1 << id; rocketSel = id; audioEvent(S_DISCOVER); flashA = .5f; saveReq = true; toast(std::string("ROCKET BUILT\n") + r.name + "\nSpeed x" + fmt("%.2f", r.speed / ROCKET[0].speed) + " compared with the Pioneer - equipped", 6);
}
std::string costText(int id) { const RocketDef& r = ROCKET[id]; std::string s; for (int i = 0; i < r.nc; i++) s += fmt("%d %s   ", r.cost[i].n, RES_NAME[r.cost[i].res]); if (r.needPrizes) s += fmt("(needs %d elder prizes)", r.needPrizes); return s; }
