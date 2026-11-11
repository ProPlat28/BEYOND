#pragma once
// Everything the game's .cpp files share: state, types and the functions they call in each other.
// Variables are C++17 "inline" variables, so each exists once even though many files include this header.
// Anything used by a single file stays "static" inside that file instead of being listed here.
#include "planets.h"
#include "audio.h"

enum St { EARTH, COUNT, LAUNCH, SPACE, SURFACE }; inline St st = EARTH;
struct Player { float x = 30, z = 50, y = 0, vx = 0, vz = 0, vy = 0, yaw = .54f, pitch = 0, step = 0, bob = 0; };
inline Player P;
struct Ship { float yaw = 0, pitch = .3f, roll = 0, speed = 200, thr = .35f, cyaw = 0, cpitch = .3f, croll = 0; V3 pos; };
inline Ship S;
struct Pick { V3 pos, col; const char* name; bool got = false, inst = false; };
inline Pick picks[5]; inline int gotN = 0, instN = 0; inline float tAcc = 0, cd = 0, vyL = 0, rocketY = .9f, flashA = 0;
struct Tree { float x, z, s; }; struct Rock { float x, z, sx, sy, sz; }; inline std::vector<Tree> trees; inline std::vector<Rock> rocks;
struct Part { V3 p, v; float l, sc; V3 tint{1, 1, 1}; }; inline std::vector<Part> parts; inline int partI = 0;
inline std::string toastT; inline float toastL = 0, toastMax = 1;
void toast(const std::string& s, float sec = 5);
// Each action has two key slots (0 = empty). Esc is not rebindable: it always opens the pause menu, so nobody gets locked out.
enum Action { ACT_FORWARD, ACT_BACK, ACT_LEFT, ACT_RIGHT, ACT_RUN, ACT_JUMP, ACT_INTERACT, ACT_EAT, ACT_ATTACK, ACT_COUNT };
inline const char* ACT_ID[ACT_COUNT] = {"forward", "back", "left", "right", "run", "jump", "interact", "eat", "attack"};
inline const char* ACT_NAME[ACT_COUNT] = {"Forward / throttle up", "Back / throttle down", "Left", "Right", "Sprint / boost", "Jump", "Interact", "Eat", "Attack (sword)"};
inline int keyBind[ACT_COUNT][2]; inline float mouseSens = 1; inline bool mouseInvert = false;
inline int captureAct = -1, captureSlot = 0; inline std::string ctrlMsg; inline bool pauseControls = false; // captureAct >= 0: waiting for a key press
void resetControls();
bool held(int a);
// ---- game controller (GLFW gamepad mapping). Buttons 0-14 are GLFW_GAMEPAD_BUTTON_*, 15/16 are the left/right triggers. Start is not rebindable: it always pauses.
inline const int PAD_LT = 15, PAD_RT = 16, PAD_NB = 17;
inline int padBind[ACT_COUNT][2];   // two controller buttons per action (-1 = none); the second is empty by default
 inline float padSens = 1; inline bool padInvert = false, padInvertX = false, padActive = false, padConnected = false, capturePad = false, padDown[PAD_NB];
inline float padMove[2] = {0, 0};   // left stick: x = right, y = forward (dead zone removed)
inline std::string padLabel;        // name of the connected controller
void pollGamepad(float dt);
std::string padName(int b);
void doAction(int a);               // what a key or button press of an action does in the game
inline const char* LATIN_FOLD = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo-ouuuuyty"; // U+00C0..U+00FF without accents
std::string keyName(int k);
std::string keysIn(std::string s);
void updParts(float dt);
inline bool saveReq = false;
void toggleFS();
inline bool muted = false; inline float sfxVol = 1, musVol = 1; inline bool pauseSettings = false;   // volumes 0..1 (SETTINGS); muted is no longer shown, old saves' mute becomes 0% volumes
 inline float perkJump = 1, perkSpeed = 1, perkGrav = 1;
