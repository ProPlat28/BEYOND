// Procedural planets and star systems.
#include "planets.h"
#include <thread>

// ---------------------------------------------------------------- planets
static const char* LIFE[] = {"no life detected", "microbial mats", "alien flora", "strange fauna", "sentient civilization", "machine intelligence", "lost civilization", "hive mind"};
static const char* ATM[] = {"none", "thin", "breathable", "dense", "toxic", "luminous", "corrosive", "frozen fog", "charged", "superdense"};
static const char* SPECIAL[] = {
  "Gravity changes during the day", "Enormous floating organisms drift in the sky", "Mostly underground", "Plants glow when approached",
  "Oceans move unusually quickly", "Mountains slowly change position", "The atmosphere reacts to certain materials", "Civilization lives entirely beneath the surface",
  "Ancient structures hum in the dark", "The ground sings when you walk", "Twin-tone auroras never fade", "Time feels slower near the poles",
  "Rain falls upward in the valleys", "Crystal forests grow overnight", "The air tastes of metal and honey", "Shadows move against the light",
  "Rivers of light cut the dark", "Stone remembers every footprint", "Storms paint the sky in new colors", "The horizon bends in on itself",
  "Dust forms living shapes", "Echoes arrive before the sound", "Ice burns and fire freezes here", "The magnetic field speaks in color",
  "Whole continents drift like ships", "Night lasts a generation", "Seeds here grow into machines", "The ocean is a single organism",
  "Lightning writes in the clouds", "Bones of a dead moon litter the plains", "Every hill is a buried temple", "The wind carries lost languages"};
