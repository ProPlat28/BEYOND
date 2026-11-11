#pragma once
// Procedural planets and star systems (planets.cpp).
#include "gfx.h"

inline const char* KL[] = {"Temperate world", "Ocean world", "Desert world", "Frozen world", "Volcanic world", "Anomalous world", "Gas giant", "Jungle world",
  "Crystal world", "Metallic world", "Swamp world", "Tidally locked world", "Shattered world", "Bioluminescent world", "Ash world", "Storm world"};
struct Moon { float d, a, s, r; V3 tint{.8f, .8f, .8f}; };
struct Planet { V3 pos; float R = 1, spin = 0, tilt = 0, grav = 1, wicked = .4f, squash = 1, spinRate = .02f, glow = 0, ringScale = 2.4f;
  GLuint tex = 0, cloudTex = 0, ringTex = 0; V3 atm, cloudTint{1, 1, 1}; bool hasAtm = false, cloud = false, ring = false, found = false, enterable = true;
  std::vector<Moon> moons; std::string name, txt, culture, special; int kind = 0, seed = 0, species = 0, qi = 0, qph = 0, prog = 0, itemI = 0, nItems = 3;
  int texPat = 0, terr = 0, floraT = 0, archT = 0, cloudSt = 0, uniq = -1, lifeI = 0, atmI = 0, layout = 0, nShrines = 3; bool hideVil = false, hideCamp = false; unsigned mask = 0, vmask = 0; C4 pal[5];
  bool isMoon = false; int parent = -1; float orbD = 0, orbA = 0, orbS = 0; };   // moons are real, landable worlds that orbit a planet
inline int nqOf(const Planet& p) { return p.isMoon ? 3 : 5; }            // planets have 5 quest chapters, moons have 3

inline std::vector<Planet> planets; inline int sysId = 0, foundN = 0;
struct SRock { V3 pos; float r = 50, sx = 1, sy = 1, sz = 1, yaw = 0; V3 tint{.7f, .7f, .7f}; };   // asteroids floating in space: obstacles for the rocket
inline std::vector<SRock> srocks;
void buildSpaceRocks(int id);
inline unsigned worldSeed = 0;                     // every world has its own universe (0 = the original one, used by old saves)
inline std::function<void(float)> buildProgress;    // optional: called while a star system is being generated
int discHere();
int totalHere();
int moonsDiscHere();
int moonsTotalHere();
std::string cultureOf(int uniq);
struct SysInfo { std::string name; int cls = 4; V3 col{1, .9f, .75f}, col2{1, .6f, .4f}; bool binary = false; float size = 1; };
inline const char* STAR_CLS[7] = {"O blue giant", "B blue-white star", "A white star", "F yellow-white star", "G yellow dwarf", "K orange dwarf", "M red dwarf"};
SysInfo sysInfo(int id);
void applyPSaves();   // defined in save.cpp: puts remembered progress back on the planets of the system that was just built
void freePlanets();
void buildSystem(int id);
// ground texture for walking on planet p (1024x1024, tiles, keeps the planet's colours/pattern/seed identity). Caller owns the GL texture.
GLuint makeSurfaceTex(const Planet& p);
std::vector<C4> surfaceTexels(const Planet& p, int W);   // the pixels behind it (W must be a power of two), GL-free
