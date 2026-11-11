#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_DECODING
#define MA_NO_GENERATION
#include "miniaudio.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"   // implementation in vorbis.cpp
#include "audio.h"
#include <atomic>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

static const float SR = 44100.f, TAU = 6.2831853f;
static ma_device dev;
static bool devOk = false;
static std::atomic<int> Q[64];
static std::atomic<unsigned> qh{0}, qt{0};
static std::atomic<float> tw{0}, tr{0}, trh{100}, td{0};
static std::atomic<bool> muted{false};

struct Vc { int kind = 0; float f0 = 0, f1 = 0, amp = 0, dur = 1, t = 0, del = 0, ph = 0, i1 = 0, i2 = 0, q = 1; bool on = false; };
static Vc vc[48];
static unsigned rs = 22222;
static float wn() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (rs & 0xffff) / 32768.f - 1.f; }

// ---------------------------------------------------------------- recorded assets (assets/audio), loaded once before the device starts, read-only afterwards
namespace fs = std::filesystem;
struct Smp { std::vector<float> d; int ch = 1; float sr = 44100; size_t frames() const { return d.size() / ch; } };
static const int NKIND = 16, NBODY = 16;
static std::vector<Smp> steps[NKIND];   // footsteps per Planet::kind
static Smp voices[NBODY][VL_COUNT];     // voice lines per Alien::body
struct Track { std::vector<unsigned char> ogg; stb_vorbis* v = nullptr; float g = 0; };
static Track trk[MUS_COUNT];
static bool anyMusic = false;
static std::atomic<int> musReq{MUS_NONE};
static std::atomic<float> volS{1}, volM{1};
static std::atomic<bool> musPaused{false}, cutReq{false};   // cutReq: back to the main menu, every game sound stops at once   // pause menu: music, footsteps and voices fade out and hold their place, then carry on from there (menu clicks still play)
static const float MUSV = .5f, STEPV = .6f, VOICEV = .6f;   // levels of the recordings next to the synthesized sounds (steps peak like the synthesized ones, voices sit above the music)

