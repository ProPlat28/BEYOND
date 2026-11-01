// BEYOND - build the rocket, leave Earth, explore a universe where every world is different.
// C++20 / OpenGL 3.3 / GLFW / GLEW. All textures and sounds are generated procedurally.
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <functional>
#include <string>
#include <vector>
#include "stb_easy_font.h"
#include "audio.h"

static const float PI = 3.14159265f;
// ---------------------------------------------------------------- math
struct V3 { float x = 0, y = 0, z = 0; };
static V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
static float len(V3 a) { return std::sqrt(dot(a, a)); }
static V3 norm(V3 a) { float l = len(a); return l > 0 ? a * (1 / l) : a; }
static float clampf(float x, float a, float b) { return x < a ? a : x > b ? b : x; }
static float lerpf(float a, float b, float t) { return a + (b - a) * t; }
struct Rng { unsigned s; float f() { s = s * 1664525u + 1013904223u; return (s >> 8) / 16777216.f; } float r(float a, float b) { return a + (b - a) * f(); } int i(int n) { return (int)(f() * n) % n; } };

static float Hh(int x, int y, int s) { unsigned n = (unsigned)x * 374761393u + (unsigned)y * 668265263u + (unsigned)s * 2246822519u; n = (n ^ (n >> 13)) * 1274126177u; n ^= n >> 16; return (n & 0xffffff) / 16777216.f; }
static float vnoise(float x, float y, int s, int p) {
  int xi = (int)std::floor(x), yi = (int)std::floor(y); float u = x - xi, v = y - yi, a = u * u * (3 - 2 * u), b = v * v * (3 - 2 * v);
  auto w = [&](int k) { return ((k % p) + p) % p; }; int x0 = w(xi), x1 = w(xi + 1), y0 = w(yi), y1 = w(yi + 1);
  return Hh(x0, y0, s) * (1 - a) * (1 - b) + Hh(x1, y0, s) * a * (1 - b) + Hh(x0, y1, s) * (1 - a) * b + Hh(x1, y1, s) * a * b;
}
static float fbm(float x, float y, int s, int o = 5, int p = 1 << 16) { float a = .5f, f = 1, t = 0; for (int i = 0; i < o; i++) { t += a * vnoise(x * f, y * f, s + i, p * (int)f); f *= 2; a *= .5f; } return t; }

// ---------------------------------------------------------------- textures
typedef std::array<float, 4> C4;
static GLuint mkTex(int w, int h, const std::function<C4(float, float)>& fn, bool rep = true) {
  std::vector<unsigned char> d((size_t)w * h * 4);
  for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) { C4 c = fn((x + .5f) / w, (y + .5f) / h); for (int k = 0; k < 4; k++) d[((size_t)y * w + x) * 4 + k] = (unsigned char)clampf(c[k], 0, 255); }
  GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, d.data()); glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, rep ? GL_REPEAT : GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, rep ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.f);
  return t;
}
static C4 mixc(C4 a, C4 b, float t) { return {lerpf(a[0], b[0], t), lerpf(a[1], b[1], t), lerpf(a[2], b[2], t), 255}; }
static C4 hsl(float h, float s, float l) {
  h -= std::floor(h); s = clampf(s, 0, 1); l = clampf(l, 0, 1);
  auto f = [&](float n) { float k = std::fmod(n + h * 12, 12.f), a = s * std::min(l, 1 - l); return (l - a * std::max(-1.f, std::min(std::min(k - 3, 9 - k), 1.f))) * 255; };
  return {f(0), f(8), f(4), 255};
}