std::string cultureOf(int u) {
  static const char* CPL[] = {"Ka", "Ve", "Lu", "Tha", "Ori", "Zor", "Mi", "Dru", "Sy", "Bel", "Ner", "Qua", "Ix", "Pa", "Rho", "Tal"};
  static const char* CSF[] = {"ren", "lix", "mar", "vyn", "tek", "ria", "gon", "dos"};
  std::string c = std::string(CPL[((u % 16) + 16) % 16]) + CSF[(((u / 16) % 8) + 8) % 8] + "an"; c[0] = (char)std::toupper(c[0]); return c;
}
// ---------------------------------------------------------------- texture generators (PLANETGEN BEGIN)
// Planet seen from space: same band/palette identity as before, plus colour variation, shallow seas and beaches on watery kinds,
// polar ice caps, craters on airless/ash/shattered worlds and fine turbulence on gas giants. Alpha = 1 - specular mask (uMode 3).
static inline float fr2(float x) { return x - std::floor(x); }
struct PTex { int kind, pat, ps; float psc, lvl, cap; bool water, craters; C4 c[5]; float cvt[40 * 21]; };   // cvt: lattice of the colour-variation noise (no hashing per pixel)
static PTex planetTexInfo(const Planet& p, float lvl, bool noAtm, float psc) {
  PTex q; q.kind = p.kind; q.pat = p.texPat; q.ps = p.seed; q.psc = psc; q.lvl = lvl; for (int i = 0; i < 5; i++) q.c[i] = p.pal[i];
  int k = p.kind, u = p.uniq; q.water = k == 0 || k == 1 || k == 7 || k == 10 || k == 13;
  q.craters = noAtm || k == 12 || k == 14 || (k == 2 && u % 4 == 0) || (k == 9 && u % 3 == 0);
  for (int y = 0; y <= 20; y++) for (int x = 0; x < 40; x++) q.cvt[y * 40 + x] = Hh(x, y, p.seed + 71);
  q.cap = k == 3 ? .62f + (u / 16) * .02f : k == 0 ? .84f : k == 1 ? .86f : k == 7 ? .91f : k == 10 ? .89f : k == 15 ? .86f : k == 5 ? .88f : 2.f;   // latitude where the ice cap starts (2 = none)
  return q;
}
static C4 planetTexel(const PTex& q, float uu, float vv) {
  const C4* c = q.c; float u2 = uu + .07f * std::sin(vv * (3 + q.ps % 6) * 6.283f), v2 = vv + .04f * std::sin(uu * (4 + q.ps % 5) * 6.283f);
  float h = pfeat(q.pat, u2, v2, q.ps, q.psc), lvl = q.lvl, spec = 0; C4 col;
  if (h < lvl) {
    float dp = h / std::max(lvl, .01f); col = mixc(c[0], c[1], dp);
    if (q.water) { spec = 1; col = mixc(col, mixc(c[1], c[3], .3f), sstep(.8f, 1.f, dp) * .5f); }          // shallow shelves
  } else {
    float t = (h - lvl) / std::max(1.f - lvl, .01f); col = t < .5f ? mixc(c[2], c[3], t * 2) : mixc(c[3], c[4], (t - .5f) * 2);
    if (q.water && t < .05f) col = mixc(col, mixc(c[3], C4{214, 196, 150, 255}, .45f), (1 - t / .05f) * .55f);   // beaches
  }
  float cx = uu * 40, cy = std::min(vv * 20, 19.999f); int ix = std::min((int)cx, 39), iy = (int)cy; float fx = cx - ix, fy = cy - iy; fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
  const float* T = q.cvt + iy * 40; int ix1 = (ix + 1) % 40;
  float cv = (T[ix] * (1 - fx) + T[ix1] * fx) * (1 - fy) + (T[40 + ix] * (1 - fx) + T[40 + ix1] * fx) * fy, k = .9f + cv * .2f;
  if (q.kind == 6) k *= .95f + .05f * std::sin(vv * 140 + cv * 6);                                           // fine cloud-band turbulence
  if (q.craters) for (int L = 0; L < 2; L++) {   // one crater per grid cell (cheap: no neighbour search), two sizes
    int nx = L ? 44 : 16, ny = nx / 2, s = q.ps + 5 + L * 7; float x = uu * nx, y = vv * ny; int ix = (int)x % nx, iy = (int)y;
    if (Hh(ix, iy, s) > (L ? .5f : .65f)) continue;
    float r = .14f + .2f * Hh(ix, iy, s + 1), dx = x - ix - (.5f + (Hh(ix, iy, s + 2) - .5f) * .3f), dy = y - iy - (.5f + (Hh(ix, iy, s + 3) - .5f) * .3f), d = std::sqrt(dx * dx + dy * dy) / r;
    if (d < 1) k *= .84f + .1f * d + dx / r * .08f; else k *= 1.f + .14f * std::max(0.f, 1 - std::fabs(d - 1.07f) / .18f);   // bowl lit on one side, bright rim
  }
  col = {col[0] * k, col[1] * k, col[2] * k, 255};
  float lat = std::fabs(vv - .5f) * 2;
  if (lat > q.cap - .08f) {
    float e = lat + (vnoise(uu * 24, vv * 24, q.ps + 9, 24) - .5f) * .14f, ci = sstep(q.cap, q.cap + .03f, e);
    C4 ice = mixc(C4{236, 242, 250, 255}, c[4], .25f); float ik = .9f + cv * .12f; col = mixc(col, {ice[0] * ik, ice[1] * ik, ice[2] * ik, 255}, ci * .92f); spec = spec * (1 - ci) + ci * .3f;
  }
  if (q.kind == 3) spec = std::max(spec, .25f); else if (q.kind == 9) spec = std::max(spec, .4f); else if (q.kind == 8) spec = std::max(spec, .3f);
  col[3] = 255 * (1 - spec); return col;
}
// Planet ground texture for walking on the surface (1024^2, tiles). Colour identity = the old enterPlanet() texture: lowC/hiC from
// p.pal, pattern pfeat(p.texPat, ...), seed p.seed; then soil noise, pebbles and gravel, crack networks and per-kind touches.
std::vector<C4> surfaceTexels(const Planet& p, int W) {
  C4 lowC = p.kind == 3 ? p.pal[3] : p.kind == 1 ? p.pal[1] : p.kind == 6 ? p.pal[1] : p.pal[2], hiC = p.kind == 3 ? p.pal[4] : p.kind == 1 ? p.pal[2] : p.kind == 6 ? p.pal[3] : p.pal[3];
  const int sd = p.seed, spat = p.texPat, K = p.kind, G = 512; const float sc = 6.f + (sd % 5), B = .2f;
  // pfeat on a G x G grid, cross-faded over the last 20% of each axis so the result tiles (pfeat itself is not periodic)
  std::vector<float> g((size_t)G * G);
  for (int y = 0; y < G; y++) for (int x = 0; x < G; x++) {
    float u = (float)x / G, v = (float)y / G, wu = sstep(1 - B, 1, u), wv = sstep(1 - B, 1, v), r = pfeat(spat, u, v, sd, sc) * (1 - wu) * (1 - wv);
    if (wu > 0) r += pfeat(spat, u - 1, v, sd, sc) * wu * (1 - wv);
    if (wv > 0) r += pfeat(spat, u, v - 1, sd, sc) * (1 - wu) * wv;
    if (wu > 0 && wv > 0) r += pfeat(spat, u - 1, v - 1, sd, sc) * wu * wv;
    g[(size_t)y * G + x] = r;
  }
  auto gs = [&](float u, float v) { float x = u * G, y = v * G; int x0 = (int)std::floor(x), y0 = (int)std::floor(y); float fx = x - x0, fy = y - y0;
    auto at = [&](int i, int j) { return g[(size_t)(((j % G) + G) % G) * G + (((i % G) + G) % G)]; };
    return (at(x0, y0) * (1 - fx) + at(x0 + 1, y0) * fx) * (1 - fy) + (at(x0, y0 + 1) * (1 - fx) + at(x0 + 1, y0 + 1) * fx) * fy; };
  float crackAmt = K == 12 ? 1.f : K == 4 ? .9f : K == 14 ? .8f : K == 2 ? .7f : K == 3 ? .6f : K == 9 ? .5f : K == 11 ? .5f : .3f;
  float pebAmt = K == 3 ? .25f : K == 2 ? .3f : K == 1 ? .45f : K == 10 ? .3f : K == 6 ? .4f : .65f;
  std::vector<C4> out((size_t)W * W);
  // which worlds are alive enough to be covered in blades (temperate, ocean shores, anomalous, jungle, swamp, bioluminescent, storm)
  float veg = K == 0 || K == 7 || K == 10 ? 1.f : K == 13 || K == 5 ? .85f : K == 15 ? .7f : K == 1 ? .5f : 0.f; bool glowTips = K == 13 || K == 5;
  // blades are shaded like Earth's grass (same lightness range), only the hue and saturation come from the planet
  auto hueSat = [](C4 c, float& h, float& s) { float r = c[0] / 255, g = c[1] / 255, b = c[2] / 255, mx = std::max({r, g, b}), mn = std::min({r, g, b}), d = mx - mn, l = (mx + mn) * .5f;
    s = d < 1e-4f ? 0 : d / (1 - std::fabs(2 * l - 1) + 1e-4f); if (d < 1e-4f) { h = 0; return; }
    h = mx == r ? std::fmod((g - b) / d, 6.f) : mx == g ? (b - r) / d + 2 : (r - g) / d + 4; h /= 6; if (h < 0) h += 1; };
  float bH, bS, bH2, bS2; hueSat(p.pal[3], bH, bS); hueSat(p.pal[2], bH2, bS2); bS = clampf(std::max(bS, bS2) * .8f, .25f, .62f);
  float grainAmt = veg > 0 ? 0.f : .32f;   // barren ground gets fine grit instead of blades
  auto texel = [&](int x, int y, double& sumNew, double& sumBase) {
    float u = (x + .5f) / W, v = (y + .5f) / W, n = gs(u, v);
    C4 c = mixc(lowC, hiC, clampf(n * 1.6f - .2f, 0, 1)); float base = c[0] * .3f + c[1] * .59f + c[2] * .11f;
    float macro = fbmP(u * 4, v * 4, sd + 5, 3, 4, 4), soil = fbmP(u * 24, v * 24, sd + 3, 3, 24, 24), sp = Hh(x & (W - 1), y & (W - 1), sd + 9);
    float mid = fbmP(u * 10, v * 10, sd + 4, 2, 10, 10), k = (.76f + soil * .5f) * (.84f + macro * .32f) * (.9f + mid * .2f) + (sp - .5f) * .14f;
    C4 other = mixc(lowC, hiC, clampf(1.f - n * 1.2f, 0, 1)); c = mixc(c, other, sstep(.55f, .75f, macro) * .25f);   // patches of the other palette colour
    // per-kind surface character
    if (K == 2 || K == 11) k *= 1 + .06f * std::sin((v * 14 + u * 3 + fbmP(u * 6, v * 6, sd + 13, 3, 6, 6) * 4) * 6.2832f);       // wind ripples
    if (K == 1) k *= 1 - .18f * sstep(.55f, .7f, fbmP(u * 6, v * 6, sd + 14, 4, 6, 6));                                            // wet sand
    if (K == 10 || K == 7) { float wet = sstep(.58f, .68f, fbmP(u * 5, v * 5, sd + 15, 4, 5, 5)); k *= 1 - .3f * wet; }             // dark mud / leaf litter
    if (K == 14) { if (sp > .96f) k += .35f; k *= .92f; }                                                                         // pale ash flecks
    if (K == 3 && sp > .992f) k += .6f;                                                                                           // frost sparkle
    if (K == 9) k *= .92f + .16f * vnoiseP(u * 6, v * 60, sd + 16, 6, 60);                                                      // metallic streaks
    // pebbles and stones (irregular outlines, matte), denser in some areas, with a contact shadow ring
    Vor pb = voroP(u * 32, v * 32, sd + 7, 32, 32, .85f); float dens = pebAmt * (.3f + macro);
    if (pb.id < dens * .6f && pb.f1 < .5f) {
      float ang = std::atan2(pb.dy, pb.dx), pr = (.1f + .26f * fr2(pb.id * 5.7f)) * (1 + .1f * std::sin(ang * 2 + pb.id * 40) + .06f * std::sin(ang * 3 + pb.id * 17));
      if (pb.f1 < pr) { float nz = std::sqrt(1 - (pb.f1 / pr) * (pb.f1 / pr)); C4 pc = mixc(lowC, hiC, fr2(pb.id * 13.1f));
        c = mixc(c, pc, .5f); k = (.8f + .18f * nz + (pb.dx + pb.dy) / pr * .1f) * (.82f + fr2(pb.id * 3.3f) * .3f) * (.9f + soil * .2f); }
      else if (pb.f1 < pr * 1.35f) k *= .75f + .25f * (pb.f1 - pr) / (pr * .35f);
    }
    // gravel
    Vor gv = voroP(u * 110, v * 110, sd + 8, 110, 110); if (gv.id < pebAmt * .6f && gv.f1 < .3f) k *= .8f + gv.id * .6f - gv.dy * .5f;
    // crack network (dry mud, frozen plates, lava seams...) masked into regions; warped by the periodic soil noise so edges wobble but still tile
    if (crackAmt > 0) {
      Vor cr = voroP(u * 9 + (soil - .5f) * .7f, v * 9 + (mid - .5f) * .7f, sd + 11, 9, 9, .9f); float w = .025f + .035f * crackAmt, cm = sstep(.62f - crackAmt * .25f, .7f - crackAmt * .2f, fbmP(u * 3, v * 3, sd + 12, 3, 3, 3));
      float ck = (1 - sstep(0, w, cr.f2 - cr.f1)) * cm;
      if (ck > 0) {
        if (K == 4) c = mixc(c, C4{255, 120, 30, 255}, ck * .85f), k = lerpf(k, 1.25f, ck);           // glowing lava seams
        else if (K == 3) c = mixc(c, C4{200, 225, 245, 255}, ck * .5f), k *= 1 + ck * .12f;          // pale blue ice fractures
        else if (K == 8) k *= 1 + ck * .45f;                                                         // bright crystal edges
        else k *= 1 - ck * .35f;
      }
    }
    if (K == 8) { Vor fc = voroP(u * 14, v * 14, sd + 17, 14, 14); k *= .82f + .36f * fc.id + (fc.dx - fc.dy) * .15f; }      // crystal facets
    if (K == 13 && sp > .985f) { c = mixc(c, hiC, .5f); k = 1.6f; }                                                           // glowing spores
    if (grainAmt > 0) { float gr = fbmP(u * 48, v * 48, sd + 23, 3, 48, 48); k *= 1 - grainAmt * .5f + gr * grainAmt; }   // fine grit like Earth's soil
    if (veg > 0) {   // living worlds: a carpet of alien grass / moss in the planet's own colours, with bare patches
      float bare = sstep(.58f, .72f, fbmP(u * 5, v * 5, sd + 21, 4, 5, 5) + (macro - .5f) * .25f), tip = 0, hue = 0, lit = 1;
      int layer = grassBlades(u, v, sd * 7 + 1, veg * (1 - bare * .95f), tip, hue, lit);
      if (layer >= 0) { float dh = std::fabs(bH2 - bH) < .5f ? bH2 - bH : 0.f, h = bH + dh * clampf(macro * 1.4f - .2f, 0, 1) + (hue - .5f) * .04f;   // patches drift toward the second colour
        C4 bc = hsl(h - std::floor(h), bS - hue * .06f, .3f + macro * .09f + tip * .14f); float depth = layer == 0 ? 1.08f : layer == 1 ? .9f : .74f;
        c = bc; k = depth * lit * (.94f + sp * .12f); if (glowTips && tip > .85f && hue > .75f) k *= 1.7f; }
      else if (bare < .5f) k *= .62f; }   // soil seen between blades is in shadow
    C4 o{c[0] * k, c[1] * k, c[2] * k, 255}; out[(size_t)y * W + x] = o;
    sumNew += o[0] * .3f + o[1] * .59f + o[2] * .11f; sumBase += base;
  };
  // rows are independent: split them over a few threads (pure function, deterministic result)
  int nt = (int)std::clamp(std::thread::hardware_concurrency(), 1u, 8u); std::vector<double> sn(nt, 0), sb(nt, 0); std::vector<std::thread> th;
  for (int t = 0; t < nt; t++) th.emplace_back([&, t] { for (int y = t; y < W; y += nt) for (int x = 0; x < W; x++) texel(x, y, sn[t], sb[t]); });
  for (auto& t : th) t.join();
  double sumNew = 0, sumBase = 0; for (int t = 0; t < nt; t++) { sumNew += sn[t]; sumBase += sb[t]; }
  // keep the average brightness the old texture had (its multiplier averaged ~1.06), so lighting and fog still match
  float f = clampf((float)(sumBase * 1.06 / std::max(sumNew, 1.0)), .75f, 1.3f);
  for (auto& o : out) { o[0] = clampf(o[0] * f, 0, 255); o[1] = clampf(o[1] * f, 0, 255); o[2] = clampf(o[2] * f, 0, 255); }
  return out;
}
// ---------------------------------------------------------------- (PLANETGEN END)
GLuint makeSurfaceTex(const Planet& p) {
  const int W = 1024; std::vector<C4> px = surfaceTexels(p, W);
  return mkTex(W, W, [&](float u, float v) -> C4 { return px[(size_t)std::min(W - 1, (int)(v * W)) * W + std::min(W - 1, (int)(u * W))]; });
}
struct PO { int kind = -1, uniq = -1; float sat = -1, lvl = -1, hL = -1, hH = -1; bool found = false, noatm = false, clouds = false, noenter = false; int moons = -1, res = 704; const char* name = nullptr; };
static Planet mkPlanet(int seed, float R0, const PO& o) {
  Rng r{(unsigned)seed * 7919u + 13u};
  Planet p; int u = o.uniq >= 0 ? o.uniq : (seed % 128); p.uniq = u; p.seed = seed; p.enterable = !o.noenter;
  p.kind = o.kind >= 0 ? o.kind : (u % 16); int kind = p.kind;
  static const float KH[16] = {.33f, .58f, .08f, .55f, .02f, .78f, .12f, .38f, .72f, .0f, .28f, .48f, .05f, .62f, .04f, .66f};
  float sat = o.sat >= 0 ? o.sat : .22f + (u % 11) * .07f, lvl = o.lvl >= 0 ? o.lvl : .12f + (u % 9) * .07f;
  float hL = KH[kind] + (u / 16) * .047f, hH = hL + .12f + (u % 5) * .05f; bool lo = false;
  int slot = u / 16;
  p.texPat = (slot * 5 + kind * 2) % 16; p.terr = (slot * 3 + kind * 7) % 16; p.floraT = slot % 8; p.archT = (slot * 3 + kind) % 8; p.cloudSt = (slot * 5 + kind * 2) % 8;
  p.lifeI = kind == 6 || kind == 14 || kind == 9 ? (slot % 2) : 2 + (slot % 6); p.atmI = 0; p.species = (slot + kind) % 4; p.itemI = (slot + kind * 2) % 5;
  p.nItems = 4 + (u % 4); p.nShrines = 3 + (u % 3); p.layout = u % 16;
  p.hideVil = p.layout == 2 || p.layout == 4 || p.layout == 5 || p.layout == 7 || p.layout == 13 || kind == 2;
  p.hideCamp = p.layout == 3 || p.layout == 8 || p.layout == 11 || p.layout == 12;
  p.squash = kind == 6 ? .62f + slot * .05f : (kind == 12 ? .78f + slot * .025f : .86f + slot * .018f);
  p.spinRate = .005f + slot * .004f + kind * .0007f; p.spin = u * .47f;
  p.glow = (kind == 4 || kind == 13) ? .25f + slot * .08f : (kind == 5 || kind == 15 || kind == 8 ? .08f + slot * .04f : (slot == 7 ? .16f : 0));
  p.ringScale = 1.7f + slot * .18f + (kind % 5) * .05f; p.R = kind == 6 ? R0 * (1.65f + slot * .12f) : R0;
  if (kind == 1) lvl = .68f + slot * .03f; if (kind == 2) { hL = .06f + slot * .025f; hH = hL + .08f; lvl = .07f + slot * .02f; sat = .32f + slot * .06f; }
  if (kind == 3) { sat = .16f + slot * .04f; lvl = .18f + slot * .03f; } if (kind == 4) { lo = true; lvl = .38f + slot * .04f; }
  if (kind == 5) { hL = slot * .11f + kind * .02f; hH = hL + .18f + slot * .06f; } if (kind == 6) { hL = slot * .08f; hH = hL + .08f + slot * .04f; p.texPat = slot % 2 ? 1 : 7; }
  if (kind == 7) { hL = .26f + slot * .035f; lvl = .3f + slot * .03f; } if (kind == 8) { hL = .58f + slot * .04f; sat = .65f + slot * .03f; lo = true; }
  if (kind == 9) { sat = .06f + slot * .03f; hL = slot * .03f; } if (kind == 10) { hL = .2f + slot * .03f; lvl = .48f + slot * .03f; }
  if (kind == 11) { p.texPat = 11; lvl = .25f + slot * .05f; } if (kind == 12) { p.texPat = (slot % 2) ? 9 : 2; }
  if (kind == 13) { lo = true; sat = .75f + slot * .03f; } if (kind == 14) { sat = .04f + slot * .02f; lvl = .12f + slot * .02f; }
  if (kind == 15) { p.texPat = (slot % 2) ? 3 : 14; }
  if (o.name) { p.texPat = 0; p.terr = 0; p.squash = 1; p.glow = 0; p.spin = 0; p.spinRate = .02f; }
  if (o.hL >= 0) { hL = o.hL; hH = o.hH; }
  bool des = kind == 2;
  C4 c[5] = {hsl(hL, sat, lo ? .4f : .08f + (u % 5) * .02f), hsl(hL + .03f, sat, lo ? .55f : .22f + (u % 4) * .03f),
    hsl(hH, sat * .8f, des ? .5f : .26f + (u % 5) * .03f), hsl(hH + .05f, sat, des ? .65f : .4f + (u % 4) * .04f), hsl(hH + .1f, .15f + (u % 4) * .05f, lo ? .12f : .82f + (u % 5) * .03f)};
  for (int i = 0; i < 5; i++) p.pal[i] = c[i];
  float psc = 3.2f + (u % 10) * .7f;
  { PTex q = planetTexInfo(p, lvl, o.noatm, psc); p.tex = mkTex(o.res, o.res / 2, [&](float uu, float vv) -> C4 { return planetTexel(q, uu, vv); }); }
  C4 ac = hsl(hL + .08f + (u % 7) * .03f, .55f + (u % 5) * .08f, .45f + (u % 6) * .05f);
  p.atm = {std::pow(ac[0] / 255, 2.2f), std::pow(ac[1] / 255, 2.2f), std::pow(ac[2] / 255, 2.2f)};
  p.hasAtm = !o.noatm && kind != 14 && !(kind == 2 && u % 4 == 0) && !(kind == 9 && u % 3 == 0) && !(kind == 3 && u % 5 == 0);
  p.atmI = !p.hasAtm ? 0 : (kind == 6 ? 9 : 1 + (u % 9));
  C4 ct = hsl(hL + .18f, .25f + (u % 6) * .08f, .78f + (u % 4) * .04f); p.cloudTint = {ct[0] / 255.f, ct[1] / 255.f, ct[2] / 255.f};
  p.cloud = o.clouds || (p.hasAtm && kind != 2 && kind != 9 && kind != 14 && (kind <= 1 || kind == 7 || kind == 10 || kind == 11 || kind == 13 || kind == 15 || slot % 2 == 1 || u % 5 == 2));
  if (p.cloud) {
    int cs = p.cloudSt; int cseed = seed + 3;
    p.cloudTex = mkTex(512, 256, [&](float uu, float vv) -> C4 {
      float a = 0;
      if (cs == 0) a = clampf((fbm(uu * 8, vv * 4, cseed, 5, 8) - .4f) * 3, 0, 1);
      else if (cs == 1) a = clampf((fbm(uu * 18, vv * 2, cseed, 4, 8) - .48f) * 3.4f, 0, 1);
      else if (cs == 2) a = clampf(1.f - cells(uu * 6, vv * 3, cseed) * 2.1f, 0, 1);
      else if (cs == 3) a = clampf((fbm(uu * 4, vv * 8, cseed, 5, 8) - .42f) * 2.6f, 0, 1);
      else if (cs == 4) a = clampf(.5f + .5f * std::sin(vv * 14 + fbm(uu * 6, vv * 3, cseed, 3) * 5) * fbm(uu * 5, vv * 5, cseed + 1, 3), 0, 1);
      else if (cs == 5) a = clampf((ridge(uu * 7, vv * 3.5f, cseed) - .35f) * 2.4f, 0, 1);
      else if (cs == 6) a = clampf(1.f - cells(uu * 10, vv * 5, cseed) * 3.2f, 0, 1) * clampf(fbm(uu * 3, vv * 2, cseed, 3), 0, 1);
      else a = clampf((fbm(uu * 12, vv * 6, cseed, 6, 8) - .38f) * 2.2f, 0, 1);
      if (a > 0) a = clampf(a * (.72f + .56f * vnoise(uu * 48, vv * 24, cseed + 40, 48)), 0, 1);   // wispy fine detail inside the cloud masses
      float sh = .86f + .14f * a;                                                                   // thicker cores slightly brighter than thin edges
      return {ct[0] * sh, ct[1] * sh, ct[2] * sh, a * (140.f + (cs % 4) * 28.f)}; });
  }
  p.ring = (kind == 6 && slot % 2 == 0) || kind == 8 || kind == 12 || slot == 3 || slot == 6 || (u % 7 == 0);
  if (p.ring) {
    p.tilt = ((u % 17) - 8) * .07f; C4 rc = mixc(c[2], c[4], .4f + (u % 5) * .1f); int rsd = seed; float rf = 40.f + (u % 11) * 8.f;
    p.ringTex = mkTex(1024, 2, [&](float uu, float) -> C4 {
      float s = .5f + .5f * std::sin(uu * rf + fbm(uu * 20, 0, rsd, 3, 20) * 9), k = fbm(uu * (22 + rsd % 12), .5f, rsd + 1, 3, 30);
      float fine = .72f + .28f * std::sin(uu * rf * 6.3f + fbm(uu * 60, 0, rsd + 2, 2, 60) * 6) * std::sin(uu * rf * 2.1f);   // ringlets
      float gap = std::fabs(std::fmod(uu * (3 + u % 5), 1.f) - .5f) < .04f + (u % 3) * .02f ? 0.f : 1.f;
      for (int g = 0; g < 3; g++) if (std::fabs(uu - (.2f + .25f * g + Hh(g, rsd, 7) * .12f)) < .004f + Hh(g, rsd, 8) * .006f) gap *= .15f;   // a few sharp narrow gaps
      float edge = sstep(.0f, .04f, uu) * sstep(1.f, .96f, uu), cv = fbm(uu * 9, .3f, rsd + 3, 3, 9); C4 cc = mixc(rc, c[2], cv * .4f);
      float l = s * (.85f + .3f * cv); return {cc[0] * l, cc[1] * l, cc[2] * l, k * 380 * s * gap * fine * edge}; }, false);
  }
  int nm = o.moons >= 0 ? o.moons : (u % 6); for (int i = 0; i < nm; i++) {
    C4 mt = hsl(hL + i * .14f, .2f + (u % 4) * .1f, .45f + (i % 3) * .12f);
    p.moons.push_back({p.R * (2.3f + i * (1.0f + (u % 4) * .15f) + r.f() * .4f), r.r(0, 6.28f), .015f + (u % 7) * .008f + i * .01f, p.R * (.05f + ((u + i * 3) % 8) * .018f), {mt[0] / 255.f, mt[1] / 255.f, mt[2] / 255.f}});
  }
  static const char* PRE[] = {"Ae", "Bel", "Cor", "Dra", "Esh", "Fal", "Gor", "Hel", "Ix", "Jor", "Kel", "Lum", "Myr", "Nox", "Ori", "Pyr", "Quel", "Ryn", "Sol", "Tor", "Umb", "Vex", "Wyn", "Xan", "Yl", "Zar", "Ash", "Bri", "Cyn", "Del", "Fen", "Gla"};
  static const char* SUF[] = {"th", "ron", "lis", "nex", "mar", "vos", "quor", "dyn", "skel", "tau", "hex", "void", "reach", "well", "spire", "mere"};
  if (o.name) p.name = o.name; else p.name = std::string(PRE[u % 32]) + SUF[u / 32] + " " + std::to_string(u + 1);
  p.culture = cultureOf(u);
  p.grav = .16f + u * .0195f + kind * .001f; p.wicked = .08f + (u % 17) * .05f; p.special = SPECIAL[u % 32];
  const char* soc = p.wicked < .35f ? "mostly kind" : p.wicked < .65f ? "kind and wicked, mixed" : "mostly wicked";
  p.txt = fmt("%s  |  gravity %.2f g  |  atmosphere %s  |  %s\nHome of the %s people (%s)\n%s", KL[kind], p.grav, kind == 6 ? "crushing" : ATM[p.atmI], LIFE[p.lifeI], p.culture.c_str(), soc, p.special.c_str());
  p.found = o.found; return p;
}
static Planet mkMoon(const Planet& par, int pidx, int mi) {
  const Moon& m = par.moons[mi]; static const int MK[5] = {2, 3, 9, 12, 14}; static const char* ROM[] = {"I", "II", "III", "IV", "V", "VI"};
  PO o; o.uniq = (par.uniq * 5 + mi * 29 + 11) % 128; o.kind = MK[(par.uniq + mi * 3) % 5]; o.noatm = true; o.moons = 0; o.res = 96;
  Planet q = mkPlanet(100000000 + par.seed * 8 + mi, std::max(34.f, m.r), o);
  q.isMoon = true; q.parent = pidx; q.orbD = m.d; q.orbA = m.a; q.orbS = m.s; q.enterable = true; q.hideVil = false; q.hideCamp = true; q.nItems = 3; q.nShrines = 3;
  if (q.ring) { glDeleteTextures(1, &q.ringTex); q.ringTex = 0; q.ring = false; }
  q.name = par.name + " " + ROM[mi % 6]; q.grav = .12f + (o.uniq % 5) * .03f; q.wicked = 0;
  q.pos = par.pos + V3{std::cos(m.a) * m.d, std::sin(m.a) * m.d * .12f, std::sin(m.a) * m.d};
  q.txt = fmt("Moon of %s  |  gravity %.2f g  |  no atmosphere\nA quiet outpost of the %s people\n%s", par.name.c_str(), q.grav, q.culture.c_str(), q.special.c_str());
  return q;
}
int discHere() { int n = 0; for (auto& p : planets) if (p.enterable && !p.isMoon && p.found) n++; return n; }
int totalHere() { int n = 0; for (auto& p : planets) if (p.enterable && !p.isMoon) n++; return n; }
int moonsDiscHere() { int n = 0; for (auto& p : planets) if (p.isMoon && p.found) n++; return n; }
int moonsTotalHere() { int n = 0; for (auto& p : planets) if (p.isMoon) n++; return n; }
// ---------------------------------------------------------------- star systems: every system has its own name, star class and (sometimes) a second sun
SysInfo sysInfo(int id) {
  SysInfo s; Rng r{(unsigned)id * 2654435761u + worldSeed * 40503u + 77u}; r.f(); r.f();
  static const char* A[] = {"Sol", "Ys", "Ka", "Vor", "Ael", "Thy", "Nym", "Or", "Zeph", "Cael", "Dra", "Eri", "Lyr", "Hy", "Mal", "Tau"};
  static const char* B[] = {"ma", "re", "ni", "lo", "xa", "ve", "dor", "li", "ta", "so", "ri", "ku"};
  static const char* C[] = {"th", "ra", "nis", "gal", "os", "ia", "ex", "ul"};
  float q = r.f(); int cls = id == 0 ? 4 : q < .04f ? 0 : q < .11f ? 1 : q < .22f ? 2 : q < .38f ? 3 : q < .62f ? 4 : q < .84f ? 5 : 6; s.cls = cls;
  static const V3 SC[7] = {{.62f, .74f, 1}, {.72f, .82f, 1}, {.92f, .95f, 1}, {1, .97f, .84f}, {1, .92f, .76f}, {1, .78f, .5f}, {1, .56f, .4f}};
  static const float SZ[7] = {1.5f, 1.3f, 1.15f, 1.05f, 1, .9f, .75f};
  s.col = SC[cls]; s.size = SZ[cls]; s.binary = id != 0 && r.f() < .28f; int c2 = std::min(6, cls + 1 + r.i(3)); s.col2 = SC[c2];
  s.name = id == 0 ? "Sol" : std::string(A[1 + r.i(15)]) + B[r.i(12)] + C[r.i(8)]; s.name += " System"; return s;
}
void freePlanets() { for (auto& p : planets) { glDeleteTextures(1, &p.tex); if (p.cloudTex) glDeleteTextures(1, &p.cloudTex); if (p.ringTex) glDeleteTextures(1, &p.ringTex); } planets.clear(); }
void buildSystem(int id) {
  freePlanets(); Rng r{((unsigned)id + worldSeed * 7919u) * 131u + 7u};
  sunDir = norm({r.f() - .5f, r.f() * .6f - .2f, r.f() - .5f}); if (id == 0) sunDir = norm({.5f, .45f, .35f});
  (void)r.f(); SysInfo si = sysInfo(id); V3 sc = id ? si.col : V3{1, .926f, .805f}; float lum = .85f + si.size * .3f; sunCol = {std::pow(sc.x, 2.2f) * 2.2f * lum, std::pow(sc.y, 2.2f) * 2.2f * lum, std::pow(sc.z, 2.2f) * 2.2f * lum};
  auto add = [&](Planet p, float x, float y, float z) { p.pos = {x, y, z}; planets.push_back(std::move(p)); };
  if (id == 0) { PO e; e.kind = 0; e.hL = .58f; e.hH = .3f; e.lvl = .52f; e.found = true; e.name = "Earth"; e.moons = 0; e.clouds = true; e.noenter = true; e.res = 1024; add(mkPlanet(1, 4000, e), 0, -4400, 0);
    PO m; m.kind = 2; m.sat = .03f; m.hL = .1f; m.hH = .1f; m.found = true; m.name = "Moon"; m.moons = 0; m.noatm = true; m.noenter = true; add(mkPlanet(2, 650, m), 7000, 900, -6000); }
  int n = 128;
  for (int i = 0; i < n; i++) {
    if (buildProgress) buildProgress((float)i / n);
    PO po; po.uniq = (i + (int)(worldSeed % 128)) % 128; po.kind = po.uniq % 16;
    float R0 = 380.f + (i % 19) * 72.f + ((i * 7) % 11) * 28.f;
    Planet p = mkPlanet(id * 1000 + i * 17 + 10 + (int)worldSeed * 20011, R0, po); V3 pos;
    float a = (i * 2.39996f) + r.f() * .08f, d = 6500.f + i * 2550.f + ((i * 13) % 17) * 90.f;
    pos = {std::cos(a) * d, ((i % 15) - 7) * 380.f, std::sin(a) * d};
    for (int tries = 0; tries < 14; tries++) { bool ok = true; for (auto& q : planets) if (!q.isMoon && len(q.pos - pos) < p.R + q.R + 2500) ok = false; if (ok) break;
      a += .37f; d += 900.f; pos = {std::cos(a) * d, pos.y + ((tries % 2) ? 400.f : -400.f), std::sin(a) * d}; }
    add(std::move(p), pos.x, pos.y, pos.z);
    int pi = (int)planets.size() - 1; std::vector<Planet> ms; for (int mi = 0; mi < (int)planets[pi].moons.size(); mi++) ms.push_back(mkMoon(planets[pi], pi, mi));
    for (auto& m : ms) planets.push_back(std::move(m));
  }
  buildSpaceRocks(id);
  applyPSaves();
}
