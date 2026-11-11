// Draws one frame of the game and its HUD.
#include "game.h"

void render() {
  glfwGetFramebufferSize(win, &FW, &FH); if (FW < 1 || FH < 1) return; glViewport(0, 0, FW, FH); makeVP();
  float t = st == LAUNCH ? clampf(rocketY / 800, 0, 1) : st == SPACE ? 1.f : 0.f; V3 bse = st == SURFACE ? surfSky : fogCol; V3 sky{lerpf(bse.x, 0, t), lerpf(bse.y, 0, t), lerpf(bse.z, 0, t)};
  glClearColor(sky.x, sky.y, sky.z, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDepthMask(GL_TRUE);
  V3 savedFog = fogCol; fogCol = sky; useMain(); glActiveTexture(GL_TEXTURE0);
  if (st == SURFACE) drawSurface();
  else if (st != SPACE) {
    DP g; g.detail = 1; g.p = {0, 0, 0}; g.tex = tGrass; g.uvs = 70; draw(mTerrain, g);
    DP pad; pad.p = {0, -.3f, 0}; pad.s = {16.5f, 1.2f, 16.5f}; pad.tex = tConc; pad.uvs = 3; pad.t = {.9f, .9f, .9f}; draw(mCylT, pad);
    for (int sx = -1; sx <= 1; sx += 2) { DP tw; tw.p = {sx * 9.f, 14, 0}; tw.s = {1.4f, 28, 1.4f}; tw.tex = tHull; tw.t = {.5f, .55f, .6f}; draw(mBox, tw); for (int y = 4; y < 27; y += 5) { DP cb; cb.p = {sx * 8.1f, (float)y, 0}; cb.s = {1.8f, .3f, 1.2f}; cb.tex = tHull; cb.t = {.5f, .55f, .6f}; draw(mBox, cb); } }
    for (auto& tr : trees) { float y = hAt(tr.x, tr.z); DP a; a.p = {tr.x, y, tr.z}; a.s = {.35f * tr.s, 3.2f * tr.s, .35f * tr.s}; a.tex = tBark; draw(mCylN, a);
      DP b; b.p = {tr.x, y + 2.4f * tr.s, tr.z}; b.s = {2 * tr.s, 4.5f * tr.s, 2 * tr.s}; b.tex = tLeaf; draw(mCone, b); b.p.y = y + 4.6f * tr.s; b.s = {1.5f * tr.s, 3.6f * tr.s, 1.5f * tr.s}; draw(mCone, b); }
    for (auto& rk : rocks) { DP a; a.p = {rk.x, hAt(rk.x, rk.z) + .2f, rk.z}; a.s = {rk.sx, rk.sy, rk.sz}; a.tex = tRock; a.uvs = 2; a.t = {.8f, .8f, .8f}; draw(mSphereLo, a); }
    for (int i = 0; i < 5; i++) if (!picks[i].got) { float a = tAcc * 2 + i; DP d; d.p = picks[i].pos + V3{0, std::sin(tAcc * 2 + i) * .4f, 0}; d.s = {1.3f, 1.3f, 1.3f}; d.X = {std::cos(a), 0, -std::sin(a)}; d.Z = {std::sin(a), 0, std::cos(a)}; d.t = picks[i].col; d.emis = 2.f; draw(mBox, d); }
    drawRocket({0, rocketY, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
  } else {
    for (auto& p : planets) { if (p.isMoon && dot(p.pos - camPos, cF) < -p.R * 1.5f) continue; float c = std::cos(p.spin), s = std::sin(p.spin); DP d; d.p = p.pos; d.s = {p.R, p.R * p.squash, p.R}; d.X = {c, 0, -s}; d.Z = {s, 0, c}; d.tex = p.tex; d.emis = p.glow; d.mode = 3; draw(p.isMoon ? mSphereLo : mSphere, d); }
    for (auto& k : srocks) { V3 dv = k.pos - camPos; float dd = len(dv); if (dd > 45000.f || dot(dv, cF) < -k.r * 1.5f) continue;
      float c = std::cos(k.yaw), sn = std::sin(k.yaw); DP d; d.p = k.pos; d.s = {k.r * k.sx, k.r * k.sy, k.r * k.sz}; d.X = {c, 0, -sn}; d.Z = {sn, 0, c}; d.tex = tRock; d.uvs = 2; d.t = k.tint; draw(mSphereLo, d); }
    V3 r, u, f; basis(S.yaw, S.pitch, S.roll, r, u, f); drawRocket(S.pos, r, f, u);
  }
  glEnable(GL_BLEND); glDepthMask(GL_FALSE);
  if (st == SPACE) for (auto& p : planets) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (p.cloud) { DP d; float R = p.R * 1.012f, c = std::cos(p.spin * 1.5f), s = std::sin(p.spin * 1.5f); d.p = p.pos; d.s = {R, R * p.squash, R}; d.X = {c, 0, -s}; d.Z = {s, 0, c}; d.tex = p.cloudTex; d.t = p.cloudTint; draw(mSphere, d); }
    if (p.ring) { DP d; float R = p.R * p.ringScale, c = std::cos(p.tilt), s = std::sin(p.tilt); d.p = p.pos; d.s = {R, R, R}; d.Y = {0, c, s}; d.Z = {0, -s, c}; d.tex = p.ringTex; d.mode = 2; d.alpha = .85f; draw(mRing, d); }
    if (p.hasAtm) { glBlendFunc(GL_ONE, GL_ONE); float a0 = 1.04f + (p.uniq % 5) * .008f, a1 = 1.10f + (p.uniq % 7) * .012f; for (float sc : {a0, a1}) { DP d; float R = p.R * sc; d.p = p.pos; d.s = {R, R * p.squash, R}; d.t = p.atm; d.emis = (sc < a1 ? 1.6f : .7f) + p.glow; d.mode = 1; draw(mSphere, d); } }
  }
  // point sprites
  glBlendFunc(GL_ONE, GL_ONE); glEnable(GL_PROGRAM_POINT_SIZE); glUseProgram(pPt); glUniformMatrix4fv(uPtVP, 1, GL_FALSE, VP);
  glUniform1f(uPtScale, FH / (2 * std::tan(fov * PI / 360))); float sf = st == SPACE ? 1.f : st == LAUNCH ? clampf(rocketY / 800, 0, 1) : st == SURFACE ? (planets[curP].hasAtm ? 0.f : 1.f) : 0.f;
  if (sf > 0) drawPoints(starVAO, starVBO, nullptr, starN, sf);
  std::vector<PT> pts; V3 sd = sunDir * 70000.f; pts.push_back({sd.x, sd.y, sd.z, 10500, sunCol.x * .5f, sunCol.y * .5f, sunCol.z * .5f, 1}); pts.push_back({sd.x, sd.y, sd.z, 28000, sunCol.x * .12f, sunCol.y * .12f, sunCol.z * .12f, 1});
  { SysInfo si = sysInfo(sysId); if (si.binary && (st == SPACE || st == LAUNCH)) { V3 d2 = norm(V3{-sunDir.z + .35f, sunDir.y * -.5f + .25f, sunDir.x - .2f}) * 70000.f; pts.push_back({d2.x, d2.y, d2.z, 6500 * si.size, si.col2.x * .4f, si.col2.y * .4f, si.col2.z * .4f, 1}); pts.push_back({d2.x, d2.y, d2.z, 16000, si.col2.x * .08f, si.col2.y * .08f, si.col2.z * .08f, 1}); } }
  if (st == EARTH || st == COUNT || st == LAUNCH) for (int i = 0; i < 5; i++) if (!picks[i].got) for (int k = 0; k < 18; k++) { V3 q = picks[i].pos - camPos + V3{0, k * 4.f, 0}; pts.push_back({q.x, q.y, q.z, 22, picks[i].col.x, picks[i].col.y, picks[i].col.z, .16f}); }
  if (st == SURFACE && !inRoom) surfPoints(pts);
  for (auto& f : parts) if (f.l > 0) { float k = std::max(f.l, 0.f); V3 q = f.p - camPos; pts.push_back({q.x, q.y, q.z, f.sc * (2.3f - k * 1.3f), (1.f * k + .2f) * f.tint.x, (.25f + k * .45f) * k * f.tint.y, (.05f * k + (f.tint.z - .5f) * .6f * k) * (f.tint.z > .6f ? 1.f : 1.f), k * .9f}); }
  endingPoints(pts); drawPoints(ptVAO, ptVBO, pts.data(), (int)pts.size(), 1.f);
  if (st == SURFACE && !cine) { glDisable(GL_BLEND); glDepthMask(GL_TRUE); glClear(GL_DEPTH_BUFFER_BIT); useMain(); if (inRoom) { glUniform3f(uAmb, .62f, .57f, .52f); glUniform3f(uSunCol, .22f, .2f, .18f); glUniform1f(uFog, 0); }
    drawViewModel(); glEnable(GL_BLEND); glDepthMask(GL_FALSE); }   // the sword / pickaxe in your hand, always in front of the world
  fogCol = savedFog;
  // HUD
  glDisable(GL_DEPTH_TEST); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glUseProgram(pHud); glUniform2f(uHScr, (float)FW, (float)FH);
  float sc = std::max(2.f, std::floor(FH / 400.f)); V3 w{1, 1, 1}, dim{.7f, .85f, 1};
  if (endT >= 0) { endingHud(sc); if (flashA > 0) rect(0, 0, (float)FW, (float)FH, w, flashA); glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); return; }   // the ending has its own screen
  rect(FW / 2 - 2.f, FH / 2 - 2.f, 4, 4, w, .6f);
  if (st == EARTH) { std::string ob = gotN < 5 ? "Find the rocket parts (" + std::to_string(gotN) + "/5) - follow the glowing beams" : instN < gotN ? "Bring the parts to the launch pad" : instN < 5 ? "Keep searching" : "Rocket ready - stand on the pad and press {E} to launch";
    hudObj = "EARTH  -  PHASE 1\n" + ob; float nearD = 1e9f; for (auto& p : picks) if (!p.got) nearD = std::min(nearD, (float)std::hypot(P.x - p.pos.x, P.z - p.pos.z)); hudStat = nearD < 1e8f ? "Nearest part " + std::to_string((int)nearD) + " m" : "All parts collected"; }
  else if (st == SPACE) { Planet* np = nullptr; float nd = 1e9f; for (auto& p : planets) if (!p.found && !p.isMoon) { float d = len(S.pos - p.pos) - p.R; if (d < nd) { nd = d; np = &p; } }
    hudObj = "DEEP SPACE  -  " + sysInfo(sysId).name + "\n" + (np ? "Nearest unexplored world: " + np->name + " (" + std::to_string((int)(nd / 1000)) + " km)" : "Everything here is discovered");
    hudStat = "SPEED " + std::to_string((int)(S.speed * 3.6f)) + " km/s   THROTTLE " + std::to_string((int)(S.thr * 100)) + "%   ROCKET: " + ROCKET[rocketSel].name; }
  else if (st == SURFACE) surfHud(hudObj, hudStat);
  else { hudObj = "LAUNCH\nLeaving Earth"; hudStat = "ALT " + std::to_string((int)rocketY) + " m   VEL " + std::to_string((int)(vyL * 3.6f)) + " km/h"; }
  if (devWorld && !cine) { std::string dv = "DEVELOPER MODE  -  F1 dev tools" + std::string(devFlyOn ? "  -  FLY" : "") + (devNoclip ? "  -  NOCLIP" : "") + (devGod ? "  -  GOD" : "") + (devFast ? "  -  ROCKET x10" : ""); textC(dv, FW / 2.f, FH - 22.f * sc, sc * .9f, {1, .55f, .3f}, .9f); }
  if (!cine) { text(keysIn(hudObj), 22, 18, sc, dim); text(keysIn(hudStat), 22, FH - 26.f * sc - 10, sc, w, .85f); }
  if (st == SURFACE && !cine && !modal) {   // lives and food are written, bottom left above the status line
    float s2 = sc * 1.3f, yL = FH - 26.f * sc - 10 - 17.f * s2; bool blink = ((int)(tAcc * 3) & 1) != 0;
    std::string lv = "LIVES " + std::to_string(hp) + "/" + std::to_string(MAXHP); text(lv, 22, yL, s2, hp <= 3 && blink ? V3{1, .8f, .8f} : V3{1, .3f, .3f});
    if (difficulty > 0) { float fx = 22 + stb_easy_font_width((char*)lv.c_str()) * s2 + 28 * sc; std::string fd = "FOOD " + std::to_string((int)std::ceil(food)) + "/20"; text(fd, fx, yL, s2, starving && blink ? V3{.7f, .15f, .1f} : V3{.95f, .65f, .3f}); }
    int nf = foodTotal(); if (difficulty > 0 && nf > 0) text(keysIn(fmt("Carrying %d food  [{EAT}] eat", nf)), 22, yL - 14.f * sc, sc * .8f, V3{1, .8f, .6f});
    std::string tl = toolsLine(); if (!tl.empty()) text(tl, 22, yL - (difficulty > 0 && nf > 0 ? 26.f : 14.f) * sc, sc * .8f, V3{.75f, .85f, 1}); }
  if ((st == SPACE || st == SURFACE) && !cine) { float ry = 17 + 12 * sc * 1.4f + 22; for (int i = 0; i < NRES; i++) if (inv[i]) { std::string t = std::string(RES_NAME[i]) + "  " + std::to_string(inv[i]); text(t, FW - stb_easy_font_width((char*)t.c_str()) * sc * .8f - 26, ry, sc * .8f, RES_COL[i] * .6f + V3{.4f, .4f, .4f}); ry += 10 * sc * .8f + 3; } }
  if (st == SPACE || st == SURFACE) { int pd = discHere(), pt = totalHere(), md = moonsDiscHere(), mt = moonsTotalHere(); float s2 = sc * 1.4f;
    std::string a = "PLANETS " + std::to_string(pd) + "/" + std::to_string(pt), b = "MOONS " + std::to_string(md) + "/" + std::to_string(mt);
    float wa = stb_easy_font_width((char*)a.c_str()) * s2, wb = stb_easy_font_width((char*)b.c_str()) * s2, wg = 14 * s2, tw = wa + wg + wb; V3 gold{1, .9f, .4f}, green{.35f, 1, .45f};
    rect(FW - tw - 40, 10, tw + 28, 12 * s2 + 14, {0, 0, .02f}, .5f); float x0 = FW - tw - 26;
    text(a, x0, 17, s2, pt > 0 && pd == pt ? green : gold); text(b, x0 + wa + wg, 17, s2, mt > 0 && md == mt ? green : gold); }
  if (toastL > 0 && !paused) { float a = clampf(toastL / .8f, 0, 1) * clampf((toastMax - toastL) / .2f, 0, 1) + 0.f; std::string toastX = keysIn(toastT); size_t nl = toastX.find('\n'); std::string head = toastX.substr(0, nl), rest = nl == std::string::npos ? "" : toastX.substr(nl + 1);
    textC(head, FW / 2.f, FH * .18f, sc * 2.2f, w, a); if (!rest.empty()) { size_t p = 0; float y = FH * .18f + 18 * sc * 2.2f; while (p <= rest.size()) { size_t e = rest.find('\n', p); if (e == std::string::npos) e = rest.size(); textC(rest.substr(p, e - p), FW / 2.f, y, sc * 1.2f, dim, a); y += 14 * sc * 1.2f; p = e + 1; } } }
  if (st == SPACE && nearEnter >= 0) prompt = "[{E}] Descend to " + planets[nearEnter].name + (planets[nearEnter].isMoon ? "  (moon, 3 quests)" : "") + (planets[nearEnter].found ? "  (already discovered)" : "  - meet its people, finish their quests, discover it");
  else if (st == SPACE) prompt.clear();
  if (!prompt.empty() && !paused && !modal && !cine) textC(keysIn(prompt), FW / 2.f, FH * .62f, sc * 1.4f, {1, .9f, .4f});
  if (st == SURFACE && talkL > 0 && !modal && !cine) { float bw = FW * .7f, bx = (FW - bw) / 2, by = FH * .68f, bh = FH * .30f; rect(bx, by, bw, bh, {0, 0, .02f}, .78f); text(talkS, bx + 16, by + 12, sc * 1.4f, {1, .85f, .3f});
    std::string t = keysIn(talkT), line; float y = by + 14 + 14 * sc * 1.4f; size_t maxc = (size_t)(bw / (6.f * sc * 1.2f)); size_t pos = 0, lim = (size_t)std::max(0, talkShown);
    while (pos < t.size() && pos < lim) { size_t e = std::min(t.size(), pos + maxc); if (e < t.size()) { size_t sp = t.rfind(' ', e); if (sp != std::string::npos && sp > pos) e = sp; } text(t.substr(pos, std::min(e, lim) - pos), bx + 16, y, sc * 1.2f, w); y += 12 * sc * 1.2f; pos = e + (e < t.size() && t[e] == ' ' ? 1 : 0); }
    { std::string cl = keysIn("[{E}] close"); text(cl, bx + bw - 16 - stb_easy_font_width((char*)cl.c_str()) * sc, by + bh - 14 * sc, sc, dim, .7f); } }
  if (savedL > 0) { std::string sv = "Progress saved"; text(sv, FW - stb_easy_font_width((char*)sv.c_str()) * sc - 22, FH - 26.f * sc - 10, sc, {.6f, 1, .7f}, clampf(savedL, 0, 1) * .9f); }
  if (flashA > 0) rect(0, 0, (float)FW, (float)FH, w, flashA);
  glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE);
}