enum AppMode { A_LOADING, A_MENU, A_GAME }; inline AppMode app = A_LOADING;
void syncModes();
struct UiRect { float x, y, w, h; int id; bool slider; }; inline std::vector<UiRect> uiRects, uiRectsPrev; inline int padNudge = 0, padTab = 0, uiFocus = -1; inline bool padNav = false;   // widgets seen this/last frame: lets the controller jump between them
inline bool uiPressed = false, uiReleased = false, uiDown = false; inline float uiWheel = 0; inline int uiActive = 0;
void uiFlushMouse();
void setPaused(bool on);
void onKey(GLFWwindow*, int key, int, int act, int);
void onChar(GLFWwindow*, unsigned cp);
void onCursor(GLFWwindow*, double x, double y);
void onMouseButton(GLFWwindow*, int button, int act, int);
void onScroll(GLFWwindow*, double, double dy);
float walk(float dt, float (*gh)(float, float), float padR, float bound, float grav, float sp0);
void updEarth(float dt);
void updLaunch(float dt);
inline int nearEnter = -1;
void emit(V3 p, V3 dir, float pw, float dt);
void updSpace(float dt);
inline std::string hudObj, hudStat;
void useMain();
void drawRocket(V3 b, V3 X, V3 Y, V3 Z);
struct Npc { V3 pos, home, col, dest; bool wicked = false, chief = false, elder = false, merchant = false, dweller = false, dead = false, angry = false; int poi = -1, look = 0, hp = 10, maxHp = 10, widx = -1; float ph = 0, yaw = 0, wphase = 0, sp = 0, voice = 1, wait = 0, hurt = 0, stuck = 0; std::string name; bool pushed = false; V3 back; };   // pushed: walking back to 'back', the spot it was shoved from
struct Poi { V3 pos; int type = 0; float yaw = 0; };
struct Obj { V3 pos; int type = 0; bool done = false; };
struct Flora { float x, z, s; int type; };
inline std::vector<Npc> npcs; inline std::vector<Obj> objs; inline std::vector<Flora> flora; inline std::vector<Rock> boulders; inline std::vector<V3> hutPos;
inline const int MAXHP = 10;   // lives
inline Mesh mSurf; inline GLuint tSurf = 0; inline int curP = -1, hp = MAXHP, surfTerr = 0, surfFlora = 0, surfArch = 0; inline float invul = 0, surfAmp = 20; inline unsigned surfSeed = 0;
inline V3 vilC, campC, relicPos, surfSky, hutCol, roofCol, spaceEntry, floraA, floraB; inline std::string talkS, talkT, prompt; inline float talkL = 0;
inline std::vector<Poi> pois; inline V3 wonderPos, wonderCol; inline int wAdj = 0, wNoun = 0; inline float raceT = 0;
void freeMesh(Mesh& m);
float shAt(float x, float z);
V3 lin(const C4& c);
V3 polar(float a, float d);
// Every planet has 5 chapters. The chain is different on every planet (quest types, order, names, jokes),
// chapter 4 is always the wicked camp, and the finale is always the planet's own signature Wonder.
enum { QA, QB, QC, QD, QE, QF, QG, QH, QI };   // gather, shrines, relic, rescue, hoops, parcel, beacons, gossip, wonder
inline const int NQ = 5, RQ = 3;               // chapters per planet, index of the relic chapter
inline const char* WVOICE[16] = {"psst... you smell like adventure.", "FINE. You may pass. Don't touch anything.", "*wiggles* Oh! Company! Watch my moves!", "...was I supposed to be doing something?",
  "hehehe. You pressed my button. hehehe.", "mmmm... mm-mm-MMM. (That means hello.)", "zzz... huh? Oh. Yes. Wonderful. Zzz.", "Look at me. LOOK. Am I not dazzling?",
  "I have waited eleven thousand years. You're late.", "Whoa, whoa, I'm up! I'm up! Where's up?", "oh! um. Hi. Please don't look directly at me.", "HEAR ME, TRAVELER! ...sorry. Indoor voice.",
  "I dreamed you would come. You were taller.", "ah... ah... AH-CHOO! Sorry. Pollen.", "Of course you found me. I'm obviously the best landmark here.", "I could tell you a secret. I won't. But I could."};
