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
            else if (type == SoundType::RICOCHET) sounds[i].duration = 0.1f;
            else if (type == SoundType::EMPTY) sounds[i].duration = 0.08f;
            else if (type == SoundType::BOSS_PHASE) sounds[i].duration = 1.2f;
            else if (type == SoundType::UI_CLICK) sounds[i].duration = 0.05f;
            else if (type == SoundType::UI_CONFIRM) sounds[i].duration = 0.3f;
            else if (type == SoundType::EMP_SHOT) sounds[i].duration = 0.4f;
            else if (type == SoundType::PIERCE_SHOT) sounds[i].duration = 0.5f;
            else if (type == SoundType::SHIELD_DOWN) sounds[i].duration = 0.6f;
            else if (type == SoundType::LOW_ENERGY) sounds[i].duration = 0.2f;
            else sounds[i].duration = 0.2f;
            break;
        }
    }
}

void AudioManager::audioCallback(void* userdata, Uint8* stream, int len) {
    AudioManager* am = (AudioManager*)userdata;
    float* buffer = (float*)stream;
    int samples = len / sizeof(float);
    std::fill(buffer, buffer + samples, 0.0f);
    am->fillBuffer(buffer, samples);
}

void AudioManager::setAmbientState(AmbientState state) {
    std::lock_guard<std::mutex> lock(audioMutex);
    if (state == AmbientState::STANDARD) targetAmbientFreq = 55.0f;
    else if (state == AmbientState::BATTLE) targetAmbientFreq = 82.0f;
    else if (state == AmbientState::BOSS) targetAmbientFreq = 41.0f;
}

void AudioManager::fillBuffer(float* buffer, int samples) {
    std::lock_guard<std::mutex> lock(audioMutex);
    float dt = 1.0f / 44100.0f;
    int frames = samples / 2;

    for (int f = 0; f < frames; ++f) {
        // Smooth ambient freq transition
        ambientFreq += (targetAmbientFreq - ambientFreq) * 0.0001f;
        
        float mono = 0.0f;
        // Layered Ambient: 3 Sines + 1 slow modulator
        float l1 = std::sin(ambientPhase);
        float l2 = std::sin(ambientPhase * 0.501f) * 0.8f;
        float l3 = std::sin(ambientPhase * 2.002f) * 0.3f;
        float mod = 0.5f + 0.5f * std::sin(ambientPhase2);
        
        // Add a "wind" whirring layer using modulated noise
        float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f);
        float wind = noise * (0.2f + 0.3f * std::sin(ambientPhase2 * 0.5f));
        
        float amb = (l1 + l2 + l3) * mod + wind * 0.2f;

        mono += amb * ambientVolume;
        ambientPhase += 2.0f * M_PI * ambientFreq * dt;
        ambientPhase2 += 2.0f * M_PI * 0.15f * dt; // Slow 0.15Hz modulation
        
        if (ambientPhase > 2.0f * M_PI * 100.0f) ambientPhase -= 2.0f * M_PI * 100.0f;
        if (ambientPhase2 > 2.0f * M_PI * 100.0f) ambientPhase2 -= 2.0f * M_PI * 100.0f;

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
                float transient = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * std::exp(-t * 100.0f);
                float bodyFreq = freq * std::exp(-t * 15.0f);
                float body = (std::sin(s.phase) > 0 ? 0.8f : -0.8f) * std::exp(-t * 10.0f);
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
            } else if (s.type == SoundType::RICOCHET) {
                float ping = std::sin(s.phase) * std::exp(-t * 25.0f);
                float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * std::exp(-t * 40.0f);
                val = ping * 0.6f + noise * 0.4f;
                s.phase += 2.0f * M_PI * (freq + 1000.0f * t) * dt;
            } else if (s.type == SoundType::EMPTY) {
                val = (std::sin(s.phase) > 0 ? 1.0f : -1.0f) * std::exp(-t * 50.0f);
                s.phase += 2.0f * M_PI * 150.0f * dt;
            } else if (s.type == SoundType::BOSS_PHASE) {
                float sub = std::sin(s.phase) * (1.0f - t);
                float texture = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.2f * std::sin(s.phase * 0.1f);
                val = sub + texture;
                s.phase += 2.0f * M_PI * (60.0f + 100.0f * t) * dt;
            } else if (s.type == SoundType::UI_CLICK) {
                val = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * std::exp(-t * 80.0f);
            } else if (s.type == SoundType::UI_CONFIRM) {
                float f_sel = freq * (t < 0.5f ? 1.0f : 1.5f);
                float sub = std::sin(s.phase);
                float harm1 = std::sin(s.phase * 2.0f) * 0.5f;
                float harm2 = std::sin(s.phase * 3.0f) * 0.25f;
                val = (sub + harm1 + harm2) * env;
                s.phase += 2.0f * M_PI * f_sel * dt;
            } else if (s.type == SoundType::EMP_SHOT) {
                float buzz = (std::sin(s.phase) * std::sin(s.phase * 1.05f)) * (1.0f - t);
                float crackle = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.3f * (1.0f - t);
                val = buzz + crackle;
                s.phase += 2.0f * M_PI * (freq + std::sin(t * 50.0f) * 100.0f) * dt;
            } else if (s.type == SoundType::PIERCE_SHOT) {
                float whistle = std::sin(s.phase) * std::exp(-t * 2.0f);
                float heavy = (std::sin(s.phase * 0.5f) > 0 ? 1.0f : -1.0f) * std::exp(-t * 10.0f);
                val = whistle * 0.4f + heavy * 0.7f;
                s.phase += 2.0f * M_PI * freq * std::exp(-t * 5.0f) * dt;
            } else if (s.type == SoundType::SHIELD_DOWN) {
                val = (std::sin(s.phase) * std::sin(s.phase * 0.5f)) * (1.0f - t);
                s.phase += 2.0f * M_PI * (freq - 400.0f * t) * dt;
            } else if (s.type == SoundType::LOW_ENERGY) {
                val = std::sin(s.phase) * (std::sin(2.0f * M_PI * 10.0f * s.elapsed) > 0 ? 1.0f : 0.0f);
                s.phase += 2.0f * M_PI * 1500.0f * dt;
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