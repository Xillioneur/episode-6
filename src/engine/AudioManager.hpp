#ifndef AUDIO_MANAGER_HPP
#define AUDIO_MANAGER_HPP

#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <mutex>

enum class SoundType {
    SHOOT, STEP, DASH, RELOAD, HIT, PICKUP, POWERUP, SANITIZE, ALERT,
    RICOCHET, EMPTY, BOSS_PHASE, UI_CLICK, UI_CONFIRM, EMP_SHOT, PIERCE_SHOT,
    SHIELD_DOWN, LOW_ENERGY, DRIP, MACHINERY, STEAM, ECHO_VOICE, ZAP,
    SHIELD_CHARGE, READY, BOSS_DIE
};

struct SoundInstance {
    SoundType type;
    float phase;
    float phase2;
    float elapsed;
    float duration;
    float volume;
    float freq;
    float pan;
    bool active = false;
};

enum class AmbientState { STANDARD, BATTLE, BOSS };

class AudioManager {
public:
    AudioManager();
    ~AudioManager();
    void play(SoundType type, float vol = 0.2f, float freq = 440.0f, float pan = 0.0f);
    void setAmbientState(AmbientState state);
    static void audioCallback(void* userdata, Uint8* stream, int len);

private:
    SDL_AudioDeviceID device;
    SoundInstance sounds[32];
    float ambientPhase = 0.0f;
    float ambientPhase2 = 0.0f;
    float ambientVolume = 0.04f;
    float ambientFreq = 55.0f;
    float targetAmbientFreq = 55.0f;
    
    float delayBuffer[8820];
    int delayIdx = 0;
    
    std::mutex audioMutex;
    void fillBuffer(float* buffer, int samples);
};

#endif
