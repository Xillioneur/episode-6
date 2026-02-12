#ifndef AUDIO_MANAGER_HPP
#define AUDIO_MANAGER_HPP

#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <mutex>

enum class SoundType {
    SHOOT, STEP, DASH, RELOAD, HIT, PICKUP, POWERUP, SANITIZE, ALERT
};

struct SoundInstance {
    SoundType type;
    float phase;
    float phase2;
    float elapsed;
    float duration;
    float volume;
    float freq;
    float pan; // -1.0 (left) to 1.0 (right)
    bool active = false;
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();
    void play(SoundType type, float vol = 0.2f, float freq = 440.0f, float pan = 0.0f);
    void updateAmbient(float dt);
    static void audioCallback(void* userdata, Uint8* stream, int len);

private:
    SDL_AudioDeviceID device;
    SoundInstance sounds[32];
    float ambientPhase = 0.0f;
    float ambientVolume = 0.04f;
    
    // Simple Reverb
    float delayBuffer[8820]; // ~200ms at 44.1k
    int delayIdx = 0;
    
    std::mutex audioMutex;
    void fillBuffer(float* buffer, int samples);
};

#endif
