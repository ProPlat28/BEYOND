#pragma once
// Fully procedural audio: no sound files, everything is synthesized in the callback.
enum Snd { S_STEP, S_CHIME, S_CLANG, S_BEEP_LO, S_BEEP_HI, S_WHOOSH, S_DISCOVER, S_JUMP };
bool audioInit();
void audioShutdown();
void audioEvent(int snd);                                          // one-shot sound
void audioSet(float wind, float rumble, float rumbleHz, float drone); // ambient mix (0..1)
void audioMute(bool m);
