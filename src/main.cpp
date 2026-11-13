// Program entry: window setup, loading base assets, and the main loop.
#include "game.h"
#include "icon_data.h"
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------- music: menu, space, and one song per family of planet kinds (Earth is temperate)
static int musicNow() {
  static const int BY_KIND[16] = {MUS_VERDANT, MUS_VERDANT, MUS_DUNES, MUS_FROST, MUS_EMBER, MUS_ANOMALY, MUS_EMBER, MUS_VERDANT,   // temperate, ocean, desert, frozen, volcanic, anomalous, gas giant, jungle
                                  MUS_FROST, MUS_ANOMALY, MUS_VERDANT, MUS_DUNES, MUS_ANOMALY, MUS_VERDANT, MUS_DUNES, MUS_EMBER};   // crystal, metallic, swamp, tidally locked, shattered, bioluminescent, ash, storm
  if (app != A_GAME) return MUS_MENU;
  if (st == LAUNCH || st == SPACE) return MUS_SPACE;
  if (st == SURFACE && curP >= 0 && curP < (int)planets.size()) return BY_KIND[((planets[curP].kind % 16) + 16) % 16];
  return MUS_VERDANT;
}
// ---------------------------------------------------------------- main
int main(int argc, char** argv) {
  bool windowed = false; int testEnd = 0; for (int i = 1; i < argc; i++) if (!std::strcmp(argv[i], "--windowed")) windowed = true; else if (!std::strcmp(argv[i], "--test-ending")) { windowed = true; testEnd = 1; } else if (!std::strcmp(argv[i], "--test-moons")) { windowed = true; testEnd = 2; } else if (!std::strcmp(argv[i], "--make-ending-outro")) { windowed = true; testEnd = 3; } else if (!std::strcmp(argv[i], "--make-developer-world")) { windowed = true; testEnd = 4; }   // testing mode: windowed, straight into the ending, nothing saved
  // started inside a distrobox/toolbox container (no GPU driver there = software rendering, slow and hot): run again on the host instead
  if (!std::getenv("BEYOND_IN_HOST") && access("/run/.containerenv", F_OK) == 0 && access("/usr/bin/distrobox-host-exec", X_OK) == 0) {
    char self[4096]; ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n > 0) { self[n] = 0; std::vector<char*> av{(char*)"distrobox-host-exec", (char*)"env", (char*)"BEYOND_IN_HOST=1", self}; for (int i = 1; i < argc; i++) av.push_back(argv[i]); av.push_back(nullptr);
      execv("/usr/bin/distrobox-host-exec", av.data()); } }
  #if defined(__linux__)
    glfwInitHint(0x00050003 /* GLFW_PLATFORM */, 0x00060004 /* GLFW_PLATFORM_X11 */);  // GLFW 3.4 on Wayland: use XWayland, the native Wayland window can stay invisible (ignored by GLFW 3.3)
  #elif defined(__APPLE__)
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_COCOA);
  #elif defined(_WIN32)
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WIN32);
  #endif
  // Added custom GLFW error logging
  if (!glfwInit()) { const char* description = nullptr;
    int code = glfwGetError(&description);

    fprintf(stderr, "GLFW error %d: %s\n",
            code,
            description ? description : "Unknown error");
    return 1; }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); glfwWindowHint(GLFW_SAMPLES, 4);
  mon = glfwGetPrimaryMonitor(); const GLFWvidmode* vm = glfwGetVideoMode(mon); fs = !windowed;
  // the window stays hidden while everything loads, and appears directly on the main menu (fullscreen is applied when it is shown)
  glfwWindowHintString(0x00024001 /* GLFW_X11_CLASS_NAME */, "beyond"); glfwWindowHintString(0x00024002 /* GLFW_X11_INSTANCE_NAME */, "beyond");   // lets Linux desktops match the window to beyond.desktop (its icon)
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); win = glfwCreateWindow(1280, 720, "BEYOND", nullptr, nullptr);
  if (!win) { std::fprintf(stderr, "Window creation failed (OpenGL 3.3 required)\n"); return 1; }
  { GLFWimage ic[2] = {{64, 64, (unsigned char*)ICON_64}, {128, 128, (unsigned char*)ICON_128}}; glfwSetWindowIcon(win, 2, ic); }   // title bar / taskbar / alt-tab icon
  glfwMakeContextCurrent(win); glfwSwapInterval(1); if (!glLoad()) { std::fprintf(stderr, "OpenGL 3.3 functions could not be loaded\n"); return 1; }
  glfwSetWindowSizeLimits(win, 800, 500, GLFW_DONT_CARE, GLFW_DONT_CARE); if (glfwRawMouseMotionSupported()) glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  glfwSetKeyCallback(win, onKey); glfwSetCharCallback(win, onChar); glfwSetCursorPosCallback(win, onCursor); glfwSetMouseButtonCallback(win, onMouseButton); glfwSetScrollCallback(win, onScroll);
  glfwSetWindowFocusCallback(win, [](GLFWwindow*, int) { mouseFresh = true; mdx = mdy = 0; });   // alt-tab / coming back to the window: the cursor is re-captured somewhere else, that is not a mouse movement
  glEnable(GL_FRAMEBUFFER_SRGB); glEnable(GL_MULTISAMPLE);
  setupShaders();
  setupHud(); setupMenuGfx(); loadSettings(); syncModes();
  tWhite = mkTex(2, 2, [](float, float) -> C4 { return {255, 255, 255, 255}; });
  // base textures (generators in gfx.cpp, all tile seamlessly)
  tGrass = mkTex(1024, 1024, texGrass); tBark = mkTex(256, 512, texBark); tLeaf = mkTex(512, 512, texLeaf); tRock = mkTex(512, 512, texRock);
  tConc = mkTex(512, 512, texConc); tHull = mkTex(512, 512, texHull);
  // neutral near-white detail textures, multiplied by a tint at draw time
  tSkin = mkTex(512, 512, texSkin); tCloth = mkTex(512, 512, texCloth); tWood = mkTex(512, 512, texWood); tMetal = mkTex(512, 512, texMetal);
  tLeafW = mkTex(512, 512, texLeafW); tBarkW = mkTex(256, 512, texBarkW);
  mSphere = mkSphere(96, 64); mSphereLo = mkSphere(20, 14); mCyl = mkCyl(1, 1, 1, 40, true); mCylT = mkCyl(1, .92f, 1, 48, true); mCylN = mkCyl(1, 1, 1, 32, true); mCone = mkCyl(1, 0, 1, 40, true);
  mBox = mkBox(); mTerrain = mkTerrain(180, 520); mRing = mkRing(1.5f / 2.4f, 160);
  setupPoints();
  { Rng r{42}; for (int i = 0; i < 130; i++) { float x = (r.f() - .5f) * 440, z = (r.f() - .5f) * 440; if (std::hypot(x, z) < 32) continue; trees.push_back({x, z, .8f + r.f() * 1.3f}); }
    for (int i = 0; i < 70; i++) { float x = (r.f() - .5f) * 440, z = (r.f() - .5f) * 440; if (std::hypot(x, z) < 30) continue; rocks.push_back({x, z, .6f + r.f() * 2.5f, .5f + r.f() * 1.6f, .6f + r.f() * 2.5f}); } }
  buildEarthHits();
  const char* names[5] = {"HULL", "FUEL BOOSTERS", "ENGINE", "GUIDANCE COMPUTER", "LANDING FINS"}; V3 cols[5] = {{1, 1, 1}, {1, .3f, .04f}, {.1f, .6f, 1}, {.35f, 1, .2f}, {1, .1f, .35f}};
  for (int i = 0; i < 5; i++) { picks[i].col = cols[i]; picks[i].name = names[i]; }
  migrateOldSave(); scanWorlds(); app = A_MENU; if (testEnd == 4) { bool ok = makeDeveloperWorld(); glfwTerminate(); return ok ? 0 : 1; }   // creates the developer world, then quits (does nothing in release builds)
  if (testEnd == 3) { bool ok = makeEndingOutro(); glfwTerminate(); return ok ? 0 : 1; }   // builds the world, then quits
  if (testEnd) testEnding(testEnd - 1);
  if (fs) glfwSetWindowMonitor(win, mon, 0, 0, vm->width, vm->height, vm->refreshRate); glfwShowWindow(win); glfwFocusWindow(win); syncModes();
  audioInit(); audioSet(0, 0, 100, .11f);   // music starts with the menu, not while the hidden window is still loading
  double last = glfwGetTime();
  while (!glfwWindowShouldClose(win)) {
    glfwPollEvents(); double now = glfwGetTime(); float dt = (float)std::min(.05, now - last); last = now; pollGamepad(dt); audioMusic(musicNow()); audioPauseMusic(app == A_GAME && paused); audioVolumes(sfxVol, musVol);
    if (fsFix > 0 && (fsFix -= dt) <= 0 && !fs) { glfwSetWindowSize(win, ww, wh); glfwSetWindowPos(win, wx, wy); }
    if (app == A_MENU) {
      menuFrame(dt); glfwSwapBuffers(win);
      if (pendingOpen >= 0) { int id = pendingOpen; pendingOpen = -1; openWorld(id); last = glfwGetTime(); }
      continue;
    }
    if (paused || modal || cine || app != A_GAME || (st != EARTH && st != SPACE && st != SURFACE)) mdx = mdy = 0;   // never store mouse movement the game is not using: it would be applied as one big turn later
    if (!paused && modal != M_DEATH) { tAcc += dt;   // market, workbench and chest windows do not pause the world
      if (st == EARTH) updEarth(dt); else if (st == COUNT || st == LAUNCH) updLaunch(dt); else if (st == SURFACE) updSurface(dt); else updSpace(dt); updParts(dt); if (toastL > 0) toastL -= dt; }
    if (flashA > 0) flashA = std::max(0.f, flashA - dt * 1.2f); if (savedL > 0) savedL -= dt;
    sinceSave += dt; { int sg = sigOf(); if (saveReq || (sg != lastSig && sinceSave > 1.f) || (sinceSave > 15.f && !paused)) { saveGame(); lastSig = sg; sinceSave = 0; saveReq = false; } }
    render(); if (paused) pauseFrame(dt); else if (modal) modalFrame(dt); glfwSwapBuffers(win);
    if (pendingRespawn) { pendingRespawn = false; respawnAtCheckpoint(); last = glfwGetTime(); }
    if (pendingMenu) { closeWorld(); last = glfwGetTime(); }
  }
  saveGame(); touchWorld(); audioShutdown(); glfwTerminate(); return 0;
}
