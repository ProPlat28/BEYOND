// Quest names and dialogue for every planet.
#include "game.h"

// ================================================================ STORY: chapters, sights, wonders
static const char* ADJI[16] = {"grumpy", "sparkly", "suspicious", "sleepy", "fluffy", "shy", "spicy", "wobbly", "smug", "glittery", "dramatic", "ticklish", "moody", "squeaky", "fancy", "feral"};
static const char* NOUNI[8] = {"moon-eggs", "space-socks", "glow-pebbles", "star-noodles", "cosmic-buttons", "thunder-berries", "lost-spoons", "rainbow-fossils"};
static const char* BEH[8] = {"hide behind rocks the moment anyone looks", "follow strangers and then get embarrassed", "roll uphill out of spite", "only move when nobody is watching",
  "pretend to be rocks", "sing off-key at night", "get lost on purpose", "run away from anyone carrying a rocket"};
static const char* HOOK[8] = {"Ever since the Great Sneeze, nothing here has gone to plan.", "Things have been tense since the goats formed a union.", "Our last visitor stole a chair and never came back.",
  "Nobody has seen the sun smile in years. It just stares.", "The wicked ones say they are 'misunderstood'. They are not.", "We held a festival and forgot to invite the weather.",
  "Our map is upside down and we refuse to change it.", "The moon here keeps lying about what time it is."};
static const char* RELA[16] = {"Golden", "Cursed", "Legendary", "Slightly Used", "Holy", "Sticky", "Enormous", "Tiny", "Ancestral", "Haunted", "Shiny", "Soggy", "Forbidden", "Glorious", "Questionable", "Eternal"};
static const char* RELN[8] = {"Soup Ladle", "Garden Gnome", "Left Boot", "Teapot", "Pocket Watch", "Trumpet", "Cheese Wheel", "Throne Cushion"};
static const char* WADJ[16] = {"Whispering", "Grumpy", "Dancing", "Forgotten", "Giggling", "Humming", "Sleepy", "Glittering", "Ancient", "Wobbly", "Bashful", "Thundering", "Dreaming", "Sneezing", "Overconfident", "Mysterious"};
static const char* WNOUN[8] = {"Obelisk", "Orb", "Archway", "Crystal Cluster", "Throne", "Totem", "Gong", "Lighthouse"};
static const char* PARC[8] = {"a suspiciously warm pie", "a sealed letter marked 'DO NOT OPEN (open it)'", "a jar of angry honey", "a box that hums", "the world's smallest trophy",
  "a very serious sandwich", "an invitation to a party that already happened", "a bag of loud socks"};
