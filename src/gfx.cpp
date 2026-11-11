// Window, OpenGL drawing helpers, textures, meshes, shaders, camera, stars and HUD text.
#include "gfx.h"

GLuint mkTex(int w, int h, const std::function<C4(float, float)>& fn, bool rep) {
  std::vector<unsigned char> d((size_t)w * h * 4);
  for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) { C4 c = fn((x + .5f) / w, (y + .5f) / h); for (int k = 0; k < 4; k++) d[((size_t)y * w + x) * 4 + k] = (unsigned char)clampf(c[k], 0, 255); }
  GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, d.data()); glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, rep ? GL_REPEAT : GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, rep ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.f);
  return t;
}
// ---------------------------------------------------------------- procedural textures (TEXGEN BEGIN)
// Every generator tiles seamlessly (periodic noise only) and returns sRGB 0..255.
static inline float fr(float x) { return x - std::floor(x); }
static inline C4 grey(float k) { return {k, k, k, 255}; }
static inline C4 scl(C4 c, float k) { return {c[0] * k, c[1] * k, c[2] * k, c[3]}; }
// distance from (x,y) to the segment a->b, t = position along it (0..1)
static inline float segD(float x, float y, float ax, float ay, float bx, float by, float& t) {
  float vx = bx - ax, vy = by - ay, wx = x - ax, wy = y - ay; t = clampf((wx * vx + wy * vy) / (vx * vx + vy * vy + 1e-6f), 0, 1);
  float dx = wx - vx * t, dy = wy - vy * t; return std::sqrt(dx * dx + dy * dy);
}
// one layer of grass blades on an N x N grid: returns true and blade shade info when a blade covers the point
static inline unsigned hU(int x, int y, int s) { unsigned n = (unsigned)x * 374761393u + (unsigned)y * 668265263u + (unsigned)s * 2246822519u; n = (n ^ (n >> 13)) * 1274126177u; return n ^ (n >> 16); }
template <int N> static bool bladeLayer(float u, float v, int seed, float dens, float& tip, float& hue, float& lit) {
  float x = u * N, y = v * N; int xi = (int)std::floor(x), yi = (int)std::floor(y); float best = -1;
  for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
    int cx = (((xi + i) % N) + N) % N, cy = (((yi + j) % N) + N) % N;
    for (int b = 0; b < 2; b++) {
      unsigned h1 = hU(cx, cy, seed + b * 101), h2 = hU(cx, cy, seed + b * 101 + 1);
      float z = (h2 & 1023) / 1024.f; if (z <= best || ((h2 >> 10) & 1023) / 1024.f > dens) continue;
      float rx = xi + i + (h1 & 1023) / 1024.f, ry = yi + j + ((h1 >> 10) & 1023) / 1024.f, L = .7f + ((h1 >> 20) & 1023) / 1024.f * 1.1f;
      if ((x - rx) * (x - rx) + (y - ry) * (y - ry) > (L + .4f) * (L + .4f)) continue;
      float ca = ((h2 >> 20) & 255) / 127.5f - 1, sa = (h2 >> 28) / 7.5f - 1 + ((h1 >> 30) - 1.5f) * .08f, il = 1 / std::sqrt(ca * ca + sa * sa + 1e-4f); ca *= il; sa *= il;
      float ex = rx + ca * L, ey = ry + sa * L + .25f * L;   // blades lean slightly "up" the texture: looks like a sward rather than hair
      float t, d = segD(x, y, rx, ry, ex, ey, t), w = .2f * (1 - t * .92f);
      if (d < w) { best = z; tip = t; hue = Hh(cx, cy, seed + b * 101 + 4); lit = 1 - (d / w) * (d / w) * .35f + sa * .06f; }
    }
  }
  return best >= 0;
}
// three layers of blades (front to back), for alien worlds' moss and grass: returns the layer hit (0..2) or -1
int grassBlades(float u, float v, int seed, float dens, float& tip, float& hue, float& lit) {
  if (bladeLayer<64>(u, v, seed + 100, dens, tip, hue, lit)) return 0; if (bladeLayer<96>(u, v, seed + 200, dens, tip, hue, lit)) return 1;
  if (bladeLayer<150>(u, v, seed + 300, dens * .9f + .1f, tip, hue, lit)) return 2; return -1;
}
C4 texGrass(float u, float v) {
  float m = fbmP(u * 4, v * 4, 11, 4, 4, 4), dry = sstep(.42f, .68f, fbmP(u * 3, v * 3, 12, 3, 3, 3));
  float dirt = sstep(.6f, .7f, fbmP(u * 5, v * 5, 13, 5, 5, 5) + (m - .5f) * .2f), soilN = fbmP(u * 64, v * 64, 14, 2, 64, 64);
  float sp = Hh((int)(u * 1024) & 1023, (int)(v * 1024) & 1023, 15);
  // soil showing between blades and in bare patches, with pebbles
  C4 soil = mixc({58, 50, 30, 255}, {112, 88, 60, 255}, dirt * .8f + soilN * .3f);
  Vor pb = voroP(u * 70, v * 70, 16, 70, 70); if (dirt > .2f && pb.f1 < .18f + pb.id * .12f && pb.id < dirt * .8f) soil = scl(mixc({128, 120, 108, 255}, {95, 84, 70, 255}, pb.id), 1.05f - pb.dy * .9f);
  soil = scl(soil, .85f + sp * .3f);
  float tip = 0, hue = 0, lit = 1, dens = 1 - dirt * .95f; int layer = -1;
  if (bladeLayer<64>(u, v, 100, dens, tip, hue, lit)) layer = 0; else if (bladeLayer<96>(u, v, 200, dens, tip, hue, lit)) layer = 1; else if (bladeLayer<150>(u, v, 300, dens * .9f + .1f, tip, hue, lit)) layer = 2;
  if (layer < 0) return scl(mixc(soil, {40, 62, 22, 255}, (1 - dirt) * .55f), 1.f);
  float h = .255f + (hue - .5f) * .05f - dry * .07f, sat = .52f - dry * .12f + hue * .08f, l = .34f + m * .1f + dry * .05f + tip * .13f;
  C4 c = hsl(h, sat, l); float depth = layer == 0 ? 1.08f : layer == 1 ? .9f : .74f;
  if (layer == 0 && hue > .985f && tip > .8f) return hue > .993f ? C4{235, 220, 90, 255} : C4{230, 230, 220, 255};   // a few tiny flowers
  return scl(c, depth * lit * (.94f + sp * .12f));
}
// rock: tilted strata, faceted fractured blocks, crack network, mineral grains, a little lichen
static float rockH(float u, float v) { return fbmP(u * 6, v * 6, 5, 5, 6, 6); }
C4 texRock(float u, float v) {
  const float e = 1.f / 512; float h = rockH(u, v), hx = rockH(u + e, v), hy = rockH(u, v + e);
  float shade = clampf(1 + ((h - hx) * 1.f + (h - hy) * .8f) * 38.f, .7f, 1.25f);
  float w = fbmP(u * 3, v * 3, 21, 3, 3, 3), sv = (v * 5 + u + w * 1.3f) * 6.2832f, strata = std::sin(sv * 2), seam = std::pow(std::fabs(std::sin(sv * 3.5f)), 10.f);
  float wu = u + (fbmP(u * 4, v * 4, 29, 3, 4, 4) - .5f) * .1f, wv = v + (fbmP(u * 4, v * 4, 30, 3, 4, 4) - .5f) * .1f;   // warped so fractures wander
  Vor big = voroP(wu * 5, wv * 5, 22, 5, 5, .9f), sm = voroP(wu * 15, wv * 15, 23, 15, 15);
  float cm = sstep(.38f, .52f, fbmP(u * 5, v * 5, 24, 3, 5, 5)), cw = .025f + fbmP(u * 12, v * 12, 31, 2, 12, 12) * .05f;
  float crack = (1 - sstep(0, cw, big.f2 - big.f1)) * cm, crack2 = (1 - sstep(0, .025f, sm.f2 - sm.f1)) * sstep(.5f, .62f, fbmP(u * 4, v * 4, 25, 3, 4, 4));
  float k = 88 + h * 95 + strata * 9 - seam * 18 + (big.id - .5f) * 16 + (sm.id - .5f) * 6;
  float g = Hh((int)(u * 512) & 511, (int)(v * 512) & 511, 26); if (g > .975f) k += 36; else if (g < .03f) k -= 32; else k += (g - .5f) * 12;
  k *= shade * (1 - crack * .62f) * (1 - crack2 * .3f);
  C4 c{k * 1.02f, k * .99f, k * .94f, 255};
  float lich = sstep(.56f, .68f, fbmP(u * 5, v * 5, 27, 4, 5, 5)) * (1 - crack); Vor lv = voroP(u * 40, v * 40, 28, 40, 40);
  if (lich > 0 && lv.f1 < .48f) c = mixc(c, lv.id < .85f ? C4{150, 154, 106, 255} : C4{182, 130, 72, 255}, lich * .6f * (1 - lv.f1 / .48f * .5f));
  return c;
}
// concrete: 2x2 slabs with bevelled seams, per-slab tone, aggregate, water stains, chipped edges, hairline cracks
C4 texConc(float u, float v) {
  float tu = u * 2, tv = v * 2, fx = fr(tu), fy = fr(tv); int ix = (int)std::floor(tu) & 1, iy = (int)std::floor(tv) & 1;
  float edge = std::min(std::min(fx, 1 - fx), std::min(fy, 1 - fy)), slab = Hh(ix, iy, 31);
  float agg = fbmP(u * 24, v * 24, 32, 3, 24, 24), stain = fbmP(u * 3, v * 3, 33, 4, 3, 3), drip = fbmP(u * 40, v * 3, 34, 3, 40, 3);
  float k = 152 + (slab - .5f) * 26 + (agg - .5f) * 26 + (fbmP(u * 8, v * 8, 35, 4, 8, 8) - .5f) * 22 + (fbmP(u * 6 + slab * 3, v * 6, 40, 3, 6, 6) - .5f) * 16;
  float g = Hh((int)(u * 512) & 511, (int)(v * 512) & 511, 36); if (g > .97f) k -= 40 * (g - .97f) / .03f; else if (g < .02f) k += 22;
  k *= 1 - .16f * sstep(.55f, .75f, stain) - .1f * sstep(.55f, .8f, drip) - .1f * (1 - sstep(.0f, .09f, edge)) * stain;   // stains, drips, grime collecting along seams
  float chip = fbmP(u * 48, v * 48, 37, 2, 48, 48);
  if (edge < .008f + (chip > .64f ? .016f : 0.f)) k *= edge < .008f ? .5f : .8f;                 // seam and chipped corners
  else if (edge < .022f) k *= (fx < .022f || fy < .022f) ? 1.07f : .93f;                             // bevel: lit side, shadow side
  Vor cr = voroP(u * 5, v * 5, 38, 5, 5); float crk = (1 - sstep(0, .02f, cr.f2 - cr.f1)) * sstep(.62f, .7f, fbmP(u * 6, v * 6, 39, 3, 6, 6)); k *= 1 - crk * .35f;
  float t = (slab - .5f) * 4; return {k + t, k + t * .5f, k - 3, 255};
}
// hull plates: staggered panels, bevelled seams, rivet rows, an occasional hatch, brushed grain, streaks of grime below seams. Stays bright so tints read.
C4 texHull(float u, float v) {
  float cu = u * 4; int col = (int)std::floor(cu) & 3; float pv = v * 4 + (col & 1) * .5f; int row = ((int)std::floor(pv) % 4 + 4) % 4;
  float fx = fr(cu), fy = fr(pv), panel = Hh(col, row, 41), ed = std::min(std::min(fx, 1 - fx), std::min(fy, 1 - fy));
  float k = 212 + (panel - .5f) * 14 + (vnoiseP(u * 6, v * 300, 42, 6, 300) - .5f) * 7 + (vnoiseP(u * 24, v * 600, 43, 24, 600) - .5f) * 5;
  float grime = fbmP(u * 32, v * 3, 44, 3, 32, 3), below = std::exp(-fy * 9.f);           // dirt runs down from the seam above
  k -= sstep(.45f, .75f, grime) * 22 * (.35f + below) + (1 - sstep(0, .25f, ed)) * 6;
  k -= sstep(.55f, .8f, fbmP(u * 5, v * 5, 45, 4, 5, 5)) * 10;                               // soft smudges
  if (ed < .008f) k -= 95; else if (ed < .02f) k += (fx < .5f && fy < .5f) ? 10 : -12;     // seam + bevel
  // rivet rows inset along the panel edges
  const float ins = .055f; float best = 9, bx = 0, by = 0, ry = (std::floor(fy * 10) + .5f) / 10, rx = (std::floor(fx * 10) + .5f) / 10;
  const float cand[4][2] = {{ins, ry}, {1 - ins, ry}, {rx, ins}, {rx, 1 - ins}};
  for (auto& c : cand) { float dx = (fx - c[0]) * 128, dy = (fy - c[1]) * 128, d = dx * dx + dy * dy; if (d < best) { best = d; bx = dx; by = dy; } }
  if (best < 4.8f) k += 16 - (bx + by) * 6; else if (best < 9) k -= 34;
  // access hatch on some panels
  if (panel < .22f) { float hx = std::fabs(fx - .5f), hy = std::fabs(fy - .6f), hd = std::max(hx / .22f, hy / .14f); if (std::fabs(hd - 1) < .03f) k -= 70; else if (hd < 1) k -= 6; }
  return {k, k + 1, k + 6, 255};
}
// bark structure (shared by tBark and the neutral tBarkW): 0 = deep fissure .. 1 = plate surface, plus fibre streaks
static float barkL(float u, float v, float& fis) {
  // wavy vertical ridges (phase warped by noise) separated by fissures that pinch, branch and break up
  float w = fbmP(u * 2, v * 3, 51, 4, 2, 3) * 1.6f + fbmP(u * 4, v * 8, 52, 3, 4, 8) * .45f;
  float s1 = std::fabs(std::sin((u * 7 + w) * PI)), s2 = std::fabs(std::sin((u * 11 + w * 1.3f + .3f) * PI));
  float br = vnoiseP(u * 10, v * 5, 57, 10, 5), depth = sstep(.25f, .55f, br);                     // where fissures fade out: plates bridge across
  fis = std::max((1 - sstep(.0f, .45f, s1)), (1 - sstep(.0f, .3f, s2)) * (1 - depth)) * (.55f + .45f * (1 - depth * .6f));
  float fib = fbmP(u * 40, v * 2, 53, 3, 40, 2), grain = std::pow(std::fabs(std::sin((u * 60 + fib * 3) * PI)), 3.f), plateId = vnoiseP(u * 14, v * 6, 58, 14, 6);
  float plate = .76f + (plateId - .5f) * .25f + (fib - .5f) * .4f - grain * .08f + sstep(.3f, .9f, s1) * .1f + (fbmP(u * 8, v * 8, 54, 3, 8, 8) - .5f) * .2f;
  return clampf(plate * (1 - fis * fis * .82f), 0, 1.2f);
}
C4 texBark(float u, float v) {
  float fis, l = barkL(u, v, fis), lich = sstep(.6f, .7f, fbmP(u * 8, v * 4, 55, 4, 8, 4)) * (1 - fis) * sstep(.3f, .6f, vnoiseP(u * 64, v * 32, 56, 64, 32));
  C4 c = scl(C4{132, 100, 70, 255}, l * 1.05f); c = mixc(c, C4{36, 25, 17, 255}, fis * fis * .5f);
  return mixc(c, C4{118, 130, 92, 255}, lich * .5f);
}
C4 texBarkW(float u, float v) { float fis, l = barkL(u, v, fis); float k = 120 + l * 112; return grey(k); }
// foliage: overlapping oriented leaves in two depth layers with midribs and dark gaps. Returns shade (0 = gap) and hue jitter.
static float leafL(float u, float v, float& hj) {
  for (int layer = 0; layer < 2; layer++) {
    int N = layer ? 13 : 8, s = 60 + layer * 10; float x = u * N, y = v * N; int xi = (int)std::floor(x), yi = (int)std::floor(y); float best = -1, val = 0;
    for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) for (int b = 0; b < 2; b++) {
      int cx = (((xi + i) % N) + N) % N, cy = (((yi + j) % N) + N) % N, sb = s + b * 3;
      float px = xi + i + Hh(cx, cy, sb), py = yi + j + Hh(cx, cy, sb + 1), a = Hh(cx, cy, sb + 2) * 6.2832f, z = Hh(cx, cy, sb + 4);
      float dx = x - px, dy = y - py, lx = dx * std::cos(a) + dy * std::sin(a), ly = -dx * std::sin(a) + dy * std::cos(a);
      float t = lx / .62f; if (t < -1 || t > 1) continue; float wdt = .26f * std::sqrt(1 - t * t) * (1 - .25f * t), e = std::fabs(ly) / std::max(wdt, 1e-3f);
      if (e < 1 && z > best) { best = z; hj = Hh(cx, cy, sb + 3);
        float rib = std::fabs(ly) < .025f ? .82f : 1.f, vein = .94f + .06f * std::cos((lx - std::fabs(ly) * 1.2f) * 40.f);
        val = (.8f + .25f * (1 - e * e) + .12f * std::sin(a) + (t + 1) * .05f) * rib * vein * (layer ? .76f : 1.f); }
    }
    if (best >= 0) return val;
  }
  hj = .5f; return .28f;
}
C4 texLeaf(float u, float v) {
  float hj, l = leafL(u, v, hj), m = fbmP(u * 3, v * 3, 66, 3, 3, 3);
  C4 c = hsl(.27f + (hj - .5f) * .07f + (m - .5f) * .05f, .5f + hj * .15f, .17f + l * .2f);
  return c;
}
C4 texLeafW(float u, float v) { float hj, l = leafL(u, v, hj); return grey(clampf(95 + l * 125 + (hj - .5f) * 20, 0, 255)); }
// neutral skin: rounded scales with dark borders, pores, mottling
C4 texSkin(float u, float v) {
  float wu = u + (fbmP(u * 6, v * 6, 71, 2, 6, 6) - .5f) * .04f, wv = v + (fbmP(u * 6, v * 6, 75, 2, 6, 6) - .5f) * .04f;
  Vor sc = voroP(wu * 28, wv * 28, 72, 28, 28, .8f), pr = voroP(u * 110, v * 110, 73, 110, 110);
  float dome = std::sqrt(clampf(1 - sc.f1 * sc.f1 * 1.6f, 0, 1)), border = 1 - sstep(0, .26f, sc.f2 - sc.f1), hl = clampf(-(sc.dx + sc.dy) * .8f, -1, 1);
  float k = 184 + dome * 42 + hl * 16 * dome - border * 22 + (sc.id - .5f) * 12 + (fbmP(u * 5, v * 5, 74, 4, 5, 5) - .5f) * 34;
  if (pr.f1 < .12f && pr.id < .45f) k -= 20 * (1 - pr.f1 / .12f);
  return grey(clampf(k, 0, 255));
}
// neutral woven fabric (plain weave with slubs and fibre fuzz)
C4 texCloth(float u, float v) {
  const int N = 96; float x = u * N, y = v * N; int ix = (int)std::floor(x), iy = (int)std::floor(y); float fx = x - ix, fy = y - iy;
  bool warp = ((ix + iy) & 1) == 0;                                  // which thread is on top at this crossing
  float across = warp ? fx : fy, along = warp ? fy : fx; int th = warp ? ix : iy;
  float thick = .82f + Hh(th, warp, 81) * .14f, prof = std::sin(clampf((across - .5f) / thick + .5f, 0, 1) * PI);
  float dip = .78f + .22f * std::sin(along * PI), fib = vnoiseP(warp ? u * N * 2 : u * 8, warp ? v * 8 : v * N * 2, 82 + warp, warp ? N * 2 : 8, warp ? 8 : N * 2);
  float k = 168 + prof * dip * 70 + (fib - .5f) * 22 + (Hh(th, warp, 83) - .5f) * 14 + (fbmP(u * 4, v * 4, 84, 3, 4, 4) - .5f) * 16;
  return grey(clampf(k, 0, 255));
}
// neutral planks: 4 rows of staggered boards with grain, rings, knots and dark gaps
C4 texWood(float u, float v) {
  float rv = v * 4; int row = (int)std::floor(rv) & 3; float fy = fr(rv), off = Hh(row, 0, 91), su = u * 2 + off; int seg = (int)std::floor(su) & 1; float fx = fr(su);
  float id = Hh(row, seg, 92), warp = fbmP(u * 4, v * 4, 93, 3, 4, 4) * 1.6f;
  // knot: grain swirls around it
  float kx = .2f + Hh(row, seg, 94) * .6f, ky = .3f + Hh(row, seg, 95) * .4f, kdx = (fx - kx) * 1.1f, kdy = (fy - ky) * 1.8f, kd = std::sqrt(kdx * kdx + kdy * kdy); bool hasK = Hh(row, seg, 96) < .6f;
  float g = fy * (4 + id * 4) + warp + id * 9 + (hasK ? .9f * std::exp(-kd * kd * 25) * (fy - ky) / (std::fabs(fy - ky) + .04f) : 0);   // grain bends around the knot
  float ring = std::pow(.5f + .5f * std::sin(g * 6.2832f), 3.f), fine = vnoiseP(u * 6, v * 220, 97, 6, 220);
  float k = 228 - ring * 38 + (fine - .5f) * 20 + (id - .5f) * 24;
  if (hasK && kd < .13f) k -= (1 - kd / .13f) * 45 + (.5f + .5f * std::cos(kd * 140)) * 25 * (1 - kd / .13f);
  if (fy < .02f || fy > .985f || fx < .006f || fx > .994f) k = 95;   // gaps between boards
  else if (fy < .045f) k -= 14;
  return grey(clampf(k, 0, 255));
}
// neutral brushed metal: fine directional grain, a few scratches, soft smudges
C4 texMetal(float u, float v) {
  float k = 214 + (vnoiseP(u * 6, v * 320, 101, 6, 320) - .5f) * 18 + (vnoiseP(u * 24, v * 640, 102, 24, 640) - .5f) * 12 + (Hh((int)(u * 512) & 511, (int)(v * 512) & 511, 103) - .5f) * 6;
  k += (fbmP(u * 4, v * 4, 104, 4, 4, 4) - .5f) * 22;
  const int N = 6; float x = u * N, y = v * N; int xi = (int)std::floor(x), yi = (int)std::floor(y);
  for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) { int cx = (((xi + i) % N) + N) % N, cy = (((yi + j) % N) + N) % N; if (Hh(cx, cy, 105) > .45f) continue;
    float ax = xi + i + Hh(cx, cy, 106), ay = yi + j + Hh(cx, cy, 107), a = Hh(cx, cy, 108) * 6.2832f, L = .2f + Hh(cx, cy, 109) * .5f, t;
    float d = segD(x, y, ax, ay, ax + std::cos(a) * L, ay + std::sin(a) * L, t); if (d < .006f) k += Hh(cx, cy, 110) < .5f ? 12 : -10; }
  return grey(clampf(k, 0, 255));
}
// ---------------------------------------------------------------- (TEXGEN END)
// ---------------------------------------------------------------- meshes
static Mesh upload(const std::vector<float>& v, const std::vector<unsigned>& ix) {
  Mesh m; m.n = (int)ix.size(); glGenVertexArrays(1, &m.vao); glBindVertexArray(m.vao);
  glGenBuffers(1, &m.vbo); glBindBuffer(GL_ARRAY_BUFFER, m.vbo); glBufferData(GL_ARRAY_BUFFER, v.size() * 4, v.data(), GL_STATIC_DRAW);
  glGenBuffers(1, &m.ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, ix.size() * 4, ix.data(), GL_STATIC_DRAW);
  for (int i = 0; i < 3; i++) { glEnableVertexAttribArray(i); glVertexAttribPointer(i, i == 2 ? 2 : 3, GL_FLOAT, GL_FALSE, 32, (void*)(size_t)(i == 0 ? 0 : i == 1 ? 12 : 24)); }
  return m;
}
static void pv(std::vector<float>& v, V3 p, V3 n, float u, float t) { v.insert(v.end(), {p.x, p.y, p.z, n.x, n.y, n.z, u, t}); }
Mesh mkSphere(int sg, int rg) {
  std::vector<float> v; std::vector<unsigned> ix;
  for (int j = 0; j <= rg; j++) { float ph = PI * j / rg; for (int i = 0; i <= sg; i++) { float th = 2 * PI * i / sg; V3 p{std::sin(ph) * std::cos(th), std::cos(ph), std::sin(ph) * std::sin(th)}; pv(v, p, p, (float)i / sg, (float)j / rg); } }
  for (int j = 0; j < rg; j++) for (int i = 0; i < sg; i++) { unsigned a = j * (sg + 1) + i, b = a + sg + 1; ix.insert(ix.end(), {a, b, a + 1, a + 1, b, b + 1}); }
  return upload(v, ix);
}
// cylinder / cone: origin at the base, height along +Y
Mesh mkCyl(float r0, float r1, float h, int sg, bool caps) {
  std::vector<float> v; std::vector<unsigned> ix; float ny = (r0 - r1) / h;
  for (int i = 0; i <= sg; i++) { float th = 2 * PI * i / sg, c = std::cos(th), s = std::sin(th); V3 n = norm({c, ny, s}); pv(v, {c * r0, 0, s * r0}, n, (float)i / sg, 0); pv(v, {c * r1, h, s * r1}, n, (float)i / sg, 1); }
  for (int i = 0; i < sg; i++) { unsigned a = 2 * i; ix.insert(ix.end(), {a, a + 1, a + 2, a + 1, a + 3, a + 2}); }
  if (caps) for (int top = 0; top < 2; top++) {
    float r = top ? r1 : r0; if (r <= 0) continue; unsigned base = (unsigned)(v.size() / 8); V3 n{0, top ? 1.f : -1.f, 0}; pv(v, {0, top ? h : 0, 0}, n, .5f, .5f);
    for (int i = 0; i <= sg; i++) { float th = 2 * PI * i / sg; pv(v, {std::cos(th) * r, top ? h : 0, std::sin(th) * r}, n, .5f + .5f * std::cos(th), .5f + .5f * std::sin(th)); }
    for (int i = 0; i < sg; i++) ix.insert(ix.end(), {base, base + 1 + i, base + 2 + i});
  }
  return upload(v, ix);
}
Mesh mkBox() {
  std::vector<float> v; std::vector<unsigned> ix; V3 N[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  for (int f = 0; f < 6; f++) { V3 n = N[f], u = norm(cross(n, std::fabs(n.y) > .5f ? V3{1, 0, 0} : V3{0, 1, 0})), w = cross(n, u); unsigned b = (unsigned)(v.size() / 8);
    for (int q = 0; q < 4; q++) { float sx = (q & 1) ? 1.f : -1.f, sy = (q & 2) ? 1.f : -1.f; pv(v, n * .5f + u * (sx * .5f) + w * (sy * .5f), n, (sx + 1) / 2, (sy + 1) / 2); }
    ix.insert(ix.end(), {b, b + 1, b + 2, b + 2, b + 1, b + 3}); }
  return upload(v, ix);
}
Mesh mkRing(float inner, int sg) {
  std::vector<float> v; std::vector<unsigned> ix;
  for (int i = 0; i <= sg; i++) { float th = 2 * PI * i / sg, c = std::cos(th), s = std::sin(th); pv(v, {c * inner, 0, s * inner}, {0, 1, 0}, 0, 0); pv(v, {c, 0, s}, {0, 1, 0}, 1, 1); }
  for (int i = 0; i < sg; i++) { unsigned a = 2 * i; ix.insert(ix.end(), {a, a + 1, a + 2, a + 1, a + 3, a + 2}); }
  return upload(v, ix);
}
float hAt(float x, float z) { return (fbm(x * .012f + 500, z * .012f + 500, 3, 4) - .45f) * 38.f * clampf((std::hypot(x, z) - 25.f) / 50.f, 0, 1); }
Mesh mkTerrain(int n, float S, float (*hf)(float, float)) {
  std::vector<float> v; std::vector<unsigned> ix; const float e = .8f;
  for (int i = 0; i <= n; i++) for (int j = 0; j <= n; j++) { float x = ((float)j / n - .5f) * S, z = ((float)i / n - .5f) * S;
    V3 nn = norm({hf(x - e, z) - hf(x + e, z), 2 * e, hf(x, z - e) - hf(x, z + e)}); pv(v, {x, hf(x, z), z}, nn, (float)j / n, (float)i / n); }
  for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) { unsigned a = i * (n + 1) + j, b = a + n + 1; ix.insert(ix.end(), {a, b, a + 1, a + 1, b, b + 1}); }
  return upload(v, ix);
}
// ---------------------------------------------------------------- shaders
GLuint compile(const char* vs, const char* fs) {
  auto sh = [](GLenum t, const char* s) { GLuint o = glCreateShader(t); glShaderSource(o, 1, &s, nullptr); glCompileShader(o); GLint ok; glGetShaderiv(o, GL_COMPILE_STATUS, &ok);
    if (!ok) { char l[2048]; glGetShaderInfoLog(o, 2048, nullptr, l); std::fprintf(stderr, "shader error: %s\n", l); } return o; };
  GLuint p = glCreateProgram(), a = sh(GL_VERTEX_SHADER, vs), b = sh(GL_FRAGMENT_SHADER, fs); glAttachShader(p, a); glAttachShader(p, b); glLinkProgram(p); return p;
}
static const char* VS_MAIN = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; layout(location=2) in vec2 aT;
uniform mat4 uVP, uM; uniform float uUVS; out vec3 vN; out vec2 vT; out vec3 vR;
void main(){ vec4 w=uM*vec4(aP,1.0); vR=w.xyz; vN=mat3(uM)*aN; vT=aT*uUVS; gl_Position=uVP*w; })";
// Main lit shader. uMode: 0 = lit, 1 = additive atmosphere rim, 2 = unlit with alpha (rings, hoops), 3 = planet seen from space
// (texture alpha = 1 - ocean/ice specular mask, output alpha = uAlpha). Lighting: hemisphere ambient (uAmb from above, darker and warmer
// from below), softly wrapped sun diffuse, small Blinn-Phong highlight (uSpec), filmic tone curve, then distance fog.
// uDetail > 0 (terrain): a second rotated high-frequency sample, non-repeating macro variation and darker, rockier steep slopes.
static const char* FS_MAIN = R"(#version 330 core
in vec3 vN; in vec2 vT; in vec3 vR; out vec4 oC;
uniform sampler2D uTex; uniform vec3 uSunDir,uSunCol,uAmb,uTint,uFogCol; uniform float uFog,uEmis,uAlpha,uDetail,uSpec; uniform int uMode;
float hash(vec2 p){ return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float vnz(vec2 p){ vec2 i=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);
  return mix(mix(hash(i),hash(i+vec2(1.0,0.0)),f.x),mix(hash(i+vec2(0.0,1.0)),hash(i+vec2(1.0,1.0)),f.x),f.y); }