// ---------------------------------------------------------------- meshes
struct Mesh { GLuint vao = 0, vbo = 0, ebo = 0; int n = 0; };
static Mesh upload(const std::vector<float>& v, const std::vector<unsigned>& ix) {
  Mesh m; m.n = (int)ix.size(); glGenVertexArrays(1, &m.vao); glBindVertexArray(m.vao);
  glGenBuffers(1, &m.vbo); glBindBuffer(GL_ARRAY_BUFFER, m.vbo); glBufferData(GL_ARRAY_BUFFER, v.size() * 4, v.data(), GL_STATIC_DRAW);
  glGenBuffers(1, &m.ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, ix.size() * 4, ix.data(), GL_STATIC_DRAW);
  for (int i = 0; i < 3; i++) { glEnableVertexAttribArray(i); glVertexAttribPointer(i, i == 2 ? 2 : 3, GL_FLOAT, GL_FALSE, 32, (void*)(size_t)(i == 0 ? 0 : i == 1 ? 12 : 24)); }
  return m;
}
static void pv(std::vector<float>& v, V3 p, V3 n, float u, float t) { v.insert(v.end(), {p.x, p.y, p.z, n.x, n.y, n.z, u, t}); }
static Mesh mkSphere(int sg, int rg) {
  std::vector<float> v; std::vector<unsigned> ix;
  for (int j = 0; j <= rg; j++) { float ph = PI * j / rg; for (int i = 0; i <= sg; i++) { float th = 2 * PI * i / sg; V3 p{std::sin(ph) * std::cos(th), std::cos(ph), std::sin(ph) * std::sin(th)}; pv(v, p, p, (float)i / sg, (float)j / rg); } }
  for (int j = 0; j < rg; j++) for (int i = 0; i < sg; i++) { unsigned a = j * (sg + 1) + i, b = a + sg + 1; ix.insert(ix.end(), {a, b, a + 1, a + 1, b, b + 1}); }
  return upload(v, ix);
}
// cylinder / cone: origin at the base, height along +Y
static Mesh mkCyl(float r0, float r1, float h, int sg, bool caps) {
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
static Mesh mkBox() {
  std::vector<float> v; std::vector<unsigned> ix; V3 N[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  for (int f = 0; f < 6; f++) { V3 n = N[f], u = norm(cross(n, std::fabs(n.y) > .5f ? V3{1, 0, 0} : V3{0, 1, 0})), w = cross(n, u); unsigned b = (unsigned)(v.size() / 8);
    for (int q = 0; q < 4; q++) { float sx = (q & 1) ? 1.f : -1.f, sy = (q & 2) ? 1.f : -1.f; pv(v, n * .5f + u * (sx * .5f) + w * (sy * .5f), n, (sx + 1) / 2, (sy + 1) / 2); }
    ix.insert(ix.end(), {b, b + 1, b + 2, b + 2, b + 1, b + 3}); }
  return upload(v, ix);
}
static Mesh mkRing(float inner, int sg) {
  std::vector<float> v; std::vector<unsigned> ix;
  for (int i = 0; i <= sg; i++) { float th = 2 * PI * i / sg, c = std::cos(th), s = std::sin(th); pv(v, {c * inner, 0, s * inner}, {0, 1, 0}, 0, 0); pv(v, {c, 0, s}, {0, 1, 0}, 1, 1); }
  for (int i = 0; i < sg; i++) { unsigned a = 2 * i; ix.insert(ix.end(), {a, a + 1, a + 2, a + 1, a + 3, a + 2}); }
  return upload(v, ix);
}
static float hAt(float x, float z) { return (fbm(x * .012f + 500, z * .012f + 500, 3, 4) - .45f) * 38.f * clampf((std::hypot(x, z) - 25.f) / 50.f, 0, 1); }
static Mesh mkTerrain(int n, float S) {
  std::vector<float> v; std::vector<unsigned> ix; const float e = .8f;
  for (int i = 0; i <= n; i++) for (int j = 0; j <= n; j++) { float x = ((float)j / n - .5f) * S, z = ((float)i / n - .5f) * S;
    V3 nn = norm({hAt(x - e, z) - hAt(x + e, z), 2 * e, hAt(x, z - e) - hAt(x, z + e)}); pv(v, {x, hAt(x, z), z}, nn, (float)j / n, (float)i / n); }
  for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) { unsigned a = i * (n + 1) + j, b = a + n + 1; ix.insert(ix.end(), {a, b, a + 1, a + 1, b, b + 1}); }
  return upload(v, ix);
}

// ---------------------------------------------------------------- shaders
static GLuint compile(const char* vs, const char* fs) {
  auto sh = [](GLenum t, const char* s) { GLuint o = glCreateShader(t); glShaderSource(o, 1, &s, nullptr); glCompileShader(o); GLint ok; glGetShaderiv(o, GL_COMPILE_STATUS, &ok);
    if (!ok) { char l[2048]; glGetShaderInfoLog(o, 2048, nullptr, l); std::fprintf(stderr, "shader error: %s\n", l); } return o; };
  GLuint p = glCreateProgram(), a = sh(GL_VERTEX_SHADER, vs), b = sh(GL_FRAGMENT_SHADER, fs); glAttachShader(p, a); glAttachShader(p, b); glLinkProgram(p); return p;
}
static const char* VS_MAIN = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; layout(location=2) in vec2 aT;
uniform mat4 uVP, uM; uniform float uUVS; out vec3 vN; out vec2 vT; out vec3 vR;
void main(){ vec4 w=uM*vec4(aP,1.0); vR=w.xyz; vN=mat3(uM)*aN; vT=aT*uUVS; gl_Position=uVP*w; })";
static const char* FS_MAIN = R"(#version 330 core
in vec3 vN; in vec2 vT; in vec3 vR; out vec4 oC;
uniform sampler2D uTex; uniform vec3 uSunDir,uSunCol,uAmb,uTint,uFogCol; uniform float uFog,uEmis,uAlpha; uniform int uMode;
void main(){
  vec4 t=texture(uTex,vT); vec3 N=normalize(vN), V=normalize(-vR);
  if(uMode==1){ float f=pow(1.0-abs(dot(N,V)),2.5); oC=vec4(uTint*f*uEmis,1.0); return; }
  if(uMode==2){ oC=vec4(t.rgb*uTint,t.a*uAlpha); return; }
  float d=max(dot(N,uSunDir),0.0);
  vec3 c=t.rgb*uTint*(uAmb+uSunCol*d)+t.rgb*uTint*uEmis;
  c=1.0-exp(-c*1.25);
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
static GLFWwindow* win; static GLFWmonitor* mon; static int FW = 1280, FH = 720;
static bool fs = false, paused = false; static int wx = 100, wy = 100, ww = 1280, wh = 720;
static double mdx = 0, mdy = 0;
static GLuint pMain, pPt, pHud;
static GLint uVP, uM, uUVS, uTex, uSunDir, uSunCol, uAmb, uTint, uFogCol, uFog, uEmis, uAlpha, uMode, uPtVP, uPtScale, uPtFade, uHO, uHScr, uHK, uHCol;
static V3 camPos, cR{1, 0, 0}, cU{0, 1, 0}, cF{0, 0, -1}; static float fov = 72, nearZ = .3f, VP[16];
static V3 sunDir = norm({.5f, .45f, .35f}), sunCol{1, .93f, .8f}, ambient{.3f, .36f, .45f}, fogCol{.28f, .55f, 1.f}; static float fogDen = .0035f;
static Mesh mSphere, mSphereLo, mCyl, mCylT, mCylN, mCone, mBox, mTerrain, mRing;
static GLuint tGrass, tBark, tLeaf, tRock, tConc, tHull, tWhite;

static void basis(float yaw, float pitch, float roll, V3& r, V3& u, V3& f) {
  f = {-std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)}; V3 r0{std::cos(yaw), 0, -std::sin(yaw)}, u0 = cross(r0, f);
  float c = std::cos(roll), s = std::sin(roll); r = r0 * c + u0 * s; u = u0 * c - r0 * s;
}
static void lookAt(V3 from, V3 to) { cF = norm(to - from); cR = norm(cross(cF, {0, 1, 0})); cU = cross(cR, cF); }
static void makeVP() {
  float a = (float)FW / FH, f = 1 / std::tan(fov * PI / 360), n = nearZ, fa = 80000.f;
  float P[16] = {f / a, 0, 0, 0, 0, f, 0, 0, 0, 0, (fa + n) / (n - fa), -1, 0, 0, 2 * fa * n / (n - fa), 0};
  float V[16] = {cR.x, cU.x, -cF.x, 0, cR.y, cU.y, -cF.y, 0, cR.z, cU.z, -cF.z, 0, 0, 0, 0, 1};
  for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) { float s = 0; for (int k = 0; k < 4; k++) s += P[k * 4 + r] * V[c * 4 + k]; VP[c * 4 + r] = s; }
}
struct DP { V3 p; V3 s{1, 1, 1}; V3 X{1, 0, 0}, Y{0, 1, 0}, Z{0, 0, 1}; V3 t{1, 1, 1}; float emis = 0, uvs = 1, alpha = 1; int mode = 0; GLuint tex = 0; };
static void draw(const Mesh& m, const DP& d) {
  V3 rel = d.p - camPos; V3 X = d.X * d.s.x, Y = d.Y * d.s.y, Z = d.Z * d.s.z;
  float M[16] = {X.x, X.y, X.z, 0, Y.x, Y.y, Y.z, 0, Z.x, Z.y, Z.z, 0, rel.x, rel.y, rel.z, 1};
  glUniformMatrix4fv(uM, 1, GL_FALSE, M); glUniform3f(uTint, d.t.x, d.t.y, d.t.z); glUniform1f(uEmis, d.emis); glUniform1f(uUVS, d.uvs); glUniform1f(uAlpha, d.alpha); glUniform1i(uMode, d.mode);
  glBindTexture(GL_TEXTURE_2D, d.tex ? d.tex : tWhite); glBindVertexArray(m.vao); glDrawElements(GL_TRIANGLES, m.n, GL_UNSIGNED_INT, nullptr);
}

// ---------------------------------------------------------------- point sprites (stars, flames, beams, sun glow)
struct PT { float x, y, z, size, r, g, b, a; };
static GLuint ptVAO, ptVBO, starVAO, starVBO; static int starN = 0;
static void setupPoints() {
  auto cfg = [](GLuint& vao, GLuint& vbo) { glGenVertexArrays(1, &vao); glBindVertexArray(vao); glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (void*)0); glEnableVertexAttribArray(1); glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 32, (void*)12);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 32, (void*)16); };
  cfg(ptVAO, ptVBO); cfg(starVAO, starVBO);
  std::vector<PT> s; Rng r{5};
  for (int i = 0; i < 6000; i++) { float a = r.r(0, 2 * PI), z = r.r(-1, 1), q = std::sqrt(1 - z * z), k = .35f + r.f() * .65f, t = r.f();
    s.push_back({std::cos(a) * q * 30000.f, z * 30000.f, std::sin(a) * q * 30000.f, -(1.4f + r.f() * 1.8f), k * (.7f + .3f * t), k * .85f, k * (1 - .3f * t), 1.f}); }
  starN = (int)s.size(); glBindBuffer(GL_ARRAY_BUFFER, starVBO); glBufferData(GL_ARRAY_BUFFER, s.size() * sizeof(PT), s.data(), GL_STATIC_DRAW);
}
static void drawPoints(GLuint vao, GLuint vbo, const PT* d, int n, float fade) {
  if (n <= 0) return; glBindVertexArray(vao); if (d) { glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, n * sizeof(PT), d, GL_STREAM_DRAW); }
  glUniform1f(uPtFade, fade); glDrawArrays(GL_POINTS, 0, n);
}