static const char* PN[10] = {"Crashed Freighter", "Ruins", "Cantina", "Watchtower", "Hot Spring", "Giant Statue", "Rocket Graveyard", "Hermit's Hut", "Radio Mast", "Night Market"};
static const char* PADJ[16] = {"Wobbly", "Soggy", "Haunted", "Cozy", "Suspicious", "Glorious", "Tiny", "Forgotten", "Questionable", "Fancy", "Lonely", "Smelly", "Legendary", "Noisy", "Moody", "Shiny"};
void applyPerk(const Planet& p) { const Perk& k = PERK[(p.uniq * 5 + p.uniq / 16) % 8]; perkJump = k.j; perkSpeed = k.s; perkGrav = k.g; }
std::string itemName(const Planet& p) { return std::string(ADJI[p.uniq % 16]) + " " + NOUNI[(p.uniq / 16) % 8]; }
std::string relicName(const Planet& p) { return std::string(RELA[(p.uniq + 5) % 16]) + " " + RELN[(p.uniq / 16) % 8]; }
std::string wonderName(const Planet& p) { return std::string(WADJ[(p.uniq * 3 + 1) % 16]) + " " + WNOUN[(p.uniq / 16) % 8]; }
std::string poiName(const Planet& p, int i) { return std::string(PADJ[(p.uniq + pois[i].type * 5) % 16]) + " " + PN[pois[i].type]; }
int sights(const Planet& p) { int n = 0; for (int i = 0; i < 5; i++) if ((p.vmask >> i) & 1) n++; return n; }
int qtype(const Planet& p, int qi) {
  int u = p.uniq; static const int F[3] = {QA, QD, QH}, ALL[7] = {QA, QB, QD, QE, QF, QG, QH}; int f0 = F[u % 3], pool[6], n = 0;
  for (int i = 0; i < 7; i++) if (ALL[i] != f0) pool[n++] = ALL[i];
  int a = (u / 3) % 6, b = (a + 1 + (u / 18) % 5) % 6; int seq[NQ] = {f0, pool[a], pool[b], QC, QI};
  if (p.isMoon) { int ms[3] = {f0, pool[a], QI}; return ms[qi < 3 ? qi : 2]; }   // moons: three chapters, no wicked camp, the last one wakes the moon's wonder
  return seq[qi < NQ ? qi : NQ - 1];
}
int nObjFor(const Planet& p, int qt) {
  switch (qt) { case QA: return p.nItems; case QB: return p.nShrines; case QC: case QI: return 1; case QD: return 3 + (p.uniq % 2); case QE: return 6 + (p.uniq % 4); case QF: return 2; case QG: return 4 + (p.uniq % 2); default: return 3; }
}
// a random spot on the surface, away from the pad, the village, the camp, the wonder and the other sights
V3 spot(Rng& r, float dmin, float dmax, float avoid) {
  V3 q;
  for (int t = 0; t < 80; t++) {
    q = polar(r.r(0, 2 * PI), r.r(dmin, dmax)); bool ok = std::hypot(q.x, q.z) > 40 && len(q - vilC) > avoid && len(q - campC) > avoid + 6 && len(q - wonderPos) > avoid;
    for (auto& s : pois) if (len(q - s.pos) <= avoid) ok = false;
    if (ok) break;
  }
  return q;
}
void placeSites(const Planet& p) {
  Rng r{(unsigned)p.seed * 911u + 5u}; pois.clear(); wonderPos = {1e5f, 0, 1e5f};
  wAdj = (p.uniq * 3 + 1) % 16; wNoun = (p.uniq / 16) % 8; wonderCol = lin(hsl(wAdj / 16.f + .03f, .75f, .55f)) * 1.6f;
  wonderPos = spot(r, 140, 265, 50);
  int ty[10]; for (int i = 0; i < 10; i++) ty[i] = i; for (int i = 9; i > 0; i--) std::swap(ty[i], ty[r.i(i + 1)]);
  for (int i = 0; i < 4; i++) { Poi q; q.type = ty[i]; q.pos = spot(r, 60, 250, 45); q.yaw = r.r(0, 6.28f); pois.push_back(q); }
}
std::string questIntro(const Planet& p, const std::string& w, int qt) {
  int v = p.uniq % 3, cnt = nObjFor(p, qt); std::string it = itemName(p), rel = relicName(p), wn = wonderName(p), hook = HOOK[(p.uniq / 16) % 8], beh = BEH[p.uniq % 8], cu = p.culture;
  std::string pa = pois.size() > 2 ? poiName(p, 0) : "", pb = pois.size() > 2 ? poiName(p, 1) : "", pc = pois.size() > 2 ? poiName(p, 2) : "", par = PARC[(p.uniq / 3) % 8];
  switch (qt) {
    case QA: return v == 0 ? fmt("Star traveler! I am %s, elder of the %s. %s Our %s have escaped again. They %s. Please bring back %d of them.", w.c_str(), cu.c_str(), hook.c_str(), it.c_str(), beh.c_str(), cnt)
      : v == 1 ? fmt("Oh thank the stars, a stranger! The %s are missing. I won't say we lost them. We 'misplaced them dramatically'. Find %d. Remember: they %s.", it.c_str(), cnt, beh.c_str())
      : fmt("Welcome, sky-person! %s Small favor before the feast: %d %s are loose in the wilds. Fair warning, they %s.", hook.c_str(), cnt, it.c_str(), beh.c_str());
    case QB: return v == 0 ? fmt("You have our thanks. %d ancient shrines have gone dark. The ancestors are sulking, or somebody tripped over a cable. Touch each pillar and press {E}.", cnt)
      : v == 1 ? fmt("The shrines stopped working after the Great Argument. Please poke all %d of them and apologize on our behalf. Press {E} at each one.", cnt)
      : fmt("%d shrines, spread far apart, each needing one firm press of {E} and a sincere look. Bring snacks. Wear sensible shoes.", cnt);
    case QC: { std::string where = p.hideCamp ? "His camp has no fire. Hunt the dark hills." : "Look for the red smoke.";
      return v == 0 ? "The wicked chief stole our sacred " + rel + "! " + where + " Take it back. The guards are in a bad mood. They are always in a bad mood."
        : v == 1 ? "It's the " + rel + "! The wicked chief took it 'for safekeeping'. " + where + " Please steal it back. Politely. Quickly. Mostly quickly."
        : "The wicked chief has our " + rel + " and a very smug look. " + where + " Grab it and run!"; }
    case QD: return v == 0 ? fmt("%d of our people ran into the wilds during the last storm and are too embarrassed to come back. Find them and tell them it's fine. (It's a little bit not fine.)", cnt)
      : v == 1 ? fmt("%d villagers are 'not lost', they say. They are extremely lost. Please find them before dinner. Look for the faint green beams.", cnt)
      : fmt("The storm scattered %d of us. Half are hiding on purpose. Please find them. Follow the faint beams of embarrassment.", cnt);
    case QE: return v == 0 ? fmt("Tonight is the Festival of Hoops! Our champion has a sore knee and a worse attitude. Run through all %d hoops in order before they fizzle out. Sprint with {RUN}!", cnt)
      : v == 1 ? fmt("The sacred hoops have appeared in the wilds. Nobody knows why. Run through all %d in order. It's tradition. We made it up this morning.", cnt)
      : fmt("%d hoops, one winner, zero prizes. Pass through them in order before they fizzle. Wear stretchy pants.", cnt);
    case QF: return v == 0 ? "I need a delivery! Collect " + par + " at the " + pa + ", then carry it to the " + pb + ". Do not shake it. Do not ask why it hums."
      : v == 1 ? "Errand time: pick up " + par + " from the " + pa + " and bring it to the " + pb + ". It is extremely important and possibly alive."
      : "Take " + par + " from the " + pa + " to the " + pb + ". Tell no one. Tell everyone you saw nothing.";
    case QG: return v == 0 ? fmt("To end our long streak of bad luck, light the %d ritual beacons in order. The next one glows brightest. Please do not improvise.", cnt)
      : v == 1 ? fmt("The sun here has been grumpy for a decade. Light the %d beacons in order and it may cheer up. Press {E} at each one.", cnt)
      : fmt("%d beacons, one ritual, zero room for creativity. Follow the brightest glow and press {E} at each.", cnt);
    case QH: return v == 0 ? "Someone ate the Festival Cake. I will not say who. Ask the folks at the " + pa + ", the " + pb + " and the " + pc + " what they saw."
      : v == 1 ? "A mystery! The Festival Cake is gone and the crumbs lead nowhere. Interview the locals at the " + pa + ", the " + pb + " and the " + pc + "."
      : "Cake crime. Three witnesses: the " + pa + ", the " + pb + " and the " + pc + ". Be gentle with the goat.";
    default: return v == 0 ? "One last thing, traveler. Visit the " + wn + " and wake it with E. It hasn't spoken in 300 years and is very sensitive. Be polite."
      : v == 1 ? "Our final wonder, the " + wn + ", sleeps far from here. Go and wake it. It may be grumpy. It may sing. Both are normal."
      : "The legend says a visitor will wake the " + wn + ". We assumed it would be someone taller. Go anyway!";
  }
}
std::string questRemind(const Planet& p, int qt) {
  int n = nObjFor(p, qt); std::string pa = pois.size() > 2 ? poiName(p, 0) : "", pb = pois.size() > 2 ? poiName(p, 1) : "", pc = pois.size() > 2 ? poiName(p, 2) : "";
  switch (qt) {
    case QA: return fmt("Still %d %s to find. They glow in the wilds, walk over them. They %s, so be sneaky.", n - p.prog, itemName(p).c_str(), BEH[p.uniq % 8]);
    case QB: return fmt("%d shrines remain dark. Look for the cold blue pillars and press {E}.", n - p.prog);
    case QC: return p.hideCamp ? "No red smoke. Search the far ground. The relic sits on a pedestal." : "The camp is marked in red. The relic rests on a pedestal. Run fast!";
    case QD: return fmt("%d villagers still missing. Look for faint green beams. Peek behind everything.", n - p.prog);
    case QE: return "Run the hoops in order. The next one is the brightest. Hurry, {RUN} sprints!";
    case QF: return p.prog == 0 ? "Collect " + std::string(PARC[(p.uniq / 3) % 8]) + " at the " + pa + " first." : "Take " + std::string(PARC[(p.uniq / 3) % 8]) + " to the " + pb + ". Don't shake it.";
    case QG: return fmt("%d beacons left. The brightest one is next.", n - p.prog);
    case QH: return "Ask at the " + pa + ", the " + pb + " and the " + pc + ". " + std::to_string(3 - p.prog) + " witnesses left.";
    default: return "The " + wonderName(p) + " is waiting. Go wake it with E. Be polite.";
  }
}
std::string questDone(const Planet& p, int qt) {
  int v = (p.uniq / 3) % 3; std::string it = itemName(p), rel = relicName(p), wn = wonderName(p);
  switch (qt) {
    case QA: return v == 0 ? "Wonderful! They are... smaller than I remembered. Thank you!" : v == 1 ? "You found them! Two ran away again, but the rest are home. Close enough." : "The " + it + " are home! Tonight's feast will smell of triumph and moon-dust.";
    case QB: return v == 0 ? "The shrines sing again! Slightly flat, but the ancestors approve." : v == 1 ? "Every shrine glows! The ancestors have forgiven the goat incident." : "You did it! The pillars hum. The hum is happy. Mostly.";
    case QC: return v == 0 ? "The " + rel + "! You have shamed the wicked chief and saved our people. And our soup." : v == 1 ? "My " + rel + "! I missed you! (This is also to the relic, not you. Thank you too.)" : "The " + rel + " is home! The wicked chief will be so annoyed. Marvelous.";
    case QD: return v == 0 ? "All safe! They are asking for dinner and an apology. Nobody is getting the apology." : v == 1 ? "Everyone is home. Three wandered off again, but I keep a list." : "You found them all! One is still wearing a bush. We'll allow it.";
    case QE: return v == 0 ? "A record time! Also the only recorded time. Still, glorious." : v == 1 ? "The hoops fizzle in your honor. A moment of silence. Okay, moment over." : "Champion! The old champion is already sulking in your honor.";
    case QF: return v == 0 ? "Delivered, and nothing exploded. Better than last time." : v == 1 ? "Nobody quite knows what was in it. Wonderful work!" : "A flawless delivery. Mostly flawless. Don't ask about the hum.";
    case QG: return v == 0 ? "The beacons burn! The sun is now 4 percent less grumpy." : v == 1 ? "Light and fire! The ritual worked. I think. Someone smiled." : "All lit, and nobody got hurt. A personal best!";
    case QH: return v == 0 ? "...Okay. Fine. It was me. It was delicious cake." : v == 1 ? "So it was the elder. I mean, me. It was me. The cake was LOVELY." : "Hm. The trail leads here. Slightly embarrassing for me.";
    default: return "The " + wn + " is awake! The whole world of the " + p.culture + " will tell this story for a thousand years. Probably with mistakes.";
  }
}