inline const char* EXCUSE[8] = {"I wasn't lost! I was exploring aggressively.", "Oh good, you found me. I was about to give up hiding.", "Is the festival over? I'll stay here a bit longer, just in case.",
  "I'm not stuck. I'm resting my legs. Vertically.", "Please don't tell the elder I ran away from the soup.", "I was following a very interesting bug. For three days.", "I live here now. Please send mail.", "Shhh. I'm hiding from my in-laws."};
inline const char* POI1[10] = {"Careful! That's my ship. It's not crashed, it's 'permanently parked'. Want to buy a slightly used engine?",
  "Ruins of a mighty empire! Or a very large patio. The experts are still arguing.",
  "Welcome to the cantina! Drinks are free because the till is also broken. Rumor says the wicked chief cries when it rains.",
  "I watch for danger! So far I have spotted: you. Verdict pending.",
  "Ahh, hot spring. Don't ask about the bubbles. They just do that. Mostly.",
  "That's the Founder! Wonderful hero, terrible sculptor. The nose is a legal dispute.",
  "Rocket graveyard. Every one of these said 'it'll be fine'. Nice ship you've got, by the way.",
  "Shh! You'll scare the silence. I came here to be alone. Thanks for ruining it. Tea?",
  "I'm tuning the mast. It only picks up one channel: a man reading recipes. Wonderful stuff.",
  "Everything is half price! The price is 'lots'. Today's special: opinions."};
inline const char* POI2[10] = {"Still no buyers for the engine. Try the other engine. Also no buyers.", "Don't touch the pillars. Okay, touch one. Gently. Maybe it'll explain itself.",
  "Another round of nothing, please. House special.", "Still watching. Still you.", "Mmm. Don't stare at the bubbles.", "Some say the nose will grow back.", "Watch the fins. They bite.",
  "You came back. Tragic.", "...and now the recipe is for pie. Excellent.", "Today's special is also opinions."};
inline const char* GOSS[3] = {"I saw someone with crumbs on their robes and a very guilty hat.", "Whoever did it left frosting footprints. They lead toward the village. And the elder's house.",
  "I'd say more, but I'm legally required to blame a goat."};
struct Perk { const char* name; const char* desc; float j, s, g; };
inline const Perk PERK[8] = {{"Moon Boots", "you jump much higher", 1.8f, 1, 1}, {"Sprint Sparks", "you run faster", 1, 1.5f, 1}, {"Feather Fall", "you float like a feather", 1.3f, 1, .45f},
  {"Rocket Legs", "faster and higher", 1.4f, 1.3f, 1}, {"Cloud Hop", "very low gravity", 1.1f, 1, .35f}, {"Gentle Gale", "a tailwind pushes you along", 1, 1.7f, 1},
  {"Bouncy Aura", "you bounce like a frog", 2.4f, 1, 1.2f}, {"Zen Stride", "effortless movement", 1.25f, 1.25f, .8f}};