vec3 aces(vec3 x){ return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14),0.0,1.0); }
vec3 tone(vec3 c){ return 0.35*(1.0-exp(-c*1.25))+0.65*aces(c*0.9); }   // filmic shoulder, toe kept close to the old curve
void main(){
  vec4 t=texture(uTex,vT); vec3 N=normalize(vN), V=normalize(-vR);
  if(uMode==1){ float f=pow(1.0-abs(dot(N,V)),2.5); oC=vec4(uTint*f*uEmis,1.0); return; }
  if(uMode==2){ oC=vec4(t.rgb*uTint,t.a*uAlpha); return; }
  vec3 alb=t.rgb;
  if(uDetail>0.0){
    vec3 avg=max(textureLod(uTex,vec2(0.5),16.0).rgb,vec3(0.02));                 // 1x1 mip = mean colour of the texture
    vec2 q=mat2(0.8,-0.6,0.6,0.8)*vT*4.37+vec2(0.31,0.17);
    float hi=clamp(dot(texture(uTex,q).rgb/avg,vec3(0.3333)),0.4,1.8);
    float m=vnz(vT*0.11)*0.6+vnz(vT*0.29+vec2(7.3,1.9))*0.4;                         // large patches that never repeat
    alb*=mix(1.0,hi,0.3*uDetail);
    alb*=mix(vec3(1.0),mix(vec3(0.84,0.84,0.8),vec3(1.12,1.1,1.06),m),uDetail);
    float slope=(1.0-smoothstep(0.6,0.88,N.y))*uDetail, l=dot(alb,vec3(0.3,0.59,0.11));
    alb=mix(alb,mix(alb,vec3(l)*vec3(1.0,0.94,0.86),0.6)*(0.62+0.25*hi),slope*0.85);
  }
  alb*=uTint;
  float ndl=dot(N,uSunDir);
  if(uMode==3){
    float d=clamp((ndl+0.04)/1.04,0.0,1.0), m=1.0-t.a; vec3 H=normalize(uSunDir+V); float nh=max(dot(N,H),0.0);
    vec3 sp=uSunCol*m*(pow(nh,90.0)*0.9+pow(nh,14.0)*0.06)*smoothstep(0.0,0.15,ndl);
    vec3 c=alb*(uAmb+uSunCol*d)+sp+alb*uEmis;
    c=tone(c); float fg=1.0-exp(-pow(length(vR)*uFog,2.0)); c=mix(c,uFogCol,fg);
    oC=vec4(c,uAlpha); return; }
  float d=clamp((ndl+0.25)/1.25,0.0,1.0); d=d*(0.65+0.35*d*(3.0-2.0*d));             // wrapped diffuse with a softened terminator
  vec3 amb=uAmb*mix(vec3(0.72,0.67,0.6),vec3(1.0),N.y*0.5+0.5);                 // sky from above, darker warm bounce from below
  vec3 H=normalize(uSunDir+V); float sp=pow(max(dot(N,H),0.0),36.0)*uSpec*smoothstep(0.0,0.2,ndl)*(0.4+0.6*dot(alb,vec3(0.3333)));
  vec3 c=alb*(amb+uSunCol*d)+uSunCol*sp+alb*uEmis;
  c=tone(c);
  float fg=1.0-exp(-pow(length(vR)*uFog,2.0)); c=mix(c,uFogCol,fg);
  oC=vec4(c,t.a*uAlpha); })";
