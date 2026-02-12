#include "AudioManager.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AudioManager::AudioManager() {
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audioCallback;
    want.userdata = this;

    device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!device) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
    } else {
        SDL_PauseAudioDevice(device, 0);
    }
    for (int i = 0; i < 32; ++i) sounds[i].active = false;
    std::fill(delayBuffer, delayBuffer + 8820, 0.0f);
}

AudioManager::~AudioManager() {
    if (device) SDL_CloseAudioDevice(device);
}

void AudioManager::play(SoundType type, float vol, float freq, float pan) {
    std::lock_guard<std::mutex> lock(audioMutex);
    for (int i = 0; i < 32; ++i) {
        if (!sounds[i].active) {
            sounds[i].type = type;
            sounds[i].volume = vol;
            sounds[i].freq = freq;
            sounds[i].pan = pan;
            sounds[i].phase = 0;
            sounds[i].phase2 = 0;
            sounds[i].elapsed = 0;
            sounds[i].active = true;
            if (type == SoundType::SHOOT) sounds[i].duration = 0.3f;
            else if (type == SoundType::STEP) sounds[i].duration = 0.12f;
            else if (type == SoundType::DASH) sounds[i].duration = 0.4f;
            else if (type == SoundType::RELOAD) sounds[i].duration = 0.25f;
            else if (type == SoundType::HIT) sounds[i].duration = 0.35f;
            else if (type == SoundType::PICKUP) sounds[i].duration = 0.4f;
            else if (type == SoundType::SANITIZE) sounds[i].duration = 0.8f;
            else if (type == SoundType::ALERT) sounds[i].duration = 0.15f;
            else sounds[i].duration = 0.2f;
            break;
        }
    }
}

void AudioManager::audioCallback(void* userdata, Uint8* stream, int len) {
    AudioManager* am = (AudioManager*)userdata;
    float* buffer = (float*)stream;
    int samples = len / sizeof(float); // This is total floats (channels * frames)
    std::fill(buffer, buffer + samples, 0.0f);
    am->fillBuffer(buffer, samples);
}

void AudioManager::fillBuffer(float* buffer, int samples) {
    std::lock_guard<std::mutex> lock(audioMutex);
    float dt = 1.0f / 44100.0f;
    int frames = samples / 2;

    for (int f = 0; f < frames; ++f) {
        float mono = 0.0f;
        
        // Ambient
        float amb = (std::sin(ambientPhase) * 0.4f + std::sin(ambientPhase * 0.49f) * 0.3f + std::sin(ambientPhase * 0.1f) * 0.3f);
        mono += amb * ambientVolume;
        ambientPhase += 2.0f * M_PI * 55.0f * dt;

        float left = 0, right = 0;

        for (int i = 0; i < 32; ++i) {
            if (!sounds[i].active) continue;
            SoundInstance& s = sounds[i];
            float t = s.elapsed / s.duration;
            if (t >= 1.0f) { s.active = false; continue; }

            float val = 0;
            float freq = s.freq;
            float env = std::exp(-t * 5.0f) * (1.0f - t);

            if (s.type == SoundType::SHOOT) {
                // Layer 1: Transient
                float transient = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * std::exp(-t * 100.0f);
                // Layer 2: Body
                float bodyFreq = freq * std::exp(-t * 15.0f);
                float body = (std::sin(s.phase) > 0 ? 0.8f : -0.8f) * std::exp(-t * 10.0f);
                // Layer 3: Noise Tail
                float tail = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * std::exp(-t * 4.0f) * 0.4f;
                val = (transient * 0.5f + body * 0.6f + tail * 0.3f);
                s.phase += 2.0f * M_PI * bodyFreq * dt;
            } else if (s.type == SoundType::STEP) {
                float thud = std::sin(s.phase) * std::exp(-t * 20.0f);
                float scuff = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * std::exp(-t * 30.0f) * 0.5f;
                val = thud + scuff;
                s.phase += 2.0f * M_PI * 80.0f * dt;
            } else if (s.type == SoundType::DASH) {
                float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f);
                float sweep = std::exp(-t * 3.0f);
                val = noise * sweep * std::sin(s.phase);
                s.phase += 2.0f * M_PI * (200.0f + 1000.0f * (1.0f - t)) * dt;
            } else if (s.type == SoundType::RELOAD) {
                float mechanical = (std::fmod(s.elapsed, 0.06f) < 0.015f) ? (std::sin(s.phase) > 0 ? 1.0f : -1.0f) : 0;
                val = mechanical * env;
                s.phase += 2.0f * M_PI * 1200.0f * dt;
            } else if (s.type == SoundType::HIT) {
                float crunch = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * std::exp(-t * 20.0f);
                float impact = std::sin(s.phase) * std::exp(-t * 10.0f);
                val = crunch * 0.7f + impact * 0.5f;
                s.phase += 2.0f * M_PI * freq * dt;
            } else if (s.type == SoundType::PICKUP) {
                float harmonic = std::sin(s.phase) + 0.5f * std::sin(s.phase * 2.01f) + 0.25f * std::sin(s.phase * 3.02f);
                val = harmonic * env;
                s.phase += 2.0f * M_PI * freq * (1.0f + t) * dt;
            } else if (s.type == SoundType::SANITIZE) {
                float pulse = std::sin(2.0f * M_PI * 10.0f * s.elapsed);
                float tone = std::sin(s.phase) * (0.5f + 0.5f * pulse);
                val = tone * (1.0f - t);
                s.phase += 2.0f * M_PI * (freq - 200.0f * t) * dt;
            } else if (s.type == SoundType::ALERT) {
                val = (std::sin(s.phase) > 0 ? 0.5f : -0.5f) * (std::sin(2.0f * M_PI * 15.0f * s.elapsed) > 0 ? 1.0f : 0.0f);
                s.phase += 2.0f * M_PI * freq * dt;
            }

            float sVol = val * s.volume;
            left += sVol * std::min(1.0f, 1.0f - s.pan);
            right += sVol * std::min(1.0f, 1.0f + s.pan);
            s.elapsed += dt;
        }

        float outL = mono + left;
        float outR = mono + right;

        // Reverb / Delay
        float delayed = delayBuffer[delayIdx];
        outL += delayed * 0.3f;
        outR += delayed * 0.35f; // Slight offset for stereo width
        delayBuffer[delayIdx] = (outL + outR) * 0.5f * 0.4f; // Feedback
        delayIdx = (delayIdx + 1) % 8820;

        buffer[f * 2] = std::clamp(outL, -1.0f, 1.0f);
        buffer[f * 2 + 1] = std::clamp(outR, -1.0f, 1.0f);
    }
}