void applyPerk(const Planet& p);
std::string itemName(const Planet& p);
std::string relicName(const Planet& p);
std::string wonderName(const Planet& p);
std::string poiName(const Planet& p, int i);
int sights(const Planet& p);
int qtype(const Planet& p, int qi);
int nObjFor(const Planet& p, int qt);
V3 spot(Rng& r, float dmin, float dmax, float avoid);
void placeSites(const Planet& p);
std::string questIntro(const Planet& p, const std::string& w, int qt);
std::string questRemind(const Planet& p, int qt);
std::string questDone(const Planet& p, int qt);
void enterPlanet(int pi, bool cinematic = true);
void interact();
void updSurface(float dt);
void surfHud(std::string& obj, std::string& stat);
void surfPoints(std::vector<PT>& pts);
void drawSurface();
inline float savedL = 0; inline int curWorld = -1;
std::string dataDir();
std::string worldDir(int id);
std::string savePath();
void saveGame();
bool loadGame();
void resetWorldState();
int sigOf();
void render();
// Each world is a folder: worlds/w<id>/world.txt (name, seed, dates) and worlds/w<id>/save.txt (its progress).
struct World { int id = 0; unsigned seed = 0; long long created = 0, played = 0; std::string name, devTag; int state = 0, sys = 0, found = 0, got = 0, difficulty = 1; };   // difficulty is chosen when the world is created and never changes
inline std::vector<World> worlds; // most recently played first
inline const size_t WORLD_NAME_LEN = 24;
std::string cleanName(std::string s);
void scanWorlds();
World* findWorld(int id);
int createWorld(const std::string& name, int diff = 1);
int copyWorld(int id);   // new world folder with the same save, named "<name> (1)", "<name> (2)"...
bool renameWorld(int id, const std::string& name);
bool deleteWorld(int id);
std::string defaultWorldName();
std::string worldInfo(const World& w);
void saveSettings();
void loadSettings();
void setMuted(bool m);
void migrateOldSave();
struct Col { float r, g, b, a; };
inline GLuint pNebula, pSpace, pWidget, pLogo, emptyVAO, nebFBO, nebTex; inline const int NEB_W = 960, NEB_H = 540; inline float menuMX = 0, menuMY = 0;
void setupMenuGfx();
void drawMenuBackground();
inline float uiMx = 0, uiMy = 0, uiDt = 0, uiU = 1, uiS = 2, uiA = 1; inline bool uiInput = true; inline std::unordered_map<int, float> uiHov;
void uiBegin(float dt);
void uiEnd();
void uiBox(float x, float y, float w, float h, float rad, Col top, Col bot, Col edge = {0, 0, 0, 0}, float ew = 0, Col glow = {0, 0, 0, 0}, float gr = 0);
float uiSmall();
float uiLarge();
float textW(const std::string& s, float sc);
void uiText(const std::string& s, float x, float y, float sc, V3 c, float a = 1);
void uiTextC(const std::string& s, float cx, float y, float sc, V3 c, float a = 1);
std::string fitText(std::string s, float maxW, float sc);
float fitScale(const std::string& s, float maxW, float sc);
void uiTextFit(const std::string& s, float x, float y, float maxW, float sc, V3 c, float a = 1);
void uiTextFitC(const std::string& s, float cx, float y, float maxW, float sc, V3 c, float a = 1);
inline const V3 UI_TXT{.94f, .97f, 1}, UI_DIM{.62f, .71f, .84f}, UI_GOLD{1, .82f, .46f}, UI_RED{1, .47f, .42f};
enum BtnStyle { BTN_NORMAL, BTN_PRIMARY, BTN_DANGER, BTN_DANGER_STRONG };
bool uiOver(float x, float y, float w, float h);
bool uiTabStep(int& tab, int n, int baseId, int skip = -1);
bool uiButton(int id, float x, float y, float w, float h, const std::string& label, int style = BTN_NORMAL, bool enabled = true);
void uiPanel(float x, float y, float w, float h);
void uiLogo(float x, float y, float h, float a);
enum MenuScreen { SC_HOME, SC_WORLDS, SC_CONTROLS, SC_SETTINGS }; inline MenuScreen menuScreen = SC_HOME; inline float menuFade = 0; inline int worldScroll = 0, pendingOpen = -1; inline std::string menuMsg;
enum Dialog { DLG_NONE, DLG_NEW, DLG_RENAME, DLG_DELETE, DLG_QUIT }; inline int dlgDiff = 1; inline Dialog dlg = DLG_NONE; inline int dlgWorld = -1; inline std::string dlgText; inline float dlgTime = 0;
void goScreen(MenuScreen s);
void menuKey(int key, int act);
void menuChar(unsigned cp);
void menuFrame(float dt);
void loadingFrame(const std::string& title, float progress);
inline float sinceSave = 0; inline int lastSig = 0; inline bool pendingMenu = false;
void openWorld(int id);
void touchWorld();
void closeWorld();
void pauseFrame(float dt);

