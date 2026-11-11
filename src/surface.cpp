// Planet surfaces: terrain, people, quests, sights, wonders and their drawing.
#include "game.h"

// ================================================================ PLANET SURFACES: people, quests, discovery
static const char* ITEMS[] = {"sunstones", "star-seeds", "ember shards", "moon pearls", "echo crystals"};
void freeMesh(Mesh& m) { if (m.vao) { glDeleteVertexArrays(1, &m.vao); glDeleteBuffers(1, &m.vbo); glDeleteBuffers(1, &m.ebo); } m = Mesh(); }
float shAt(float x, float z) {
  float w = std::min({clampf((std::hypot(x, z) - 22) / 40, 0, 1), clampf((std::hypot(x - vilC.x, z - vilC.z) - 26) / 40, 0, 1), clampf((std::hypot(x - campC.x, z - campC.z) - 24) / 40, 0, 1)});
  int t = surfTerr, s = (int)surfSeed; float uq = 1.f + (s % 11) * .035f, n = 0;
  float hills = fbm(x * .0085f * uq + 80, z * .0085f * uq + 80, s, 5) - .42f;
  float detail = (fbm(x * .03f, z * .03f, s + 5, 3) - .5f) * .2f;
  switch (t) {
    case 0: n = hills * 1.2f + ridge(x * .012f, z * .012f, s) * .4f + detail; break;
    case 1: n = hills * .85f + ridge(x * .01f, z * .01f, s + 1) * .95f + std::sin(std::hypot(x, z) * .018f + fbm(x * .01f, z * .01f, s, 3) * 4) * .28f; break;
    case 2: n = ridge(x * .013f * uq, z * .013f * uq, s) * .9f + std::sin(x * .028f + fbm(x * .008f, z * .008f, s, 3) * 5) * .45f + hills * .35f; break;
    case 3: n = hills * 1.1f + (fbm(x * .02f, z * .02f, s, 4) - .5f) * .7f + ridge(x * .007f, z * .007f, s + 2) * .45f; break;
    case 4: n = clampf(1.f - cells(x * .022f, z * .022f, s) * 1.5f, 0, 1) * 1.1f - .3f + hills * .7f + ridge(x * .018f, z * .018f, s + 3) * .4f; break;
    case 5: { float c = cells(x * .016f, z * .016f, s); n = (c < .2f ? -.5f : (c - .28f) * .95f) + hills * .8f + detail; break; }
    case 6: n = hills * 1.05f + std::sin(x * .035f + fbm(x * .01f, z * .01f, s, 3) * 4) * .5f + (fbm(x * .014f, z * .014f, s, 4) - .5f) * .45f; break;
    case 7: n = (fbm(x * .0075f * uq, z * .0075f * uq, s, 6) - .4f) * 1.35f + ridge(x * .02f, z * .02f, s + 1) * .4f; break;
    case 8: n = std::fabs(fbm(x * .014f, z * .014f, s, 5) - .5f) * 1.6f + ridge(x * .022f, z * .022f, s) * .5f + hills * .4f - .15f; break;
    case 9: n = (cells(x * .028f, z * .028f, s) < .14f ? .85f : -.05f) + hills * .9f + (fbm(x * .012f, z * .012f, s, 4) - .5f) * .4f; break;
    case 10: n = hills * 1.05f + std::sin(z * .026f + fbm(x * .01f, z * .01f, s, 3) * 3) * .4f + ridge(x * .015f, z * .015f, s + 2) * .35f; break;
    case 11: n = (x * .0016f) + hills * 1.15f + ridge(x * .011f, z * .011f, s) * .5f + detail; break;
    case 12: n = (fbm(x * .02f, z * .02f, s, 4) > .58f ? .85f : -.15f) + hills * .75f + ridge(x * .016f, z * .016f, s + 1) * .55f; break;
    case 13: n = ridge(x * .018f, z * .018f, s) * .85f + hills * .7f + std::sin(std::hypot(x, z) * .025f) * .28f + detail; break;
    case 14: n = hills * 1.1f + ridge(x * .01f, z * .01f, s) * .55f + (fbm(x * .024f, z * .024f, s + 3, 4) - .5f) * .5f; break;
    default: n = (fbm(x * .014f * uq, z * .014f * uq, s, 5) - .42f) * 1.2f + std::sin(x * .045f) * std::sin(z * .045f) * .4f + ridge(x * .009f, z * .009f, s + 2) * .35f; break;
  }
  float h = n * surfAmp * w;
  for (auto& fl : flats) { float dx = x - fl.x, dz = z - fl.z, fade = 6.f + fl.r * .6f, d2 = dx * dx + dz * dz, R = fl.r + fade; if (d2 >= R * R) continue;   // a smooth flat pad at the structure's own height
    float t = clampf((std::sqrt(d2) - fl.r) / fade, 0, 1); t = t * t * (3 - 2 * t); h = lerpf(fl.h, h, t); }
  return h;
}
V3 lin(const C4& c) { return {std::pow(c[0] / 255, 2.2f), std::pow(c[1] / 255, 2.2f), std::pow(c[2] / 255, 2.2f)}; }
static V3 itemCol(const Planet& p) { static const V3 c[5] = {{1, .8f, .2f}, {.4f, 1, .5f}, {1, .4f, .1f}, {.8f, .8f, 1}, {.4f, .7f, 1}}; return c[p.itemI]; }
static std::string npcName(Rng& r) { static const char* a[] = {"Ka", "Ve", "Lu", "Zor", "Mi", "Tha", "Ori", "Bel", "Dru", "Sy"}; static const char* b[] = {"ren", "lix", "mar", "dos", "vyn", "tek", "ria", "gon"}; return std::string(a[r.i(10)]) + b[r.i(8)]; }
V3 polar(float a, float d) { return {std::cos(a) * d, 0, std::sin(a) * d}; }
static V3 rot2(V3 p, float a) { float c = std::cos(a), s = std::sin(a); return {p.x * c - p.z * s, p.y, p.x * s + p.z * c}; }
static void objsForQuest(Planet& p) {
  objs.clear(); if (p.qph < 1 || p.qi >= nqOf(p)) return;
  float rot = (p.uniq / 16) * (PI / 4); int lay = p.uniq % 16, qt = qtype(p, p.qi), n = nObjFor(p, qt);
  auto put = [&](V3 q, int ty, int i) { q = rot2(q, rot); q.y = shAt(q.x, q.z) + (ty == 1 ? 0.f : 1.4f); objs.push_back({q, ty, ((p.mask >> i) & 1) != 0}); };
  if (qt == QA) {
    static const V3 PATH[16][7] = {
      {{90,0,0},{140,0,40},{180,0,-30},{70,0,120},{160,0,110},{110,0,-140},{200,0,20}},
      {{0,0,100},{40,0,160},{-50,0,190},{90,0,70},{-120,0,140},{30,0,230},{-80,0,80}},
      {{120,0,120},{-120,0,120},{120,0,-120},{-120,0,-120},{200,0,0},{0,0,200},{-200,0,40}},
      {{60,0,200},{100,0,230},{20,0,250},{-40,0,210},{140,0,180},{-90,0,240},{80,0,270}},
      {{-80,0,40},{-140,0,90},{-190,0,30},{-160,0,-70},{-90,0,-130},{-210,0,80},{-40,0,160}},
      {{70,0,-90},{130,0,-140},{40,0,-180},{180,0,-60},{-30,0,-150},{110,0,-220},{200,0,-120}},
      {{50,0,50},{90,0,90},{140,0,40},{180,0,100},{80,0,170},{160,0,160},{220,0,70}},
      {{-100,0,0},{-150,0,50},{-200,0,-20},{-80,0,-110},{-170,0,-90},{0,0,-180},{-230,0,40}},
      {{0,0,-80},{70,0,-130},{-70,0,-130},{140,0,-80},{-140,0,-80},{0,0,-200},{90,0,-190}},
      {{110,0,30},{150,0,90},{190,0,20},{80,0,140},{170,0,150},{-40,0,180},{210,0,80}},
      {{40,0,80},{80,0,40},{120,0,100},{30,0,160},{160,0,50},{-20,0,200},{200,0,120}},
      {{180,0,180},{-180,0,180},{180,0,-180},{-180,0,-180},{240,0,60},{-60,0,240},{0,0,-240}},
      {{0,0,140},{60,0,180},{-60,0,180},{0,0,230},{110,0,150},{-110,0,150},{40,0,260}},
      {{90,0,-40},{140,0,20},{40,0,-100},{190,0,-70},{-20,0,-160},{160,0,-140},{220,0,10}},
      {{-50,0,90},{-100,0,140},{-20,0,180},{-150,0,80},{30,0,220},{-180,0,160},{70,0,250}},
      {{100,0,0},{0,0,100},{-100,0,0},{0,0,-100},{170,0,170},{-170,0,170},{0,0,240}}
    };
    for (int i = 0; i < n; i++) put(PATH[lay][i], 0, i);
  } else if (qt == QB) {
    static const V3 SH[16][5] = {
      {{100,0,80},{-90,0,110},{40,0,-150},{170,0,-40},{-130,0,-80}},
      {{0,0,170},{80,0,90},{-90,0,90},{140,0,200},{-40,0,240}},
      {{150,0,150},{-150,0,150},{0,0,-170},{200,0,0},{-200,0,80}},
      {{80,0,210},{-70,0,230},{20,0,270},{140,0,190},{-130,0,250}},
      {{-120,0,60},{-180,0,-40},{-70,0,-140},{40,0,-180},{-210,0,100}},
      {{90,0,-120},{160,0,-80},{-40,0,-170},{200,0,-150},{30,0,-230}},
      {{70,0,70},{160,0,40},{110,0,160},{210,0,90},{-30,0,190}},
      {{-110,0,40},{-180,0,80},{-70,0,-120},{20,0,-190},{-220,0,-30}},
      {{0,0,-140},{100,0,-90},{-100,0,-90},{0,0,-220},{160,0,-160}},
      {{130,0,50},{190,0,110},{70,0,170},{-50,0,200},{210,0,20}},
      {{50,0,110},{140,0,70},{20,0,190},{180,0,150},{-80,0,160}},
      {{200,0,200},{-200,0,120},{160,0,-180},{-40,0,-220},{0,0,250}},
      {{0,0,160},{90,0,200},{-90,0,200},{40,0,250},{-40,0,250}},
      {{120,0,-50},{180,0,40},{60,0,-140},{220,0,-90},{-30,0,-190}},
      {{-80,0,120},{-160,0,90},{40,0,210},{-200,0,170},{90,0,240}},
      {{140,0,0},{0,0,140},{-140,0,0},{0,0,-140},{200,0,200}}
    };
    for (int i = 0; i < n; i++) put(SH[lay][i], 1, i);
  } else if (qt == QC) objs.push_back({relicPos, 2, p.prog > 0});
  else {
    Rng r{(unsigned)p.seed * 31u + (unsigned)p.qi * 7u + 3u};
    auto mk = [&](V3 q, int ty, int i, float yo) { q.y = shAt(q.x, q.z) + yo; objs.push_back({q, ty, ((p.mask >> i) & 1) != 0}); };
    if (qt == QD) { for (int i = 0; i < n; i++) mk(spot(r, 55, 250, 30), 3, i, 0); }
    else if (qt == QE) { float a = r.r(0, 2 * PI); V3 q = spot(r, 70, 120, 30);
      for (int i = 0; i < n; i++) { mk(q, 4, i, 2.2f); a += r.r(-.8f, .8f); V3 nq = q + polar(a, r.r(38, 55)); if (std::hypot(nq.x, nq.z) > 270) { a += PI; nq = q + polar(a, 45); } q = nq; } }
    else if (qt == QF) { mk(pois[0].pos + V3{-4, 0, -3}, 7, 0, 1.2f); mk(pois[1].pos + V3{4, 0, -3}, 7, 1, 1.2f); }
    else if (qt == QG) { for (int i = 0; i < n; i++) mk(spot(r, 60, 240, 40), 5, i, 0); }
    else if (qt == QH) { for (int i = 0; i < 3; i++) mk(pois[i].pos, 8, i, 0); }
    else mk(wonderPos, 6, 0, 0);
  }
  // nothing to pick up or touch may sit inside a plant or a boulder (the wonder, the relic and the witnesses stay where they are)
  for (auto& o : objs) { if (o.type == 2 || o.type == 6 || o.type == 8) continue; float yo = o.pos.y - shAt(o.pos.x, o.pos.z);
    for (int k = 0; k < 3; k++) for (auto& c : hits) if (c.h < 9.f) pushOut(c, o.pos.x, o.pos.z, 1.2f); o.pos.y = shAt(o.pos.x, o.pos.z) + yo; }
}
// ---- cutscene camera: around the pad, at the angle with the clearest view (no plant, boulder, house or hill between it and the rocket)
static float cineA = .785f;
static float viewClearance(float a, float rad, float camY) {
  float cx = std::cos(a) * rad, cz = std::sin(a) * rad, best = 1e9f;
  for (auto& c : hits) { if (std::hypot(c.x, c.z) < 5.f) continue;   // the pad itself
    float vx = -cx, vz = -cz, wx = c.x - cx, wz = c.z - cz, t = clampf((wx * vx + wz * vz) / (vx * vx + vz * vz), 0, 1), dx = wx - vx * t, dz = wz - vz * t;
    float y = lerpf(camY, 8.f, t); if (c.y + c.h < y - 2.f) continue;   // too low to hide anything
    best = std::min(best, std::sqrt(dx * dx + dz * dz) - c.r); }
  for (int k = 0; k <= 8; k++) { float t = k / 8.f, x = cx * (1 - t), z = cz * (1 - t); if (shAt(x, z) > lerpf(camY, 8.f, t) - 1.5f) best = std::min(best, -5.f); }   // a hill in the way
  return best;
}
static float pickCineAngle(float rad, float camY, float arc) {
  float bestA = .785f, bestS = -1e9f;
  for (int i = 0; i < 24; i++) { float a = i * 2 * PI / 24, sc = 1e9f; for (int k = 0; k <= 4; k++) sc = std::min(sc, viewClearance(a + arc * k / 4, rad, camY)); if (sc > bestS + .01f) { bestS = sc; bestA = a; } }
  return bestA;
}
static std::string landToast;
void enterPlanet(int pi, bool cinematic) {
  Planet& p = planets[pi]; curP = pi; Rng r{(unsigned)p.seed * 53u + 9u}; surfSeed = (unsigned)p.seed; spaceEntry = S.pos;
  surfTerr = p.kind; surfFlora = p.floraT; surfArch = p.archT;
  static const float AMP[16] = {34, 26, 38, 32, 30, 28, 24, 42, 30, 26, 24, 32, 36, 28, 24, 34};
  surfAmp = AMP[p.kind] * (.9f + (p.uniq % 7) * .05f);
  int lay = p.uniq % 16; float rot = (p.uniq / 16) * (PI / 4);
  static const V3 VL[16] = {{70,0,0},{0,0,110},{-80,0,40},{40,0,200},{-160,0,50},{90,0,-140},{55,0,55},{-120,0,-20},{0,0,-90},{140,0,40},{60,0,90},{180,0,180},{0,0,150},{110,0,-70},{-70,0,130},{100,0,0}};
  static const V3 CP[16] = {{-160,0,20},{40,0,70},{0,0,-150},{90,0,60},{-40,0,-190},{-70,0,80},{180,0,40},{40,0,-170},{0,0,200},{-50,0,180},{-90,0,-160},{-160,0,-40},{80,0,-200},{200,0,90},{160,0,-80},{0,0,-180}};
  vilC = rot2(VL[lay], rot); campC = rot2(CP[lay], rot);
  flats.clear(); relicPos = campC + rot2({8, 1.6f, 4}, rot); placeSites(p); raceT = 0;
  if (tSurf) glDeleteTextures(1, &tSurf);
  tSurf = makeSurfaceTex(p);
  if (p.hasAtm) { surfSky = {p.atm.x * .6f + .04f, p.atm.y * .6f + .05f, p.atm.z * .6f + .07f}; fogDen = .0018f + (p.uniq % 9) * .0007f; ambient = surfSky * .7f + V3{.05f, .05f, .05f}; }
  else { surfSky = {0, 0, .004f}; fogDen = 0; ambient = {.05f, .05f, .06f}; }
  fogCol = surfSky;
  C4 hc = hsl(.05f + (p.uniq % 20) * .047f, .25f + (p.uniq % 6) * .08f, .4f + (p.uniq % 5) * .06f); hutCol = lin(hc); roofCol = lin(hsl(.55f + (p.uniq % 11) * .07f, .4f, .18f + (p.uniq % 4) * .05f));
  floraA = lin(p.pal[2]) * 1.3f; floraB = lin(p.pal[3]) * 1.5f;
  hutPos.clear();
  static const V3 HOFF[8][6] = {
    {{18,0,0},{12,0,14},{-8,0,16},{-16,0,4},{10,0,-14},{-14,0,-10}},
    {{22,0,4},{8,0,20},{-18,0,8},{4,0,-20},{-20,0,-6},{16,0,-12}},
    {{14,0,14},{-14,0,14},{14,0,-14},{-14,0,-14},{24,0,0},{0,0,24}},
    {{8,0,22},{20,0,10},{-10,0,18},{-22,0,2},{6,0,-18},{18,0,-8}},
    {{-20,0,6},{-8,0,18},{16,0,12},{22,0,-4},{-12,0,-16},{4,0,-22}},
    {{10,0,-18},{20,0,-8},{-16,0,-12},{-6,0,20},{18,0,10},{-22,0,8}},
    {{16,0,8},{8,0,16},{-16,0,8},{-8,0,-16},{20,0,-6},{-18,0,-10}},
    {{-12,0,16},{18,0,12},{12,0,-18},{-20,0,-4},{0,0,22},{-8,0,-20}}
  };
  int nh = p.isMoon ? 2 : 4 + (lay % 3); for (int i = 0; i < nh; i++) hutPos.push_back(vilC + rot2(HOFF[lay % 8][i], rot));
  npcs.clear(); int nW = p.isMoon ? 0 : 5 + (lay % 4), nK = p.isMoon ? 2 : 3 + ((lay / 2) % 3);   // more wicked people: 5 to 8 on every planet (the last two roam the land), none on moons
  static const V3 KOFF[6] = {{0,0,0},{8,0,6},{-7,0,9},{10,0,-5},{-11,0,-4},{5,0,12}};
  static const V3 WOFF[6] = {{0,0,0},{9,0,5},{-8,0,7},{11,0,-6},{-6,0,-10},{4,0,13}};
  static const char* EN[16] = {"Kael","Vesh","Luma","Tharn","Orin","Zara","Mira","Drusk","Syl","Bela","Nera","Quor","Ixis","Pala","Rhel","Talo"};
  for (int i = 0; i < nK; i++) { Npc n; n.elder = i == 0; n.home = vilC + rot2(KOFF[i], rot); n.pos = n.home; n.dest = n.home; n.name = (i == 0 ? "Elder " : "") + std::string(EN[(lay + i * 3) % 16]); n.col = lin(hsl(.08f + ((p.uniq + i * 9) % 16) * .06f, .55f, .55f)); n.wphase = i * 1.1f; n.wait = i * .4f; npcs.push_back(n); }
  static const V3 ROAM[16] = {{40,0,90},{-70,0,50},{110,0,-30},{-30,0,-100},{80,0,140},{-140,0,20},{0,0,160},{60,0,-120},{-90,0,-70},{130,0,40},{-50,0,170},{20,0,-150},{150,0,-60},{-110,0,80},{70,0,-160},{-160,0,-40}};
  for (int i = 0; i < nW; i++) { Npc n; n.wicked = true; n.chief = i == 0; n.home = i >= nW - 2 ? rot2(ROAM[(lay + (i - (nW - 2)) * 5) % 16], rot) : campC + rot2(WOFF[i % 6], rot);   // the last two roam the land (there are only 6 camp spots)
    n.pos = n.home; n.dest = n.home; n.name = (i == 0 ? "Chief " : "") + std::string(EN[(lay + 8 + i) % 16]); n.col = {.06f, .02f, .03f}; n.wphase = i * 1.3f; n.wait = i * .35f;
    n.widx = i; n.maxHp = n.hp = (n.chief ? 20 : 10) + (difficulty == 2 ? (n.chief ? 6 : 3) : 0); n.dead = ((p.vmask >> (5 + i)) & 1) != 0; npcs.push_back(n); }   // defeated wicked stay gone (vmask bits 5-12)
  for (int i = 0; i < (int)pois.size(); i++) { Npc n; n.poi = i; n.home = pois[i].pos + V3{5.5f, 0, 2.5f}; n.pos = n.home; n.dest = n.home; n.name = std::string(EN[(lay * 5 + i * 7 + 3) % 16]); n.col = lin(hsl(.03f + ((p.uniq + i * 5) % 14) * .07f, .6f, .55f)); n.wphase = i * 2.3f; n.wait = .8f + i * .5f; npcs.push_back(n); }
  flora.clear(); boulders.clear(); int fa = p.floraT % 8, barren = (p.kind == 2 || p.kind == 3 || p.kind == 4 || p.kind == 9 || p.kind == 12 || p.kind == 14), fn = (barren ? 110 : 210) + (p.uniq % 13) * 14;
  auto clearSpot = [&](float x, float z, float m) { if (std::hypot(x, z) < 15 + m || len(V3{x, 0, z} - vilC) < 26 + m || len(V3{x, 0, z} - campC) < 20 + m || len(V3{x, 0, z} - wonderPos) < 12 + m) return false; for (auto& q : pois) if (len(V3{x, 0, z} - q.pos) < 13 + m) return false; return true; };
  for (int i = 0; i < fn; i++) { float x = r.r(-320, 320), z = r.r(-320, 320); if (!clearSpot(x, z, 0)) continue; flora.push_back({x, z, (.8f + r.f() * 2.f + (p.uniq % 5) * .1f) * (r.f() < .15f ? 1.6f : 1.f), r.f() < .75f ? fa : (fa + 1 + r.i(3)) % 8}); if (flora.back().type == 4 || flora.back().type == 6) flora.pop_back(); }   // no box trees (pillar + floating slab) and no tall pole trees with flat cone layers   // a few giants among them
  { Rng rb{(unsigned)p.seed * 71u + 3u}; int nb = (barren ? 90 : 50) + (p.uniq % 7) * 6; for (int i = 0; i < nb; i++) { float x = rb.r(-315, 315), z = rb.r(-315, 315); if (!clearSpot(x, z, 3)) continue; float s = .5f + rb.f() * rb.f() * 3.2f; boulders.push_back({x, z, s * rb.r(.8f, 1.3f), s * rb.r(.45f, .8f), s * rb.r(.8f, 1.3f)}); } }
  // a market with a merchant in every village, a workbench next to the rocket, the wicked house in the camp, and ore nodes to mine
  { V3 best = vilC + polar(.3f, 22); float bd = -1; for (int k = 0; k < 8; k++) { V3 c = vilC + polar(k * PI / 4 + .3f, 22); float md = 1e9f; for (auto& h : hutPos) md = std::min(md, len(c - h)); md = std::min(md, len(c - vilC)); if (md > bd) { bd = md; best = c; } }
    marketPos = best; marketYaw = std::atan2(vilC.x - best.x, vilC.z - best.z); V3 zf{std::sin(marketYaw), 0, std::cos(marketYaw)};
    Npc m; m.merchant = true; m.home = marketPos - zf * 1.7f; m.pos = m.home; m.dest = m.home; m.name = "Merchant " + std::string(EN[(lay * 3 + 5) % 16]); m.col = lin(hsl(.12f + (p.uniq % 9) * .08f, .6f, .5f)); m.wphase = 4.f; m.wait = 1.2f; npcs.push_back(m); }
  benchPos = {0, 0, -8.5f}; wHouse = campC + V3{13, 0, 0};
  { // every building stands on its own flat pad (heights read before any pad exists), then the terrain mesh is built
    std::vector<Flat> fl; auto add = [&](V3 c, float r) { fl.push_back({c.x, c.z, r, shAt(c.x, c.z)}); };
    static const float PFR[10] = {9, 9, 7, 4, 8, 4, 9, 4.5f, 2, 9}; for (auto& q : pois) add(q.pos, PFR[q.type % 10]); add(wonderPos, 7.f);
    for (auto& h : hutPos) add(h, hutRadius(p.archT) + 1.5f); add(marketPos, 3.5f); if (!p.isMoon) add(wHouse, 5.5f); flats = fl; }
  freeMesh(mSurf); mSurf = mkTerrain(200, 700, shAt);
  // ore: some needs a pickaxe (this people's own tier at most, so the local market always sells one that works); worlds with a rare find hide two of it, only for their own pickaxe
  nodes.clear(); { Rng rn{(unsigned)p.seed * 17u + 5u}; int nn = p.isMoon ? 10 : 14, tier = pickTier(pkey(p)); for (int i = 0; i < nn; i++) { V3 q = spot(rn, 28, 285, 14); q.y = shAt(q.x, q.z); Node nd{q, resOf(p, i % 3 == 2 ? 1 : 0), 0.f}; nd.need = i % 3 == 2 ? tier : i % 4 == 1 ? 1 : 0; nodes.push_back(nd); }
    if (hasRare(p)) for (int i = 0; i < 2; i++) { V3 q = spot(rn, 90, 290, 20); q.y = shAt(q.x, q.z); Node nd{q, 200, 0.f}; nd.need = -1; nodes.push_back(nd); } }
  if (difficulty > 0) { Rng rf{(unsigned)p.seed * 29u + 11u}; for (int i = 0; i < (p.isMoon ? 6 : 11); i++) { V3 q = i < 3 ? vilC + polar(i * 2.1f + .5f, 15.f + rf.f() * 8) : spot(rf, 28, 285, 14); q.y = shAt(q.x, q.z); nodes.push_back({q, 100 + (i & 1), 0.f}); } }   // wild food (res 100/101); none at all on easy
  for (size_t i = 0; i < npcs.size(); i++) { Npc& n = npcs[i]; unsigned h = 2166136261u; for (char c : n.name) h = (h ^ (unsigned char)c) * 16777619u; n.look = (int)(h % 7) + (int)i; n.voice = .75f + (h % 100) / 140.f; }
  inRoom = 0; roomNpcs.clear(); hitsSaved.clear(); buildHits(p);
  for (auto& n : npcs) { if (n.merchant) continue; for (int k = 0; k < 3; k++) for (auto& c : hits) pushOut(c, n.home.x, n.home.z, .6f); n.pos = n.dest = n.home; n.pos.y = shAt(n.pos.x, n.pos.z); }   // nobody starts inside a wall
  for (auto& nd : nodes) { for (int k = 0; k < 3; k++) for (auto& c : hits) pushOut(c, nd.pos.x, nd.pos.z, 1.f); nd.pos.y = shAt(nd.pos.x, nd.pos.z); }
  objsForQuest(p); hp = MAXHP; invul = 2; P = Player(); if (p.found || (p.vmask & VM_WONDER)) applyPerk(p); else perkJump = perkSpeed = perkGrav = 1;
  static const V3 LAND[16] = {{8,0,6},{10,0,-4},{-6,0,10},{12,0,2},{-10,0,-6},{6,0,12},{-8,0,8},{4,0,-12},{9,0,9},{-12,0,4},{7,0,-9},{-5,0,11},{11,0,-3},{-9,0,-8},{3,0,13},{-11,0,7}};
  V3 lp = LAND[lay]; P.x = lp.x; P.z = lp.z; P.yaw = std::atan2(vilC.x - lp.x, vilC.z - lp.z); P.y = shAt(lp.x, lp.z); st = SURFACE; flashA = 1; talkL = 0; fov = 72; nearZ = .3f; mdx = mdy = 0; p.vmask |= VM_VISIT; visitedSys.insert(sysId); cine = 0; surfRocketY = .9f; cineT = 0;
  if (cinematic) { cine = 1; surfRocketY = 320.f; cineA = pickCineAngle(50.f, 14.f, .7f); }
  static const char* HINT[16] = {
    "A village sits east of the pad. Find the elder.",
    "The village is north. Walk the ridge.",
    "Dunes hide the village far to the west. No beacon.",
    "The people live on a far northern mesa.",
    "Search the western waste. The village is buried in sand.",
    "South of here, among the dunes, someone still lives.",
    "A small village clings nearby to the northeast.",
    "No light. The village is southwest, low in a hollow.",
    "Look due south. The village sits in a crater.",
    "East, then a little north. Follow the stone line.",
    "Northeast, past the first ridge.",
    "Far on the southeast plateau. A long walk.",
    "Due north, a long way. Bring water.",
    "Southeast dunes. The village is half-sunk.",
    "Northwest, behind the dark hills.",
    "East of the pad, close. You can almost see the roofs."
  };
  landToast = p.name + "\n" + p.txt + "\n" + HINT[lay] + "\nThis world hides 4 sights and 1 wonder. Planets " + std::to_string(discHere()) + "/" + std::to_string(totalHere()) + "   Moons " + std::to_string(moonsDiscHere()) + "/" + std::to_string(moonsTotalHere());
  if (cinematic) toast("", .01f); else toast(landToast, 8); audioEvent(S_WHOOSH);   // arrival whoosh (also when a world saved on a planet is loaded)
}
static void finishLeave() {
  Planet& p = planets[curP]; st = SPACE; cine = 0; surfRocketY = .9f; nodes.clear(); hits.clear(); roomNpcs.clear(); inRoom = 0; mdx = mdy = 0; talkL = 0; flashA = 1; fogDen = 0; ambient = {.012f, .012f, .018f}; V3 dv = norm(spaceEntry - p.pos); S.pos = p.pos + dv * (p.R * 1.5f); S.speed = 100; S.thr = .25f;
  S.pitch = std::asin(clampf(dv.y, -1, 1)); S.yaw = std::atan2(-dv.x, -dv.z); S.roll = 0; S.cyaw = S.yaw; S.cpitch = S.pitch; S.croll = 0; nearZ = 2;
  V3 r, u, f; basis(S.yaw, S.pitch, 0, r, u, f); camPos = S.pos + f * -26.f + u * 5.f; cR = r; cU = u; cF = f; boulders.clear(); freeMesh(mSurf); if (tSurf) { glDeleteTextures(1, &tSurf); tSurf = 0; } npcs.clear(); objs.clear(); pois.clear(); perkJump = perkSpeed = perkGrav = 1; curP = -1;
  toast("BACK IN ORBIT\nFly on - planets " + std::to_string(discHere()) + "/" + std::to_string(totalHere()) + "   moons " + std::to_string(moonsDiscHere()) + "/" + std::to_string(moonsTotalHere()), 4);
}
// ---------------------------------------------------------------- landing and take-off cutscenes
void startTakeoff() { cineA = pickCineAngle(45.f, 12.f, 0); cine = 2; cineT = 0; talkL = 0; audioEvent(S_BEEP_LO); toast("IGNITION\n", 1.6f); }
static void dust(float amount, float dt) { for (int k = 0; k < 2; k++) { float a = (float)std::rand() * 6.2831853f / RAND_MAX; emit({std::cos(a) * 2.f, .8f, std::sin(a) * 2.f}, {std::cos(a), .06f, std::sin(a)}, amount, dt * .5f); } }
void updCine(float dt) {
  Planet& p = planets[curP]; cineT += dt; invul = std::max(invul, .5f); prompt.clear(); auto rv = [] { return (float)std::rand() / RAND_MAX - .5f; };
  if (cine == 1) {   // landing: the rocket brakes with its engines, hovers down onto the pad and kicks up dust
    const float T = 7.f; float t = clampf(cineT / T, 0, 1), q = std::pow(1 - t, 2.4f); surfRocketY = .9f + 320.f * q;
    emit({0, surfRocketY + .5f, 0}, {0, -1, 0}, .45f + .55f * (1 - t), dt); if (surfRocketY < 26) dust(.4f * (1 - surfRocketY / 26), dt);
    float ca = cineA + t * .7f, cr = 62.f - t * 24.f; camPos = {std::cos(ca) * cr, 10.f + (1 - t) * 8.f, std::sin(ca) * cr}; camPos.y = std::max(camPos.y, shAt(camPos.x, camPos.z) + 3.f); lookAt(camPos, {0, std::max(6.f, surfRocketY * .32f + 3.f), 0}); fov = 66; nearZ = .3f;
    audioSet(0, .12f + .18f * (1 - t), 110.f + 40.f * (1 - t), 0);   // landing: engine fire only, no wind hiss
    if (t >= 1) { cine = 0; surfRocketY = .9f; flashA = 0; audioEvent(S_CLANG); mdx = mdy = 0; fov = 72; nearZ = .3f; toast(landToast, 8); }
    return; }
  const float T0 = 1.6f, T1 = 6.f; float t = clampf((cineT - T0) / (T1 - T0), 0, 1); surfRocketY = .9f + 340.f * std::pow(t, 2.3f);   // take-off: ignition, shaking, then up and away
  float pw = cineT < T0 ? .15f + .5f * cineT / T0 : .7f + .3f * t; emit({0, surfRocketY + .5f, 0}, {0, -1, 0}, pw, dt); if (surfRocketY < 34) dust(.5f * (1 - surfRocketY / 34), dt);
  float sh = cineT < T0 ? .35f * cineT / T0 : .12f; camPos = {std::cos(cineA) * 45.f + rv() * sh, 9 + rv() * sh + t * 12, std::sin(cineA) * 45.f + rv() * sh}; camPos.y = std::max(camPos.y, shAt(camPos.x, camPos.z) + 3.f); lookAt(camPos, {0, std::max(8.f, surfRocketY * .85f), 0}); fov = 60; nearZ = .3f;
  audioSet(0, cineT < T0 ? .08f + .17f * cineT / T0 : .25f + .1f * t, 110.f + 50.f * t, 0);   // take-off: engine fire only, no wind hiss
  if (cineT > T1 - .6f) flashA = clampf((cineT - (T1 - .6f)) / .6f, 0, 1);
  if (cineT >= T1) finishLeave();
}
static void talkElder(Planet& p, Npc& n) {
  const std::string& w = n.name; int qt = p.qi < nqOf(p) ? qtype(p, p.qi) : -1; const char* wl = p.isMoon ? "moon" : "world";
  if (!(p.vmask & VM_MET)) { p.vmask |= VM_MET; audioEvent(S_DISCOVER); toast("FIRST CONTACT\nThe " + p.culture + " people\n" + alienDesc(p.uniq, p.isMoon), 6); }
  if (p.qi >= nqOf(p)) {
    if (!(p.vmask & VM_PRIZE)) { std::string g = grantPrize(p); say(w, "Wait, traveler! I nearly forgot. Take this prize in thanks for everything you did. " + g); return; }
    say(w, "The " + wonderName(p) + " is awake and our " + wl + " is open to you, star traveler. The " + p.culture + " people will remember you. (Mostly kindly.)"); return; }
  if (p.qph == 0) { p.qph = 1; p.prog = 0; p.mask = 0; objsForQuest(p); raceT = 0; say(w, questIntro(p, w, qt)); toast(fmt("NEW QUEST %d/%d", p.qi + 1, nqOf(p)), 3); audioEvent(S_CHIME); }
  else if (p.qph == 1) say(w, questRemind(p, qt));
  else { std::string line = questDone(p, qt); p.qi++; p.qph = 0; p.prog = 0; p.mask = 0; objs.clear(); raceT = 0; audioEvent(S_DISCOVER);
    if (p.qi >= nqOf(p)) { p.found = true; if (!p.isMoon) foundN++; std::string g = grantPrize(p); say(w, line + " Our " + wl + " is open to you. And please take this prize. " + g);
      toast(std::string(p.isMoon ? "MOON DISCOVERED\n" : "PLANET DISCOVERED\n") + p.name + "\n" + p.txt + "\nELDER'S PRIZE: " + prizeName(p.uniq, p.isMoon) + "\nPlanets " + std::to_string(discHere()) + "/" + std::to_string(totalHere()) + "   Moons " + std::to_string(moonsDiscHere()) + "/" + std::to_string(moonsTotalHere()), 10);
      makeCheckpoint(p); }   // hidden checkpoint: nothing is shown or played
    else { say(w, line + " Come see me when you are ready for the next task."); toast(fmt("QUEST COMPLETE\nChapter %d/%d done", p.qi, nqOf(p)), 3); } }
}
static void poiTalk(Planet& p, Npc& n) {
  int i = n.poi, ty = pois[i].type; bool first = !((p.vmask >> i) & 1); p.vmask |= 1u << i; std::string line = first ? POI1[ty] : POI2[ty];
  if (first) { audioEvent(S_DISCOVER); toast(fmt("SIGHT DISCOVERED\n%s\nSights %d/5", poiName(p, i).c_str(), sights(p)), 3.5f); }
  if (p.qi < nqOf(p) && p.qph == 1 && qtype(p, p.qi) == QH) for (size_t k = 0; k < objs.size(); k++) { Obj& o = objs[k]; if (o.type != 8 || o.done || len(V3{o.pos.x, 0, o.pos.z} - V3{pois[i].pos.x, 0, pois[i].pos.z}) > 14) continue;
    o.done = true; p.mask |= 1u << k; p.prog++; line = std::string(GOSS[(p.prog - 1) % 3]) + "  (Witness " + std::to_string(p.prog) + "/3)"; audioEvent(S_CHIME);
    if (p.prog >= (int)objs.size()) { p.qph = 2; toast("ALL WITNESSES HEARD\nReturn to the elder", 4); } break; }
  say(n.name + ", " + poiName(p, i), line);
}
static void contact(Planet& p) { if (p.vmask & VM_MET) return; p.vmask |= VM_MET; audioEvent(S_DISCOVER); toast("FIRST CONTACT\nThe " + p.culture + " people\n" + alienDesc(p.uniq, p.isMoon), 6); }
static std::string nodeNeed(const Planet& p, const Node& nd) {   // empty when you can mine it now, otherwise what is missing
  if (nd.need < 0) return pickaxes.count(pkey(p)) ? "" : "Only the " + pickName(pkey(p)) + " can cut it (sold at this world's market)";
  if (nd.need > bestPickTier()) return nd.need == 1 && pickaxes.empty() ? std::string("Needs a pickaxe (markets sell them)") : fmt("Needs a tier %d pickaxe (yours: %s)", nd.need, pickaxes.empty() ? "none" : std::to_string(bestPickTier()).c_str());
  return "";
}
static void mine(Planet& p, Node& nd) {
  int n = 1 + (int)(std::fabs(nd.pos.x) * 3 + std::fabs(nd.pos.z)) % 3;
  if (nd.res == 100 || nd.res == 101) { int id = pkey(p) * 4 + (nd.res - 100); foods[id] += n; nd.cool = 90.f; audioEvent(S_CLICK); saveReq = true; toast(fmt("+%d %s\nPress {EAT} to eat (you carry %d food)", n, foodName(id).c_str(), foodTotal()), 2.6f); return; }
  std::string why = nodeNeed(p, nd); if (!why.empty()) { audioEvent(S_BEEP_LO); toast("TOO HARD TO MINE\n" + why, 3); return; }
  if (!pickaxes.empty()) mineT = .4f;
  if (nd.res == 200) { rares[pkey(p)] += 1; nd.cool = 240.f; audioEvent(S_DISCOVER); saveReq = true; toast("RARE FIND\n+1 " + rareName(pkey(p)) + "\nMarkets on other worlds trade goods for it", 4); return; }
  if (!pickaxes.empty()) n += 1;   // a pickaxe always digs out a little more
  giveRes(nd.res, n); nd.cool = 120.f; audioEvent(S_CLANG); saveReq = true;
  toast(fmt("+%d %s\n(you now have %d)", n, RES_NAME[nd.res], inv[nd.res]), 2.2f);
}
static void surfInteract() {
  if (talkL > 0) { if (talkShown < (int)keysIn(talkT).size()) { talkAge = 1e4f; return; } talkL = 0; return; }   // E first shows the whole line, then closes it
  Planet& p = planets[curP]; V3 me{P.x, 0, P.z};
  if (inRoom) { roomInteract(p); return; }
  for (auto& n : npcs) if (n.merchant && len(V3{marketPos.x, 0, marketPos.z} - me) < 3.6f) { setVoice(&n); contact(p); modalWho = n.name; openModal(M_MARKET); return; }   // anywhere around the stall, front included
  for (auto& n : npcs) if (!n.dead && len(V3{n.pos.x, 0, n.pos.z} - me) < 3.8f) {
    setVoice(&n);
    if (n.elder) { talkElder(p, n); return; }
    if (n.merchant) { contact(p); modalWho = n.name; openModal(M_MARKET); return; }
    if (!n.wicked) contact(p);
    if (n.wicked) { static const char* WL[6] = {"Leave, skyling, or be crushed!", "Your rocket would make fine scrap...", "I am not wicked, I am just very committed to the bit.", "Boo. Was that scary? I have been practicing.", "My mother says I should go into accounting.", "Leave, skyling! Also, do you have snacks?"};
      say(n.name, n.chief ? (p.qi > RQ ? std::string("I only stole it to feel something. Now I feel nothing. Thanks for that, skyling.") : "You dare enter my camp, skyling? That " + relicName(p) + " is MINE. Guards - crush them!") : std::string(WL[(n.name.size() + (size_t)(tAcc * .15f)) % 6])); return; }
    if (n.poi >= 0) { poiTalk(p, n); return; }
    static const char* L[] = {"Welcome to %s, traveler. The elder needs your help - look for the glowing beacon.", "They say a wicked chief rules the far camp. Stay away from his guards.", "%s", "We are the %s people. We have never seen a ship like yours."};
    int k = (int)(n.name.size() + (size_t)(tAcc * .3f)) % 4; say(n.name, k == 2 ? p.special + "." : fmt(L[k], k == 3 ? p.culture.c_str() : p.name.c_str())); return; }
  for (size_t i = 0; i < objs.size(); i++) { Obj& o = objs[i]; if (o.done || o.type == 0 || o.type == 4 || o.type == 8 || len(V3{o.pos.x, 0, o.pos.z} - me) > 3.8f) continue; int n = (int)objs.size();
    if (o.type == 1) { o.done = true; p.mask |= 1u << i; p.prog++; audioEvent(S_CLANG); toast(fmt("SHRINE AWAKENED\n%d/%d", p.prog, n), 3); if (p.prog >= n) { p.qph = 2; toast("ALL SHRINES LIT\nReturn to the elder", 4); } }
    else if (o.type == 2) { o.done = true; p.mask |= 1u << i; p.prog++; audioEvent(S_CHIME); p.qph = 2; toast("RELIC TAKEN\nRun! Return it to the elder", 5); }
    else if (o.type == 3) { o.done = true; p.mask |= 1u << i; p.prog++; audioEvent(S_CHIME); setVoice(nullptr); say("Lost villager", EXCUSE[(p.uniq + (int)i * 3) % 8]); toast(fmt("VILLAGER FOUND\n%d/%d", p.prog, n), 3); if (p.prog >= n) { p.qph = 2; toast("EVERYONE FOUND\nReturn to the elder", 4); } }
    else if (o.type == 5) { int nx = 0; while (nx < n && objs[nx].done) nx++; if ((int)i != nx) { toast("WRONG BEACON\nFollow the brightest glow", 2.5f); audioEvent(S_BEEP_LO); return; }
      o.done = true; p.mask |= 1u << i; p.prog++; audioEvent(S_CLANG); toast(fmt("BEACON LIT\n%d/%d", p.prog, n), 2.5f); if (p.prog >= n) { p.qph = 2; toast("RITUAL COMPLETE\nReturn to the elder", 4); } }
    else if (o.type == 6) { o.done = true; p.mask |= 1u << i; p.prog++; p.vmask |= VM_WONDER; applyPerk(p); p.qph = 2; audioEvent(S_DISCOVER); say("The " + wonderName(p), WVOICE[wAdj]); const Perk& k = PERK[(p.uniq * 5 + p.uniq / 16) % 8]; toast(fmt("WONDER AWAKENED\nGift: %s (%s)\nReturn to the elder", k.name, k.desc), 6); }
    else if (o.type == 7) { if (i == 1 && !objs[0].done) { toast("You have nothing to deliver yet\nCollect the parcel first", 3); return; }
      o.done = true; p.mask |= 1u << i; p.prog++; audioEvent(i == 0 ? S_CHIME : S_DISCOVER);
      if (i == 0) { toast("PARCEL COLLECTED\nDo not shake it", 3); say(poiName(p, 0), "Here you go. It was ticking earlier but it stopped. That is a good sign. Probably."); }
      else { p.qph = 2; toast("PARCEL DELIVERED\nReturn to the elder", 4); say(poiName(p, 1), "I did not order this. But I am keeping it. Thank you!"); } }
    return; }
  for (auto& nd : nodes) if (nd.cool <= 0 && len(V3{nd.pos.x, 0, nd.pos.z} - me) < 3.6f) { mine(p, nd); return; }
  if (len(benchPos - me) < 3.4f) { openModal(M_CRAFT); return; }
  for (size_t i = 0; i < hutPos.size(); i++) if (len(V3{hutDoor((int)i).x, 0, hutDoor((int)i).z} - me) < 3.6f) { enterRoom(1, (int)i); return; }
  if (!p.isMoon && len(V3{wDoor().x, 0, wDoor().z} - me) < 3.8f) { enterRoom(2, 0); return; }
  if (len(V3{wonderPos.x, 0, wonderPos.z} - me) < 7) { say("The " + wonderName(p), (p.vmask & VM_WONDER) ? std::string(WVOICE[wAdj]) : std::string("(It is fast asleep. Whoever wakes it will need to be polite.)")); return; }
  if (std::hypot(P.x, P.z) < 12) startTakeoff();
}
void interact() { if (paused || modal) return; if (st == EARTH && instN == 5 && std::hypot(P.x, P.z) < 24) { st = COUNT; cd = 0; audioEvent(S_BEEP_LO); } else if (st == SPACE && nearEnter >= 0) enterPlanet(nearEnter); else if (st == SURFACE && !cine) surfInteract(); }
void updSurface(float dt) {
  Planet& p = planets[curP];
  if (cine) { updCine(dt); updTalk(dt); for (auto& n : npcs) n.ph += dt; return; }
  if (inRoom) { updRoom(dt); return; }
  walk(dt, shAt, 10.5f, 320, p.grav * perkGrav, clampf(1 / std::sqrt(std::max(.3f, p.grav)), .7f, 1.5f) * perkSpeed * (starving ? .65f : 1.f)); invul = std::max(0.f, invul - dt); swingT = std::max(0.f, swingT - dt); mineT = std::max(0.f, mineT - dt); updTalk(dt); prompt.clear(); V3 me{P.x, 0, P.z};
  int qt = p.qi < nqOf(p) ? qtype(p, p.qi) : -1, no = (int)objs.size();
  for (size_t i = 0; i < objs.size(); i++) { Obj& o = objs[i]; if (o.done) continue; float d = len(V3{o.pos.x, 0, o.pos.z} - me);
    if (o.type == 0 && d < 2.8f) { o.done = true; p.mask |= 1u << i; p.prog++; audioEvent(S_CHIME); toast(fmt("%s collected\n%d/%d", itemName(p).c_str(), p.prog, p.nItems), 2.5f); if (p.prog >= p.nItems) { p.qph = 2; toast("ALL COLLECTED\nReturn to the elder", 4); } }
    else if (o.type == 4) { bool next = true; for (size_t j = 0; j < i; j++) if (!objs[j].done) next = false;
      if (next && d < 3.4f) { o.done = true; p.mask |= 1u << i; p.prog++; audioEvent(S_CHIME); toast(fmt("HOOP %d/%d", p.prog, no), 1.5f); if (p.prog >= no) { p.qph = 2; toast("ALL HOOPS CLEARED\nReturn to the elder", 4); } } }
    else if (d < 3.8f) { if (o.type == 1) prompt = "[{E}] Awaken the shrine"; else if (o.type == 2) prompt = "[{E}] Take the relic"; else if (o.type == 3) prompt = "[{E}] Talk to the lost villager"; else if (o.type == 5) prompt = "[{E}] Light the beacon";
      else if (o.type == 6) prompt = "[{E}] Awaken the " + wonderName(p); else if (o.type == 7) prompt = i == 0 ? "[{E}] Collect the parcel" : "[{E}] Deliver the parcel"; } }
  if (qt == QE && p.qph == 1 && p.prog > 0 && p.prog < no) { raceT += dt; if (raceT > 25.f + 14.f * no) { p.prog = 0; p.mask = 0; objsForQuest(p); raceT = 0; audioEvent(S_BEEP_LO); toast("TOO SLOW\nThe hoops fizzled - start over", 3.5f); } } else if (qt != QE || p.prog == 0) raceT = 0;
  bool rage = qt == QC && p.prog > 0 && !p.found; float gs = gaitSpeed(p.uniq, p.isMoon);
  // Moving people: they steer around everything that has a hitbox (a few probe directions, then a new goal if they are boxed in),
  // turn smoothly, keep apart from each other, and never end a frame inside a wall.
  auto freeAt = [&](V3 q, float y) { return !npcBlocked(q.x, q.z, y, .55f) && std::hypot(q.x, q.z) > 11.f; };
  auto steer = [&](Npc& n, V3 want, float y) -> V3 {
    static const float TRY[7] = {0, .55f, -.55f, 1.1f, -1.1f, 1.7f, -1.7f};
    for (float a : TRY) { float c = std::cos(a), s = std::sin(a); V3 d{want.x * c - want.z * s, 0, want.x * s + want.z * c}; if (freeAt(n.pos + d * 1.3f, y)) return d; }
    return V3{};
  };
  for (auto& n : npcs) {
    if (n.dead) continue;
    n.ph += dt; n.hurt = std::max(0.f, n.hurt - dt); V3 np{n.pos.x, 0, n.pos.z}, dv = me - np; float dist = len(dv), spd = 0; V3 mv{};
    auto pickDest = [&](float rad) {
      for (int t = 0; t < 6; t++) { float a = n.ph * 1.7f + n.wphase * 3.1f + n.look * .73f + t * 2.3f; V3 d = n.home + V3{std::sin(a) * rad, 0, std::cos(a * .81f + n.wphase) * rad};
        if (std::hypot(d.x, d.z) < 16) continue; n.dest = d; if (freeAt(d, shAt(d.x, d.z))) break; }
      n.wait = 1.1f + std::fmod(std::fabs(n.ph * 1.3f + n.wphase), 2.4f);
    };
    if (n.wicked) {
      bool hostile = !p.found || n.angry; float hd = difficulty == 2 ? 1.45f : 1.f, aggro = hostile ? (n.angry ? 34.f : rage ? 45.f : 20.f) * hd : 0.f;
      if (n.wait > 0 && n.hurt > 0) n.wait -= dt;   // staggered by a sword hit
      else if (dist < aggro) { n.pushed = false; mv = norm(dv); spd = (rage || n.angry ? 5.6f : 4.6f) * (difficulty == 2 ? 1.3f : 1.f); n.wait = 0; }
      else {
        V3 t = n.dest - np; float td = len(t);
        if (n.wait > 0) n.wait -= dt;
        else if (td > 1.2f) { mv = norm(t); spd = 2.2f + std::fmod(n.wphase, 1.f) * .8f; }
        else pickDest(n.chief ? 10.f : 22.f);
      }
      if (dist < 1.5f && invul <= 0 && hostile && n.hurt <= 0) { int dmg = hitDamage(); P.vx += norm(dv).x * 9; P.vz += norm(dv).z * 9; hurtPlayer(dmg, n.name); }
    } else if (n.merchant) {   // the merchant stays behind the counter and paces along it
      if (n.wait > 0) n.wait -= dt;
      else {
        V3 t = n.dest - np; float td = len(t);
        if (td > .5f) { mv = norm(t); spd = 1.1f; }
        else { V3 X{std::cos(marketYaw), 0, -std::sin(marketYaw)}; float a = n.ph * .9f + n.wphase; n.dest = n.home + X * (std::sin(a) * 1.3f); n.wait = .8f + std::fmod(n.ph, 1.6f); }
      }
    } else {
      if (n.wait > 0) n.wait -= dt;
      else {
        V3 t = n.dest - np; float td = len(t);
        if (td > 1.1f) { mv = norm(t); spd = n.elder ? 1.35f : 2.05f + std::fmod(n.wphase, 1.f); }
        else pickDest(n.elder ? 9.f : n.poi >= 0 ? 14.f : 18.f);
      }
    }
    if (n.merchant && modal == M_MARKET) { mv = {}; spd = 0; }   // trading: the merchant stays put and faces you
    if (n.pushed) { V3 t = n.back - np; float td = len(t);
      if (n.wait > 0) n.wait += dt;   // the route's pause is kept for later
      if (td > .2f) { mv = norm(t); spd = 1.8f; } else { n.pos.x = n.back.x; n.pos.z = n.back.z; n.pushed = false; mv = {}; spd = 0; } }
    spd *= gs;
    if (spd > 0 && !n.merchant) { V3 d = steer(n, mv, n.pos.y); if (len(d) < .5f) { n.stuck += dt; spd = 0; if (n.stuck > .6f) { n.stuck = 0; if (n.pushed) n.pushed = false; else { if (!n.wicked || dist >= 1.5f) pickDest(14.f); n.wait = .3f; } }   /* cannot get back after a shove: carry on with the route from here */ } else { n.stuck = 0; mv = d; } }
    { float nx = n.pos.x + mv.x * spd * dt - me.x, nz = n.pos.z + mv.z * spd * dt - me.z, nd = std::sqrt(nx * nx + nz * nz);   // the player is in the way: stop instead of walking into them
      if (spd > 0 && nd < 1.f && nd < dist) spd = 0; }
    n.pos.x += mv.x * spd * dt; n.pos.z += mv.z * spd * dt;
    if (!n.merchant) for (auto& o : npcs) if (&o != &n && !o.dead) { float dx = n.pos.x - o.pos.x, dz = n.pos.z - o.pos.z, d2 = dx * dx + dz * dz; if (d2 < .81f && d2 > 1e-6f) { float d = std::sqrt(d2), k = (.9f - d) * .5f / d; n.pos.x += dx * k; n.pos.z += dz * k; } }   // a little personal space
    { float px = n.pos.x - P.x, pz = n.pos.z - P.z, pd = std::sqrt(px * px + pz * pz);   // you can shove people: they step aside, then walk back to where they were
      if (pd < .9f && std::fabs(n.pos.y - P.y) < 2.f) { if (!n.pushed) { n.pushed = true; n.back = n.pos; } if (pd < 1e-3f) { px = -std::sin(P.yaw); pz = -std::cos(P.yaw); pd = 1; } n.pos.x = P.x + px / pd * .9f; n.pos.z = P.z + pz / pd * .9f; npcCollide(n, .45f); } }
    if (!n.merchant) npcCollide(n, .45f); { float r = std::hypot(n.pos.x, n.pos.z); if (r > 318) { n.pos.x *= 318 / r; n.pos.z *= 318 / r; } }
    n.pos.y = shAt(n.pos.x, n.pos.z); n.sp += (spd - n.sp) * std::min(1.f, dt * 6);
    V3 face = (dist < 9 && !n.wicked) ? dv : spd > .15f ? mv : dv; if (len(face) > .2f) { float ty = std::atan2(face.x, face.z), dy = std::remainder(ty - n.yaw, 2 * PI); n.yaw += dy * std::min(1.f, dt * 7.f); }   // no spinning when the target is right on top of them   // turn smoothly instead of snapping
    if (prompt.empty() && dist < 3.8f) prompt = n.wicked ? (swordSel >= 0 && (n.angry || !p.found) ? "[{ATK}] Strike " + n.name + "   [{E}] Confront" : "[{E}] Confront " + n.name) : n.merchant ? "[{E}] Trade with " + n.name : "[{E}] Talk to " + n.name;
  }
  if (prompt.empty() && len(V3{marketPos.x, 0, marketPos.z} - me) < 3.6f) for (auto& n : npcs) if (n.merchant) prompt = "[{E}] Trade with " + n.name;
  for (auto& nd : nodes) { if (nd.cool > 0) { nd.cool -= dt; continue; } if (!prompt.empty() || len(V3{nd.pos.x, 0, nd.pos.z} - me) >= 3.4f) continue;
    if (nd.res == 100 || nd.res == 101) prompt = "[{E}] Pick " + foodName(pkey(p) * 4 + nd.res - 100);
    else { std::string nm = nd.res == 200 ? rareName(pkey(p)) : std::string(RES_NAME[nd.res]), why = nodeNeed(p, nd); prompt = why.empty() ? "[{E}] Mine " + nm : nm + " - " + why; } }
  if (prompt.empty() && len(benchPos - me) < 3.2f) prompt = "[{E}] Use the workbench  (craft rockets)";
  for (size_t i = 0; i < hutPos.size(); i++) if (prompt.empty() && len(V3{hutDoor((int)i).x, 0, hutDoor((int)i).z} - me) < 3.4f) prompt = "[{E}] Enter the house";
  if (prompt.empty() && !p.isMoon && len(V3{wDoor().x, 0, wDoor().z} - me) < 3.6f) prompt = "[{E}] Enter the wicked house (careful...)";
  updSurvival(dt);
  if (prompt.empty() && len(V3{wonderPos.x, 0, wonderPos.z} - me) < 7) prompt = (p.vmask & VM_WONDER) ? "[{E}] Touch the " + wonderName(p) : "[{E}] Examine the " + wonderName(p);
  if (prompt.empty() && std::hypot(P.x, P.z) < 12) prompt = "[{E}] Return to your rocket and leave " + p.name;
  audioSet(0, 0, 100, 0);   // no constant wind hiss on the ground: it was the drone heard on every planet
}
void surfHud(std::string& obj, std::string& stat) {
  Planet& p = planets[curP]; std::string q; int qt = p.qi < nqOf(p) ? qtype(p, p.qi) : -1, n = (int)objs.size(); bool ord = qt == QE || qt == QG || qt == QF; float nd = 1e9f;
  for (auto& o : objs) if (!o.done) { nd = std::min(nd, len(V3{o.pos.x, 0, o.pos.z} - V3{P.x, 0, P.z})); if (ord) break; }
  if (p.qi >= nqOf(p)) q = "Planet discovered! Explore freely, hunt the last sights, or return to your rocket (E).";
  else if (p.qph == 0) q = fmt("Quest %d/%d: talk to the elder", p.qi + 1, nqOf(p)) + (p.hideVil ? " - no beacon, search the land" : " (glowing beacon in the village)");
  else if (p.qph == 2) q = "Return to the elder";
  else { std::string t;
    switch (qt) {
      case QA: t = fmt("gather %s (%d/%d)", itemName(p).c_str(), p.prog, p.nItems); break;
      case QB: t = fmt("awaken the shrines (%d/%d)", p.prog, n); break;
      case QC: t = p.hideCamp ? "find the hidden camp and take the " + relicName(p) : "retrieve the " + relicName(p) + " from the wicked camp"; break;
      case QD: t = fmt("find the lost villagers (%d/%d)", p.prog, n); break;
      case QE: t = fmt("run the hoops in order (%d/%d)", p.prog, n) + (p.prog > 0 ? fmt("   time left %d s", std::max(0, (int)(25.f + 14.f * n - raceT))) : std::string()); break;
      case QF: t = p.prog == 0 ? "collect the parcel at the " + poiName(p, 0) : "deliver it to the " + poiName(p, 1); break;
      case QG: t = fmt("light the beacons in order (%d/%d)", p.prog, n); break;
      case QH: t = fmt("question the witnesses (%d/3)", p.prog); break;
      default: t = "wake the " + wonderName(p); }
    q = fmt("Quest %d/%d: ", p.qi + 1, nqOf(p)) + t; }
  if (p.qph == 1 && nd < 1e8f && (ord || !p.hideVil)) q += fmt("  [%d m]", (int)nd);
  obj = p.name + "  -  " + KL[p.kind] + "\n" + q; stat = fmt("gravity %.1f g   SIGHTS %d/5", p.grav, sights(p)) + (meds ? fmt("   MEDPACKS %d", meds) : std::string());
}
void surfPoints(std::vector<PT>& pts) {
  Planet& p = planets[curP]; int qt = p.qi < nqOf(p) ? qtype(p, p.qi) : -1; bool ord = qt == QE || qt == QG || qt == QF; int nx = -1;
  for (size_t i = 0; i < objs.size(); i++) if (!objs[i].done) { nx = (int)i; break; }
  for (size_t i = 0; i < objs.size(); i++) { Obj& o = objs[i]; if (o.done) continue; bool nxt = (int)i == nx; float al = .16f, sz = 20; V3 c;
    switch (o.type) { case 0: c = itemCol(p); break; case 1: c = {.3f, .8f, 1}; break; case 2: c = {1, .8f, .2f}; break; case 3: c = {.4f, 1, .5f}; break; case 4: c = {1, .7f, .2f}; break; case 5: c = {1, .45f, .2f}; break; case 6: c = wonderCol; break; case 7: c = {1, .9f, .4f}; break; default: c = {.9f, .6f, 1}; }
    if (ord) { if (nxt) { al = .5f; sz = 34; } else al = .04f; }
    int kk = o.type == 6 ? 26 : 14; for (int k = 0; k < kk; k++) { V3 q = o.pos - camPos + V3{0, k * 3.5f, 0}; pts.push_back({q.x, q.y, q.z, sz, c.x, c.y, c.z, al}); } }
  for (auto& n : npcs) if (n.elder && p.qi < nqOf(p) && (p.qph == 0 || p.qph == 2) && !p.hideVil) { V3 q = n.pos - camPos + V3{0, 4.3f, 0}; pts.push_back({q.x, q.y, q.z, 60, 1, .85f, .2f, .9f}); }
  if (!p.hideVil) for (int k = 0; k < 16; k++) { V3 q = vilC - camPos + V3{0, k * 5.f, 0}; pts.push_back({q.x, q.y, q.z, 26, .5f, .8f, 1, .14f}); }
  for (auto& nd : nodes) if (nd.cool <= 0) { V3 bc = nd.res == 200 ? rareColor(pkey(p)) : nd.res >= 100 ? foodColor(pkey(p) * 4 + nd.res - 100) : RES_COL[nd.res]; for (int k = 0; k < 5; k++) { V3 q = nd.pos - camPos + V3{0, k * 1.6f, 0}; pts.push_back({q.x, q.y, q.z, 10, bc.x, bc.y, bc.z, .13f}); } }
  if (!p.hideCamp) for (int k = 0; k < 10; k++) { V3 q = campC - camPos + V3{0, k * 4.f, 0}; pts.push_back({q.x, q.y, q.z, 24, 1, .1f, .05f, .12f}); }
  for (size_t i = 0; i < pois.size(); i++) if (!((p.vmask >> i) & 1)) { V3 b = pois[i].pos; b.y = shAt(b.x, b.z); for (int k = 0; k < 8; k++) { V3 q = b - camPos + V3{0, k * 3.f, 0}; pts.push_back({q.x, q.y, q.z, 18, .85f, .85f, .95f, .08f}); } }
  { V3 b = wonderPos; b.y = shAt(b.x, b.z); float al = (p.vmask & VM_WONDER) ? .1f : .2f; for (int k = 0; k < 30; k++) { V3 q = b - camPos + V3{0, k * 4.f, 0}; pts.push_back({q.x, q.y, q.z, 24, wonderCol.x, wonderCol.y, wonderCol.z, al}); } }
}
// ---- the planet's signature Wonder (8 shapes x 16 moods = 128 different wonders) and the 10 kinds of sights
static void drawWonder(const Planet& p) {
  V3 b = wonderPos; b.y = shAt(b.x, b.z); float pul = 1.f + .25f * std::sin(tAcc * 2 + wAdj), spin = tAcc * (.3f + (wAdj % 5) * .15f), bob = std::sin(tAcc * (.6f + wAdj * .05f)) * .5f;
  float ang = (wNoun == 1 || wNoun == 3) ? spin : wAdj * .39f; V3 X{std::cos(ang), 0, -std::sin(ang)}, Z{std::sin(ang), 0, std::cos(ang)}, c = wonderCol * pul, sn{.55f, .55f, .6f};
  auto S_ = [&](const Mesh& m, float lx, float ly, float lz, V3 s, V3 t, float em, GLuint tx = 0) { DP d; d.p = b + X * lx + Z * lz + V3{0, ly, 0}; d.s = s; d.X = X; d.Z = Z; d.t = t; d.emis = em; d.tex = tx ? tx : tWhite; draw(m, d); };
  S_(mCylT, 0, 0, 0, {6, .8f, 6}, sn, 0, tConc);
  switch (wNoun) {
    case 0: S_(mBox, 0, 7.5f, 0, {2.2f, 14, 2.2f}, sn, 0, tConc); S_(mBox, 0, 9, 0, {2.4f, .5f, 2.4f}, c, 2.5f); S_(mCone, 0, 14.5f, 0, {1.6f, 3, 1.6f}, c, 1.5f); break;
    case 1: S_(mCylN, 0, .8f, 0, {.8f, 3, .8f}, sn, 0, tConc); S_(mSphereLo, 0, 7 + bob, 0, {3, 3, 3}, c, 1.8f);
      for (int k = 0; k < 3; k++) { float a = tAcc * 1.2f + k * 2.09f; S_(mSphereLo, std::cos(a) * 5.5f, 7 + bob + std::sin(a * 1.3f) * 1.5f, std::sin(a) * 5.5f, {.6f, .6f, .6f}, c, 3); } break;
    case 2: S_(mBox, -5, 6.8f, 0, {2, 12, 2}, sn, 0, tConc); S_(mBox, 5, 6.8f, 0, {2, 12, 2}, sn, 0, tConc); S_(mBox, 0, 13, 0, {12.5f, 2, 2.2f}, sn, 0, tConc);
      S_(mSphereLo, 0, 5.5f + bob * .5f, 0, {2.2f, 2.2f, .5f}, c, 2.5f); S_(mBox, 0, 14.4f, 0, {2, 1, 2}, c, 2); break;
    case 3: { static const float OX[5] = {0, 3, -3, 1.5f, -2}, OZ[5] = {0, 1.5f, 1, -3, -2.5f}, OH[5] = {12, 8, 9, 7, 6};
      for (int k = 0; k < 5; k++) S_(mCone, OX[k], .8f, OZ[k], {1.8f - k * .12f, OH[k], 1.8f - k * .12f}, c, 1.4f + k * .2f); break; }
    case 4: S_(mBox, 0, 1.6f, 0, {5, 1.5f, 5}, sn, 0, tConc); S_(mBox, 0, 4.5f, -2.3f, {5, 6, 1}, sn, 0, tConc); S_(mBox, -2.5f, 2.6f, 0, {1, 2.5f, 5}, sn, 0, tConc); S_(mBox, 2.5f, 2.6f, 0, {1, 2.5f, 5}, sn, 0, tConc);
      S_(mSphereLo, 0, 9 + bob, -2.3f, {1.1f, 1.1f, 1.1f}, c, 3); break;
    case 5: S_(mBox, 0, 2, 0, {3, 3, 3}, sn, 0, tRock); S_(mBox, 0, 5, 0, {2.6f, 3, 2.6f}, c * .5f, .4f, tRock); S_(mBox, 0, 8, 0, {2.2f, 3, 2.2f}, sn, 0, tRock); S_(mSphereLo, 0, 10.5f, 0, {1.6f, 1.6f, 1.6f}, c, 1.2f);
      for (int e = -1; e <= 1; e += 2) { S_(mSphereLo, e * .6f, 10.8f, 1.4f, {.3f, .3f, .3f}, {1, 1, 1}, 4); S_(mBox, e * 2.6f, 6, 0, {.4f, 3, 2.5f}, c, 1); } break;
    case 6: S_(mCylN, -5, .8f, 0, {.4f, 11, .4f}, sn, 0, tConc); S_(mCylN, 5, .8f, 0, {.4f, 11, .4f}, sn, 0, tConc); S_(mBox, 0, 11.4f, 0, {11, .6f, .6f}, sn, 0, tConc);
      { DP d; d.p = b + V3{0, 7 + std::sin(tAcc * 2) * .15f, 0}; d.s = {3.6f, .5f, 3.6f}; d.X = {1, 0, 0}; d.Y = {0, 0, 1}; d.Z = {0, 1, 0}; d.t = c; d.emis = 1.5f + .5f * std::sin(tAcc * 3); d.tex = tWhite; draw(mCylN, d); } break;
    default: S_(mCylT, 0, .8f, 0, {2.6f, 12, 2.6f}, sn, 0, tConc); S_(mCylN, 0, 6, 0, {2.7f, 1.2f, 2.7f}, c, .8f); S_(mBox, 0, 13.2f, 0, {2.8f, 1.8f, 2.8f}, c, 3); S_(mCone, 0, 14.1f, 0, {2.2f, 2.2f, 2.2f}, sn, 0); break;
  }
}
static void drawPoi(const Planet& p, const Poi& q, int idx) {
  V3 b = q.pos; b.y = shAt(b.x, b.z); V3 Z{std::sin(q.yaw), 0, std::cos(q.yaw)}, X{Z.z, 0, -Z.x}; int u = p.uniq;
  V3 tn = lin(hsl(.05f + ((u + idx * 5) % 12) * .08f, .3f, .5f)), ac = lin(hsl(.1f + ((u * 3 + idx * 7) % 14) * .07f, .7f, .55f)) * 1.5f, dk{.03f, .025f, .02f}; float bob = std::sin(tAcc * 1.5f + idx) * .3f;
  auto S_ = [&](const Mesh& m, float lx, float ly, float lz, V3 s, V3 t, float em, GLuint tx = 0) { DP d; d.p = b + X * lx + Z * lz + V3{0, ly, 0}; d.s = s; d.X = X; d.Z = Z; d.t = t; d.emis = em; d.tex = tx ? tx : tWhite; draw(m, d); };
  switch (q.type) {
    case 0: { DP h; h.p = b + V3{0, 2.3f, 0} - Z * 6.f; h.s = {2.6f, 12, 2.6f}; h.X = X; h.Y = Z; h.Z = {0, 1, 0}; h.t = tn * .7f; h.tex = tHull; draw(mCylN, h);
      DP nz; nz.p = b + V3{0, 2.3f, 0} + Z * 6.f; nz.s = {2.6f, 4, 2.6f}; nz.X = X; nz.Y = Z; nz.Z = {0, 1, 0}; nz.t = tn * .9f; nz.tex = tHull; draw(mCone, nz);
      S_(mBox, 3, 3.2f, -5, {.4f, 4, 3}, tn, 0, tHull); S_(mBox, -3.4f, 1.2f, -2, {.4f, 2.2f, 3}, tn, 0, tHull); S_(mSphereLo, 1, 5.3f + bob, 0, {.3f, .3f, .3f}, {1, .7f, .2f}, 4); break; }
    case 1: for (int k = 0; k < 6; k++) { float a = k * 1.047f; S_(mCylN, std::cos(a) * 7, 0, std::sin(a) * 7, {.8f, k % 3 == 0 ? 3.f : 6.f, .8f}, tn, 0, tRock); }
      S_(mBox, 0, .5f, 0, {5, 1, 5}, tn, 0, tConc); S_(mSphereLo, 0, 1.8f + bob, 0, {.5f, .5f, .5f}, ac, 2.5f); break;
    case 2: S_(mBox, 0, 2.2f, 0, {10, 4.4f, 7}, tn, 0, tConc); S_(mBox, 0, 4.7f, 0, {11, .6f, 8}, dk, 0); S_(mBox, 0, 5.6f, 3.8f, {4, 1.2f, .3f}, ac, 2.2f); S_(mBox, 0, 1.2f, 3.55f, {1.6f, 2.4f, .2f}, dk, 0);
      for (int k = 0; k < 3; k++) S_(mCylT, -3.f + k * 3.f, 0, 6, {.5f, 1, .5f}, ac * .6f, .2f); break;
    case 3: for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) S_(mBox, sx * 2.f, 5, sz * 2.f, {.5f, 10, .5f}, tn, 0, tConc);
      S_(mBox, 0, 10.7f, 0, {5, 2.6f, 5}, tn, 0, tConc); S_(mCone, 0, 12, 0, {3.8f, 2, 3.8f}, ac * .5f, .3f); S_(mSphereLo, 0, 10.9f, 0, {.9f, .9f, .9f}, ac, 3.5f); break;
    case 4: S_(mCylN, 0, 0, 0, {6, .25f, 6}, ac, 1.5f); S_(mCylN, 0, .1f, 0, {4.6f, .2f, 4.6f}, ac * 1.3f, 2.2f);
      for (int k = 0; k < 7; k++) { float a = k * .897f; S_(mSphereLo, std::cos(a) * 6.5f, .5f, std::sin(a) * 6.5f, {1.2f, .9f, 1.1f}, tn, 0, tRock); }
      S_(mSphereLo, 0, 2.5f + bob * 2, 0, {1.2f, .9f, 1.2f}, {.8f, .8f, .9f}, .6f); break;
    case 5: S_(mBox, 0, 1.5f, 0, {5, 3, 5}, tn, 0, tConc); S_(mCylN, 0, 3, 0, {1.8f, 7, 1.8f}, tn * 1.2f, 0, tRock); S_(mSphereLo, 0, 11.3f, 0, {1.6f, 1.6f, 1.6f}, tn * 1.2f, 0, tRock);
      S_(mBox, 2.2f, 8.6f, 0, {.8f, .8f, 3}, tn * 1.2f, 0, tRock); S_(mSphereLo, 0, 11.2f, 1.7f, {.35f, .7f, .35f}, tn * 1.2f, 0, tRock); break;
    case 6: for (int k = 0; k < 4; k++) { float t = .3f + k * .25f, h = 7.f + k; V3 Yv = norm(X * std::sin(t) + V3{0, std::cos(t), 0}), Xv = norm(cross(Z, Yv)); V3 base = b + X * (k * 4.f - 6) + Z * (std::sin(k * 3.f) * 3);
        DP d; d.p = base; d.s = {1.2f, h, 1.2f}; d.X = Xv; d.Y = Yv; d.Z = Z; d.t = tn * 1.4f; d.tex = tHull; draw(mCylN, d);
        DP c2; c2.p = base + Yv * h; c2.s = {1.2f, 2.5f, 1.2f}; c2.X = Xv; c2.Y = Yv; c2.Z = Z; c2.t = ac * .5f; c2.tex = tHull; draw(mCone, c2); } break;
    case 7: S_(mSphereLo, 0, .2f, 0, {3.2f, 2.6f, 3.2f}, tn, 0, tRock); S_(mBox, 0, 1.1f, 2.9f, {1.2f, 2, .3f}, dk, 0); S_(mCylN, 1.5f, 2.2f, 0, {.4f, 2.4f, .4f}, tn * .7f, 0, tRock); S_(mSphereLo, 5, .5f, 3, {.6f, .7f, .6f}, {1, .5f, .1f}, 4); break;
    case 8: S_(mCylN, 0, 0, 0, {.5f, 12, .5f}, tn, 0, tConc); S_(mBox, 0, 12, 0, {3, .4f, 3}, tn, 0, tConc);
      for (int k = 0; k < 4; k++) { float a = tAcc * 1.5f + k * 1.5708f; V3 rad = X * std::cos(a) + V3{0, std::sin(a), 0}, per = X * -std::sin(a) + V3{0, std::cos(a), 0};
        DP d; d.p = b + V3{0, 12.5f, 0} + Z * 1.f + rad * 3.5f; d.s = {.7f, 7, .15f}; d.X = per; d.Y = rad; d.Z = Z; d.t = ac; d.emis = 1.f; d.tex = tWhite; draw(mBox, d); } break;
    default: for (int k = -1; k <= 1; k++) { V3 tc = lin(hsl(.08f + ((u + k + 2) % 9) * .1f, .7f, .5f)) * 1.3f;
        S_(mBox, k * 6.f, .6f, 0, {4, 1.2f, 2}, tn, 0, tConc); S_(mBox, k * 6.f, 3, -.3f, {4.6f, .3f, 3}, tc, .5f); S_(mSphereLo, k * 6.f, 1.5f + bob * .3f, 0, {.5f, .5f, .5f}, tc, 1.5f); } break;
  }
}
void drawSurface() {
  Planet& p = planets[curP];
  if (inRoom) { drawRoom(p); for (auto& n : roomNpcs) drawNpc(n, p); return; } DP g; g.detail = 1; g.tex = tSurf; g.uvs = 84.f + (p.uniq % 9) * 4.f; draw(mSurf, g);
  DP pad; pad.p = {0, -.3f, 0}; pad.s = {11, 1.2f, 11}; pad.tex = tConc; pad.uvs = 3; pad.t = {.9f, .9f, .9f}; draw(mCylT, pad); drawRocket({0, surfRocketY, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
  V3 tA = floraA, tB = floraB, dark = lin(p.pal[0]) * .6f;
  for (auto& f : flora) { float y = shAt(f.x, f.z), s = f.s; auto Q = [&](const Mesh& m, float oy, V3 sc, V3 t, float em, float ox = 0, float oz = 0) { DP d; d.p = {f.x + ox * s, y + oy * s, f.z + oz * s}; d.s = sc * s; d.t = t; d.emis = em; d.tex = (&m == &mCylN || &m == &mCylT) ? (tBarkW ? tBarkW : tWhite) : (tLeafW ? tLeafW : tWhite); draw(m, d); };
    if (f.type == 0) { Q(mCylN, 0, {.3f, 3, .3f}, dark, 0); Q(mCone, 2.2f, {1.7f, 4.6f, 1.7f}, tB, 0); }
    else if (f.type == 1) { Q(mCylN, 0, {.25f, 2.2f, .25f}, tA * .8f, 0); Q(mSphereLo, 2.2f, {1.6f, .8f, 1.6f}, tB, .15f); }
    else if (f.type == 2) { Q(mCone, 0, {.5f, 3.2f, .5f}, tB, .7f); Q(mCone, 0, {.4f, 2.2f, .4f}, tB, .7f, .7f); Q(mCone, 0, {.35f, 1.6f, .35f}, tB, .7f, -.6f); }
    else if (f.type == 3) { Q(mCylN, 0, {.12f, 2.6f, .12f}, tA * .7f, 0); Q(mSphereLo, 2.7f, {.5f, .5f, .5f}, tB, 1.2f); }
    else if (f.type == 4) { Q(mBox, 0, {.4f, 2.8f, .4f}, tA, .4f); Q(mBox, 2.6f, {1.4f, .3f, 1.4f}, tB, .8f); }
    else if (f.type == 5) { Q(mSphereLo, .6f, {1.1f, .6f, 1.1f}, tB, .3f); Q(mSphereLo, 1.4f, {.7f, .5f, .7f}, tA, .2f, .4f); }
    else if (f.type == 6) { Q(mCylN, 0, {.18f, 4.2f, .18f}, dark, 0); Q(mCone, 3.6f, {2.2f, 2.4f, 2.2f}, tB, .05f); Q(mCone, 2.2f, {1.4f, 1.6f, 1.4f}, tA, 0, .5f); }
    else { Q(mCylT, 0, {.55f, 1.1f, .55f}, tA, .15f); Q(mSphereLo, 1.3f, {1.3f, 1.3f, 1.3f}, tB, p.glow > 0 ? 1.4f : .2f); } }
  { V3 rc = lin(mixc(p.pal[1], p.pal[4], .45f)) * 1.25f + V3{.08f, .08f, .08f}; for (size_t i = 0; i < boulders.size(); i++) { const Rock& b = boulders[i]; float a = i * 2.39f; DP d; d.p = {b.x, shAt(b.x, b.z) + b.sy * .15f, b.z}; d.s = {b.sx, b.sy, b.sz}; d.X = {std::cos(a), 0, -std::sin(a)}; d.Z = {std::sin(a), 0, std::cos(a)}; d.tex = tRock; d.uvs = 1.5f; d.t = rc * (.85f + (i % 5) * .06f); draw(mSphereLo, d); } }
  for (size_t i = 0; i < hutPos.size(); i++) { V3 h = hutPos[i]; float ang = i * 1.3f; V3 Z{std::sin(ang), 0, std::cos(ang)}, X{Z.z, 0, -Z.x}; float hy = shAt(h.x, h.z); int ar = p.archT;
    if (ar == 0) { DP w; w.p = h + V3{0, hy + 1.2f, 0}; w.s = {3.4f, 2.4f, 3.4f}; w.X = X; w.Z = Z; w.tex = tConc; w.t = hutCol; draw(mBox, w);
      DP rf; rf.p = h + V3{0, hy + 2.4f, 0}; rf.s = {2.9f, 1.8f, 2.9f}; rf.X = X; rf.Z = Z; rf.tex = tRock; rf.t = roofCol; draw(mCone, rf); }
    else if (ar == 1) { DP w; w.p = h + V3{0, hy, 0}; w.s = {2.2f, 4.8f, 2.2f}; w.X = X; w.Z = Z; w.tex = tHull; w.t = hutCol; draw(mCylN, w);
      DP cap; cap.p = h + V3{0, hy + 4.8f, 0}; cap.s = {2.4f, 1.6f, 2.4f}; cap.X = X; cap.Z = Z; cap.t = roofCol; cap.emis = .4f; draw(mCone, cap); }
    else if (ar == 2) { DP w; w.p = h + V3{0, hy + .8f, 0}; w.s = {4.2f, 1.6f, 4.2f}; w.X = X; w.Z = Z; w.tex = tRock; w.t = hutCol; draw(mSphereLo, w); }
    else if (ar == 3) { DP w; w.p = h + V3{0, hy, 0}; w.s = {2.6f, 3.6f, 2.6f}; w.X = X; w.Z = Z; w.tex = tConc; w.t = hutCol; draw(mCylT, w);
      DP rf; rf.p = h + V3{0, hy + 3.6f, 0}; rf.s = {3.4f, .4f, 3.4f}; rf.X = X; rf.Z = Z; rf.t = roofCol; draw(mBox, rf); }
    else if (ar == 4) { DP w; w.p = h + V3{0, hy + 1.6f, 0}; w.s = {2.8f, 3.2f, 2.8f}; w.X = X; w.Z = Z; w.tex = tHull; w.t = hutCol; w.emis = .25f; draw(mBox, w);
      DP sp; sp.p = h + V3{0, hy + 4.4f, 0}; sp.s = {.3f, 2.2f, .3f}; sp.t = roofCol; sp.emis = 1.2f; draw(mCylN, sp); }
    else if (ar == 5) { DP w; w.p = h + V3{0, hy, 0}; w.s = {3.8f, 2.2f, 3.8f}; w.X = X; w.Z = Z; w.tex = tRock; w.t = hutCol; draw(mCone, w); }
    else if (ar == 6) { for (int k = 0; k < 3; k++) { DP w; w.p = h + X * (k * 1.4f - 1.4f) + V3{0, hy + 1.1f, 0}; w.s = {1.4f, 2.2f, 1.4f}; w.X = X; w.Z = Z; w.tex = tConc; w.t = hutCol; draw(mBox, w); } }
    else { DP w; w.p = h + V3{0, hy + 1.85f, 0}; w.s = {2.1f, 2.1f, 2.1f}; w.X = X; w.Z = Z; w.tex = tHull; w.t = hutCol; w.emis = .6f; draw(mSphereLo, w); }
    DP door; door.p = h + Z * hutDoorDist(ar) + V3{0, hy + .85f, 0}; door.s = {.9f, 1.7f, .1f}; door.X = X; door.Z = Z; door.t = {.02f, .015f, .01f}; draw(mBox, door); }
  if (!p.hideVil) { DP bc; bc.p = vilC; bc.s = {.6f, 12, .6f}; bc.t = {.5f, .8f, 1}; bc.emis = p.found ? 3.f : 1.2f; draw(mCylN, bc); }
  if (!p.hideCamp) for (int i = 0; i < 8; i++) { float a = i * PI / 4; DP s; s.p = campC + V3{std::cos(a) * 11, 0, std::sin(a) * 11}; s.s = {.8f, 5.f + (i % 3), .8f}; s.t = {.03f, .02f, .02f}; s.emis = .3f; draw(mCone, s); }
  drawProps(p);
  if (!p.isMoon) { DP pe; pe.p = relicPos - V3{0, 1.6f, 0}; pe.s = {.9f, 1.2f, .9f}; pe.tex = tConc; draw(mCylN, pe); }
  if (!p.isMoon && (p.qi < RQ || (p.qi == RQ && p.prog == 0))) { DP rl; rl.p = relicPos + V3{0, .5f + std::sin(tAcc * 2) * .2f, 0}; rl.s = {.8f, .8f, .8f}; rl.t = {1, .8f, .2f}; rl.emis = 3; draw(mSphereLo, rl); }
  int nxo = -1; for (size_t i = 0; i < objs.size(); i++) if (!objs[i].done) { nxo = (int)i; break; }
  for (size_t i = 0; i < objs.size(); i++) { Obj& o = objs[i]; float a = tAcc * 2 + i; bool nxt = (int)i == nxo;
    if (o.done) {
      if (o.type == 1) { DP d; d.p = o.pos; d.s = {.6f, 3.5f, .6f}; d.tex = tConc; d.t = {.5f, .55f, .6f}; draw(mCylN, d); DP t; t.p = o.pos + V3{0, 4, 0}; t.s = {.9f, .9f, .9f}; t.t = {.3f, 1, .6f}; t.emis = 4; draw(mSphereLo, t); }
      else if (o.type == 5) { DP d; d.p = o.pos; d.s = {.7f, 4.5f, .7f}; d.tex = tConc; d.t = {.5f, .5f, .55f}; draw(mCylN, d); DP t; t.p = o.pos + V3{0, 5, 0}; t.s = {1, 1, 1}; t.t = {.4f, 1, .5f}; t.emis = 4; draw(mSphereLo, t); }
      continue; }
    if (o.type == 0) { DP d; d.p = o.pos + V3{0, std::sin(tAcc * 2 + i) * .3f, 0}; d.s = {1, 1.6f, 1}; d.X = {std::cos(a), 0, -std::sin(a)}; d.Z = {std::sin(a), 0, std::cos(a)}; d.t = itemCol(p); d.emis = 2.5f; draw(mBox, d); }
    else if (o.type == 1) { DP d; d.p = o.pos; d.s = {.6f, 3.5f, .6f}; d.tex = tConc; d.t = {.5f, .55f, .6f}; draw(mCylN, d); DP t; t.p = o.pos + V3{0, 4, 0}; t.s = {.7f, .7f, .7f}; t.t = {.3f, .8f, 1}; t.emis = 1.5f; draw(mSphereLo, t); }
    else if (o.type == 3) { float bob = std::fabs(std::sin(tAcc * 3 + i)) * .5f; DP bd; bd.p = o.pos + V3{0, bob, 0}; bd.s = {.45f, 1.1f, .45f}; bd.t = {.5f, .9f, .6f}; bd.emis = .3f; draw(mCylN, bd);
      DP hd; hd.p = o.pos + V3{0, 1.45f + bob, 0}; hd.s = {.32f, .32f, .32f}; hd.t = {.6f, 1.f, .7f}; hd.emis = .4f; draw(mSphereLo, hd); }
    else if (o.type == 4) { V3 hdg = i + 1 < objs.size() ? objs[i + 1].pos - o.pos : (i > 0 ? o.pos - objs[i - 1].pos : V3{1, 0, 0}); hdg = norm(V3{hdg.x, 0, hdg.z});
      DP d; d.p = o.pos; d.s = {4, 4, 4}; d.X = {hdg.z, 0, -hdg.x}; d.Y = hdg; d.Z = {0, 1, 0}; d.mode = 2; d.t = nxt ? V3{1, .7f, .2f} : V3{.25f, .2f, .1f}; draw(mRing, d); }
    else if (o.type == 5) { DP d; d.p = o.pos; d.s = {.7f, 4.5f, .7f}; d.tex = tConc; d.t = {.5f, .5f, .55f}; draw(mCylN, d); DP t; t.p = o.pos + V3{0, 5, 0}; float sz = nxt ? 1.2f + .15f * std::sin(tAcc * 4) : .8f; t.s = {sz, sz, sz}; t.t = {1, .45f, .2f}; t.emis = nxt ? 5.f : .25f; draw(mSphereLo, t); }
    else if (o.type == 7) { if (i == 0) { DP d; d.p = o.pos + V3{0, std::sin(a) * .25f, 0}; d.s = {1.1f, 1.1f, 1.1f}; d.X = {std::cos(a), 0, -std::sin(a)}; d.Z = {std::sin(a), 0, std::cos(a)}; d.t = {1, .8f, .3f}; d.emis = 1.8f; draw(mBox, d); }
      else { DP d; d.p = o.pos - V3{0, 1.2f, 0}; d.s = {1.3f, 1.2f, 1.3f}; d.tex = tConc; d.t = nxt ? V3{1, .9f, .4f} : V3{.4f, .4f, .4f}; d.emis = nxt ? 1.2f : 0.f; draw(mCylN, d); } } }
  drawWonder(p); for (int i = 0; i < (int)pois.size(); i++) drawPoi(p, pois[i], i);
  for (auto& n : npcs) if (!n.dead) drawNpc(n, p);
}

// ---------------------------------------------------------------- survival: hunger, regeneration, death, respawn at the last checkpoint
// Eating: picks what fits the hunger best (the biggest food that does not overflow, else the smallest one)
void eatFood() {
  if (difficulty == 0) { toast("EASY MODE\nNo hunger here - nothing to eat", 2.5f); return; }
  if (foods.empty()) { toast("No food\nPick wild food with {E} or buy dishes at a market", 2.4f); return; } if (food >= 19.5f) { toast("You are full", 1.5f); return; }
  float miss = 20.f - food; int best = -1, bv = 0, small = -1, sv = 99;
  for (auto& kv : foods) { if (kv.second <= 0) continue; int v = foodValue(kv.first); if (v <= miss + 1.f && v > bv) { bv = v; best = kv.first; } if (v < sv) { sv = v; small = kv.first; } }
  int id = best >= 0 ? best : small; if (id < 0) return; itemAdd(IT_FOOD + id, -1); food = std::min(20.f, food + foodValue(id)); starving = false; starveT = 0; hungryWarned = false; saveReq = true;
  audioEvent(S_CHIME); toast(fmt("MUNCH\n%s (+%d)\nFood %d/20   (%d food left)", foodName(id).c_str(), foodValue(id), (int)food, foodTotal()), 2.4f);
}
int hitDamage() { return difficulty == 0 ? 1 + std::rand() % 2 : difficulty == 1 ? 1 + std::rand() % 3 : 3 + std::rand() % 3; }
void hurtPlayer(int dmg, const std::string& who) {
  if (devGod) return;   // developer god mode
  if (invul > 0 || dmg <= 0 || hp <= 0) return; hp -= dmg; invul = difficulty == 2 ? 1.2f : 1.6f; regenT = 0; audioEvent(S_BEEP_HI); modalWho = who;
  toast(fmt("-%d %s\nLIVES %d/%d", dmg, dmg == 1 ? "LIFE" : "LIVES", std::max(0, hp), MAXHP) + (who == "a long fall" ? std::string("\nThat was a long fall") : "\nHit by " + who), 1.6f);
}
// Sword: a short arc in front of you. Wicked people have a few hit points; once defeated they are gone from this world for good.
void attack() {
  if (st != SURFACE || cine || modal || paused || curP < 0) return;
  if (swordSel < 0) return;   // no sword: the attack button simply does nothing
  if (swingT > 0) return; swingT = .42f; if (inRoom) return;
  Planet& p = planets[curP]; V3 me{P.x, 0, P.z}, fw{-std::sin(P.yaw), 0, -std::cos(P.yaw)}; int dmg = 1 + std::rand() % 2; bool hit = false;   // every hit takes 1 or 2 lives
  for (auto& n : npcs) { if (!n.wicked || n.dead) continue; V3 d = V3{n.pos.x, 0, n.pos.z} - me; float dl = len(d); if (dl > 3.1f || (dl > .7f && dot(d * (1 / dl), fw) < .4f)) continue;
    hit = true; n.hp -= dmg; n.hurt = .35f; n.angry = true; n.wait = .6f; V3 kb = dl > .01f ? d * (1 / dl) : fw; n.pos.x += kb.x * 1.4f; n.pos.z += kb.z * 1.4f; npcCollide(n, .45f);
    if (n.hp <= 0) { n.dead = true; if (n.widx >= 0 && n.widx < 8) p.vmask |= 1u << (5 + n.widx); int r = resOf(p, std::rand() % 2), nr = n.chief ? 4 : 2; giveRes(r, nr); audioEvent(S_DISCOVER); saveReq = true;
      toast(fmt("DEFEATED\n%s\n+%d %s", n.name.c_str(), nr, RES_NAME[r]), 3); }
    else { audioEvent(S_CLANG); toast(fmt("%s\n%d/%d", n.name.c_str(), std::max(0, n.hp), n.maxHp), .9f); }
    for (auto& o : npcs) if (o.wicked && !o.dead && len(o.pos - n.pos) < 25) o.angry = true;   // the others come running
    break; }
  if (!hit) audioEvent(S_JUMP);
}
// The tool in your hand, drawn in front of the camera only while it is used: the sword while swinging, the pickaxe while mining
void drawViewModel() {
  bool pk = mineT > 0 && !pickaxes.empty(); if (cine || (!pk && (swordSel < 0 || swingT <= 0))) return;   // like the pickaxe, the sword only shows while it is used
  float t = pk ? 1.f - mineT / .4f : swingT > 0 ? 1.f - swingT / .42f : 0.f, s = std::sin(t * PI), bob = std::sin(P.bob) * .015f;
  V3 base = camPos + cR * (.42f - s * .25f) + cU * (-.42f + bob + s * .05f) + cF * (.72f + s * .2f);
  V3 Yb = norm(cU * (.9f - 1.5f * s) + cF * (.35f + .9f * s) - cR * (.12f + .45f * s)), Xb = norm(cross(Yb, cF)), Zb = cross(Xb, Yb);
  auto D = [&](const Mesh& m, float along, V3 sz, V3 col, float em, GLuint tex = 0, float side = 0) { DP d; d.p = base + Yb * along + Xb * side; d.s = sz; d.X = Xb; d.Y = Yb; d.Z = Zb; d.t = col; d.emis = em; d.tex = tex ? tex : tWhite; draw(m, d); };
  GLuint mt = tMetal ? tMetal : tWhite, wd = tWood ? tWood : tWhite;
  if (pk) { int k = *pickaxes.begin(); for (int q : pickaxes) if (pickTier(q) > pickTier(k)) k = q; V3 c = pickColor(k);
    D(mCylN, -.15f, {.025f, .5f, .025f}, {.4f, .26f, .14f}, 0, wd); DP h; h.p = base + Yb * .33f; h.s = {.36f, .045f, .05f}; h.X = Zb * -1.f; h.Y = Xb; h.Z = Yb; h.t = c; h.emis = .15f; h.tex = mt; draw(mBox, h); return; }
  V3 c = swordColor(swordSel), g = lin(hsl((swordSel * 37 % 100) / 100.f, .7f, .45f)) * 1.3f; int sh = swordShape(swordSel);
  D(mCylN, -.14f, {.022f, .14f, .022f}, {.25f, .16f, .1f}, 0, wd); D(mSphereLo, -.15f, {.035f, .035f, .035f}, g, .4f); D(mBox, .0f, {.2f, .025f, .05f}, g, .3f, mt);   // grip, pommel, guard
  switch (sh) {
    case 0: D(mBox, .29f, {.045f, .56f, .012f}, c, .12f, mt); D(mCone, .57f, {.032f, .07f, .008f}, c, .12f, mt); break;                                   // longsword
    case 1: D(mBox, .23f, {.09f, .44f, .016f}, c, .1f, mt); D(mCone, .45f, {.065f, .1f, .01f}, c, .1f, mt); break;                                       // broad blade
    case 2: for (int k = 0; k < 4; k++) D(mBox, .08f + k * .13f, {.045f, .14f, .012f}, c, .12f, mt, k * k * .012f); break;                              // curved saber
    case 3: D(mCone, .02f, {.05f, .62f, .03f}, c, .9f + .3f * std::sin(tAcc * 3)); break;                                                                   // glowing crystal shard
    default: D(mBox, .27f, {.035f, .5f, .012f}, c, .12f, mt, -.025f); D(mBox, .27f, {.035f, .5f, .012f}, c, .12f, mt, .025f); break;                     // twin glaive
  }
}
// Dying: back in space in front of the last discovered world with its save. Before the first discovery there is no checkpoint, so you just wake up at the landing site.
void respawnAtCheckpoint() {
  if (respawnFromCheckpoint()) return;
  if (inRoom) exitRoom(); hp = MAXHP; food = std::max(food, 14.f); for (auto& n : npcs) n.angry = false; starving = false; starveT = 0; hungryWarned = false; invul = 3.f; regenT = 0;
  P.x = 7.f; P.z = 7.f; P.y = shAt(P.x, P.z); P.vx = P.vz = P.vy = 0; mdx = mdy = 0; flashA = .8f; toast("YOU WAKE UP\nBack at your landing site", 3.5f);
}
void updSurvival(float dt) {
  if (devGod) { food = 20; starving = false; starveT = 0; }
  if (hp > 0 && difficulty > 0) {
    foodT += dt; float per = difficulty == 2 ? 26.f : 40.f; while (foodT >= per) { foodT -= per; food = std::max(0.f, food - 1.f); }
    if (food <= 6.f && !hungryWarned && food > 0) { hungryWarned = true; toast("YOU ARE HUNGRY\nPick wild food or buy a dish at a market, then press {EAT}", 4); }
    if (food <= 0) { if (!starving) { starving = true; starveT = 0; toast(difficulty == 2 ? "STARVING!\nEat now or you will fall" : "STARVING!\nYou move slower until you eat", 4); }
      if (difficulty == 2) { starveT += dt; if (starveT >= 9.f) { starveT = 0; hp--; invul = std::max(invul, .5f); audioEvent(S_BEEP_HI); toast(fmt("STARVING\nLIVES %d/%d", std::max(0, hp), MAXHP), 1.5f); modalWho = "starvation"; } } } else { starving = false; starveT = 0; }
  } else if (difficulty == 0) { starving = false; food = 20; }
  // slow natural healing (not while starving, and only when fed on harder settings)
  if (hp > 0 && hp < MAXHP && invul <= 0 && (difficulty == 0 || food >= (difficulty == 2 ? 16.f : 12.f))) { regenT += dt; if (regenT >= (difficulty == 2 ? 45.f : 30.f)) { regenT = 0; hp++; toast(fmt("Recovered\nLIVES %d/%d", hp, MAXHP), 1.5f); } } else if (hp >= MAXHP) regenT = 0;
  if (hp <= 0 && modal && modal != M_DEATH) closeModal();   // hit while trading: the window closes
  if (hp <= 0 && !modal) { if (meds > 0) { meds--; hp = MAXHP; invul = 3; starveT = 0; audioEvent(S_DISCOVER); toast("MEDPACK USED\nYou are back on your feet", 3); } else { deaths++; commitCheckpoint(); saveReq = true; openModal(M_DEATH); } }
}
