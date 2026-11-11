// Foods, rare finds, swords, pickaxes and the item ids used by chests. Everything is named from the "people key"
// (uniq * 2 + moon), so every people has its own dishes, its own sword and its own pickaxe.
#include "game.h"

static unsigned hk(int key, unsigned salt) { unsigned x = (unsigned)key * 2654435761u + salt * 0x9E3779B1u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; x *= 3266489917u; x ^= x >> 16; return x; }
static std::string people(int key) { return std::string(key & 1 ? "Lunar " : "") + cultureOf(key / 2); }   // "Kalixan", "Lunar Kalixan"
// ---------------------------------------------------------------- food
static const char* FADJ[16] = {"Glow", "Sun", "Moon", "Ember", "Frost", "Spice", "Dew", "Star", "Velvet", "Thunder", "Ash", "Coral", "Bramble", "Silk", "Dusk", "Bubble"};
static const char* FWILD[8] = {"Berries", "Caps", "Root", "Eggs", "Nuts", "Melon", "Pods", "Honeycomb"};
static const char* FDISH[12] = {"Stew", "Pie", "Skewers", "Dumplings", "Flatbread", "Soup", "Roast", "Noodles", "Tart", "Curry", "Porridge", "Buns"};
int foodShape(int id) { return (int)(hk(id / 4, 11 + (id & 1) * 5) % 8); }   // wild foods: what grows (bush, mushrooms, root...)
std::string foodName(int id) {
  int key = id / 4, slot = id & 3;
  if (slot < 2) { int sh = foodShape(id); std::string a = FADJ[hk(key, 3 + slot) % 16]; return (key & 1 ? "Lunar " : "") + a + " " + FWILD[sh]; }
  int d = (int)(hk(key, 21) % 12); if (slot == 3) d = (d + 1 + (int)(hk(key, 23) % 11)) % 12; return people(key) + " " + FDISH[d];
}
int foodValue(int id) { int slot = id & 3; unsigned h = hk(id, 31); return slot < 2 ? 4 + (int)(h % 3) : slot == 2 ? 10 + (int)(h % 3) : 12 + (int)(h % 3); }   // wild 4-6, dishes 10-14 (food bar is 20)
V3 foodColor(int id) { return lin(hsl((hk(id, 41) % 1000) / 1000.f, .75f, (id & 3) < 2 ? .55f : .5f)) * 1.3f; }
int foodTotal() { int n = 0; for (auto& kv : foods) n += kv.second; return n; }
// ---------------------------------------------------------------- rare finds
static const char* RGEM[8] = {"Heartstone", "Starglass", "Sunpearl", "Voidgem", "Dreamquartz", "Skyopal", "Emberheart", "Moonsilver"};
bool hasRare(const Planet& p) { return hk(pkey(p), 51) % 5 < 2; }   // about two worlds in five
std::string rareName(int key) { return people(key) + " " + RGEM[hk(key, 53) % 8]; }
V3 rareColor(int key) { return lin(hsl((hk(key, 57) % 1000) / 1000.f, .9f, .62f)) * 1.6f; }
// ---------------------------------------------------------------- tools
static const char* SBLADE[5] = {"Longsword", "Broadblade", "Crescent Saber", "Crystal Shard", "Twin Glaive"};
static const char* SADJ[8] = {"Whispering", "Proud", "Sunlit", "Stormforged", "Gleaming", "Ancient", "Merciless", "Starbound"};
static const char* PNOUN[3] = {"Pick", "Mattock", "Drill-Pick"};
int swordShape(int key) { return (int)(hk(key, 61) % 5); }
int swordDamage(int key) { return 1 + (int)(hk(key, 63) % 3); }
std::string swordName(int key) { return std::string(SADJ[hk(key, 65) % 8]) + " " + people(key) + " " + SBLADE[swordShape(key)]; }
V3 swordColor(int key) { return lin(hsl((hk(key, 67) % 1000) / 1000.f, .35f + (hk(key, 69) % 40) / 100.f, .72f)) * 1.25f; }
int pickTier(int key) { return 1 + (int)(hk(key, 71) % 3); }
std::string pickName(int key) { static const char* T[3] = {"Copper", "Steel", "Starmetal"}; return people(key) + " " + T[pickTier(key) - 1] + " " + PNOUN[hk(key, 73) % 3]; }
V3 pickColor(int key) { return lin(hsl((hk(key, 75) % 1000) / 1000.f, .5f, .55f)) * 1.3f; }
int bestPickTier() { int t = 0; for (int k : pickaxes) t = std::max(t, pickTier(k)); return t; }
std::string toolsLine() {
  std::string s; if (swordSel >= 0) s += "SWORD " + swordName(swordSel);
  if (!pickaxes.empty()) { if (!s.empty()) s += "   "; s += fmt("PICKAXE tier %d", bestPickTier()); } return s;
}
// ---------------------------------------------------------------- unified items (chests and bags)
int itemHave(int it) {
  if (it >= 0 && it < NRES) return inv[it]; if (it == IT_MED) return meds;
  if (it >= IT_RARE && it < IT_MED) { auto f = rares.find(it - IT_RARE); return f == rares.end() ? 0 : f->second; }
  if (it >= IT_FOOD && it < IT_RARE) { auto f = foods.find(it - IT_FOOD); return f == foods.end() ? 0 : f->second; }
  return 0;
}
void itemAdd(int it, int n) {
  if (it >= 0 && it < NRES) inv[it] = std::max(0, inv[it] + n); else if (it == IT_MED) meds = std::max(0, meds + n);
  else if (it >= IT_RARE && it < IT_MED) { int& c = rares[it - IT_RARE]; c = std::max(0, c + n); if (!c) rares.erase(it - IT_RARE); }
  else if (it >= IT_FOOD && it < IT_RARE) { int& c = foods[it - IT_FOOD]; c = std::max(0, c + n); if (!c) foods.erase(it - IT_FOOD); }
}
std::string itemLabel(int it) {
  if (it >= 0 && it < NRES) return RES_NAME[it]; if (it == IT_MED) return "Medpack";
  if (it >= IT_RARE && it < IT_MED) return rareName(it - IT_RARE); if (it >= IT_FOOD && it < IT_RARE) return foodName(it - IT_FOOD); return "?";
}
V3 itemColor(int it) {
  if (it >= 0 && it < NRES) return RES_COL[it]; if (it == IT_MED) return {1, .35f, .35f};
  if (it >= IT_RARE && it < IT_MED) return rareColor(it - IT_RARE); if (it >= IT_FOOD && it < IT_RARE) return foodColor(it - IT_FOOD); return {1, 1, 1};
}
std::vector<std::pair<int, int>> playerItems() {
  std::vector<std::pair<int, int>> v; for (int i = 0; i < NRES; i++) if (inv[i] > 0) v.push_back({i, inv[i]});
  for (auto& kv : foods) if (kv.second > 0) v.push_back({IT_FOOD + kv.first, kv.second});
  for (auto& kv : rares) if (kv.second > 0) v.push_back({IT_RARE + kv.first, kv.second});
  if (meds > 0) v.push_back({IT_MED, meds}); return v;
}