static const char* VS_PT = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in float aS; layout(location=2) in vec4 aC;
uniform mat4 uVP; uniform float uScale; out vec4 vC;
void main(){ gl_Position=uVP*vec4(aP,1.0); gl_PointSize = aS<0.0 ? -aS : aS*uScale/max(gl_Position.w,0.001); vC=aC; })";
static const char* FS_PT = R"(#version 330 core
in vec4 vC; out vec4 oC; uniform float uFade;
void main(){ float d=length(gl_PointCoord-vec2(0.5))*2.0; float a=pow(clamp(1.0-d,0.0,1.0),2.0)*vC.a*uFade; oC=vec4(vC.rgb*a,1.0); })";
static const char* VS_HUD = R"(#version 330 core
layout(location=0) in vec3 aP; uniform vec2 uO,uScr; uniform float uK;
void main(){ vec2 p=aP.xy*uK+uO; gl_Position=vec4(p.x*2.0/uScr.x-1.0,1.0-p.y*2.0/uScr.y,0.0,1.0); })";
static const char* FS_HUD = R"(#version 330 core
out vec4 oC; uniform vec4 uCol; void main(){ oC=uCol; })";
// ---------------------------------------------------------------- globals
void basis(float yaw, float pitch, float roll, V3& r, V3& u, V3& f) {
  f = {-std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)}; V3 r0{std::cos(yaw), 0, -std::sin(yaw)}, u0 = cross(r0, f);
  float c = std::cos(roll), s = std::sin(roll); r = r0 * c + u0 * s; u = u0 * c - r0 * s;
}
void lookAt(V3 from, V3 to) { cF = norm(to - from); cR = norm(cross(cF, {0, 1, 0})); cU = cross(cR, cF); }
void makeVP() {
  float a = (float)FW / FH, f = 1 / std::tan(fov * PI / 360), n = nearZ, fa = farZ;
  float P[16] = {f / a, 0, 0, 0, 0, f, 0, 0, 0, 0, (fa + n) / (n - fa), -1, 0, 0, 2 * fa * n / (n - fa), 0};
  float V[16] = {cR.x, cU.x, -cF.x, 0, cR.y, cU.y, -cF.y, 0, cR.z, cU.z, -cF.z, 0, 0, 0, 0, 1};
  for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) { float s = 0; for (int k = 0; k < 4; k++) s += P[k * 4 + r] * V[c * 4 + k]; VP[c * 4 + r] = s; }
}
void draw(const Mesh& m, const DP& d) {
  V3 rel = d.p - camPos; V3 X = d.X * d.s.x, Y = d.Y * d.s.y, Z = d.Z * d.s.z;
  float M[16] = {X.x, X.y, X.z, 0, Y.x, Y.y, Y.z, 0, Z.x, Z.y, Z.z, 0, rel.x, rel.y, rel.z, 1};
  glUniformMatrix4fv(uM, 1, GL_FALSE, M); glUniform3f(uTint, d.t.x, d.t.y, d.t.z); glUniform1f(uEmis, d.emis); glUniform1f(uUVS, d.uvs); glUniform1f(uAlpha, d.alpha); glUniform1i(uMode, d.mode); glUniform1f(uDetail, d.detail); glUniform1f(uSpec, d.spec);
  glBindTexture(GL_TEXTURE_2D, d.tex ? d.tex : tWhite); glBindVertexArray(m.vao); glDrawElements(GL_TRIANGLES, m.n, GL_UNSIGNED_INT, nullptr);
}
// ---------------------------------------------------------------- point sprites (stars, flames, beams, sun glow)
void setupPoints() {
  auto cfg = [](GLuint& vao, GLuint& vbo) { glGenVertexArrays(1, &vao); glBindVertexArray(vao); glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (void*)0); glEnableVertexAttribArray(1); glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 32, (void*)12);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 32, (void*)16); };
  cfg(ptVAO, ptVBO); cfg(starVAO, starVBO);
  std::vector<PT> s; Rng r{5};
  for (int i = 0; i < 6000; i++) { float a = r.r(0, 2 * PI), z = r.r(-1, 1), q = std::sqrt(1 - z * z), k = .35f + r.f() * .65f, t = r.f();
    s.push_back({std::cos(a) * q * 70000.f, z * 70000.f, std::sin(a) * q * 70000.f, -(1.4f + r.f() * 1.8f), k * (.7f + .3f * t), k * .85f, k * (1 - .3f * t), 1.f}); }
  starN = (int)s.size(); glBindBuffer(GL_ARRAY_BUFFER, starVBO); glBufferData(GL_ARRAY_BUFFER, s.size() * sizeof(PT), s.data(), GL_STATIC_DRAW);
}
void drawPoints(GLuint vao, GLuint vbo, const PT* d, int n, float fade) {
  if (n <= 0) return; glBindVertexArray(vao); if (d) { glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, n * sizeof(PT), d, GL_STREAM_DRAW); }
  glUniform1f(uPtFade, fade); glDrawArrays(GL_POINTS, 0, n);
}
// ---------------------------------------------------------------- HUD text
static GLuint hVAO, hVBO, hEBO; static char sbuf[60000];
void setupHud() {
  glGenVertexArrays(1, &hVAO); glBindVertexArray(hVAO); glGenBuffers(1, &hVBO); glBindBuffer(GL_ARRAY_BUFFER, hVBO); glBufferData(GL_ARRAY_BUFFER, sizeof sbuf, nullptr, GL_DYNAMIC_DRAW);
  std::vector<unsigned> ix; for (unsigned q = 0; q < sizeof sbuf / 64; q++) ix.insert(ix.end(), {q * 4, q * 4 + 1, q * 4 + 2, q * 4, q * 4 + 2, q * 4 + 3});
  glGenBuffers(1, &hEBO); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hEBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, ix.size() * 4, ix.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, (void*)0);
}
static void hudColor(V3 c, float a) { glUniform4f(uHCol, c.x, c.y, c.z, a); }
void text(const std::string& s, float x, float y, float sc, V3 c, float a) {
  if (a <= 0 || s.empty()) return; int n = stb_easy_font_print(0, 0, (char*)s.c_str(), nullptr, sbuf, sizeof sbuf);
  glBindVertexArray(hVAO); glBindBuffer(GL_ARRAY_BUFFER, hVBO); glBufferSubData(GL_ARRAY_BUFFER, 0, n * 64, sbuf); glUniform1f(uHK, sc);
  for (int pass = 0; pass < 2; pass++) { float o = pass ? 0.f : sc * 1.3f; glUniform2f(uHO, x + o, y + o); hudColor(pass ? c : V3{0, 0, 0}, pass ? a : a * .8f); glDrawElements(GL_TRIANGLES, n * 6, GL_UNSIGNED_INT, nullptr); }
}
void rect(float x, float y, float w, float h, V3 c, float a) {
  float q[16] = {x, y, 0, 0, x + w, y, 0, 0, x + w, y + h, 0, 0, x, y + h, 0, 0}; glBindVertexArray(hVAO); glBindBuffer(GL_ARRAY_BUFFER, hVBO); glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof q, q);
  glUniform1f(uHK, 1); glUniform2f(uHO, 0, 0); hudColor(c, a); glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