// ---------------------------------------------------------------- HUD text
static GLuint hVAO, hVBO, hEBO; static char sbuf[60000];
static void setupHud() {
  glGenVertexArrays(1, &hVAO); glBindVertexArray(hVAO); glGenBuffers(1, &hVBO); glBindBuffer(GL_ARRAY_BUFFER, hVBO); glBufferData(GL_ARRAY_BUFFER, sizeof sbuf, nullptr, GL_DYNAMIC_DRAW);
  std::vector<unsigned> ix; for (unsigned q = 0; q < sizeof sbuf / 64; q++) ix.insert(ix.end(), {q * 4, q * 4 + 1, q * 4 + 2, q * 4, q * 4 + 2, q * 4 + 3});
  glGenBuffers(1, &hEBO); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hEBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, ix.size() * 4, ix.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, (void*)0);
}
static void hudColor(V3 c, float a) { glUniform4f(uHCol, c.x, c.y, c.z, a); }
static void text(const std::string& s, float x, float y, float sc, V3 c, float a = 1) {
  if (a <= 0 || s.empty()) return; int n = stb_easy_font_print(0, 0, (char*)s.c_str(), nullptr, sbuf, sizeof sbuf);
  glBindVertexArray(hVAO); glBindBuffer(GL_ARRAY_BUFFER, hVBO); glBufferSubData(GL_ARRAY_BUFFER, 0, n * 64, sbuf); glUniform1f(uHK, sc);
  for (int pass = 0; pass < 2; pass++) { float o = pass ? 0.f : sc * 1.3f; glUniform2f(uHO, x + o, y + o); hudColor(pass ? c : V3{0, 0, 0}, pass ? a : a * .8f); glDrawElements(GL_TRIANGLES, n * 6, GL_UNSIGNED_INT, nullptr); }
}
static void rect(float x, float y, float w, float h, V3 c, float a) {
  float q[16] = {x, y, 0, 0, x + w, y, 0, 0, x + w, y + h, 0, 0, x, y + h, 0, 0}; glBindVertexArray(hVAO); glBindBuffer(GL_ARRAY_BUFFER, hVBO); glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof q, q);
  glUniform1f(uHK, 1); glUniform2f(uHO, 0, 0); hudColor(c, a); glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
static void textC(const std::string& s, float cx, float y, float sc, V3 c, float a = 1) { text(s, cx - stb_easy_font_width((char*)s.c_str()) * sc * .5f, y, sc, c, a); }

// ---------------------------------------------------------------- planets
static const char* KL[] = {"Temperate world", "Ocean world", "Desert world", "Frozen world", "Volcanic world", "Anomalous world", "Gas giant"};
static const char* LIFE[] = {"no life detected", "microbial life", "alien flora", "strange fauna", "sentient civilization", "machine intelligence", "lost civilization"};
static const char* ATM[] = {"none", "thin", "dense", "toxic", "luminous"};
static const char* SPECIAL[] = {"Gravity changes during the day", "Enormous floating organisms drift in the sky", "Mostly underground", "Plants glow when approached", "Oceans move unusually quickly",
  "Mountains slowly change position", "The atmosphere reacts to certain materials", "Civilization lives entirely beneath the surface", "Ancient structures hum in the dark"};
