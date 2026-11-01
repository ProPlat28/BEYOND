#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_DECODING
#define MA_NO_GENERATION
#include "miniaudio.h"
#include "audio.h"
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdlib>
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

// kinds: 0 sine, 1 square, 2 triangle, 3 band-pass noise, 4 low-pass noise, 5 high-pass noise
static void spawn(int kind, float f0, float f1, float amp, float dur, float del = 0, float q = 1) {
  for (auto& v : vc) if (!v.on) { v = Vc(); v.kind = kind; v.f0 = f0; v.f1 = f1; v.amp = amp; v.dur = dur; v.del = del; v.q = q; v.on = true; return; }
}
static void handle(int e) {
  switch (e) {
    case S_STEP: spawn(3, 1400 + (wn() + 1) * 450, 1400, .5f, .11f, 0, .7f); spawn(4, 180, 180, .6f, .1f); break;
    case S_CHIME: { float f[4] = {660, 880, 1320, 1760}; for (int i = 0; i < 4; i++) spawn(0, f[i], f[i], .15f, 1.2f, i * .09f); break; }
    case S_CLANG: spawn(1, 220, 110, .12f, .35f); spawn(0, 660, 660, .14f, 1.6f); spawn(5, 5000, 5000, .5f, .25f); break;
    case S_BEEP_LO: spawn(1, 440, 440, .1f, .18f); break;
    case S_BEEP_HI: spawn(1, 880, 880, .1f, .3f); break;
    case S_WHOOSH: spawn(3, 200, 5000, .8f, 2.2f, 0, 1.5f); spawn(0, 110, 25, .4f, 2.f); break;
    case S_DISCOVER: { float f[5] = {392, 494, 587, 784, 988}; for (int i = 0; i < 5; i++) spawn(2, f[i], f[i], .13f, 2.2f, i * .14f); break; }
    case S_JUMP: spawn(4, 500, 150, .3f, .2f); break;
  }
}
void audioEvent(int e) { unsigned h = qh.load(); Q[h % 64] = e; qh.store(h + 1); }
void audioSet(float w, float r, float rh, float d) { tw = w; tr = r; trh = rh; td = d; }
void audioMute(bool m) { muted = m; }

struct Svf {
  float i1 = 0, i2 = 0;
  void run(float x, float f, float q, float& lp, float& bp, float& hp) {
    float g = std::tan(3.14159265f * std::min(f, 18000.f) / SR), k = 1.f / q, a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
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

static void cb(ma_device*, void* out, const void*, ma_uint32 n) {
  static Svf windF, vF[48];
  static Comb c1(1687, .8f), c2(1601, .8f), c3(2053, .79f), c4(2251, .78f);
  static Ap a1(556), a2(441);
  static float cw = 0, cr = 0, crh = 100, cd = 0, brown = 0, rlp = 0, dlp = 0, dph[4] = {0, 0, 0, 0}, lfo = 0, rph = 0, tm = 0;
  float* o = (float*)out;
  while (qt.load() != qh.load()) { handle(Q[qt.load() % 64]); qt.store(qt.load() + 1); }
  const float dt = 1.f / SR;
  for (ma_uint32 s = 0; s < n; s++) {
    cw += (tw.load() - cw) * .0004f; cr += (tr.load() - cr) * .0004f; crh += (trh.load() - crh) * .0004f; cd += (td.load() - cd) * .0002f;
    tm += dt; lfo += dt;
    float dry = 0, wet = 0, lp, bp, hp;
    // wind
    float wf = 600.f + 250.f * std::sin(tm * .5f) + 120.f * std::sin(tm * 1.7f);
    windF.run(wn(), wf, .5f, lp, bp, hp); dry += bp * cw * 2.2f;
    // rocket rumble: brown noise + sub sine
    brown = (brown + .02f * wn()) / 1.02f;
    float ka = 1.f - std::exp(-TAU * crh / SR); rlp += ka * (brown * 3.5f - rlp); rph += 45.f * dt; if (rph > 1) rph -= 1;
    dry += (rlp * 1.6f + std::sin(rph * TAU) * .25f) * cr;
    // space drone
    float df = 380.f + 220.f * std::sin(TAU * .07f * tm), fr[4] = {55.f, 82.5f, 110.4f, 164.9f}, dsum = 0;
    for (int i = 0; i < 4; i++) { dph[i] += fr[i] * (1.f + (i - 1.5f) * .004f) * dt; if (dph[i] > 1) dph[i] -= 1; dsum += (i % 2 ? (dph[i] * 2 - 1) : std::sin(dph[i] * TAU)) * .25f; }
    float dk = 1.f - std::exp(-TAU * df / SR); dlp += dk * (dsum - dlp);
    dry += dlp * cd; wet += dlp * cd * .6f;
    // one-shot voices
    for (int i = 0; i < 48; i++) {
      Vc& v = vc[i]; if (!v.on) continue;
      if (v.del > 0) { v.del -= dt; continue; }
      float t = v.t / v.dur; if (t >= 1.f) { v.on = false; continue; }
      float f = v.f0 == v.f1 ? v.f0 : v.f0 * std::pow(v.f1 / v.f0, t), env = std::min(1.f, v.t / .006f) * (1 - t) * (1 - t), x = 0;
      if (v.kind <= 2) { v.ph += f * dt; v.ph -= std::floor(v.ph);
        x = v.kind == 0 ? std::sin(v.ph * TAU) : v.kind == 1 ? (v.ph < .5f ? 1.f : -1.f) * .7f : (std::fabs(v.ph * 4 - 2) - 1.f); }
      else { float l, b, h; vF[i].run(wn(), f, v.q, l, b, h); x = v.kind == 3 ? b : v.kind == 4 ? l : h; }
      x *= env * v.amp; dry += x; wet += x * .45f; v.t += dt;
    }
    // reverb
    float r = (c1.run(wet) + c2.run(wet) + c3.run(wet) + c4.run(wet)) * .25f; r = a2.run(a1.run(r));
    float m = muted.load() ? 0.f : .8f;
    float y = std::tanh((dry + r * .9f) * m);
    o[s * 2] = y; o[s * 2 + 1] = y;
  }
}
bool audioInit() {
  ma_device_config c = ma_device_config_init(ma_device_type_playback);
  c.playback.format = ma_format_f32; c.playback.channels = 2; c.sampleRate = 44100; c.dataCallback = cb;
  if (ma_device_init(NULL, &c, &dev) != MA_SUCCESS) return false;
  if (ma_device_start(&dev) != MA_SUCCESS) { ma_device_uninit(&dev); return false; }
  devOk = true; return true;
}
void audioShutdown() { if (devOk) { ma_device_uninit(&dev); devOk = false; } }