static fs::path exeDir() {
  std::error_code ec;
#ifdef _WIN32
  wchar_t b[2048]; DWORD n = GetModuleFileNameW(NULL, b, 2048); if (n == 0 || n >= 2048) return {}; return fs::path(b).parent_path();
#elif defined(__APPLE__)
  char b[4096]; uint32_t n = sizeof b; if (_NSGetExecutablePath(b, &n) != 0) return {}; return fs::weakly_canonical(fs::path(b), ec).parent_path();
#else
  fs::path p = fs::read_symlink("/proc/self/exe", ec); return ec ? fs::path() : p.parent_path();
#endif
}
static fs::path findAudioDir() {   // next to the executable (CMake copies it there), or up from it / the working folder (running from the source tree)
  std::vector<fs::path> c; fs::path e = exeDir();
  if (!e.empty()) for (const char* up : {"", "..", "../..", "../../.."}) c.push_back(e / up / "assets" / "audio");
  for (const char* up : {".", "..", "../.."}) c.push_back(fs::path(up) / "assets" / "audio");
  std::error_code ec; for (auto& p : c) if (fs::is_directory(p, ec)) return p;
  return {};
}
static bool readAll(const fs::path& p, std::vector<unsigned char>& out) {
  std::ifstream f(p, std::ios::binary); if (!f) return false;
  f.seekg(0, std::ios::end); std::streamoff n = f.tellg(); if (n <= 0) return false; f.seekg(0);
  out.resize((size_t)n); return (bool)f.read((char*)out.data(), n);
}
static bool loadWav(const fs::path& p, Smp& s) {   // 16-bit PCM WAV
  std::vector<unsigned char> b; if (!readAll(p, b) || b.size() < 12 || std::memcmp(b.data(), "RIFF", 4) || std::memcmp(b.data() + 8, "WAVE", 4)) return false;
  auto u16 = [&](size_t o) { return (unsigned)b[o] | (unsigned)b[o + 1] << 8; };
  auto u32 = [&](size_t o) { return u16(o) | u16(o + 2) << 16; };
  int ch = 0, bits = 0; unsigned sr = 0; size_t o = 12;
  while (o + 8 <= b.size()) {
    size_t n = u32(o + 4), d = o + 8; if (d + n > b.size()) n = b.size() - d;
    if (!std::memcmp(b.data() + o, "fmt ", 4) && n >= 16) { if (u16(d) != 1) return false; ch = (int)u16(d + 2); sr = u32(d + 4); bits = (int)u16(d + 14); }
    else if (!std::memcmp(b.data() + o, "data", 4)) {
      if (bits != 16 || ch < 1 || ch > 2 || !sr) return false;
      s.ch = ch; s.sr = (float)sr; s.d.resize(n / 2);
      for (size_t i = 0; i < s.d.size(); i++) s.d[i] = (short)u16(d + i * 2) / 32768.f;
      return !s.d.empty();
    }
    o = d + n + (n & 1);
  }
  return false;
}
static bool loadOgg(const fs::path& p, Smp& s) {   // whole file decoded to memory (short clips)
  std::vector<unsigned char> b; if (!readAll(p, b)) return false;
  int ch = 0, sr = 0; short* pcm = nullptr; int n = stb_vorbis_decode_memory(b.data(), (int)b.size(), &ch, &sr, &pcm);
  if (n <= 0 || !pcm || ch < 1 || ch > 2) { std::free(pcm); return false; }
  s.ch = ch; s.sr = (float)sr; s.d.resize((size_t)n * ch); for (size_t i = 0; i < s.d.size(); i++) s.d[i] = pcm[i] / 32768.f;
  std::free(pcm); return true;
}
static int numPrefix(const fs::path& p) {   // "07_jungle" -> 7
  std::string s = p.filename().string(); if (s.size() < 3 || !std::isdigit((unsigned char)s[0]) || !std::isdigit((unsigned char)s[1]) || s[2] != '_') return -1;
  return (s[0] - '0') * 10 + (s[1] - '0');
}
static void loadAssets() {
  fs::path root = findAudioDir(); if (root.empty()) { std::fprintf(stderr, "assets/audio not found: using synthesized sounds only\n"); return; }
  std::error_code ec; int nSteps = 0, nVoices = 0, nMusic = 0;
  static const char* MUSF[MUS_COUNT] = {"menu", "space", "planet_verdant", "planet_dunes", "planet_frost", "planet_ember", "planet_anomaly"};
  for (int i = 0; i < MUS_COUNT; i++) {
    Track& t = trk[i]; if (!readAll(root / "music" / (std::string(MUSF[i]) + ".ogg"), t.ogg)) continue;
    int err = 0; t.v = stb_vorbis_open_memory(t.ogg.data(), (int)t.ogg.size(), &err, nullptr);
    if (t.v && stb_vorbis_get_info(t.v).channels > 2) { stb_vorbis_close(t.v); t.v = nullptr; }
    if (t.v) nMusic++; else t.ogg.clear();
  }
  anyMusic = nMusic > 0;
  for (auto& d : fs::directory_iterator(root / "footsteps", ec)) {
    int k = numPrefix(d.path()); if (k < 0 || k >= NKIND || !d.is_directory()) continue;
    std::vector<fs::path> files; for (auto& f : fs::directory_iterator(d.path(), ec)) if (f.path().extension() == ".wav") files.push_back(f.path());
    std::sort(files.begin(), files.end());
    for (auto& f : files) { Smp s; if (loadWav(f, s)) { steps[k].push_back(std::move(s)); nSteps++; } }
  }
  static const char* LINEF[VL_COUNT] = {"hello", "chatter", "question", "laugh", "bye"};
  for (auto& d : fs::directory_iterator(root / "voices", ec)) {
    int b = numPrefix(d.path()); if (b < 0 || b >= NBODY || !d.is_directory()) continue;
    for (int l = 0; l < VL_COUNT; l++) if (loadOgg(d.path() / (std::string(LINEF[l]) + ".ogg"), voices[b][l])) nVoices++;
  }
  std::fprintf(stderr, "audio: %d music tracks, %d footsteps, %d voice lines from %s\n", nMusic, nSteps, nVoices, root.string().c_str());
}