struct Moon { float d, a, s, r; };
struct Planet { V3 pos; float R = 1, spin = 0, tilt = 0; GLuint tex = 0, cloudTex = 0, ringTex = 0; V3 atm; bool hasAtm = false, cloud = false, ring = false, found = false; std::vector<Moon> moons; std::string name, txt; int kind = 0; };
struct PO { int kind = -1; float sat = -1, lvl = -1, hL = -1, hH = -1; bool found = false, noatm = false, clouds = false; int moons = -1; const char* name = nullptr; };
static Planet mkPlanet(int seed, float R0, const PO& o) {
  Rng r{(unsigned)(seed * 7919 + 13)}; static const int KT[8] = {0, 1, 2, 3, 4, 5, 6, 6};
  Planet p; p.kind = o.kind >= 0 ? o.kind : KT[r.i(8)]; int kind = p.kind;
  float sat = o.sat >= 0 ? o.sat : .45f + r.f() * .5f, lvl = o.lvl >= 0 ? o.lvl : .35f + r.f() * .25f, hL = .58f, hH = .28f; bool lo = false; int bands = 0; float hj = r.f() * .12f - .06f;
  p.R = kind == 6 ? R0 * 2.1f : R0;
  if (kind == 1) lvl = .8f; if (kind == 2) { hL = .08f; hH = .1f; lvl = .12f; } if (kind == 3) { hL = .58f; hH = .55f; lvl = .25f; sat = .3f; } if (kind == 4) { hL = .02f; hH = .03f; lvl = .45f; lo = true; }
  if (kind == 5) { hL = r.f(); hH = r.f(); } if (kind == 6) { hL = r.f(); hH = hL + .1f + r.f() * .3f; bands = 4 + r.i(8); } if (o.hL >= 0) { hL = o.hL; hH = o.hH; }
  hL += hj; hH += hj; bool des = kind == 2;
  C4 c[5] = {hsl(hL, sat, lo ? .4f : .1f), hsl(hL + .02f, sat, lo ? .55f : .3f), hsl(hH, sat * .8f, des ? .5f : .28f), hsl(hH + .04f, sat, des ? .65f : .42f), hsl(hH + .08f, .2f, lo ? .1f : .88f)};
  p.tex = mkTex(1024, 512, [&](float u, float v) -> C4 {
    float h = clampf((fbm(u * 6, v * 3, seed, 6, 6) - .22f) * 1.9f, 0, 1);
    if (bands) { float t = .5f + .5f * std::sin(v * bands * 6.283f + fbm(u * 6, v * 6, seed + 9, 4, 6) * 7); return mixc(mixc(c[1], c[3], t), c[4], .12f * h); }
    if (h < lvl) return mixc(c[0], c[1], h / lvl); float t = (h - lvl) / (1 - lvl); return t < .5f ? mixc(c[2], c[3], t * 2) : mixc(c[3], c[4], (t - .5f) * 2); });
  C4 ac = hsl(hL + (kind <= 1 ? 0.f : .05f), .8f, .6f); p.atm = {ac[0] / 255, ac[1] / 255, ac[2] / 255};
  p.atm = {std::pow(p.atm.x, 2.2f), std::pow(p.atm.y, 2.2f), std::pow(p.atm.z, 2.2f)}; p.hasAtm = !o.noatm && !(kind == 4 && r.f() < .5f);
  if (o.clouds || kind <= 1) { p.cloud = true; p.cloudTex = mkTex(512, 256, [&](float u, float v) -> C4 { return {255, 255, 255, clampf((fbm(u * 8, v * 4, seed + 3, 5, 8) - .4f) * 3, 0, 1) * 230}; }); }
  if (r.f() < .3f || (kind == 6 && r.f() < .5f)) { p.ring = true; p.tilt = (r.f() - .5f) * .8f;
    p.ringTex = mkTex(256, 2, [&](float u, float) -> C4 { float s = .5f + .5f * std::sin(u * 80 + fbm(u * 20, 0, seed, 3, 20) * 9), k = fbm(u * 30, .5f, seed + 1, 3, 30); return {200 * s + 30, 170 * s + 30, 130 * s + 30, (u < .02f || u > .98f) ? 0.f : k * 400 * s}; }, false); }
  int nm = o.moons >= 0 ? o.moons : r.i(3); for (int i = 0; i < nm; i++) p.moons.push_back({p.R * (2.6f + i * 1.1f + r.f()), r.r(0, 6.28f), .02f + r.f() * .05f, p.R * (.07f + r.f() * .1f)});
  static const char* sy[] = {"th", "ra", "lon", "qui", "mar", "dis", "vek", "nor", "ae", "vor", "kel", "zan", "ith", "pyr"}; static const char* sf[] = {"a", "is", "on", "ex", "ium", "or"}; static const char* nu[] = {"I", "II", "III", "IV", "V", "VI", "Prime", "Minor"};
  std::string n = std::string(sy[r.i(14)]) + sy[r.i(14)] + sf[r.i(6)]; n[0] = (char)std::toupper(n[0]); p.name = o.name ? o.name : n + " " + nu[r.i(8)];
  char b[400]; std::snprintf(b, sizeof b, "%s  |  gravity %.1f g  |  atmosphere %s  |  %s\n%s", KL[kind], .2f + r.f() * 2.8f, kind == 6 ? "crushing" : ATM[r.i(5)], LIFE[r.i(7)], SPECIAL[r.i(9)]); p.txt = b; p.found = o.found; return p;
}
static std::vector<Planet> planets; static int sysId = 0, foundN = 0;
static void freePlanets() { for (auto& p : planets) { glDeleteTextures(1, &p.tex); if (p.cloudTex) glDeleteTextures(1, &p.cloudTex); if (p.ringTex) glDeleteTextures(1, &p.ringTex); } planets.clear(); }
static void buildSystem(int id) {
  freePlanets(); Rng r{(unsigned)(id * 131 + 7)};
  sunDir = norm({r.f() - .5f, r.f() * .6f - .2f, r.f() - .5f}); if (id == 0) sunDir = norm({.5f, .45f, .35f});
  C4 sc = id ? hsl(r.f() * .15f + .02f, .55f, .88f) : C4{255, 236, 205, 255}; sunCol = {std::pow(sc[0] / 255, 2.2f) * 2.2f, std::pow(sc[1] / 255, 2.2f) * 2.2f, std::pow(sc[2] / 255, 2.2f) * 2.2f};
  auto add = [&](Planet p, float x, float y, float z) { p.pos = {x, y, z}; planets.push_back(std::move(p)); };
  if (id == 0) { PO e; e.kind = 0; e.hL = .58f; e.hH = .3f; e.lvl = .52f; e.found = true; e.name = "Earth"; e.moons = 0; e.clouds = true; add(mkPlanet(1, 4000, e), 0, -4400, 0);
    PO m; m.kind = 2; m.sat = .03f; m.hL = .1f; m.hH = .1f; m.found = true; m.name = "Moon"; m.moons = 0; m.noatm = true; add(mkPlanet(2, 650, m), 7000, 900, -6000); }
  for (int i = 0; i < (id ? 6 : 5); i++) { float a = r.r(0, 6.283f), d = 9000 + i * 4800 + r.f() * 3000; add(mkPlanet(id * 100 + i + 10, 500 + r.f() * 1400, PO{}), std::cos(a) * d, (r.f() - .5f) * 3000, std::sin(a) * d); }
}

// ---------------------------------------------------------------- game state
enum St { EARTH, COUNT, LAUNCH, SPACE }; static St st = EARTH;
struct Player { float x = 30, z = 50, y = 0, vx = 0, vz = 0, vy = 0, yaw = .54f, pitch = 0, step = 0, bob = 0; } P;
struct Ship { float yaw = 0, pitch = .3f, roll = 0, speed = 200, thr = .35f, cyaw = 0, cpitch = .3f, croll = 0; V3 pos; } S;
struct Pick { V3 pos, col; const char* name; bool got = false, inst = false; };
static Pick picks[5]; static int gotN = 0, instN = 0; static float tAcc = 0, cd = 0, vyL = 0, rocketY = .9f, flashA = 0; static bool pendingJump = false;
struct Tree { float x, z, s; }; struct Rock { float x, z, sx, sy, sz; }; static std::vector<Tree> trees; static std::vector<Rock> rocks;
struct Part { V3 p, v; float l, sc; }; static std::vector<Part> parts; static int partI = 0;
static std::string toastT; static float toastL = 0, toastMax = 1;
static void toast(const std::string& s, float sec = 5) { toastT = s; toastL = toastMax = sec; }
static bool K(int k) { return glfwGetKey(win, k) == GLFW_PRESS; }
static void emit(V3 p, V3 dir, float pw, float dt) {
  int n = (int)std::ceil(pw * 90 * dt); if (parts.empty()) parts.resize(240, Part{{}, {}, 0, 1});
  for (int i = 0; i < n; i++) { Part& f = parts[partI++ % parts.size()]; auto rv = [] { return (float)std::rand() / RAND_MAX - .5f; };
    f.l = 1; f.p = p + V3{rv(), rv(), rv()} * 1.6f; f.v = dir * (18 + pw * 45) + V3{rv(), rv(), rv()} * 8.f; f.sc = 2.5f + pw * 3.5f; }
}
static void updParts(float dt) { for (auto& f : parts) if (f.l > 0) { f.l -= dt * 1.5f; f.p = f.p + f.v * dt; } }
static void doJump() { audioEvent(S_WHOOSH); flashA = 1; pendingJump = true; }
static void interact() { if (paused) return; if (st == EARTH && instN == 5 && std::hypot(P.x, P.z) < 24) { st = COUNT; cd = 0; audioEvent(S_BEEP_LO); } }
static void toggleFS() {
  if (!fs) { glfwGetWindowPos(win, &wx, &wy); glfwGetWindowSize(win, &ww, &wh); const GLFWvidmode* m = glfwGetVideoMode(mon); glfwSetWindowMonitor(win, mon, 0, 0, m->width, m->height, m->refreshRate); }
  else glfwSetWindowMonitor(win, nullptr, wx, wy, ww, wh, 0); fs = !fs;
}
static bool muted = false;
static void onKey(GLFWwindow*, int key, int, int act, int) {
  if (act != GLFW_PRESS) return;
  if (key == GLFW_KEY_F || key == GLFW_KEY_F11) toggleFS();
  else if (key == GLFW_KEY_M) { muted = !muted; audioMute(muted); }
  else if (key == GLFW_KEY_E) interact();
  else if (key == GLFW_KEY_J && st == SPACE && !paused) doJump();
  else if (key == GLFW_KEY_ESCAPE) { paused = !paused; glfwSetInputMode(win, GLFW_CURSOR, paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED); } // Esc only pauses - it never closes the game
  else if (key == GLFW_KEY_Q && paused) glfwSetWindowShouldClose(win, 1);
}
static void onCursor(GLFWwindow*, double x, double y) { static double lx = x, ly = y; static bool first = true; if (first) { lx = x; ly = y; first = false; } if (!paused) { mdx += x - lx; mdy += y - ly; } lx = x; ly = y; }

