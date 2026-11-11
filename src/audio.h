#pragma once
// Synthesized sound effects plus recorded assets (assets/audio): looping music, footsteps per planet kind and alien voices per body plan.
// Every recorded sound is optional: when its file is missing the game keeps the synthesized sound (or silence for the music).
// voice blips (Animal Crossing style babble, not real speech): audioEvent(S_VOICE + kind * 100 + pitch), kind 0 soft, 1 round, 2 low and gruff, pitch 0..99 (about 140 Hz to 2 kHz)
inline const int S_VOICE = 1000;
enum Snd { S_STEP, S_CHIME, S_CLANG, S_BEEP_LO, S_BEEP_HI, S_WHOOSH, S_DISCOVER, S_JUMP, S_CLICK };
enum Music { MUS_NONE = -1, MUS_MENU, MUS_SPACE, MUS_VERDANT, MUS_DUNES, MUS_FROST, MUS_EMBER, MUS_ANOMALY, MUS_COUNT };
enum VoiceLine { VL_HELLO, VL_CHATTER, VL_QUESTION, VL_LAUGH, VL_BYE, VL_COUNT };
bool audioInit();
void audioShutdown();
void audioEvent(int snd);                                          // one-shot sound
void audioSet(float wind, float rumble, float rumbleHz, float drone); // ambient mix (0..1); drone is unused (the menu/space drone was removed on request)
void audioMute(bool m);
void audioVolumes(float sound, float music);                       // 0..1 each: every sound effect / footstep / voice, and the music
void audioMusic(int track);                                        // Music id; crossfades from the current track, call it every frame
bool audioStep(int planetKind, bool run, bool land);               // recorded footstep for that planet kind; false = none loaded, play the synthesized one
float audioVoice(int body, int line, float rate);                  // alien voice line (Alien::body, VoiceLine), replaces the one playing; returns its length in seconds, 0 = none loaded
void audioVoiceStop();
void audioCut();                                                    // every game sound stops at once (back to the main menu)
void audioPauseMusic(bool p);                                       // true = pause menu: music, footsteps, voices and the engine hold their place (not muted); false = they carry on
