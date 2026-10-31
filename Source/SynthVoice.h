#pragma once
#include "DSPUtils.h"
#include "Params.h"
#include "Wavetables.h"

// Snapshot of all synth-engine parameters, refreshed once per audio block
struct SynthParams
{
    struct Osc
    {
        bool on = true; int wt = 0; float pos = 0.5f; int oct = 0, semi = 0; float fine = 0.0f;
        int uni = 1; float det = 0.25f, blend = 0.75f, width = 0.8f;
        int warp = 0; float warpAmt = 0.0f, level = 0.75f, pan = 0.0f, phase = 0.0f, rand = 1.0f;
    } osc[2];

    bool subOn = false; int subWave = 0, subOct = 0; float subLevel = 0.6f; bool subDirect = true;
    bool noiseOn = false; int noiseType = 0; float noiseLevel = 0.3f;

    bool fOn = true; int fType = 0; float cutoff = 20000.0f, res = 0.1f, drive = 0.0f, key = 0.0f, envAmt = 0.0f;

    struct Env { float a = 0.002f, d = 0.3f, s = 1.0f, r = 0.15f; } env[3];
    struct Lfo { int shape = 0; float rate = 2.0f; bool sync = true; int div = 8; int mode = 1; float delay = 0.0f; } lfo[2];
    struct Slot { int src = 0, dst = 0; float amt = 0.0f; } mod[kNumModSlots];
    float macro[4] {};

    int voices = 8, mode = 0, chord = 0; float glide = 0.0f; int bend = 2; float analog = 0.35f, velSens = 0.5f;
};

// Per-block global state shared by all voices
struct GlobalState
{
    double sampleRate = 44100.0;
    double bpm = 124.0;
    float pitchBendSemis = 0.0f;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    double lfoPhase[2] { 0.0, 0.0 };
    const WavetableBank* bank = nullptr;
};

class SynthVoice
{
public:
    static constexpr int kCtrl = 8;   // control-rate interval (samples)

    void prepare (double sampleRate, uint32_t seed);
    void noteOn (int triggerNote, float pitchNote, float velocity, float glideFrom, bool legato,
                 const SynthParams& P, const GlobalState& G, juce::uint64 order);
    void noteOff() noexcept;
    void kill() noexcept;
    void render (float* L, float* R, int numSamples, const SynthParams& P, const GlobalState& G) noexcept;

    bool isActive() const noexcept     { return active; }
    bool isGateOn() const noexcept     { return gate; }
    int  getTriggerNote() const noexcept { return triggerNote; }
    float getPitchNote() const noexcept  { return noteTarget; }
    float getLevel() const noexcept      { return env[0].level; }
    juce::uint64 getOrder() const noexcept { return order; }

    // Glide target change while gate is held (mono / legato modes)
    void setPitchTarget (float pitchNote) noexcept { noteTarget = pitchNote; }

private:
    void updateControl (const SynthParams& P, const GlobalState& G) noexcept;
    void renderChunk (float* L, float* R, int n, const SynthParams& P, const GlobalState& G) noexcept;
    inline float noise() noexcept
    {
        rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
        return (float) (int32_t) rngState * (1.0f / 2147483648.0f);
    }

    double sr = 44100.0;
    bool active = false, gate = false;
    int triggerNote = -1;
    float noteTarget = 60.0f, noteCurrent = 60.0f, velocity = 1.0f, noteRandom = 0.0f;
    juce::uint64 order = 0;
    int ctrlLeft = 0;

    dsp::Envelope env[3];
    dsp::Lfo lfo[2];
    double samplesSinceOn = 0.0;

    // modulation results
    float srcVal[NumModSrc] {};
    float dstVal[NumModDst] {};

    // oscillators
    float phase[2][kMaxUnison] {};
    float inc[2][kMaxUnison] {};
    int   mip[2][kMaxUnison] {};
    float gainL[2][kMaxUnison] {}, gainR[2][kMaxUnison] {}, gainM[2][kMaxUnison] {};
    float drift[2][kMaxUnison] {}, driftTarget[2][kMaxUnison] {};
    int   driftCount[2][kMaxUnison] {};
    float curPos[2] {}, curWarp[2] {};
    int   curUni[2] { 1, 1 };
    float oscMono[2] {};
    float voiceDetune = 0.0f, cutoffOffset = 0.0f;

    // sub / noise
    float subPhase = 0.0f, subInc = 0.0f, subGain = 0.0f, noiseGain = 0.0f;
    float pinkL[7] {}, pinkR[7] {}, crackle = 0.0f;
    uint32_t rngState = 0x12345678u;

    // filter
    dsp::Ladder ladder[2];
    dsp::SVF svfA[2], svfB[2];
    float g = 0.5f, gTarget = 0.5f, gStep = 0.0f, res = 0.0f, drive = 0.0f;
    bool firstControl = true;
    float driftCoef = 0.001f;

    // amp
    float ampMod = 1.0f, pan = 0.0f, panL = 1.0f, panR = 1.0f, velGain = 1.0f;
    float ampModTarget = 1.0f, panLT = 1.0f, panRT = 1.0f;
};