// game thread -> audio thread: recorded one-shots (single producer, single consumer)
struct Cmd { int type = 0; const Smp* s = nullptr; float rate = 1, gain = 1, del = 0; };   // type 0 one-shot, 1 voice (replaces the current one), 2 stop the voice
static Cmd cq[64];
static std::atomic<unsigned> ch_{0}, ct_{0};
static void push(const Cmd& c) { unsigned h = ch_.load(std::memory_order_relaxed); if (h - ct_.load(std::memory_order_acquire) >= 64) return; cq[h % 64] = c; ch_.store(h + 1, std::memory_order_release); }
struct Play { const Smp* s = nullptr; double pos = 0; float rate = 1, gain = 1, del = 0, fade = 1; bool on = false, voice = false, out = false; };
static Play pl[32];
static void startPlay(const Cmd& c, bool voice) {
  for (auto& p : pl) if (!p.on) { p = Play(); p.s = c.s; p.rate = c.rate * c.s->sr / SR; p.gain = c.gain; p.del = c.del; p.voice = voice; p.on = true; return; }
}
static void runCmd(const Cmd& c) {
  if (c.type == 1 || c.type == 2) for (auto& p : pl) if (p.on && p.voice) p.out = true;   // a new line (or a stop) fades the old one out quickly
  if (c.type == 0 && c.s) startPlay(c, false); else if (c.type == 1 && c.s) startPlay(c, true);
}
static unsigned grs = 987654321u;   // game-thread random numbers (never touched by the audio thread)
static float grnd() { grs ^= grs << 13; grs ^= grs >> 17; grs ^= grs << 5; return (grs & 0xffffff) / 16777216.f; }