// ================================================================ v2: voices, economy, houses, moons, star systems, cutscenes
// ---- modal windows (market, workbench, death screen): the game is frozen and the mouse is free, like in the pause menu
enum Modal { M_NONE, M_MARKET, M_CRAFT, M_DEATH, M_CHEST };
inline int modal = 0; inline float modalT = 0;
void openModal(int m);
void closeModal();
void modalFrame(float dt);
inline bool pauseEncy = false;
bool encyclopediaPanel();
inline bool mouseInvertX = false;
inline const int voiceMode = 1;   // alien voices: the ElevenLabs recording when the species has one, otherwise the classic babble, never both (2 babble only, 3 both)
inline bool mouseFresh = true; inline double mlx = 0, mly = 0;   // the first mouse event after the cursor is captured again only re-anchors the position: that fixed the camera jump after the pause menu
inline std::string modalWho;
float hutDoorDist(int ar);
void drawProps(const Planet& p);   // workbench, market stall, wicked house and ore nodes
// ---- resources: every planet mines its own two, the rest has to be traded for
inline const int NRES = 17;
inline const char* RES_NAME[NRES] = {"Greenwood", "Pearl Brine", "Sandglass", "Frost Crystal", "Magma Core", "Warp Dust", "Cloud Essence", "Spore Fiber", "Prism Shard", "Ferrite Ore", "Bog Resin", "Twilight Amber", "Void Fragment", "Glow Moss", "Cinder Coal", "Thunder Salt", "Lunar Dust"};
inline const V3 RES_COL[NRES] = {{.35f, .9f, .35f}, {.55f, .85f, 1}, {1, .82f, .45f}, {.75f, .93f, 1}, {1, .4f, .12f}, {.85f, .45f, 1}, {1, .72f, .82f}, {.55f, 1, .4f}, {1, .5f, .9f},
  {.78f, .78f, .84f}, {.7f, .7f, .25f}, {1, .62f, .2f}, {.5f, .4f, 1}, {.3f, 1, .85f}, {.55f, .45f, .4f}, {.95f, .95f, .45f}, {.85f, .87f, .92f}};
inline int inv[NRES]; inline int meds = 0, deaths = 0;
struct Cost { int res, n; };
struct RocketDef { const char* name; const char* desc; float speed; V3 hull, boost, nose, fin, flame; int nc; Cost cost[7]; int needPrizes; };
inline const RocketDef ROCKET[6] = {
  {"Pioneer Mk I", "The rocket you built on Earth. Reliable and very slow.", .45f, {.9f, .9f, .9f}, {1, .22f, .03f}, {.7f, .04f, .04f}, {.03f, .05f, .12f}, {1, 1, 1}, 0, {}, 0},
  {"Skiff Mk II", "A light hull of wood and ore. Not fast, but fresher.", .8f, {1, .85f, .45f}, {.9f, .5f, .1f}, {.8f, .3f, .05f}, {.35f, .2f, .05f}, {1, .9f, .6f}, 3, {{9, 4}, {0, 3}, {2, 2}}, 0},
  {"Comet Runner", "Four boosters and an ice-cooled engine.", 1.4f, {.65f, .8f, 1}, {.2f, .5f, 1}, {.1f, .3f, .9f}, {.9f, .95f, 1}, {.6f, .8f, 1}, 4, {{9, 5}, {2, 4}, {3, 3}, {1, 2}}, 0},
  {"Nova Dart", "Swept wings, a needle nose and a furious engine.", 2.4f, {.95f, .35f, .3f}, {1, .5f, .1f}, {.15f, .15f, .2f}, {1, .8f, .2f}, {1, .6f, .3f}, 4, {{4, 4}, {1, 4}, {8, 3}, {9, 4}}, 0},
  {"Quasar Arrow", "Warp-tuned. Crosses a star system in seconds.", 4.f, {.45f, .3f, .75f}, {.8f, .4f, 1}, {.9f, .8f, 1}, {.15f, .1f, .3f}, {.8f, .5f, 1}, 4, {{5, 4}, {12, 4}, {15, 3}, {6, 3}}, 0},
  {"Starforged Eclipse", "One of a kind. Forged from the gifts of many worlds.", 7.f, {1, .8f, .3f}, {1, .85f, .4f}, {1, 1, 1}, {1, .75f, .2f}, {.7f, 1, 1}, 6, {{13, 3}, {11, 3}, {7, 3}, {10, 3}, {14, 3}, {16, 3}}, 3}};