void textC(const std::string& s, float cx, float y, float sc, V3 c, float a) { text(s, cx - stb_easy_font_width((char*)s.c_str()) * sc * .5f, y, sc, c, a); }
void setupShaders() {
  pMain = compile(VS_MAIN, FS_MAIN); pPt = compile(VS_PT, FS_PT); pHud = compile(VS_HUD, FS_HUD);
  auto U = [](GLuint p, const char* n) { return glGetUniformLocation(p, n); };
  uVP = U(pMain, "uVP"); uM = U(pMain, "uM"); uUVS = U(pMain, "uUVS"); uTex = U(pMain, "uTex"); uSunDir = U(pMain, "uSunDir"); uSunCol = U(pMain, "uSunCol"); uAmb = U(pMain, "uAmb"); uTint = U(pMain, "uTint");
  uFogCol = U(pMain, "uFogCol"); uFog = U(pMain, "uFog"); uEmis = U(pMain, "uEmis"); uAlpha = U(pMain, "uAlpha"); uMode = U(pMain, "uMode"); uDetail = U(pMain, "uDetail"); uSpec = U(pMain, "uSpec");
  uPtVP = U(pPt, "uVP"); uPtScale = U(pPt, "uScale"); uPtFade = U(pPt, "uFade"); uHO = U(pHud, "uO"); uHScr = U(pHud, "uScr"); uHK = U(pHud, "uK"); uHCol = U(pHud, "uCol");
}