// ---------------------------------------------------------------- synthesized sounds
// kinds: 0 sine, 1 square, 2 triangle, 3 band-pass noise, 4 low-pass noise, 5 high-pass noise
static void spawn(int kind, float f0, float f1, float amp, float dur, float del = 0, float q = 1) {
  for (auto& v : vc) if (!v.on) { v = Vc(); v.kind = kind; v.f0 = f0; v.f1 = f1; v.amp = amp; v.dur = dur; v.del = del; v.q = q; v.on = true; return; }
}
static void handle(int e) {
  if (e >= 1000) { int k = (e - 1000) / 100, idx = (e - 1000) % 100; float f = 140.f * std::pow(2.f, idx / 24.f); float j = 1.f + wn() * .015f; // a tiny random drift keeps it from sounding like a ringtone
    // levels measured against the ElevenLabs lines, so a species sounds as loud with either kind of voice
    if (k == 0) { const float g = 1.67f; spawn(2, f * j, f * j * 1.06f, .11f * g, .075f); spawn(0, f * 2 * j, f * 2.1f, .025f * g, .05f); }
    else if (k == 1) { const float g = .92f; spawn(0, f * j, f * j * 1.1f, .13f * g, .085f); spawn(2, f * .5f, f * .5f, .04f * g, .06f); }
    else { const float g = 1.5f; spawn(1, f * .55f * j, f * .5f, .075f * g, .09f); spawn(4, f * 1.5f, f, .12f * g, .06f); }
    return; }
  if (e >= 100 && e < 150) {   // footsteps: 100 + surface + 10 * running; landings: 120 + surface; jump: 140
    if (e == 140) { spawn(4, 1100, 350, .1f, .13f); spawn(0, 190, 140, .06f, .08f); return; }
    static bool alt = false; alt = !alt; bool land = e >= 120; int ty = (e - 100) % 10, run = land ? 0 : ((e - 100) / 10) % 2; float j = 1.f + wn() * .1f, a = (alt ? 1.f : .86f) * (run ? 1.3f : 1.f) * (land ? 1.7f : 1.f), L = land ? 1.5f : 1.f, tp = alt ? 1.f : 1.1f;
    switch (ty) {
      case 1: spawn(0, 170 * j * tp, 95, .3f * a, .1f * L); spawn(3, 900 * j, 600, .2f * a, .05f, 0, 1.4f); spawn(3, 2400 * j, 1800, .06f * a, .03f, .045f, 1.2f); break;                     // wooden floor: a knock and a tiny creak
      case 2: spawn(4, 900 * j, 200, .3f * a, .17f * L); spawn(0, 85 * j, 50, .22f * a, .1f); spawn(3, 3200 * j, 2000, .05f * a, .13f, .02f, .6f); break;                               // sand: a long soft hiss
      case 3: spawn(3, 3800 * j, 2600, .2f * a, .045f, 0, 1.6f); spawn(3, 3000 * j, 2200, .17f * a, .05f, .04f, 1.6f); spawn(4, 400 * j, 150, .2f * a, .08f); break;                  // snow: a crunch in two bites
      case 4: spawn(3, 1700 * j, 1100, .25f * a, .035f, 0, 2.5f); spawn(0, 1250 * j * tp, 1150, .07f * a, .13f * L); spawn(0, 120, 70, .2f * a, .07f); break;                           // metal or glass: a tap with a faint ring
      case 5: spawn(3, 1000 * j, 650, .25f * a, .05f, 0, 1.2f); spawn(0, 100 * j * tp, 55, .3f * a, .1f * L); spawn(3, 3000 * j, 2200, .06f * a, .05f, .05f, 1.f); break;               // stone
      default: spawn(0, 100 * j * tp, 52, .32f * a, .1f * L); spawn(4, 700 * j, 220, .3f * a, .085f * L); spawn(3, 2400 * j, 1500, .07f * a, .07f, .012f, .8f); spawn(4, 1200 * j, 400, .08f * a, .07f, .045f); break;   // grass: heel, then toe, plus rustle
    }
    return; }
  switch (e) {
    case S_STEP: { static bool alt = false; alt = !alt; float j = 1.f + wn() * .12f, a = alt ? 1.f : .88f;   // soft heel thump + muffled scuff + a faint grit, slightly different every step
      spawn(0, 105 * j * (alt ? 1.f : 1.12f), 52, .34f * a, .1f); spawn(4, 650 * j, 220, .32f * a, .075f); spawn(3, 2300 * j, 1500, .07f * a, .06f, .012f, .8f); break; }
    case S_CHIME: { float f[4] = {660, 880, 1320, 1760}; for (int i = 0; i < 4; i++) spawn(0, f[i], f[i], .15f, 1.2f, i * .09f); break; }
    case S_CLANG: spawn(1, 220, 110, .12f, .35f); spawn(0, 660, 660, .14f, 1.6f); spawn(5, 5000, 5000, .5f, .25f); break;
    case S_BEEP_LO: spawn(1, 440, 440, .1f, .18f); break;
    case S_BEEP_HI: spawn(1, 880, 880, .1f, .3f); break;
    case S_WHOOSH: spawn(3, 200, 5000, .8f, 2.2f, 0, 1.5f); spawn(0, 110, 25, .4f, 2.f); break;
    case S_DISCOVER: { float f[5] = {392, 494, 587, 784, 988}; for (int i = 0; i < 5; i++) spawn(2, f[i], f[i], .13f, 2.2f, i * .14f); break; }
    case S_JUMP: spawn(4, 500, 150, .3f, .2f); break;
    case S_CLICK: spawn(0, 1250, 760, .09f, .07f); break;
  }
}
void audioEvent(int e) { unsigned h = qh.load(); Q[h % 64] = e; qh.store(h + 1); }
void audioSet(float w, float r, float rh, float d) { tw = w; tr = r; trh = rh; td = d; }
void audioMute(bool m) { muted = m; }
void audioVolumes(float s, float m) { volS = std::max(0.f, std::min(1.f, s)); volM = std::max(0.f, std::min(1.f, m)); }
void audioCut() { cutReq.store(true); }
void audioPauseMusic(bool p) { musPaused.store(p, std::memory_order_relaxed); }
void audioMusic(int t) { musReq.store(t >= 0 && t < MUS_COUNT ? t : MUS_NONE, std::memory_order_relaxed); }
bool audioStep(int kind, bool run, bool land) {
  if (kind < 0 || kind >= NKIND || steps[kind].empty()) return false;
  static int last[NKIND] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const std::vector<Smp>& v = steps[kind]; int n = (int)v.size();
  auto pick = [&]() { int i = (int)(grnd() * n) % n; if (n > 1 && i == last[kind]) i = (i + 1 + (int)(grnd() * (n - 1))) % n; last[kind] = i; return i; };   // never the same step twice in a row
  float g = STEPV * (.85f + grnd() * .15f) * (run ? 1.2f : 1.f) * (land ? 1.35f : 1.f), r = .94f + grnd() * .12f;
  push({0, &v[pick()], land ? r * .92f : r, g, 0});
  if (land) push({0, &v[pick()], r * .88f, g * .8f, .04f});   // both feet hit the ground
  return true;
}
float audioVoice(int body, int line, float rate) {
  if (body < 0 || body >= NBODY || line < 0 || line >= VL_COUNT || rate <= 0) return 0;
  const Smp& s = voices[body][line]; if (s.d.empty()) return 0;
  push({1, &s, rate, VOICEV, 0}); return (float)s.frames() / s.sr / rate;
}
void audioVoiceStop() { push({2, nullptr, 1, 0, 0}); }