// ---------------------------------------------------------------- update
static void updEarth(float dt) {
  float fz = (K(GLFW_KEY_W) || K(GLFW_KEY_UP)) - (K(GLFW_KEY_S) || K(GLFW_KEY_DOWN)), fx = (K(GLFW_KEY_D) || K(GLFW_KEY_RIGHT)) - (K(GLFW_KEY_A) || K(GLFW_KEY_LEFT)), sp = K(GLFW_KEY_LEFT_SHIFT) ? 9.5f : 5.2f;
  P.yaw -= (float)mdx * .0022f; P.pitch = clampf(P.pitch - (float)mdy * .0022f, -1.5f, 1.5f); mdx = mdy = 0;
  float s = std::sin(P.yaw), c = std::cos(P.yaw), tx = -s * fz + c * fx, tz = -c * fz - s * fx, l = std::hypot(tx, tz); if (l > 0) { tx = tx / l * sp; tz = tz / l * sp; }
  float k = 1 - std::exp(-dt * 10); P.vx += (tx - P.vx) * k; P.vz += (tz - P.vz) * k; P.x += P.vx * dt; P.z += P.vz * dt;
  float d = std::hypot(P.x, P.z); if (d > 215) { P.x *= 215 / d; P.z *= 215 / d; d = 215; }
  float gy = hAt(P.x, P.z) + (d < 17 ? .9f : 0); bool grounded = P.y <= gy + .02f; if (grounded && K(GLFW_KEY_SPACE)) P.vy = 6.8f; P.vy -= 18 * dt; P.y += P.vy * dt; if (P.y < gy) { P.y = gy; P.vy = 0; }
  float spd = std::hypot(P.vx, P.vz); if (grounded && spd > 1) { P.step += spd * dt; P.bob += spd * dt * 2.2f; if (P.step > 2.3f) { P.step = 0; audioEvent(S_STEP); } }
  camPos = {P.x, P.y + 1.75f + std::sin(P.bob) * .05f * (spd > 1), P.z}; basis(P.yaw, P.pitch, 0, cR, cU, cF); fov = 72; nearZ = .3f;
  for (int i = 0; i < 5; i++) { Pick& p = picks[i]; if (p.got) continue; if (std::hypot(P.x - p.pos.x, P.z - p.pos.z) < 3.2f && std::fabs(P.y - p.pos.y) < 6) { p.got = true; gotN++; audioEvent(S_CHIME);
    toast(std::string(p.name) + "\nRocket part found (" + std::to_string(gotN) + "/5)" + (gotN == 5 ? " - bring them to the launch pad" : ""), 3.5f); } }
  if (d < 20 && gotN > instN) { for (auto& p : picks) if (p.got && !p.inst) { p.inst = true; instN++; } audioEvent(S_CLANG); toast("INSTALLED\n" + std::to_string(instN) + "/5 parts on the rocket" + (instN == 5 ? " - press E to launch" : ""), 4); }
  audioSet(.05f + .04f * std::sin(tAcc * .3f) + spd * .004f, 0, 100, 0);
}
static void updLaunch(float dt) {
  V3 base{0, rocketY + .5f, 0};
  if (st == COUNT) { float prev = cd; cd += dt; int n = 6 - (int)cd; if ((int)cd != (int)prev && n > 0) { audioEvent(n == 1 ? S_BEEP_HI : S_BEEP_LO); toast("T-" + std::to_string(n), .9f); }
    float pw = clampf((cd - 2.5f) / 3.5f, 0, 1); if (cd > 2.5f) emit(base, {0, -1, 0}, pw * .6f, dt); audioSet(0, .2f + pw * .7f, 140 + pw * 80, 0);
    auto rv = [] { return (float)std::rand() / RAND_MAX - .5f; }; camPos = {42 + rv() * pw * .6f, 9 + rv() * pw * .6f, 42}; lookAt(camPos, {0, 11, 0}); fov = 60; nearZ = .3f;
    if (cd >= 6) { st = LAUNCH; vyL = 0; toast("LIFTOFF", 3); } return; }
  vyL += (16 + rocketY * .012f) * dt; rocketY += vyL * dt; emit({0, rocketY + .5f, 0}, {0, -1, 0}, 1, dt);
  V3 want{38, std::max(10.f, rocketY * .8f), 38}; camPos = camPos + (want - camPos) * (1 - std::exp(-dt * 3)); lookAt(camPos, {0, rocketY + 8, 0}); fov = 60;
  float t = clampf(rocketY / 800, 0, 1); fogDen = .0035f * (1 - t); ambient = {lerpf(.3f, .01f, t), lerpf(.36f, .01f, t), lerpf(.45f, .015f, t)};
  audioSet(.25f * (1 - t), .9f, 220, 0);
  if (rocketY > 900) { st = SPACE; flashA = 1; fogDen = 0; ambient = {.012f, .012f, .018f}; S.pos = {0, 0, 0}; S.pitch = S.cpitch = .3f; S.yaw = S.cyaw = 0; S.speed = 200; nearZ = 2;
    V3 f; basis(0, .3f, 0, cR, cU, f); camPos = f * -26.f + cU * 5.f; cF = f;
    toast("BEYOND\nW/S throttle  |  mouse steer  |  Shift boost  |  fly to the worlds around you\nJ = hyperjump to a brand-new star system", 9); }
}
static void updSpace(float dt) {
  S.yaw -= (float)mdx * .0018f; S.pitch = clampf(S.pitch - (float)mdy * .0018f, -1.45f, 1.45f); S.roll += (clampf(-(float)mdx * .02f, -.7f, .7f) - S.roll) * std::min(1.f, dt * 3); mdx = mdy = 0;
  S.thr = clampf(S.thr + ((K(GLFW_KEY_W) || K(GLFW_KEY_UP)) - (K(GLFW_KEY_S) || K(GLFW_KEY_DOWN))) * dt * .7f, 0, 1); bool boost = K(GLFW_KEY_LEFT_SHIFT);
  float tg = S.thr * 900 * (boost ? 6 : 1); S.speed += (tg - S.speed) * std::min(1.f, dt * 1.5f);
  V3 r, u, f; basis(S.yaw, S.pitch, S.roll, r, u, f); S.pos = S.pos + f * (S.speed * dt);
  Planet* nearP = nullptr; float nd = 1e9f;
  for (auto& p : planets) { V3 dv = S.pos - p.pos; float d = len(dv); if (d < p.R * 1.04f) { S.pos = p.pos + norm(dv) * (p.R * 1.04f); S.speed *= .6f; }
    p.spin += dt * .02f; for (auto& m : p.moons) m.a += dt * m.s;
    float sd = d - p.R; if (!p.found && sd < p.R * 2.2f + 400) { p.found = true; foundN++; audioEvent(S_DISCOVER); toast(p.name + "\n" + p.txt + "\nWorlds discovered: " + std::to_string(foundN), 8); }
    if (!p.found && sd < nd) { nd = sd; nearP = &p; } }
  float k = 1 - std::exp(-dt * 8); S.cyaw += (S.yaw - S.cyaw) * k; S.cpitch += (S.pitch - S.cpitch) * k; S.croll += (S.roll - S.croll) * k;
  V3 r2, u2, f2; basis(S.cyaw, S.cpitch, S.croll, r2, u2, f2); V3 want = S.pos + f * -26.f + u * 5.f; camPos = camPos + (want - camPos) * (1 - std::exp(-dt * 7)); cR = r2; cU = u2; cF = f2;
  fov = 72 + clampf(S.speed / 6000, 0, .12f) * 220; nearZ = 2; emit(S.pos, f * -1.f, .35f + S.thr * .65f, dt);
  audioSet(0, .15f + S.thr * .35f + (boost ? .2f : 0), 90 + S.speed * .1f, .1f);
  (void)nearP; (void)nd;
}
// ---------------------------------------------------------------- render
static std::string hudObj, hudStat;
static void useMain() { glUseProgram(pMain); glUniformMatrix4fv(uVP, 1, GL_FALSE, VP); glUniform3f(uSunDir, sunDir.x, sunDir.y, sunDir.z); glUniform3f(uSunCol, sunCol.x, sunCol.y, sunCol.z);
  glUniform3f(uAmb, ambient.x, ambient.y, ambient.z); glUniform3f(uFogCol, fogCol.x, fogCol.y, fogCol.z); glUniform1f(uFog, fogDen); glUniform1i(uTex, 0); }
