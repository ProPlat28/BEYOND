#pragma once
// Window, OpenGL drawing helpers, textures, meshes, camera, stars and HUD text (gfx.cpp).
#include "common.h"
#include "gl_min.h"
#include "stb_easy_font.h"

GLuint mkTex(int w, int h, const std::function<C4(float, float)>& fn, bool rep = true);
// ---- tileable noise helpers for texture generation (pure, no GL). x spans px cells and y spans py cells over one texture repeat.
inline float vnoiseP(float x, float y, int s, int px, int py) {
  int xi = (int)std::floor(x), yi = (int)std::floor(y); float u = x - xi, v = y - yi, a = u * u * (3 - 2 * u), b = v * v * (3 - 2 * v);
  int x0 = ((xi % px) + px) % px, x1 = (x0 + 1) % px, y0 = ((yi % py) + py) % py, y1 = (y0 + 1) % py;
  return (Hh(x0, y0, s) * (1 - a) + Hh(x1, y0, s) * a) * (1 - b) + (Hh(x0, y1, s) * (1 - a) + Hh(x1, y1, s) * a) * b;
}
inline float fbmP(float x, float y, int s, int o, int px, int py) { float a = .5f, t = 0; int f = 1; for (int i = 0; i < o; i++) { t += a * vnoiseP(x * f, y * f, s + i, px * f, py * f); f *= 2; a *= .5f; } return t; }
// tileable Worley/Voronoi: f1/f2 = distance to nearest/second nearest feature point, id = hash of the nearest cell, (dx,dy) = vector from the pixel to the nearest point
struct Vor { float f1, f2, id, dx, dy; int cx, cy; };
inline Vor voroP(float x, float y, int s, int px, int py, float jit = .9f, int R = 1) {   // R = 2: exact second distance (slower)
  int xi = (int)std::floor(x), yi = (int)std::floor(y); Vor r{8, 8, 0, 0, 0, 0, 0}; float d1 = 64, d2 = 64;
  for (int j = -R; j <= R; j++) for (int i = -R; i <= R; i++) {
    int cx = (((xi + i) % px) + px) % px, cy = (((yi + j) % py) + py) % py;
    float ox = xi + i + .5f + (Hh(cx, cy, s) - .5f) * jit - x, oy = yi + j + .5f + (Hh(cx, cy, s + 17) - .5f) * jit - y, d = ox * ox + oy * oy;
    if (d < d1) { d2 = d1; d1 = d; r.dx = ox; r.dy = oy; r.cx = cx; r.cy = cy; } else if (d < d2) d2 = d;
  }
  r.f1 = std::sqrt(d1); r.f2 = std::sqrt(d2); r.id = Hh(r.cx, r.cy, s + 31); return r;
}
inline float sstep(float a, float b, float x) { float t = clampf((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); }
// base and neutral texture generators (gfx.cpp); fn(u,v) for mkTex, all tile seamlessly
int grassBlades(float u, float v, int seed, float dens, float& tip, float& hue, float& lit); C4 texGrass(float u, float v); C4 texBark(float u, float v); C4 texLeaf(float u, float v); C4 texRock(float u, float v); C4 texConc(float u, float v); C4 texHull(float u, float v);
C4 texSkin(float u, float v); C4 texCloth(float u, float v); C4 texWood(float u, float v); C4 texMetal(float u, float v); C4 texLeafW(float u, float v); C4 texBarkW(float u, float v);
struct Mesh { GLuint vao = 0, vbo = 0, ebo = 0; int n = 0; };
Mesh mkSphere(int sg, int rg);
Mesh mkCyl(float r0, float r1, float h, int sg, bool caps);
Mesh mkBox();
Mesh mkRing(float inner, int sg);
float hAt(float x, float z);
Mesh mkTerrain(int n, float S, float (*hf)(float, float) = hAt);
GLuint compile(const char* vs, const char* fs);
inline GLFWwindow* win; inline GLFWmonitor* mon; inline int FW = 1280, FH = 720;
inline float farZ = 80000.f;   // far clip plane (the ending pulls it far out to see the whole system)
inline bool fs = false, paused = false; inline int wx = 100, wy = 100, ww = 1280, wh = 720; inline float fsFix = 0;   // fsFix: seconds until the windowed size is applied again after leaving fullscreen
inline double mdx = 0, mdy = 0;
inline GLuint pMain, pPt, pHud;
inline GLint uVP, uM, uUVS, uTex, uSunDir, uSunCol, uAmb, uTint, uFogCol, uFog, uEmis, uAlpha, uMode, uDetail, uSpec, uPtVP, uPtScale, uPtFade, uHO, uHScr, uHK, uHCol;
inline V3 camPos, cR{1, 0, 0}, cU{0, 1, 0}, cF{0, 0, -1}; inline float fov = 72, nearZ = .3f, VP[16];
inline V3 sunDir = norm({.5f, .45f, .35f}), sunCol{1, .93f, .8f}, ambient{.3f, .36f, .45f}, fogCol{.28f, .55f, 1.f}; inline float fogDen = .0035f;
inline Mesh mSphere, mSphereLo, mCyl, mCylT, mCylN, mCone, mBox, mTerrain, mRing;
inline GLuint tGrass, tBark, tLeaf, tRock, tConc, tHull, tWhite;
// neutral (near-white, tinted at draw time) detail textures: alien skin, cloth, planks/wood, metal, foliage, bark. 0 until created in main.cpp; callers fall back to tWhite.
inline GLuint tSkin = 0, tCloth = 0, tWood = 0, tMetal = 0, tLeafW = 0, tBarkW = 0;
void basis(float yaw, float pitch, float roll, V3& r, V3& u, V3& f);
void lookAt(V3 from, V3 to);
void makeVP();
struct DP { V3 p; V3 s{1, 1, 1}; V3 X{1, 0, 0}, Y{0, 1, 0}, Z{0, 0, 1}; V3 t{1, 1, 1}; float emis = 0, uvs = 1, alpha = 1; int mode = 0; GLuint tex = 0;
  float detail = 0;    // > 0 for large terrain: extra high-frequency sample, non-repeating macro variation and slope darkening (1 = full)
  float spec = .05f;   // sun highlight strength (0 = matte, ~.3 = polished metal / wet)
};
void draw(const Mesh& m, const DP& d);
struct PT { float x, y, z, size, r, g, b, a; };
inline GLuint ptVAO, ptVBO, starVAO, starVBO; inline int starN = 0;
void setupPoints();
void drawPoints(GLuint vao, GLuint vbo, const PT* d, int n, float fade);
void setupHud();
void text(const std::string& s, float x, float y, float sc, V3 c, float a = 1);
void rect(float x, float y, float w, float h, V3 c, float a);
void textC(const std::string& s, float cx, float y, float sc, V3 c, float a = 1);
void setupShaders();