struct Svf {
  float i1 = 0, i2 = 0;
  void run(float x, float f, float q, float& lp, float& bp, float& hp) {
    float g = std::tan(3.14159265f * (std::min)(f, 18000.f) / SR), k = 1.f / q, a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
    float v3 = x - i2, v1 = a1 * i1 + a2 * v3, v2 = i2 + a2 * i1 + a3 * v3;
    i1 = 2 * v1 - i1; i2 = 2 * v2 - i2; bp = v1; lp = v2; hp = x - k * v1 - v2;
  }
};
struct Comb { std::vector<float> b; int i = 0; float fb, st = 0;
  Comb(int n, float f) : b(n, 0.f), fb(f) {}
  float run(float x) { float y = b[i]; st = y * .7f + st * .3f; b[i] = x + st * fb; if (++i >= (int)b.size()) i = 0; return y; } };
struct Ap { std::vector<float> b; int i = 0;
  Ap(int n) : b(n, 0.f) {}
  float run(float x) { float y = b[i]; float o = -x + y; b[i] = x + y * .5f; if (++i >= (int)b.size()) i = 0; return o; } };

// music: every track that is fading in or out is decoded block by block (stb_vorbis from memory), looping at its end
static const int BLK = 1024;
static float musL[BLK], musR[BLK], musTmp[BLK * 2];
static void mixMusic(int n, float duck0, float duck1) {   // duck0 -> duck1: the ducking level glides across the block, no zipper noise
  std::fill(musL, musL + n, 0.f); std::fill(musR, musR + n, 0.f);
  static float pg = 1; const bool hold = musPaused.load(std::memory_order_relaxed); const float stepP = 1.f / (SR * .005f);   // 5 ms: silent at once on pause, just long enough not to click
  if (hold && pg <= 0) return;   // paused: nothing is decoded, so every track stays exactly where it was
  const int req = musReq.load(std::memory_order_relaxed); const float stepG = 1.f / (SR * 2.5f);   // 2.5 s crossfades
  for (int k = 0; k < MUS_COUNT; k++) {
    Track& t = trk[k]; if (!t.v) continue; const float tg = k == req ? 1.f : 0.f;
    if (t.g <= 0 && tg <= 0) continue;
    int got = 0, ends = 0;
    while (got < n && ends < 2) { int r = stb_vorbis_get_samples_float_interleaved(t.v, 2, musTmp + got * 2, (n - got) * 2); if (r > 0) got += r; else { stb_vorbis_seek_start(t.v); ends++; } }
    std::fill(musTmp + got * 2, musTmp + n * 2, 0.f);
    for (int i = 0; i < n; i++) {
      if (!hold) t.g = tg > t.g ? (std::min)(tg, t.g + stepG) : (std::max)(tg, t.g - stepG);   // no crossfade while pausing
      const float pgi = (std::max)(0.f, (std::min)(1.f, pg + (hold ? -stepP : stepP) * (i + 1)));
      float g = t.g * t.g * (3 - 2 * t.g) * MUSV * pgi * (duck0 + (duck1 - duck0) * i / n); musL[i] += musTmp[i * 2] * g; musR[i] += musTmp[i * 2 + 1] * g;
    }
    if (t.g <= 0) stb_vorbis_seek_start(t.v);   // faded out: it starts from the top next time
  }
  pg = (std::max)(0.f, (std::min)(1.f, pg + (hold ? -stepP : stepP) * n));
}

