#pragma once
// Math, random numbers and noise used everywhere. No graphics in here.
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

inline const float PI = 3.14159265f;
inline std::string fmt(const char* f, ...) { char b[700]; va_list a; va_start(a, f); vsnprintf(b, sizeof b, f, a); va_end(a); return b; }
struct V3 { float x = 0, y = 0, z = 0; };
inline V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline float len(V3 a) { return std::sqrt(dot(a, a)); }
inline V3 norm(V3 a) { float l = len(a); return l > 0 ? a * (1 / l) : a; }
inline float clampf(float x, float a, float b) { return x < a ? a : x > b ? b : x; }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
struct Rng { unsigned s; float f() { s = s * 1664525u + 1013904223u; return (s >> 8) / 16777216.f; } float r(float a, float b) { return a + (b - a) * f(); } int i(int n) { return (int)(f() * n) % n; } };
inline float Hh(int x, int y, int s) { unsigned n = (unsigned)x * 374761393u + (unsigned)y * 668265263u + (unsigned)s * 2246822519u; n = (n ^ (n >> 13)) * 1274126177u; n ^= n >> 16; return (n & 0xffffff) / 16777216.f; }
inline float vnoise(float x, float y, int s, int p) {
  int xi = (int)std::floor(x), yi = (int)std::floor(y); float u = x - xi, v = y - yi, a = u * u * (3 - 2 * u), b = v * v * (3 - 2 * v);
  auto w = [&](int k) { return ((k % p) + p) % p; }; int x0 = w(xi), x1 = w(xi + 1), y0 = w(yi), y1 = w(yi + 1);
  return Hh(x0, y0, s) * (1 - a) * (1 - b) + Hh(x1, y0, s) * a * (1 - b) + Hh(x0, y1, s) * (1 - a) * b + Hh(x1, y1, s) * a * b;
}
inline float fbm(float x, float y, int s, int o = 5, int p = 1 << 16) { float a = .5f, f = 1, t = 0; for (int i = 0; i < o; i++) { t += a * vnoise(x * f, y * f, s + i, p * (int)f); f *= 2; a *= .5f; } return t; }
inline float cells(float x, float y, int s) {
  int xi = (int)std::floor(x), yi = (int)std::floor(y); float md = 8;
  for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) {
    float hx = Hh(xi + i, yi + j, s), hy = Hh(xi + i, yi + j, s + 17);
    float px = xi + i + hx - x, py = yi + j + hy - y; md = std::min(md, px * px + py * py);
  }
  return std::sqrt(md);
}
inline float ridge(float x, float y, int s) { float n = fbm(x, y, s, 5); return 1.f - std::fabs(n * 2 - 1); }
inline float pfeat(int pat, float u, float v, int s, float sc) {
  pat = ((pat % 16) + 16) % 16; float x = u * sc, y = v * sc * .5f;
  if (pat == 0) return clampf((fbm(x, y, s, 6, std::max(4, (int)sc)) - .22f) * 1.9f, 0, 1);
  if (pat == 1) { float b = 3.5f + (s % 9); return .5f + .5f * std::sin(v * b * 6.283f + fbm(u * 6, v * 6, s + 9, 4, 6) * 7); }
  if (pat == 2) { float cr = cells(u * sc * 1.7f, v * sc * .85f, s); return clampf(fbm(x, y, s, 4) * .45f + clampf(cr * 2.4f - .25f, 0, 1) * .55f, 0, 1); }
  if (pat == 3) { float w = fbm(u * 3, v * 2, s, 5); return clampf(.5f + .5f * std::sin(u * (10 + sc) + w * 8) * std::sin(v * 8 + w * 6), 0, 1); }
  if (pat == 4) { float c = cells(u * 7, v * 3.5f, s); return clampf(1.f - std::fabs(c - .35f) * 3.f, 0, 1) * .5f + fbm(x, y, s, 3) * .5f; }
  if (pat == 5) { float r = ridge(u * sc, v * sc * .5f, s); return clampf(r * .72f + fbm(x * 2, y * 2, s + 2, 3) * .28f, 0, 1); }
  if (pat == 6) { float h = fbm(x, y, s, 6); return h > .48f ? clampf((h - .48f) * 2.6f, 0, 1) : h * .12f; }
  if (pat == 7) return clampf(.5f + .5f * std::sin(u * sc * 8 + fbm(u * 4, v * 2, s, 3) * 4) * (.55f + .45f * fbm(x, y, s + 1, 3)), 0, 1);
  if (pat == 8) { float c = cells(u * 5, v * 2.5f, s); return clampf(1.f - c * 1.8f, 0, 1); }
  if (pat == 9) { float c = cells(u * 9, v * 4.5f, s); return clampf(fbm(x, y, s, 4) * .4f + (c < .16f ? .85f : 0.f), 0, 1); }
  if (pat == 10) { float g = (std::fabs(std::fmod(u * 24, 1.f) - .5f) < .035f || std::fabs(std::fmod(v * 12, 1.f) - .5f) < .035f) ? 1.f : 0.f; return clampf(fbm(x, y, s, 4) * .48f + g * .52f, 0, 1); }
  if (pat == 11) { float split = .5f + .12f * std::sin(u * 18); return v < split ? fbm(x, y, s, 5) * .45f : .55f + fbm(x, y, s + 4, 5) * .45f; }
  if (pat == 12) { float d = std::hypot(u - .5f, (v - .5f) * 2.f); return clampf(.5f + .5f * std::sin(d * (9 + (s % 8)) + fbm(u * 5, v * 5, s, 3) * 4), 0, 1); }
  if (pat == 13) { float c = cells(u * 4, v * 2, s); return clampf(std::fabs(c - .4f) * 2.5f * fbm(x, y, s, 4), 0, 1); }
  if (pat == 14) { float n = fbm(u * 2.5f, v * 1.2f, s, 5); return clampf(std::fabs(std::sin(n * 18.f)), 0, 1); }
  return clampf(fbm(u * (4 + sc * .3f) + fbm(u * 3, v * 2, s + 3, 3) * 2, v * (2 + sc * .2f), s, 6), 0, 1);
}
typedef std::array<float, 4> C4;
inline C4 mixc(C4 a, C4 b, float t) { return {lerpf(a[0], b[0], t), lerpf(a[1], b[1], t), lerpf(a[2], b[2], t), 255}; }
inline C4 hsl(float h, float s, float l) {
  h -= std::floor(h); s = clampf(s, 0, 1); l = clampf(l, 0, 1);
  auto f = [&](float n) { float k = std::fmod(n + h * 12, 12.f), a = s * std::min(l, 1 - l); return (l - a * std::max(-1.f, std::min(std::min(k - 3, 9 - k), 1.f))) * 255; };
  return {f(0), f(8), f(4), 255};
}
