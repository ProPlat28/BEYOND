// Unique aliens (every planet and moon has its own species), their drawing, and the Animal-Crossing style voices.
#include "game.h"

// ---------------------------------------------------------------- voices
// Recorded gibberish (assets/audio/voices) per body plan of the planet's species, pitched per person; the synthesized blips are the fallback.
static int talkBody = -1, talkClips = 0, talkLast = -1; static float talkRate = 1, talkNext = 0; static bool talkRec = false, talkClassic = false;   // talkClassic: this people speaks with the classic babble
void setVoice(const Npc* n) {
  if (!n) { talkPitch = 26; talkKind = 0; talkRate = 1; return; }
  talkPitch = 26 + (int)((n->voice - .75f) * 22); talkKind = n->wicked ? 2 : (n->look + (n->elder ? 1 : 0)) % 2;
  if (n->wicked) talkPitch -= 12; if (n->chief) talkPitch -= 4; if (n->elder) talkPitch -= 3;
  talkRate = clampf(std::pow(2.f, (talkPitch - 26) / 40.f), .78f, 1.25f);   // the same pitch spread as the blips: wicked, chiefs and elders sound lower
}
static bool hasWord(const std::string& low, const char* w) {
  for (size_t i = low.find(w); i != std::string::npos; i = low.find(w, i + 1)) {
    size_t e = i + std::strlen(w); if ((i == 0 || !std::isalpha((unsigned char)low[i - 1])) && (e >= low.size() || !std::isalpha((unsigned char)low[e]))) return true; }
  return false;
}
static int pickLine(const std::string& t, int shown, int nth) {   // which recorded line fits this part of the text
  std::string low; for (char c : t) low += (char)std::tolower((unsigned char)c);
  size_t e = t.find_last_not_of(" .\"')*"); char endc = e == std::string::npos ? 0 : t[e];
  bool bye = hasWord(low, "bye") || hasWord(low, "goodbye") || hasWord(low, "farewell") || low.find("safe travels") != std::string::npos || hasWord(low, "good luck");
  bool laugh = low.find("haha") != std::string::npos || low.find("hehe") != std::string::npos || low.find("*giggle") != std::string::npos || low.find("*laugh") != std::string::npos;
  if (nth == 0 && laugh) return VL_LAUGH;
  if ((int)t.size() - shown < 70) return endc == '?' ? VL_QUESTION : bye ? VL_BYE : nth == 0 ? VL_HELLO : endc == '!' ? VL_LAUGH : VL_HELLO;   // the last bit of the line
  return nth == 0 || talkLast != VL_CHATTER ? VL_CHATTER : VL_HELLO;
}
// about one people in three speaks with the classic babble instead of an ElevenLabs voice (fixed per world, never both)
bool classicVoice(const Planet& p) { unsigned h = (unsigned)p.uniq * 2654435761u + (p.isMoon ? 0x9e37u : 0u); h ^= h >> 15; return h % 3 == 0; }
void say(const std::string& who, const std::string& t) {
  talkS = who; talkT = t; talkAge = 0; talkShown = 0; talkL = 10.f + (float)t.size() / 38.f;
  if (talkRec) audioVoiceStop();
  talkRec = false; talkClips = 0; talkLast = -1; talkNext = 0;
  talkBody = st == SURFACE && curP >= 0 && curP < (int)planets.size() ? alienOf(planets[curP].uniq, planets[curP].isMoon).body : -1;   // everyone on a planet is of its species
  talkClassic = talkBody >= 0 && classicVoice(planets[curP]);
}
void updTalk(float dt) {
  if (talkL <= 0) { if (talkRec) { audioVoiceStop(); talkRec = false; } return; }   // closed or cut off: the voice stops too
  talkL -= dt; talkAge += dt;
  std::string t = keysIn(talkT); int shown = std::min((int)t.size(), (int)(talkAge * 38.f));
  const bool rec = (voiceMode == 1 || voiceMode == 3) && !talkClassic, classic = voiceMode == 2 || voiceMode == 3 || talkClassic;
  if (rec && shown < (int)t.size() && talkAge >= talkNext && (talkRec || talkClips == 0)) {   // a new ElevenLabs clip while the text is still typing out
    int l = pickLine(t, shown, talkClips); float d = audioVoice(talkBody, l, talkRate);
    if (d > 0) { talkRec = true; talkNext = talkAge + d + .1f; talkLast = l; }
    talkClips++;
  }
  if (talkRec && shown >= (int)t.size()) { audioVoiceStop(); talkRec = false; }   // the text has finished typing out: the voice stops with it
  if (shown > talkShown) {
    if ((classic || (rec && !talkRec)) && shown - talkShown < 6)   // classic babble (also stands in when a species has no recording)
      for (int i = talkShown; i < shown; i++) { unsigned char c = (unsigned char)t[i]; if (c == ' ' || (i & 1)) continue; audioEvent(S_VOICE + talkKind * 100 + std::max(0, std::min(99, talkPitch + (int)(c % 9) - 4))); }
    talkShown = shown; }
}
// ---------------------------------------------------------------- the species: a body plan plus independent feature axes, each from its own hash stream of (uniq, moon)
static const int NB = 13;
static const char* BODYD[NB] = {"upright", "stilt-legged", "gelatinous", "four-legged", "floating", "six-legged", "stone golem", "tentacled", "bird-like", "serpentine", "mushroom-capped", "crab-like", "centaur-like"};
static const float GAITK[NB] = {1, 1.12f, .85f, 1.25f, .8f, 1.35f, .7f, .9f, 1.12f, .95f, .75f, 1.18f, 1.3f};   // walking speed per body plan
static const float FREQ[NB] = {7, 4.5f, 4.2f, 9, 2, 15, 3.5f, 6, 8, 6, 7.5f, 13, 8.5f};                       // step rate (rad/s) per body plan
Alien alienOf(int u, bool moon) {
  Alien a; unsigned h = ((unsigned)u + 1u) * 2654435761u ^ (moon ? 0x5bd1e995u : 0x27d4eb2fu);
  auto hv = [&](int k) { unsigned x = h ^ ((unsigned)k * 0x9E3779B1u); x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; x *= 3266489917u; x ^= x >> 16; return (x & 0xffff) / 65536.f; };
  auto pk = [&](int k, int n) { return std::min(n - 1, (int)(hv(k) * n)); };
  int b = a.body = pk(0, NB); float es = hv(2);
  a.eyes = 1 + pk(1, 4); a.eyeSt = es < .5f ? 0 : es < .68f ? 1 : es < .84f ? 2 : 3; if (b == 11 && hv(30) < .6f) a.eyeSt = 1;
  a.head = pk(3, 6); a.headS = .82f + hv(4) * .4f; a.ears = pk(5, 5); a.horns = pk(6, 6); a.back = pk(8, 5); a.tailT = pk(9, 5); a.mouth = pk(10, 5); a.pat = pk(11, 4); a.cloth = pk(12, 5); a.hat = pk(13, 5);
  if (b == 8) a.mouth = 1; if (b == 10) { a.horns = 0; if (a.head == 4) a.head = 0; } if (b == 3 || b == 12) a.tailT = 1 + pk(14, 4); if (b == 2 || b == 4 || b == 7 || b == 9) a.tailT = 0;
  a.tail = a.tailT > 0; a.arms = (b == 0 || b == 6 || b == 9 || b == 10 || b == 12) && hv(15) < .3f ? 4 : 2;
  a.girth = .82f + hv(16) * .4f; a.legL = .82f + hv(17) * .36f; a.armL = .82f + hv(18) * .38f; a.size = .9f + hv(7) * .25f;
  float hue = hv(19); a.skin = lin(hsl(hue, .4f + hv(20) * .35f, .4f + hv(21) * .22f)); a.accent = lin(hsl(hue + .33f + hv(22) * .3f, .85f, .55f)) * 1.6f;
  a.skin2 = hv(23) < .5f ? a.skin * .4f : a.accent * .5f; return a;
}
float gaitSpeed(int u, bool moon) { Alien a = alienOf(u, moon); return clampf(GAITK[a.body] * (.86f + a.legL * .16f), .7f, 1.35f); }
std::string alienDesc(int u, bool moon) {
  Alien a = alienOf(u, moon);
  static const char *HD[6] = {"", "a tall head", "a wide head", "a square head", "a pointed head", "a long head"}, *EA[5] = {"", "pointy ears", "round ears", "fin ears", "droopy ears"},
    *HO[6] = {"", "twin horns", "a bony crest", "antennae", "a single horn", "ram horns"}, *BK[5] = {"", "a spiked back", "a shell", "little wings", "a fin sail"},
    *TL[5] = {"", "a whip tail", "a thick tail", "a fluffy tail", "a club tail"}, *MO[5] = {"", "a beak", "tusks", "a snout", "mandibles"}, *PT[4] = {"", "stripes", "spots", "a pale belly"};
  std::string e = a.eyeSt == 2 ? std::string("a glowing visor") : a.eyeSt == 3 ? std::string("compound eyes") : (a.eyes == 1 ? std::string("one eye") : fmt("%d eyes", a.eyes)) + (a.eyeSt == 1 ? " on stalks" : "");
  const char* f[8] = {a.arms == 4 ? "four arms" : "", HO[a.horns], BK[a.back], EA[a.ears], a.body == 8 ? "" : MO[a.mouth], TL[a.tailT], HD[a.head], PT[a.pat]};
  std::vector<std::string> l{e}; for (auto s : f) if (*s) l.push_back(s);
  for (size_t m = std::min<size_t>(4, l.size());; m--) {   // up to 3 features after the eyes, fewer if the line gets too long for the toast
    std::string r = fmt("A %s species with ", BODYD[a.body]); for (size_t i = 0; i < m; i++) r += (i ? (i + 1 == m ? " and " : ", ") : "") + l[i];
    if (r.size() <= 88 || m <= 2) return r; }
}
// ---------------------------------------------------------------- drawing: every person is built from spheres, boxes, cylinders and cones in a local frame (X right, Y up, Z forward)
namespace {
GLuint TXS = 0, TXC = 0;   // skin / cloth texture of the person being drawn
struct Ctx { V3 b, X, Y, Z; float sc; };
struct Hd { float rx = .3f, ry = .3f, rz = .3f, top = .3f; bool box = false; };   // head half-extents around its centre; top = highest point
V3 at(const Ctx& c, V3 l) { return c.b + c.X * (l.x * c.sc) + c.Y * (l.y * c.sc) + c.Z * (l.z * c.sc); }
// a sub-frame at local point pv, turned by yaw (around Y), pitch (+ = nose down / lean forward) and roll (+ = X side tips up)
Ctx sub(const Ctx& c, V3 pv, float yw, float pt, float rl) {
  float cy = std::cos(yw), sy = std::sin(yw), cp = std::cos(pt), sp = std::sin(pt), cr = std::cos(rl), sr = std::sin(rl);
  V3 Z = c.Z * cy + c.X * sy, X = c.X * cy - c.Z * sy, Y = c.Y * cp + Z * sp; Z = Z * cp - c.Y * sp; return {at(c, pv), X * cr + Y * sr, Y * cr - X * sr, Z, c.sc};
}
void part(const Ctx& c, const Mesh& m, V3 l, V3 s, V3 col, float em = 0, GLuint tx = 0) { DP d; d.p = at(c, l); d.s = s * c.sc; d.X = c.X; d.Y = c.Y; d.Z = c.Z; d.t = col; d.emis = em; d.tex = tx ? tx : TXS; draw(m, d); }
void ell(const Ctx& c, V3 l, V3 s, V3 col, float em = 0, GLuint tx = 0) { part(c, mSphereLo, l, s, col, em, tx); }
void limb(const Ctx& c, V3 a, V3 b, float r, V3 col, float em = 0, bool hand = false, const Mesh* mesh = nullptr, GLuint tx = 0) {
  V3 A = at(c, a), B = at(c, b), dv = B - A; float L = len(dv); if (L < 1e-4f) return; V3 Yv = dv * (1 / L), ref = std::fabs(Yv.y) > .9f ? V3{1, 0, 0} : V3{0, 1, 0}, Xv = norm(cross(ref, Yv)), Zv = cross(Yv, Xv);
  DP d; d.p = A; d.s = {r * c.sc, L, r * c.sc}; d.X = Xv; d.Y = Yv; d.Z = Zv; d.t = col; d.emis = em; d.tex = tx ? tx : TXS; draw(mesh ? *mesh : mCylN, d);
  if (hand) { DP h; float hr = r * 1.55f * c.sc; h.p = B; h.s = {hr, hr, hr}; h.t = col; h.emis = em; h.tex = d.tex; draw(mSphereLo, h); }
}
float sz(const Hd& d, float x, float y) { return d.box ? d.rz : d.rz * std::sqrt(std::max(.08f, 1 - (x / d.rx) * (x / d.rx) - (y / d.ry) * (y / d.ry))); }   // front surface of the head at (x, y)
Hd headShape(const Ctx& h, int k, float r, V3 skin) {
  switch (k) {
    case 1: ell(h, {}, {r * .82f, r * 1.28f, r * .9f}, skin); return {r * .82f, r * 1.28f, r * .9f, r * 1.28f};    // tall oval
    case 2: ell(h, {}, {r * 1.35f, r * .72f, r * 1.05f}, skin); return {r * 1.35f, r * .72f, r * 1.05f, r * .72f}; // wide flat
    case 3: part(h, mBox, {}, {r * 1.7f, r * 1.6f, r * 1.6f}, skin); return {r * .85f, r * .8f, r * .8f, r * .8f, true};
    case 4: ell(h, {}, {r, r, r}, skin); part(h, mCone, {0, r * .45f, 0}, {r * .72f, r * 1.45f, r * .72f}, skin); return {r, r, r, r * 1.9f};   // cone-top
    case 5: ell(h, {}, {r * .82f, r * .85f, r * 1.3f}, skin); return {r * .82f, r * .85f, r * 1.3f, r * .85f};     // long
    default: ell(h, {}, {r, r, r}, skin); return {r, r, r, r};
  }
}
void eyes(const Ctx& h, const Hd& d, const Alien& a, V3 skin, bool W, float blink, float wob) {
  V3 ew = W ? V3{1, .15f, .1f} : V3{.95f, .97f, 1}; float ee = W ? 2.5f : .35f, m = std::min(d.rx, d.ry); int n = a.eyes;
  if (a.eyeSt == 2) { float y = d.ry * .14f; part(h, mBox, {0, y, sz(d, 0, y) - d.rz * .2f}, {d.rx * 1.75f, m * .34f * std::max(blink, .4f), d.rz * .5f}, W ? ew : a.accent, W ? 2.5f : 1.4f, tWhite); return; }   // visor band
  if (a.eyeSt == 3) { for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 3; k++) { float er = m * (k ? .14f : .21f), ex = s * d.rx * (k == 2 ? .62f : .4f), ey = d.ry * (k == 1 ? .4f : k == 2 ? .02f : .12f);   // compound clusters
      ell(h, {ex, ey, sz(d, ex, ey) - er * .3f}, {er, er * blink, er}, W ? ew : a.accent * .25f + V3{.02f, .02f, .06f}, W ? 2.5f : .9f, tWhite); } return; }
  float er = m * (n == 1 ? .34f : n == 2 ? .25f : n == 3 ? .21f : .18f), sp = d.rx * (n == 2 ? .6f : n == 3 ? .5f : .38f);
  for (int i = 0; i < n; i++) {
    float ex = n == 1 ? 0.f : (i - (n - 1) * .5f) * sp, ey = d.ry * (.1f + (n == 3 && i == 1 ? .32f : 0.f) + (n == 4 && (i == 1 || i == 2) ? .12f : 0.f)), r = er; V3 e{ex, ey, sz(d, ex, ey) - er * .25f};
    if (a.eyeSt == 1) { e = {ex * 1.4f + std::sin(wob + i * 2.1f) * m * .12f, d.ry + m * (.7f + (i & 1) * .2f), d.rz * .3f}; limb(h, {ex * .7f, d.ry * .6f, d.rz * .2f}, e, m * .07f, skin * .8f); r = er * 1.15f; }   // on stalks
    ell(h, e, {r, r * blink, r}, ew, ee, tWhite); ell(h, e + V3{0, 0, r * .66f}, {r * .52f, r * (W ? .8f : .52f) * blink, r * .45f}, {.015f, .015f, .03f}, 0, tWhite);
  }
}
void mouth(const Ctx& h, const Hd& d, const Alien& a, V3 skin, bool W, float tw) {
  float m = std::min(d.rx, d.ry), my = -d.ry * .45f, mz = sz(d, 0, my); V3 dk = W ? V3{.5f, .02f, .02f} : V3{.05f, .02f, .03f}; int k = a.body == 8 ? 1 : a.mouth;
  if (k == 1) { V3 bc = a.accent * .4f + V3{.45f, .32f, .06f}; limb(h, {0, my * .55f, mz * .8f}, {0, my * .9f, mz + m * .75f}, m * .24f, bc, 0, false, &mCone); limb(h, {0, my * .95f, mz * .8f}, {0, my * 1.1f - tw, mz + m * .45f}, m * .15f, bc * .8f, 0, false, &mCone); return; }   // beak
  if (k == 3) { limb(h, {0, my * .5f, mz * .7f}, {0, my * .6f, mz + m * .45f}, m * .3f, skin * .92f); part(h, mBox, {0, my * .58f, mz + m * .46f}, {m * .32f, m * .08f, m * .04f}, dk, 0, tWhite); return; }   // snout
  if (k == 4) for (int s = -1; s <= 1; s += 2) limb(h, {s * m * .3f, my, mz * .9f}, {s * m * (.12f + tw), my - m * .3f, mz + m * .4f}, m * .07f, a.accent * .3f, 0, false, &mCone);   // mandibles
  if (k == 2) for (int s = -1; s <= 1; s += 2) limb(h, {s * m * .3f, my - m * .05f, mz * .92f}, {s * m * .4f, my + m * .4f, mz + m * .14f}, m * .07f, {.92f, .9f, .8f}, 0, false, &mCone, tWhite);   // tusks
  if (k != 4 || W) part(h, mBox, {0, my, mz * .97f}, {m * (W ? .7f : .4f), m * .07f, m * .07f}, dk, W ? .8f : 0, tWhite);
}
void ears(const Ctx& h, const Hd& d, const Alien& a, V3 skin, float wig) {
  float m = std::min(d.rx, d.ry);
  for (int s = -1; s <= 1; s += 2) { V3 e{s * d.rx * .9f, d.ry * .25f, -d.rz * .05f};
    if (a.ears == 1) limb(h, e, e + V3{s * m * .55f, m * .8f + wig, -m * .15f}, m * .17f, skin, 0, false, &mCone);
    else if (a.ears == 2) { V3 q{s * d.rx * .72f, d.ry * .8f, -d.rz * .1f}; ell(h, q, {m * .3f, m * .3f, m * .08f}, skin * .95f); ell(h, q + V3{0, 0, m * .04f}, {m * .2f, m * .2f, m * .06f}, a.accent * .5f); }
    else if (a.ears == 3) ell(sub(h, e, 0, 0, s * (.35f + wig)), {s * m * .1f, m * .15f, -m * .2f}, {m * .05f, m * .4f, m * .38f}, a.accent * .6f, .25f);
    else if (a.ears == 4) limb(h, e, e + V3{s * m * .35f, -m * .95f + wig, -m * .1f}, m * .13f, skin * .9f, 0, true);
  }
}
void horns(const Ctx& h, const Hd& d, const Alien& a, V3 skin, bool W, bool chief, float wob) {
  float m = std::min(d.rx, d.ry); int hn = W ? 1 : a.horns; V3 hc = W ? V3{.03f, .02f, .02f} : a.accent * .55f;
  if (hn == 1) for (int s = -1; s <= 1; s += 2) limb(h, {s * d.rx * .5f, d.ry * .75f, 0}, {s * d.rx * (W ? 1.05f : .85f), d.ry * .75f + m * (W ? 1.25f : .85f), -m * .15f}, m * .17f, hc, W ? .3f : 0, false, &mCone);
  else if (hn == 2) for (int k = -1; k <= 1; k++) limb(h, {0, d.top - m * .15f, k * d.rz * .45f}, {0, d.top + m * (.65f - std::abs(k) * .35f), k * d.rz * .6f - m * .15f}, m * .14f, hc, .4f, false, &mCone);
  else if (hn == 3) for (int s = -1; s <= 1; s += 2) { V3 tip{s * d.rx * .5f + std::sin(wob + s) * m * .1f, d.top + m, m * .1f}; limb(h, {s * d.rx * .3f, d.top * .85f, 0}, tip, m * .05f, skin * .7f); ell(h, tip, {m * .12f, m * .12f, m * .12f}, a.accent, 2.5f, tWhite); }
  else if (hn == 4) { float y = d.ry * .55f, z = sz(d, 0, y); limb(h, {0, y, z * .8f}, {0, y + m, z + m * .45f}, m * .17f, hc, .3f, false, &mCone); }
  else if (hn == 5) for (int s = -1; s <= 1; s += 2) { V3 p0{s * d.rx * .65f, d.ry * .6f, -d.rz * .15f}, p1 = p0 + V3{s * m * .45f, m * .3f, -m * .25f}, p2 = p1 + V3{s * m * .2f, -m * .5f, m * .2f}, p3 = p2 + V3{-s * m * .08f, -m * .2f, m * .3f};
    limb(h, p0, p1, m * .15f, hc); limb(h, p1, p2, m * .12f, hc); limb(h, p2, p3, m * .09f, hc, 0, false, &mCone); }   // curled ram horns
  if (chief) for (int k = 0; k < 5; k++) { float an = k * 1.2566f; V3 b{std::cos(an) * d.rx * .55f, d.top - m * .2f, std::sin(an) * d.rz * .55f}; limb(h, b, b + V3{0, m * .7f, 0}, m * .1f, {1, .75f, .1f}, 1.5f, false, &mCone, tWhite); }
}
void hat(const Ctx& h, const Hd& d, int k, V3 cc, V3 acc) {
  float m = std::min(d.rx, d.ry);
  if (k == 1) part(h, mCone, {0, d.top * .78f, 0}, {d.rx, m * 1.25f, d.rz}, cc, .05f, TXC);   // cone hat
  else if (k == 2) { part(h, mCylN, {0, d.top * .82f, 0}, {d.rx * 1.45f, m * .07f, d.rz * 1.45f}, cc, .05f, TXC); part(h, mCylN, {0, d.top * .82f, 0}, {d.rx * .78f, m * .38f, d.rz * .78f}, cc * .8f, .05f, TXC); }   // disc cap
  else if (k == 3) { float br = d.box ? 1.45f : 1.03f, y = d.ry * .38f; part(h, mCylN, {0, d.ry * .3f, 0}, {d.rx * br, m * .16f, d.rz * br}, cc, .05f, TXC); ell(h, {0, y, (d.box ? d.rz : sz(d, 0, y)) + .01f}, {m * .1f, m * .1f, m * .06f}, acc, 1.5f, tWhite); }   // headband + gem
  else if (k == 4) ell(h, {0, d.ry * .15f, -d.rz * .25f}, {d.rx * 1.1f, d.ry * 1.12f, d.rz}, cc, .05f, TXC);   // hood
}
// skin pattern on the torso ellipsoid (tc, tr); hz = the body lies horizontally
void pattern(const Ctx& c, V3 tc, V3 tr, bool hz, int k, V3 col, V3 skin, int seed) {
  if (k == 1) for (int i = 0; i < 3; i++) { float f = (i - 1) * .42f, q = std::sqrt(1 - f * f) * 1.02f;   // bands
    if (hz) ell(c, tc + V3{0, 0, f * tr.z}, {tr.x * q + .015f, tr.y * q + .015f, tr.z * .07f}, col); else ell(c, tc + V3{0, f * tr.y, 0}, {tr.x * q + .015f, tr.y * .07f, tr.z * q + .015f}, col); }
  else if (k == 2) for (int i = 0; i < 5; i++) { float th = i * 2.4f + (seed % 7), el = (hz ? .3f : -.3f) + .13f * i, r = std::min(tr.x, std::min(tr.y, tr.z)) * .2f;   // spots
    V3 dir{std::cos(el) * std::sin(th), std::sin(el), std::cos(el) * std::cos(th)}; ell(c, tc + V3{dir.x * tr.x, dir.y * tr.y, dir.z * tr.z} * .98f, {r, r, r}, col); }
  else if (k == 3) { V3 bc = skin * 1.4f + V3{.06f, .06f, .05f};   // pale belly
    if (hz) ell(c, tc - V3{0, tr.y * .3f, 0}, {tr.x * .8f, tr.y * .75f, tr.z * .88f}, bc); else ell(c, tc + V3{0, -tr.y * .1f, tr.z * .32f}, {tr.x * .72f, tr.y * .78f, tr.z * .72f}, bc); }
}
void backFx(const Ctx& c, V3 tc, V3 tr, bool hz, int k, V3 skin, V3 acc, float flap) {
  auto bp = [&](float f) { float q = std::sqrt(std::max(0.f, 1 - f * f)) * .95f; return hz ? tc + V3{0, tr.y * q, f * tr.z} : tc + V3{0, f * tr.y, -tr.z * q}; };
  V3 N = hz ? V3{0, 1, -.25f} : V3{0, .25f, -1};
  if (k == 1) for (int i = 0; i < 4; i++) { V3 q = bp(hz ? .5f - i * .35f : .65f - i * .32f); limb(c, q, q + N * (.3f - std::abs(i - 1.5f) * .06f), .07f, acc * .5f + skin * .2f, 0, false, &mCone); }   // spike row
  else if (k == 2) { V3 sc = acc * .35f + skin * .35f; if (hz) ell(c, tc + V3{0, tr.y * .3f, 0}, {tr.x * 1.07f, tr.y * .85f, tr.z * 1.05f}, sc, .05f); else ell(c, tc + V3{0, tr.y * .1f, -tr.z * .45f}, {tr.x * 1.05f, tr.y * .9f, tr.z * .8f}, sc, .05f); }   // shell
  else if (k == 3) for (int s = -1; s <= 1; s += 2) { Ctx w = hz ? sub(c, bp(.35f) + V3{s * tr.x * .3f, 0, 0}, 0, 0, s * (.25f + flap)) : sub(c, bp(.55f) + V3{s * tr.x * .3f, 0, 0}, -s * flap, 0, s * .45f);   // little wings
    ell(w, {s * .38f, 0, hz ? -.15f : -.04f}, hz ? V3{.4f, .03f, .22f} : V3{.4f, .2f, .03f}, acc * .6f + V3{.1f, .1f, .1f}, .35f); }
  else if (k == 4) { if (hz) ell(c, tc + V3{0, tr.y * .95f, 0}, {.03f, .3f, tr.z * .75f}, acc * .7f, .3f); else ell(c, tc + V3{0, tr.y * .15f, -tr.z * .95f}, {.03f, tr.y * .85f, .28f}, acc * .7f, .3f); }   // fin sail
}
void tail(const Ctx& c, V3 r0, int k, V3 skin, V3 acc, V3 tip, float sw) {
  V3 sk = skin * .85f;
  if (k == 1) { V3 p1 = r0 + V3{sw, .15f, -.55f}, p2 = r0 + V3{sw * 1.8f, .35f, -1.f}; limb(c, r0, p1, .07f, sk); limb(c, p1, p2, .05f, sk); ell(c, p2, {.08f, .08f, .08f}, acc, 1.5f, tWhite); }   // whip, glowing tip
  else if (k == 2) limb(c, r0 + V3{0, 0, .1f}, {r0.x + sw * 1.4f, std::max(.08f, r0.y - .6f), r0.z - 1.f}, .17f, sk, 0, false, &mCone);   // thick lizard tail
  else if (k == 3) { limb(c, r0, r0 + V3{sw * .3f, .15f, -.25f}, .07f, sk); ell(c, r0 + V3{sw * .45f, .25f, -.38f}, {.22f, .22f, .22f}, tip); }   // fluffy
  else if (k == 4) { V3 p1 = r0 + V3{sw * .7f, -.1f, -.55f}, p2 = r0 + V3{sw * 1.3f, -.15f, -1.f}; limb(c, r0, p1, .09f, sk); limb(c, p1, p2, .07f, sk); ell(c, p2, {.17f, .15f, .2f}, tip); limb(c, p2, p2 + V3{0, .26f, -.05f}, .06f, acc * .5f, 0, false, &mCone); }   // club
}
}
void drawNpc(const Npc& n, const Planet& p) {
  Alien a = alienOf(p.uniq, p.isMoon); const int B = a.body; const bool W = n.wicked; TXS = tSkin ? tSkin : tWhite; TXC = tCloth ? tCloth : tWhite;
  float sc = (n.chief ? 1.28f : n.elder ? 1.1f : 1.f) * a.size * (B == 1 ? .72f : 1.f); if (n.dweller && !W) sc *= .95f;
  float jit = ((n.look * 37) % 11) / 11.f; V3 skin = W ? V3{.07f, .02f, .03f} + a.skin * .08f : a.skin * (.82f + jit * .36f), cloth = W ? V3{.1f, .015f, .02f} : n.col, legC = W ? V3{.04f, .015f, .02f} : skin * .75f, acc = W ? V3{1, .15f, .08f} : a.accent, pc = W ? V3{.3f, .02f, .02f} : a.skin2;
  const float L = a.legL, G = a.girth, A = a.armL, t = n.ph * FREQ[B] + n.wphase * 2.f, walkK = std::min(1.f, n.sp / 1.4f), idle = 1.f - walkK;
  float br = std::sin(n.ph * 1.8f + n.wphase) * idle, wave = std::sin(n.ph * 2.2f + n.wphase) * (idle * .22f + walkK * .06f);   // breathing, idle arm wave
  float look = std::sin(n.ph * .55f + n.wphase * 3.f) * .6f * idle, nod = 0, lean = 0, roll = 0, yawO = 0, lift = 0, flap = std::sin(t) * .35f * walkK + (std::sin(n.ph * .45f + n.wphase) > .9f ? std::sin(n.ph * 16.f) * .35f : 0.f) * idle;
  auto st = [&](float ph, float amp) { return -std::cos(t + ph) * amp * walkK; };                    // stride offset of a foot with phase ph
  auto lf = [&](float ph, float hg) { return std::max(0.f, std::sin(t + ph)) * hg * walkK; };       // foot lift while it swings forward
  // ---- gait: per species lean, sway, hop, hover, head jerks, sideways scuttle
  switch (B) {
    case 0: case 12: lean = .06f * walkK; break;
    case 1: roll = std::sin(t) * .07f * walkK + std::sin(n.ph * .9f + n.wphase) * .035f * idle; lean = .05f * walkK; break;   // stilts sway
    case 2: lift = std::fabs(std::sin(t)) * .38f * walkK; lean = .1f * walkK; break;                                          // blobs hop
    case 3: nod = std::max(0.f, std::sin(n.ph * .8f + n.wphase) - .3f) * .7f * idle; break;                                  // sniffs the ground
    case 4: lift = .35f + std::sin(n.ph * 1.7f + n.wphase) * .12f; lean = .28f * walkK; roll = std::sin(n.ph * 1.1f + n.wphase) * .07f; break;   // hover and drift
    case 5: look = std::sin(std::floor(n.ph * 1.4f + n.wphase) * 12.99f) * .5f * idle; break;                                // twitchy head
    case 6: roll = std::sin(t) * .08f * walkK + br * .012f; break;                                                           // heavy sway
    case 8: lean = .06f * walkK; look = std::sin(std::floor(n.ph * .9f + n.wphase) * 7.31f) * .7f * idle; nod = std::max(0.f, std::sin(n.ph * 1.3f + n.wphase * 2.f) - .85f) * 3.5f * idle; break;   // jerky looks, pecks
    case 10: roll = std::sin(t) * .13f * walkK + std::sin(n.ph * 1.3f + n.wphase) * .03f * idle; break;                     // waddle
    case 11: yawO = walkK * 1.35f; break;                                                                                     // crabs walk sideways
  }
  { float pd = std::hypot(P.x - n.pos.x, P.z - n.pos.z); if (!W && pd < 9.f) look *= clampf((pd - 6.f) / 3.f, 0, 1); }   // near you, people stop glancing around and look at you
  float yw = n.yaw + yawO; V3 Z0{std::sin(yw), 0, std::cos(yw)}; Ctx c = sub(Ctx{n.pos + V3{0, lift * sc, 0}, {Z0.z, 0, -Z0.x}, {0, 1, 0}, Z0, sc}, {}, 0, lean, roll);
  auto E = [&](V3 q, V3 r, V3 col, float em = 0, GLuint tx = 0) { ell(c, q, r, col, em, tx); };
  auto Lb = [&](V3 q0, V3 q1, float r, V3 col, bool hand = false) { limb(c, q0, q1, r, col, 0, hand); };
  auto Bx = [&](V3 q, V3 s, V3 col) { part(c, mBox, q, s, col); };
  auto legs2 = [&](float hx, float hp, float r, float amp, float hg) { for (int s = -1; s <= 1; s += 2) { float ph = s > 0 ? 0.f : PI, z = st(ph, amp), y = lf(ph, hg); Lb({s * hx, hp, 0}, {s * hx, .07f + y, z}, r, legC); Bx({s * hx, .05f + y, z + .08f}, {r * 2.f, .1f, r * 3.5f}, legC * .6f); } };
  auto arms = [&](float cx, float hw, float y, float z, float drop, float r, float swing, float fw) {   // shoulders at cx +- hw, arms swing against the legs
    for (int s = -1; s <= 1; s += 2) { float ph = s > 0 ? 0.f : PI; Lb({cx + s * hw, y, z}, {cx + s * (hw + .15f), y - drop * A + wave, z - st(ph, swing) + fw}, r, skin, true);
      if (a.arms == 4) Lb({cx + s * hw * .95f, y - .28f, z}, {cx + s * (hw + .2f), y - .28f - drop * .85f * A - wave, z + st(ph, swing) + fw + .08f}, r * .9f, skin, true); } };
  V3 H, tc, tr, r0, pc0, pr; float hr = .3f * a.headS, hip = 0, tw = 0; bool hz = false, own = false, noTail = false; Hd d;
  switch (B) {
    case 0: { hip = .85f * L; float bob = (std::fabs(std::sin(t)) - .6f) * .07f * walkK; legs2(.17f * G, hip + bob, .085f, .32f, .16f);   // upright
      tc = {0, hip + .5f + bob, 0}; tr = {.4f * G, .56f * (1 + br * .04f), .3f * G}; E(tc, tr, skin); arms(0, tr.x * .95f, tc.y + .3f, 0, .65f, .06f, .45f, .1f);
      H = {0, tc.y + tr.y + hr * .55f, .02f}; r0 = {0, tc.y - tr.y * .55f, -tr.z * .9f}; break; }
    case 1: { hip = 1.9f * L; hr = .27f * a.headS; legs2(.13f * G, hip, .06f, .5f, .22f); tc = {0, hip + .32f, 0}; tr = {.27f * G, .42f, .21f * G}; E(tc, tr, skin);   // stilts
      Lb({0, tc.y + .3f, .02f}, {0, tc.y + .85f, .08f}, .07f, skin); arms(0, tr.x * 1.05f, tc.y + .28f, 0, .95f, .045f, .6f, 0);
      H = {0, tc.y + 1.f, .1f}; r0 = {0, tc.y - tr.y * .6f, -tr.z * .9f}; break; }
    case 2: { own = noTail = true; float sq = std::cos(t * 2.f) * .12f * walkK + br * .04f;   // blob: squash on landing, stretch in the air
      tr = {.85f * G * (1 + sq * .6f), .8f * (1 - sq), .78f * G * (1 + sq * .6f)}; tc = {0, tr.y, 0}; E(tc, tr, skin, .18f);
      for (int s = -1; s <= 1; s += 2) { float ph = s > 0 ? 0.f : PI; Lb({s * tr.x * .8f, tr.y * 1.1f, .1f}, {s * (tr.x + .22f), tr.y * (1.1f - .45f * A) + wave, .3f - st(ph, .25f)}, .12f, skin, true); E({s * .32f * G, .06f, .3f + st(ph, .2f)}, {.22f, .09f, .28f}, skin * .8f); }
      H = {0, tr.y * 1.25f, 0}; d = {tr.x * .8f, tr.y * .7f, tr.z * .97f, tr.y * .75f}; break; }
    case 3: { hip = .8f * L; hz = true; float bob = std::fabs(std::sin(t)) * .05f * walkK; tc = {0, hip + .15f + bob, 0}; tr = {.5f * G, .45f * (1 + br * .03f), .95f}; E(tc, tr, skin);   // trotting quadruped
      for (int fx = -1; fx <= 1; fx += 2) for (int fz = -1; fz <= 1; fz += 2) { float ph = fx * fz > 0 ? 0.f : PI, z = fz * .55f + st(ph, .3f), y = lf(ph, .15f); Lb({fx * .28f * G, hip, fz * .55f}, {fx * .28f * G, .07f + y, z}, .1f, legC); E({fx * .28f * G, .07f + y, z + .05f}, {.13f, .09f, .16f}, skin * .8f); }
      Lb({0, tc.y + .15f, .7f}, {0, tc.y + .5f, 1.f}, .17f * G, skin); H = {0, tc.y + .57f, 1.12f}; r0 = {0, tc.y + .1f, -tr.z * .95f}; break; }
    case 4: { own = noTail = true; tc = {0, 1.7f, 0}; tr = {.62f * G, .46f * (1 + br * .05f), .62f * G}; E(tc, tr, skin, .3f);   // floater, tentacles trail behind
      for (int k = 0; k < 5; k++) { float an = k * 1.2566f, sx = std::sin(n.ph * 2.f + k) * .12f; limb(c, {std::cos(an) * .36f * G, tc.y - .3f, std::sin(an) * .36f * G}, {std::cos(an) * .5f * G + sx, .3f, std::sin(an) * .5f * G + sx - walkK * .4f}, .06f, acc * .8f, .4f); }
      for (int s = -1; s <= 1; s += 2) limb(c, {s * tr.x * .93f, tc.y - .1f, .05f}, {s * (tr.x + .2f), tc.y - .1f - .3f * A + wave, .35f - st(s > 0 ? 0.f : PI, .2f)}, .055f, skin, .2f, true);
      H = {0, tc.y + .05f, 0}; d = {tr.x * .8f, tr.y * .8f, tr.z * .98f, tr.y * .95f}; break; }
    case 5: { hip = 1.12f * L; hz = true; hr = .27f * a.headS; float bob = std::sin(t * 2.f) * .02f * walkK; E({0, hip - .05f + bob, -.55f}, {.45f * G, .4f, .62f}, skin); tc = {0, hip + .05f + bob, .15f}; tr = {.38f * G, .34f, .42f}; E(tc, tr, skin * .9f);   // insect, tripod scuttle
      for (int k = 0; k < 3; k++) for (int s = -1; s <= 1; s += 2) { float zk = .35f - k * .45f, ph = (k + (s > 0)) % 2 ? 0.f : PI, z = st(ph, .3f), y = lf(ph, .14f); V3 kn{s * .75f * G, hip * .7f + y, zk + z * .4f}; Lb({s * .25f * G, hip, zk}, kn, .05f, legC); Lb(kn, {s * .98f * G, .04f + y * .5f, zk + z}, .045f, legC); }
      for (int s = -1; s <= 1; s += 2) Lb({s * .2f, tc.y + .15f, .5f}, {s * .34f, tc.y + .3f + wave, .5f + .5f * A}, .05f, skin, true);
      H = {0, tc.y + .05f, .72f}; r0 = {0, hip, -1.12f}; break; }
    case 6: { skin = skin * .55f + V3{.18f, .18f, .2f}; hr = .27f * a.headS; float bob = -std::fabs(std::sin(t)) * .06f * walkK; hip = .84f * L;   // golem: slow stomp
      for (int s = -1; s <= 1; s += 2) { float ph = s > 0 ? 0.f : PI; Bx({s * .3f * G, hip * .5f + lf(ph, .12f), st(ph, .25f)}, {.34f * G, hip, .38f}, skin * .85f); }
      Bx({0, hip + .1f, 0}, {.9f * G, .3f, .5f}, skin * .7f); tc = {0, hip + .75f + bob, 0}; tr = {.48f * G, .48f * (1 + br * .03f), .3f}; Bx(tc, tr * 2.f, skin); E({0, tc.y, tr.z + .01f}, {.2f, .2f, .1f}, acc, 3.2f, tWhite);
      for (int s = -1; s <= 1; s += 2) { float ph = s > 0 ? 0.f : PI; part(c, mCone, {s * (tr.x + .14f), tc.y + .45f, 0}, {.22f, .42f, .22f}, acc, 1.2f, tWhite);
        for (int j = 0; j < (a.arms == 4 ? 2 : 1); j++) { float k = j ? .75f : 1.f; V3 sh{s * (tr.x + .12f), tc.y + .35f - j * .4f, j * .1f}, hd{s * (tr.x + .24f), sh.y - .65f * A + (j ? -wave : wave), -st(ph, .35f) * (j ? -1.f : 1.f) + j * .15f};
          Lb(sh, hd, .14f * k, skin * .9f); Bx(hd - V3{0, .12f, 0}, V3{.3f, .3f, .3f} * k, skin * .8f); } }
      H = {0, tc.y + tr.y + hr * .75f, .02f}; r0 = {0, tc.y - tr.y * .7f, -tr.z}; tw = tr.x + .45f; break; }
    case 7: { own = noTail = true; hip = 1.4f * L; float bob = std::sin(t * 2.f) * .04f * walkK; tc = {0, hip + .6f + bob, -.05f}; tr = {.5f * G, .68f * (1 + br * .04f), .5f * G}; E(tc, tr, skin, .2f);   // tentacle walker
      for (int k = 0; k < 4; k++) { float an = k * 1.5708f + .785f, ph = std::sin(t + k * 1.7f) * walkK * .3f + std::sin(n.ph * 1.5f + k) * idle * .06f, cx = std::cos(an), sx = std::sin(an); V3 ft{cx * .62f + ph * sx, .06f, sx * .62f + ph * cx};
        Lb({cx * .25f, hip, sx * .25f}, ft, .09f, skin * .9f); E(ft, {.12f, .07f, .12f}, skin * .8f); }
      for (int s = -1; s <= 1; s += 2) Lb({s * .35f * G, tc.y - .3f, .2f}, {s * .58f * G, tc.y - .3f - .5f * A + wave, .55f - st(s > 0 ? 0.f : PI, .4f)}, .065f, skin, true);
      H = {0, tc.y - .12f, .05f}; d = {tr.x * .85f, tr.y * .6f, tr.z * .95f, tr.y + .12f}; hip = 0; break; }
    case 8: { hip = L; hr = .26f * a.headS; float bob = (std::fabs(std::sin(t)) - .5f) * .05f * walkK, hb = std::pow(.5f + .5f * std::sin(t * 2.f), 3.f) * .2f * walkK; V3 lg = acc * .4f + V3{.4f, .3f, .06f};   // bird: backward knees, head thrusts each step
      for (int s = -1; s <= 1; s += 2) { float ph = s > 0 ? 0.f : PI, z = st(ph, .3f), y = lf(ph, .16f); V3 hp{s * .15f * G, hip + bob, -.05f}, ft{s * .15f * G, .06f + y, z + .05f}, kn{s * .16f * G, (hp.y + ft.y) * .5f, (hp.z + ft.z) * .5f - .22f};
        Lb(hp, kn, .07f, legC); Lb(kn, ft, .04f, lg); part(c, mBox, ft + V3{0, -.02f, .1f}, {.16f, .04f, .26f}, lg * .8f); }
      tc = {0, hip + .4f + bob, .05f}; tr = {.36f * G, .48f * (1 + br * .04f), .4f * G}; E(tc, tr, skin);
      for (int s = -1; s <= 1; s += 2) { Ctx w = sub(c, {s * tr.x * .9f, tc.y + .25f, -.02f}, 0, 0, s * (.12f + std::fabs(flap) + wave * .3f));   // wing-arms
        ell(w, {s * .05f, -.32f * A, -.08f}, {.07f, .36f * A, .24f}, skin * .85f); ell(w, {s * .04f, -.5f * A, -.22f}, {.05f, .22f * A, .14f}, acc * .6f); ell(w, {s * .06f, -.64f * A, .08f}, {.07f, .07f, .07f}, skin); }
      H = {0, tc.y + tr.y + .22f, .3f + hb}; Lb({0, tc.y + tr.y * .6f, .15f}, H, .09f * G, skin); r0 = {0, tc.y - .1f, -tr.z * .85f}; break; }
    case 9: { noTail = true; float sx = std::sin(t * .5f) * .08f * walkK + std::sin(n.ph * 1.1f + n.wphase) * .1f * idle;   // serpent: body wiggles on the ground, chest raised
      for (int k = 0; k < 6; k++) { float r = (.3f - k * .03f) * G, x = std::sin(k * .9f - t) * (.04f + .05f * k) * walkK + std::sin(k * .7f - n.ph * 1.5f) * .012f * k * idle; E({x, r * .85f, -.1f - k * .4f * L}, {r, r * .85f, r * 1.35f}, k & 1 ? skin * .9f : skin); }
      Lb({0, .25f, .05f}, {sx * .5f, .95f * L, .2f}, .26f * G, skin); tc = {sx * .6f, .95f * L + .22f, .24f}; tr = {.27f * G, .38f * (1 + br * .04f), .24f * G}; E(tc, tr, skin);
      arms(tc.x, tr.x * .9f, tc.y + .2f, tc.z, .5f, .05f, .2f, .3f);
      H = {sx * 1.2f, tc.y + tr.y + .35f, tc.z + .2f}; Lb({tc.x, tc.y + .25f, tc.z}, {H.x, H.y - hr * .5f, H.z}, .13f * G, skin); break; }
    case 10: { hip = .5f * L; hr = .28f * a.headS; legs2(.19f * G, hip, .1f, .18f, .1f); tc = {0, hip + .42f, 0}; tr = {.36f * G, .46f * (1 + br * .05f), .33f * G}; E(tc, tr, skin * 1.1f);   // mushroom folk waddle
      arms(0, tr.x * .9f, tc.y + .18f, 0, .45f, .07f, .25f, .08f); H = {0, tc.y + tr.y + hr * .45f, .02f}; r0 = {0, tc.y - tr.y * .5f, -tr.z * .9f}; break; }
    case 11: { hip = .95f * L; hz = true; hr = .22f * a.headS; float bob = std::sin(t * 2.f) * .025f * walkK; tc = {0, hip + .18f + bob, 0}; tr = {.75f * G, .38f, .62f}; E(tc, tr, skin); tw = tr.x + .6f;   // crab
      for (int k = 0; k < 3; k++) for (int s = -1; s <= 1; s += 2) { float zk = .3f - k * .32f, ph = (k + (s > 0)) % 2 ? 0.f : PI, y = lf(ph, .14f); V3 kn{s * (tr.x + .3f), tc.y + .3f + y, zk * 1.2f};
        Lb({s * tr.x * .75f, tc.y - .08f, zk}, kn, .07f, legC); Lb(kn, {s * (tr.x + .6f) + st(ph, .25f), .04f + y * .4f, zk * 1.5f}, .055f, legC); }
      for (int s = -1; s <= 1; s += 2) { float op = .1f + (.15f + .15f * std::sin(n.ph * 4.f + s)) * idle; V3 el{s * (tr.x * .75f + .15f), tc.y + .25f + wave * .5f, tr.z + .25f * A}, cl = el + V3{0, .05f, .3f * A};   // claws clack when idle
        Lb({s * tr.x * .6f, tc.y + .05f, tr.z * .75f}, el, .08f, skin); E(cl, {.17f, .14f, .27f}, skin * 1.05f); limb(c, cl + V3{0, .06f, .12f}, cl + V3{0, .06f + op, .45f}, .06f, skin * .9f, 0, false, &mCone); }
      H = {0, tc.y + tr.y * .65f, tr.z * .7f}; r0 = {0, tc.y, -tr.z * .95f}; break; }
    default: { hip = .8f * L; hr = .28f * a.headS; float bob = std::fabs(std::sin(t)) * .05f * walkK; pc0 = {0, hip + .12f + bob, -.25f}; pr = {.42f * G, .38f, .78f}; E(pc0, pr, skin * .92f);   // centaur: rotary gallop
      for (int fx = -1; fx <= 1; fx += 2) for (int fz = -1; fz <= 1; fz += 2) { float ph = (fz > 0 ? 0.f : PI) + (fx > 0 ? 0.f : .6f), z0 = pc0.z + fz * .5f, z = z0 + st(ph, .3f), y = lf(ph, .17f);
        Lb({fx * .25f * G, hip + bob, z0}, {fx * .25f * G, .07f + y, z}, .09f, legC); E({fx * .25f * G, .07f + y, z + .05f}, {.12f, .09f, .15f}, legC * .7f); }
      tc = {0, pc0.y + .62f, .38f}; tr = {.32f * G, .45f * (1 + br * .04f), .26f * G}; E(tc, tr, skin); arms(0, tr.x * .95f, tc.y + .27f, tc.z, .62f, .055f, .35f, .1f);
      H = {0, tc.y + tr.y + hr * .55f, tc.z + .03f}; r0 = {0, pc0.y + .1f, pc0.z - pr.z * .95f}; hip = 0; break; }
  }
  if (tw <= 0) tw = std::max(tr.x + .1f, .35f);
  // ---- head (turns and nods on its own) with eyes, mouth, ears, horns, crown and hats
  Ctx h = sub(c, H, look, nod, 0); if (!own) d = headShape(h, a.head, hr, skin);
  if (B == 10) { float cb = std::sin(n.ph * 2.6f + n.wphase) * .04f * hr, cy = d.ry * 1.05f + cb; ell(h, {0, cy, -hr * .05f}, {hr * 2.2f, hr * .72f, hr * 2.2f}, W ? V3{.12f, .02f, .03f} : a.accent * .5f + skin * .25f, .08f);   // mushroom cap with pale spots
    for (int k = 0; k < 4; k++) { float an = k * 1.5708f + .5f; ell(h, {std::cos(an) * hr, cy + hr * .6f, std::sin(an) * hr}, {hr * .25f, hr * .14f, hr * .25f}, W ? V3{.4f, .05f, .05f} : V3{.9f, .88f, .8f}, .1f); }
    d.top = cy + hr * .72f; }
  float blink = std::fmod(n.ph + std::fabs(n.wphase) * 3.f, 3.7f) < .13f ? .12f : 1.f, wob = n.ph * 3.f + n.wphase;
  eyes(h, d, a, skin, W, blink, wob); mouth(h, d, a, skin, W, B == 8 ? nod * .1f : std::max(0.f, std::sin(n.ph * 5.f)) * .08f * idle); ears(h, d, a, skin, std::sin(wob * .7f) * .05f + flap * .1f); horns(h, d, a, skin, W, n.chief, wob);
  if (!W && !n.chief && !n.merchant && B != 10 && a.hat && n.look % 4 != 3) hat(h, d, a.hat, cloth, acc);
  // ---- body features: skin pattern, back, tail (the centaur shows them on its horse body)
  bool cen = B == 12, phz = cen || hz; V3 ptc = cen ? pc0 : tc, ptr = cen ? pr : tr;
  if (a.pat) pattern(c, ptc, B == 6 && a.pat != 2 ? ptr * 1.2f : ptr, phz, a.pat, pc, skin, p.uniq);
  if (a.back) backFx(c, ptc, ptr, phz, a.back, skin, acc, std::fabs(flap) + wave * .5f);
  if (a.tailT && !noTail) tail(c, r0, a.tailT, skin, acc, pc * .5f + skin * .7f, std::sin(n.ph * 2.f + n.wphase) * .2f * idle + std::sin(t) * .25f * walkK);
  // ---- clothing (villagers): blanket on horizontal bodies, otherwise belt / robe / sash / poncho / tunic
  if (!W) {
    if (hz) part(c, mBox, {tc.x, tc.y + tr.y * .8f, tc.z}, {tr.x * 1.2f, .07f, tr.z * .7f}, cloth, .08f, TXC);
    else { int k = a.cloth; if (own && (k == 1 || k == 3 || k == 4)) k = 0; if (k == 1 && !(B == 0 || B == 1 || B == 6 || B == 10)) k = 4; float wy = tc.y - tr.y * .5f;
      if (k == 0 || k == 2) { ell(c, {tc.x, wy, tc.z}, {tr.x * .9f, .07f, tr.z * .9f}, cloth * .8f, .08f, TXC);
        if (k == 0 && !own) part(c, mBox, {tc.x, wy - tr.y * .28f, tc.z + tr.z * .62f}, {tr.x * .55f, tr.y * .5f, .05f}, cloth, .08f, TXC);
        if (k == 2) { float an = .6f, ca = std::cos(an), sa = std::sin(an), R = 1.f / std::sqrt(ca * ca / (tr.x * tr.x) + sa * sa / (tr.y * tr.y)); ell(sub(c, tc, 0, 0, an), {}, {R * 1.04f, .08f, tr.z * 1.04f}, cloth * 1.1f, .1f, TXC); } }
      else if (k == 1) { float y0 = std::max(.15f, hip * .3f), y1 = tc.y - tr.y * .15f; part(c, mCylT, {tc.x, y0, tc.z}, {tr.x * 1.2f, y1 - y0, tr.z * 1.25f}, cloth, .08f, TXC); }
      else if (k == 3) part(c, mCone, {tc.x, tc.y + tr.y * .05f, tc.z}, {tr.x * 1.4f, tr.y, tr.z * 1.45f}, cloth, .08f, TXC);
      else ell(c, {tc.x, tc.y - tr.y * .12f, tc.z}, {tr.x * 1.05f, tr.y * .85f, tr.z * 1.06f}, cloth, .08f, TXC); }
  }
  // ---- roles: wicked cloak + spear, elder staff, merchant pack + hat
  if (W) { V3 ck{.14f, .01f, .02f};
    if (hz) part(c, mBox, {tc.x, tc.y + tr.y * .78f, tc.z}, {tr.x * 1.5f, .08f, tr.z * 1.3f}, ck, 0, TXC);
    else { float y1 = tc.y + tr.y * .75f, y0 = std::max(.15f, hip > 0 ? hip * .4f : cen ? pc0.y : tc.y - tr.y); part(c, mBox, {tc.x, (y0 + y1) * .5f, tc.z - tr.z * .95f}, {tr.x * 2.1f, y1 - y0, .07f}, ck, 0, TXC); }
    if (!n.chief) { float x = -tw - .35f; limb(c, {x, .1f, .35f}, {x, 2.5f, .35f}, .035f, {.1f, .08f, .08f}, 0, false, nullptr, tWhite); limb(c, {x, 2.35f, .35f}, {x, 2.9f, .35f}, .09f, {.7f, .7f, .75f}, .3f, false, &mCone, tWhite); } }
  if (n.elder) { float x = tw + .35f; limb(c, {x, .05f, .3f}, {x, 2.4f, .3f}, .04f, {.45f, .3f, .15f}, 0, false, nullptr, tWhite); ell(c, {x, 2.55f, .3f}, {.15f, .15f, .15f}, a.accent, 3.f, tWhite); }
  if (n.merchant) { if (hz) for (int s = -1; s <= 1; s += 2) part(c, mBox, {tc.x + s * (tr.x + .08f), tc.y + .05f, tc.z}, {.18f, .4f, .5f}, cloth * 1.2f, .15f, TXC);
    else part(c, mBox, {tc.x, tc.y, tc.z - tr.z - .1f}, {.55f, .62f, .3f}, cloth * 1.2f, .15f, TXC);
    part(h, mCylN, {0, d.top * .8f, 0}, {d.rx * .75f, .13f, d.rz * .75f}, a.accent * .6f, .5f, TXC); part(h, mCylN, {0, d.top * .82f, 0}, {d.rx * 1.1f, .03f, d.rz * 1.1f}, a.accent * .6f, .5f, TXC); }
}