static void cb(ma_device*, void* out, const void*, ma_uint32 n) {
  static Svf windF, vF[48];
  static Comb c1(1687, .8f), c2(1601, .8f), c3(2053, .79f), c4(2251, .78f);
  static Ap a1(556), a2(441);
  static float cw = 0, cr = 0, crh = 100, brown = 0, rlp = 0, lfo = 0, rph = 0, tm = 0, gain = 0, duck = 1;
  float* o = (float*)out;
  while (qt.load() != qh.load()) { handle(Q[qt.load() % 64]); qt.store(qt.load() + 1); }
  while (ct_.load(std::memory_order_relaxed) != ch_.load(std::memory_order_acquire)) { unsigned t = ct_.load(std::memory_order_relaxed); runCmd(cq[t % 64]); ct_.store(t + 1, std::memory_order_release); }
  const float dt = 1.f / SR;
  if (cutReq.exchange(false)) {   // leaving the game: music, footsteps, voices, blips, engine and wind are cut, nothing fades out
    for (auto& t : trk) if (t.v) { t.g = 0; stb_vorbis_seek_start(t.v); }
    for (auto& p : pl) p.on = false; for (auto& v : vc) v.on = false; tw = 0; tr = 0; cw = cr = 0; }
  const bool hold = musPaused.load(std::memory_order_relaxed); static float pz = 1;
  bool talking = false; for (auto& p : pl) if (p.on && p.voice && !p.out) talking = true;
  for (ma_uint32 b0 = 0; b0 < n; b0 += BLK) {
    const int bn = (int)(std::min)((ma_uint32)BLK, n - b0);
    const float d0 = duck; duck += ((talking ? .6f : 1.f) - duck) * (std::min)(1.f, bn / (SR * .25f));   // music steps back while an alien talks
    mixMusic(bn, d0, duck);
    for (int bi = 0; bi < bn; bi++) {
      const ma_uint32 s = b0 + bi;
      cw += (tw.load() - cw) * .0004f; cr += (tr.load() - cr) * .0004f; crh += (trh.load() - crh) * .0004f;
      tm += dt; lfo += dt;
      float dry = 0, wet = 0, lp, bp, hp;
      // wind: dry high-band hiss, no slow wave so it never sounds like the sea
      float wf = 2200.f + 400.f * std::sin(tm * 2.4f) + 180.f * std::sin(tm * 5.1f);
      windF.run(wn(), wf, 1.1f, lp, bp, hp); dry += hp * cw * 1.4f * pz;
      // rocket rumble: brown noise + sub sine
      brown = (brown + .02f * wn()) / 1.02f;
      float ka = 1.f - std::exp(-TAU * crh / SR); rlp += ka * (brown * 3.5f - rlp); rph += 45.f * dt; if (rph > 1) rph -= 1;
      dry += (rlp * 1.6f + std::sin(rph * TAU) * .25f) * cr * pz;
      // one-shot voices
      for (int i = 0; i < 48; i++) {
        Vc& v = vc[i]; if (!v.on) continue;
        if (v.del > 0) { v.del -= dt; continue; }
        float t = v.t / v.dur; if (t >= 1.f) { v.on = false; continue; }
        float f = v.f0 == v.f1 ? v.f0 : v.f0 * std::pow(v.f1 / v.f0, t), env = (std::min)(1.f, v.t / .006f) * (1 - t) * (1 - t), x = 0;
        if (v.kind <= 2) { v.ph += f * dt; v.ph -= std::floor(v.ph);
          x = v.kind == 0 ? std::sin(v.ph * TAU) : v.kind == 1 ? (v.ph < .5f ? 1.f : -1.f) * .7f : (std::fabs(v.ph * 4 - 2) - 1.f); }
        else { float l, b, h; vF[i].run(wn(), f, v.q, l, b, h); x = v.kind == 3 ? b : v.kind == 4 ? l : h; }
        x *= env * v.amp; dry += x; wet += x * .45f; v.t += dt;
      }
      // recorded one-shots: footsteps and alien voices (linear interpolation, so the playback rate can vary)
      float pL = 0, pR = 0;
      pz += ((hold ? 0.f : 1.f) - pz) * (std::min)(1.f, dt / .0015f); if (hold && pz < 1e-4f) pz = 0;   // ~5 ms: silent at once, no click
      if (pz > 0) for (auto& p : pl) {
        if (!p.on) continue;
        if (p.del > 0) { p.del -= dt; continue; }
        const Smp& sm = *p.s; size_t fi = (size_t)p.pos, nf = sm.frames(); if (fi + 1 >= nf) { p.on = false; continue; }
        if (p.out) { p.fade -= dt / .06f; if (p.fade <= 0) { p.on = false; continue; } }
        float fr2 = (float)(p.pos - (double)fi), g = p.gain * p.fade, l, r;
        if (sm.ch == 1) { l = r = sm.d[fi] + (sm.d[fi + 1] - sm.d[fi]) * fr2; }
        else { l = sm.d[fi * 2] + (sm.d[fi * 2 + 2] - sm.d[fi * 2]) * fr2; r = sm.d[fi * 2 + 1] + (sm.d[fi * 2 + 3] - sm.d[fi * 2 + 1]) * fr2; }
        g *= pz; pL += l * g; pR += r * g; if (p.voice) wet += (l + r) * g * .08f;
        p.pos += p.rate;
      }
      // reverb
      float rv = (c1.run(wet) + c2.run(wet) + c3.run(wet) + c4.run(wet)) * .25f; rv = a2.run(a1.run(rv));
      // master gain glides to its target so muting never pops; once faded out, the output is exactly zero
      gain += ((muted.load() ? 0.f : .8f) - gain) * .003f; if (gain < 1e-5f && muted.load()) gain = 0;
      float mono = dry + rv * .9f;
      const float vs = volS.load(std::memory_order_relaxed), vm = volM.load(std::memory_order_relaxed);
      o[s * 2] = std::tanh(((mono + pL) * vs + musL[bi] * vm) * gain); o[s * 2 + 1] = std::tanh(((mono + pR) * vs + musR[bi] * vm) * gain);
    }
  }
}
bool audioInit() {
  loadAssets();   // before the device starts: the audio thread only ever reads these
  ma_device_config c = ma_device_config_init(ma_device_type_playback);
  c.playback.format = ma_format_f32; c.playback.channels = 2; c.sampleRate = 44100; c.dataCallback = cb;
  if (ma_device_init(NULL, &c, &dev) != MA_SUCCESS) return false;
  if (ma_device_start(&dev) != MA_SUCCESS) { ma_device_uninit(&dev); return false; }
  devOk = true; return true;
}
void audioShutdown() {
  if (devOk) { ma_device_uninit(&dev); devOk = false; }
  for (auto& t : trk) if (t.v) { stb_vorbis_close(t.v); t.v = nullptr; }
}