static void drawRocket(V3 b, V3 X, V3 Y, V3 Z) {
  auto L = [&](const Mesh& m, float lx, float ly, float lz, V3 sc, V3 t, float em, GLuint tex, V3 ax, V3 az) { DP d; d.p = b + X * lx + Y * ly + Z * lz; d.s = sc; d.X = ax; d.Y = Y; d.Z = az; d.t = t; d.emis = em; d.tex = tex; draw(m, d); };
  if (picks[0].inst || st == SPACE) { L(mCylT, 0, 2, 0, {2.5f, 13, 2.5f}, {.9f, .9f, .9f}, 0, tHull, X, Z); L(mCylN, 0, 12.4f, 0, {2.57f, 1.2f, 2.57f}, {.02f, .03f, .05f}, 0, tHull, X, Z); }
  if (picks[1].inst || st == SPACE) for (int s = -1; s <= 1; s += 2) { L(mCylN, s * 3.6f, 2, 0, {.9f, 9, .9f}, {1, .22f, .03f}, 0, tHull, X, Z); L(mCone, s * 3.6f, 11, 0, {.9f, 2, .9f}, {.7f, .7f, .7f}, 0, tHull, X, Z); }
  if (picks[2].inst || st == SPACE) { L(mCylT, 0, 0, 0, {2.3f, 3, 2.3f}, {.08f, .09f, .1f}, 0, tHull, X, Z); L(mCylN, 0, .9f, 0, {1.3f, .1f, 1.3f}, {.5f, .8f, 1}, 2.5f, 0, X, Z); }
  if (picks[3].inst || st == SPACE) { L(mCone, 0, 15, 0, {2.3f, 6, 2.3f}, {.7f, .04f, .04f}, 0, tHull, X, Z); L(mSphereLo, 0, 11, 2.2f, {.75f, .75f, .75f}, {.2f, .5f, 1}, 2.f, 0, X, Z); }
  if (picks[4].inst || st == SPACE) for (int i = 0; i < 4; i++) { float a = i * PI / 2, ca = std::cos(a), sa = std::sin(a); L(mBox, ca * 3, 4, sa * 3, {.3f, 4.5f, 2.8f}, {.03f, .05f, .12f}, 0, tHull, X * -sa + Z * ca, X * ca + Z * sa); }
}
static void render() {
  glfwGetFramebufferSize(win, &FW, &FH); if (FW < 1 || FH < 1) return; glViewport(0, 0, FW, FH); makeVP();
  float t = st == LAUNCH ? clampf(rocketY / 800, 0, 1) : st == SPACE ? 1.f : 0.f; V3 sky{lerpf(fogCol.x, 0, t), lerpf(fogCol.y, 0, t), lerpf(fogCol.z, 0, t)};
  glClearColor(sky.x, sky.y, sky.z, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDepthMask(GL_TRUE);
  V3 savedFog = fogCol; fogCol = sky; useMain(); glActiveTexture(GL_TEXTURE0);
  if (st != SPACE) {
    DP g; g.p = {0, 0, 0}; g.tex = tGrass; g.uvs = 70; draw(mTerrain, g);
    DP pad; pad.p = {0, -.3f, 0}; pad.s = {16.5f, 1.2f, 16.5f}; pad.tex = tConc; pad.uvs = 3; pad.t = {.9f, .9f, .9f}; draw(mCylT, pad);
    for (int sx = -1; sx <= 1; sx += 2) { DP tw; tw.p = {sx * 9.f, 14, 0}; tw.s = {1.4f, 28, 1.4f}; tw.tex = tHull; tw.t = {.5f, .55f, .6f}; draw(mBox, tw); for (int y = 4; y < 27; y += 5) { DP cb; cb.p = {sx * 8.1f, (float)y, 0}; cb.s = {1.8f, .3f, 1.2f}; cb.tex = tHull; cb.t = {.5f, .55f, .6f}; draw(mBox, cb); } }
    for (auto& tr : trees) { float y = hAt(tr.x, tr.z); DP a; a.p = {tr.x, y, tr.z}; a.s = {.35f * tr.s, 3.2f * tr.s, .35f * tr.s}; a.tex = tBark; draw(mCylN, a);
      DP b; b.p = {tr.x, y + 2.4f * tr.s, tr.z}; b.s = {2 * tr.s, 4.5f * tr.s, 2 * tr.s}; b.tex = tLeaf; draw(mCone, b); b.p.y = y + 4.6f * tr.s; b.s = {1.5f * tr.s, 3.6f * tr.s, 1.5f * tr.s}; draw(mCone, b); }
    for (auto& rk : rocks) { DP a; a.p = {rk.x, hAt(rk.x, rk.z) + .2f, rk.z}; a.s = {rk.sx, rk.sy, rk.sz}; a.tex = tRock; a.uvs = 2; a.t = {.8f, .8f, .8f}; draw(mSphereLo, a); }
    for (int i = 0; i < 5; i++) if (!picks[i].got) { float a = tAcc * 2 + i; DP d; d.p = picks[i].pos + V3{0, std::sin(tAcc * 2 + i) * .4f, 0}; d.s = {1.3f, 1.3f, 1.3f}; d.X = {std::cos(a), 0, -std::sin(a)}; d.Z = {std::sin(a), 0, std::cos(a)}; d.t = picks[i].col; d.emis = 2.f; draw(mBox, d); }
    drawRocket({0, rocketY, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
  } else {
    for (auto& p : planets) { float c = std::cos(p.spin), s = std::sin(p.spin); DP d; d.p = p.pos; d.s = {p.R, p.R, p.R}; d.X = {c, 0, -s}; d.Z = {s, 0, c}; d.tex = p.tex; draw(mSphere, d);
      for (auto& m : p.moons) { DP mm; mm.p = p.pos + V3{std::cos(m.a) * m.d, std::sin(m.a) * m.d * .12f, std::sin(m.a) * m.d}; mm.s = {m.r, m.r, m.r}; mm.tex = tRock; mm.t = {.8f, .8f, .8f}; mm.uvs = 3; draw(mSphereLo, mm); } }
    V3 r, u, f; basis(S.yaw, S.pitch, S.roll, r, u, f); drawRocket(S.pos, r, f, u);
  }
  glEnable(GL_BLEND); glDepthMask(GL_FALSE);
  if (st == SPACE) for (auto& p : planets) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (p.cloud) { DP d; float R = p.R * 1.012f, c = std::cos(p.spin * 1.5f), s = std::sin(p.spin * 1.5f); d.p = p.pos; d.s = {R, R, R}; d.X = {c, 0, -s}; d.Z = {s, 0, c}; d.tex = p.cloudTex; draw(mSphere, d); }
    if (p.ring) { DP d; float R = p.R * 2.4f, c = std::cos(p.tilt), s = std::sin(p.tilt); d.p = p.pos; d.s = {R, R, R}; d.Y = {0, c, s}; d.Z = {0, -s, c}; d.tex = p.ringTex; d.mode = 2; d.alpha = .85f; draw(mRing, d); }
    if (p.hasAtm) { glBlendFunc(GL_ONE, GL_ONE); for (float sc : {1.05f, 1.14f}) { DP d; float R = p.R * sc; d.p = p.pos; d.s = {R, R, R}; d.t = p.atm; d.emis = sc < 1.1f ? 1.6f : .7f; d.mode = 1; draw(mSphere, d); } }
  }
  // point sprites
  glBlendFunc(GL_ONE, GL_ONE); glEnable(GL_PROGRAM_POINT_SIZE); glUseProgram(pPt); glUniformMatrix4fv(uPtVP, 1, GL_FALSE, VP);
  glUniform1f(uPtScale, FH / (2 * std::tan(fov * PI / 360))); float sf = st == SPACE ? 1.f : st == LAUNCH ? clampf(rocketY / 800, 0, 1) : 0.f;
  if (sf > 0) drawPoints(starVAO, starVBO, nullptr, starN, sf);
  std::vector<PT> pts; V3 sd = sunDir * 30000.f; pts.push_back({sd.x, sd.y, sd.z, 4500, sunCol.x * .5f, sunCol.y * .5f, sunCol.z * .5f, 1}); pts.push_back({sd.x, sd.y, sd.z, 12000, sunCol.x * .12f, sunCol.y * .12f, sunCol.z * .12f, 1});
  if (st != SPACE) for (int i = 0; i < 5; i++) if (!picks[i].got) for (int k = 0; k < 18; k++) { V3 q = picks[i].pos - camPos + V3{0, k * 4.f, 0}; pts.push_back({q.x, q.y, q.z, 22, picks[i].col.x, picks[i].col.y, picks[i].col.z, .16f}); }
  for (auto& f : parts) if (f.l > 0) { float k = std::max(f.l, 0.f); V3 q = f.p - camPos; pts.push_back({q.x, q.y, q.z, f.sc * (2.3f - k * 1.3f), 1.f * k + .2f, (.25f + k * .45f) * k, .05f * k, k * .9f}); }
  drawPoints(ptVAO, ptVBO, pts.data(), (int)pts.size(), 1.f);
  fogCol = savedFog;
  // HUD
  glDisable(GL_DEPTH_TEST); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glUseProgram(pHud); glUniform2f(uHScr, (float)FW, (float)FH);
  float sc = std::max(2.f, std::floor(FH / 400.f)); V3 w{1, 1, 1}, dim{.7f, .85f, 1};
  rect(FW / 2 - 2.f, FH / 2 - 2.f, 4, 4, w, .6f);
  if (st == EARTH) { std::string ob = gotN < 5 ? "Find the rocket parts (" + std::to_string(gotN) + "/5) - follow the glowing beams" : instN < gotN ? "Bring the parts to the launch pad" : instN < 5 ? "Keep searching" : "Rocket ready - stand on the pad and press E to launch";
    hudObj = "EARTH  -  PHASE 1\n" + ob; float nearD = 1e9f; for (auto& p : picks) if (!p.got) nearD = std::min(nearD, (float)std::hypot(P.x - p.pos.x, P.z - p.pos.z)); hudStat = nearD < 1e8f ? "Nearest part " + std::to_string((int)nearD) + " m" : "All parts collected"; }
  else if (st == SPACE) { Planet* np = nullptr; float nd = 1e9f; for (auto& p : planets) if (!p.found) { float d = len(S.pos - p.pos) - p.R; if (d < nd) { nd = d; np = &p; } }
    hudObj = "DEEP SPACE  -  SYSTEM " + std::to_string(sysId) + "\n" + (np ? "Nearest unexplored world: " + np->name + " (" + std::to_string((int)(nd / 1000)) + " km)" : "Everything here is discovered - press J to jump");
    hudStat = "SPEED " + std::to_string((int)(S.speed * 3.6f)) + " km/s   THROTTLE " + std::to_string((int)(S.thr * 100)) + "%\nWORLDS DISCOVERED " + std::to_string(foundN); }
  else { hudObj = "LAUNCH\nLeaving Earth"; hudStat = "ALT " + std::to_string((int)rocketY) + " m   VEL " + std::to_string((int)(vyL * 3.6f)) + " km/h"; }
  text(hudObj, 22, 18, sc, dim); text(hudStat, 22, FH - 26.f * sc - 10, sc, w, .85f);
  if (toastL > 0) { float a = clampf(toastL / .8f, 0, 1) * clampf((toastMax - toastL) / .2f, 0, 1) + 0.f; size_t nl = toastT.find('\n'); std::string head = toastT.substr(0, nl), rest = nl == std::string::npos ? "" : toastT.substr(nl + 1);
    textC(head, FW / 2.f, FH * .18f, sc * 2.2f, w, a); if (!rest.empty()) { size_t p = 0; float y = FH * .18f + 18 * sc * 2.2f; while (p <= rest.size()) { size_t e = rest.find('\n', p); if (e == std::string::npos) e = rest.size(); textC(rest.substr(p, e - p), FW / 2.f, y, sc * 1.2f, dim, a); y += 14 * sc * 1.2f; p = e + 1; } } }
  if (flashA > 0) rect(0, 0, (float)FW, (float)FH, w, flashA);
  if (paused) { rect(0, 0, (float)FW, (float)FH, {0, 0, .02f}, .65f); textC("PAUSED", FW / 2.f, FH * .35f, sc * 4, w); textC("Esc resume   |   F fullscreen   |   M mute   |   Q quit", FW / 2.f, FH * .5f, sc * 1.3f, dim); }
  glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE);
}
// ---------------------------------------------------------------- main
int main(int argc, char** argv) {
  bool windowed = false; for (int i = 1; i < argc; i++) if (!std::strcmp(argv[i], "--windowed")) windowed = true;
  if (!glfwInit()) { std::fprintf(stderr, "GLFW init failed\n"); return 1; }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); glfwWindowHint(GLFW_SAMPLES, 4);
  mon = glfwGetPrimaryMonitor(); const GLFWvidmode* vm = glfwGetVideoMode(mon); fs = !windowed;
  win = fs ? glfwCreateWindow(vm->width, vm->height, "BEYOND", mon, nullptr) : glfwCreateWindow(1280, 720, "BEYOND", nullptr, nullptr);
  if (!win) { std::fprintf(stderr, "Window creation failed (OpenGL 3.3 required)\n"); return 1; }
  glfwMakeContextCurrent(win); glfwSwapInterval(1); glewExperimental = GL_TRUE; if (glewInit() != GLEW_OK) { std::fprintf(stderr, "GLEW init failed\n"); return 1; }
  glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED); if (glfwRawMouseMotionSupported()) glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  glfwSetKeyCallback(win, onKey); glfwSetCursorPosCallback(win, onCursor);
  glEnable(GL_FRAMEBUFFER_SRGB); glEnable(GL_MULTISAMPLE);
  pMain = compile(VS_MAIN, FS_MAIN); pPt = compile(VS_PT, FS_PT); pHud = compile(VS_HUD, FS_HUD);
  auto U = [](GLuint p, const char* n) { return glGetUniformLocation(p, n); };
  uVP = U(pMain, "uVP"); uM = U(pMain, "uM"); uUVS = U(pMain, "uUVS"); uTex = U(pMain, "uTex"); uSunDir = U(pMain, "uSunDir"); uSunCol = U(pMain, "uSunCol"); uAmb = U(pMain, "uAmb"); uTint = U(pMain, "uTint");
  uFogCol = U(pMain, "uFogCol"); uFog = U(pMain, "uFog"); uEmis = U(pMain, "uEmis"); uAlpha = U(pMain, "uAlpha"); uMode = U(pMain, "uMode");
  uPtVP = U(pPt, "uVP"); uPtScale = U(pPt, "uScale"); uPtFade = U(pPt, "uFade"); uHO = U(pHud, "uO"); uHScr = U(pHud, "uScr"); uHK = U(pHud, "uK"); uHCol = U(pHud, "uCol");
  std::printf("BEYOND: generating textures and the universe...\n");
  tWhite = mkTex(2, 2, [](float, float) -> C4 { return {255, 255, 255, 255}; });
  tGrass = mkTex(1024, 1024, [](float u, float v) -> C4 { float n = fbm(u * 8, v * 8, 1, 5, 8), m = fbm(u * 32, v * 32, 2, 3, 32), sp = Hh((int)(u * 1024), (int)(v * 1024), 9); return {28 + n * 70 + m * 30 + sp * 20, 70 + n * 100 + m * 40 + sp * 24, 18 + n * 34 + sp * 10, 255}; });
  tBark = mkTex(128, 256, [](float u, float v) -> C4 { float n = fbm(u * 6, v * 40, 3, 4, 6); return {70 + n * 70, 45 + n * 45, 28 + n * 25, 255}; });
  tLeaf = mkTex(256, 256, [](float u, float v) -> C4 { float n = fbm(u * 8, v * 8, 4, 4, 8); return {20 + n * 40, 80 + n * 110, 30 + n * 40, 255}; });
  tRock = mkTex(256, 256, [](float u, float v) -> C4 { float k = 70 + fbm(u * 6, v * 6, 5, 5, 6) * 110; return {k, k - 4, k - 10, 255}; });
  tConc = mkTex(512, 512, [](float u, float v) -> C4 { float l = (std::fabs(std::fmod(u * 2, 1.f) - .5f) > .49f || std::fabs(std::fmod(v * 2, 1.f) - .5f) > .49f) ? -45.f : 0.f, k = 130 + fbm(u * 8, v * 8, 4, 4, 8) * 60 + l; return {k, k, k - 3, 255}; });
  tHull = mkTex(512, 512, [](float u, float v) -> C4 { float l = std::max(std::fabs(std::fmod(u * 4, 1.f) - .5f), std::fabs(std::fmod(v * 4, 1.f) - .5f)) > .485f ? -90.f : 0.f,
    dx = std::fmod(u * 32, 1.f) - .5f, dy = std::fmod(v * 32, 1.f) - .5f, rv = (dx * dx + dy * dy) < .012f ? -55.f : 0.f, k = 215 + fbm(u * 6, v * 30, 5, 4, 6) * 45 + l + rv; return {k, k, k + 6, 255}; });
  mSphere = mkSphere(96, 64); mSphereLo = mkSphere(20, 14); mCyl = mkCyl(1, 1, 1, 40, true); mCylT = mkCyl(1, .92f, 1, 48, true); mCylN = mkCyl(1, 1, 1, 32, true); mCone = mkCyl(1, 0, 1, 40, true);
  mBox = mkBox(); mTerrain = mkTerrain(180, 520); mRing = mkRing(1.5f / 2.4f, 160);
  setupPoints(); setupHud();
  { Rng r{42}; for (int i = 0; i < 130; i++) { float x = (r.f() - .5f) * 440, z = (r.f() - .5f) * 440; if (std::hypot(x, z) < 32) continue; trees.push_back({x, z, .8f + r.f() * 1.3f}); }
    for (int i = 0; i < 70; i++) { float x = (r.f() - .5f) * 440, z = (r.f() - .5f) * 440; if (std::hypot(x, z) < 30) continue; rocks.push_back({x, z, .6f + r.f() * 2.5f, .5f + r.f() * 1.6f, .6f + r.f() * 2.5f}); } }
  const char* names[5] = {"HULL", "FUEL BOOSTERS", "ENGINE", "GUIDANCE COMPUTER", "LANDING FINS"}; V3 cols[5] = {{1, 1, 1}, {1, .3f, .04f}, {.1f, .6f, 1}, {.35f, 1, .2f}, {1, .1f, .35f}};
  for (int i = 0; i < 5; i++) { Rng r{(unsigned)(77 + i)}; float a = i / 5.f * 6.283f + r.f() * .9f, d = 70 + r.f() * 95, x = std::cos(a) * d, z = std::sin(a) * d; picks[i].pos = {x, hAt(x, z) + 1.5f, z}; picks[i].col = cols[i]; picks[i].name = names[i]; }
  P.y = hAt(P.x, P.z); buildSystem(0); sunDir = norm({.5f, .45f, .35f});
  audioInit(); toast("BEYOND\nBuild the rocket. Leave Earth. Discover a universe that has never existed.\nWASD/ZQSD move  |  mouse look  |  Shift sprint  |  Space jump  |  E interact  |  F fullscreen  |  M mute  |  Esc pause", 9);
  double last = glfwGetTime();
  while (!glfwWindowShouldClose(win)) {
    glfwPollEvents(); double now = glfwGetTime(); float dt = (float)std::min(.05, now - last); last = now;
    if (!paused) { tAcc += dt; if (st == EARTH) updEarth(dt); else if (st == COUNT || st == LAUNCH) updLaunch(dt); else updSpace(dt); updParts(dt); if (toastL > 0) toastL -= dt; }
    if (flashA > 0) flashA = std::max(0.f, flashA - dt * 1.2f);
    render(); glfwSwapBuffers(win);
    if (pendingJump) { pendingJump = false; buildSystem(++sysId); S.pos = {0, 0, 0}; camPos = S.pos; toast("HYPERJUMP\nStar system " + std::to_string(sysId) + " - everything here is new", 6); }
  }
  audioShutdown(); glfwTerminate(); return 0;
}