inline int rocketMask = 1, rocketSel = 0; inline V3 flameTint{1, 1, 1};
int resOf(const Planet& p, int slot);
struct Offer { int gr, gn, rr, rn; };
inline const int NOFFER = 4; void marketOffers(const Planet& p, Offer out[NOFFER]);
bool canCraft(int id);
void craftRocket(int id);
std::string costText(int id);
std::string prizeName(int uniq, bool moon);
std::string prizeDesc(int uniq);
std::string grantPrize(Planet& p);
int prizesCount();
void giveRes(int r, int n);
std::string invLine();
// ---- progress of every planet and moon ever visited, in every star system (so you can fly back to an old system and nothing is lost)
struct PSave { int qi = 0, qph = 0, prog = 0, found = 0, sys = 0, uniq = 0, kind = 0; unsigned mask = 0, vmask = 0; bool moon = false; std::string name; };
inline std::map<int, PSave> psaves; inline std::set<int> visitedSys;
void syncPSaves();
inline const unsigned VM_MET = 1u << 20, VM_WONDER = 1u << 4, VM_PRIZE = 1u << 22, VM_VISIT = 1u << 23;   // flags kept in Planet::vmask (bits 0-4 are the sights, 24-30 the opened chests)
// ---- unique aliens
// body plan 0..12 plus hashed feature axes: eyeSt 0 normal/1 stalks/2 visor/3 compound, head 0..5, ears 0..4, horns 0..5, back 0..4, tailT 0..4, mouth 0..4, pat 0..3, cloth 0..4, hat 0..4
struct Alien { int body = 0, eyes = 2, eyeSt = 0, head = 0, ears = 0, horns = 0, back = 0, tailT = 0, mouth = 0, pat = 0, cloth = 0, hat = 0, arms = 2; bool tail = false; V3 skin, accent, skin2; float size = 1, headS = 1, girth = 1, legL = 1, armL = 1; };
Alien alienOf(int uniq, bool moon);
float gaitSpeed(int uniq, bool moon);   // walking-speed multiplier of the species (~0.7 golems .. 1.35 insects)
std::string alienDesc(int uniq, bool moon);
void drawNpc(const Npc& n, const Planet& p);
// ---- hitboxes (circles; the people have none) and houses you can walk into
// A hitbox is a circle, or an oriented box when hx > 0 (half sizes hx/hz along the box's own X/Z axes; (ca, sa) = its X axis on the ground). y = base, h = height.
struct Cir { float x, z, r, y, h; float hx = 0, hz = 0, ca = 1, sa = 0; };
inline std::vector<Cir> hits, hitsSaved;
inline const float STEP_UP = .85f;   // low things (a wonder's base, a rock) can be walked onto; taller ones block
Cir boxHit(V3 c, float hx, float hz, float yaw, float y, float h);
bool insideHit(const Cir& c, float x, float z, float rad);
bool pushOut(const Cir& c, float& x, float& z, float rad);
float hitFloor(float x, float z, float y);   // the top of a low hitbox under the player, or -1e9
bool npcBlocked(float x, float z, float y, float rad);
void npcCollide(Npc& n, float rad);
void resolveHits();
void buildHits(const Planet& p);
void buildEarthHits();
inline int inRoom = 0, roomIdx = 0; inline float outX = 0, outZ = 0, outYaw = 0;   // inRoom: 0 outside, 1 in a house, 2 in the wicked house
inline std::vector<Npc> roomNpcs; inline V3 wHouse, marketPos, benchPos; inline float marketYaw = 0;
inline const float ROOMY = 4000.f, RS = 6.f;
V3 hutDoor(int i);
V3 wDoor();
float hutRadius(int ar);
void enterRoom(int kind, int idx);
void exitRoom();
void updRoom(float dt);
void drawRoom(const Planet& p);
bool roomInteract(Planet& p);
// ---- mining nodes, workbench and market
struct Node { V3 pos; int res; float cool = 0; int need = 0; };   // res: 0..NRES-1 ore, 100/101 wild food, 200 the planet's rare find. need: pickaxe tier (0 = bare hands), -1 = this planet's own pickaxe
inline std::vector<Node> nodes;
// ---- talking with voices (typewriter text + Animal-Crossing style blips)
inline float talkAge = 0; inline int talkShown = 0, talkPitch = 24, talkKind = 0;
void setVoice(const Npc* n);
void say(const std::string& who, const std::string& t);
void updTalk(float dt);
// ---- survival: difficulty (per world, fixed at creation), hunger, fruit, hidden checkpoints
inline int difficulty = 1;   // 0 easy: no hunger / 1 normal: hunger slows you down but never hurts / 2 hard: starving hurts and the wicked hit harder
inline float food = 20, foodT = 0, starveT = 0, regenT = 0; inline bool starving = false, hungryWarned = false;
// Hidden checkpoint: a full save written the moment a planet or moon is discovered. Dying puts you back in space in front of that world with that save.
std::string checkpointPath();
bool hasCheckpoint();
void makeCheckpoint(const Planet& p);
void commitCheckpoint();       // at the moment of death: the checkpoint becomes the current save, so quitting on the death screen cannot dodge it
bool respawnFromCheckpoint();  // reloads the checkpoint; false if there is none
inline bool pendingRespawn = false;
// developer tools (devtools.cpp): only for the one world whose world.txt carries a valid dev tag (never decided by its name)
inline bool devWorld = false, devPanel = false, devNoclip = false, devFlyOn = false, devGod = false, devFast = false;
std::string devTagFor(int id, unsigned seed, long long created); bool devAllowed(const World& w); void devReset(); void toggleDevPanel();
bool devFly(float dt, float (*gh)(float, float), float bound); void devFrame(float dt); bool makeDeveloperWorld(); bool classicVoice(const Planet& p); bool signDeveloperWorld(const std::string& name);
// the endings (ending.cpp): one plays once when every planet is discovered, another when every moon is; endT < 0 = not playing
inline float endT = -1; inline bool endSeen = false, moonEndSeen = false;
void startEnding(int mode); bool endingOn(); void skipEnding(); void updEnding(float dt); void endingPoints(std::vector<PT>& pts); void endingHud(float sc); void testEnding(int mode); bool makeEndingOutro();
inline const char* DIFF_NAME[3] = {"EASY", "NORMAL", "HARD"};
void updSurvival(float dt);
void respawnAtCheckpoint();
void eatFood();
void hurtPlayer(int dmg, const std::string& who);   // takes lives (never during invulnerability)
int hitDamage();                                    // lives lost to one hit: easy 1-2, normal 1-3, hard 3-5
void attack();                                      // swing the equipped sword
inline float swingT = 0, mineT = 0;                 // view-model animations: sword swing, pickaxe swing
void drawViewModel();
// terrain is flattened under every structure so nothing floats or sinks into a hill
struct Flat { float x, z, r, h; };
inline std::vector<Flat> flats;
// ================================================================ items: foods, rare finds, tools, chests (items.cpp)
// a "people key" names who made something: uniq * 2 + moon. It is the same in every star system, like the planet names.
inline int pkey(const Planet& p) { return p.uniq * 2 + (p.isMoon ? 1 : 0); }
// food id = people key * 4 + slot: slots 0-1 grow in the wild, slots 2-3 are dishes sold at the market (they fill you much more)
inline std::map<int, int> foods;
std::string foodName(int id); int foodValue(int id); V3 foodColor(int id); int foodShape(int id); int foodTotal();
// rare finds: some worlds hide a unique resource that only their own pickaxe can mine. key = people key.
inline std::map<int, int> rares;
bool hasRare(const Planet& p); std::string rareName(int key); V3 rareColor(int key);
// tools: one sword and one pickaxe per people, each with its own name, look and strength
inline std::set<int> swords, pickaxes; inline int swordSel = -1;
std::string swordName(int key); int swordDamage(int key); V3 swordColor(int key); int swordShape(int key);
std::string pickName(int key); int pickTier(int key); V3 pickColor(int key); int bestPickTier();
// one id for everything that can sit in a chest or a bag
inline const int IT_FOOD = 100000, IT_RARE = 200000, IT_MED = 300000;
int itemHave(int it); void itemAdd(int it, int n); std::string itemLabel(int it); V3 itemColor(int it);
std::vector<std::pair<int, int>> playerItems();
inline std::map<long long, std::map<int, int>> chests; inline long long chestOpen = 0;   // key = planet seed * 64 + chest bit
std::string toolsLine();
// ---- landing and take-off cutscenes
inline int cine = 0; inline float cineT = 0, surfRocketY = .9f;
void updCine(float dt);
void startTakeoff();
