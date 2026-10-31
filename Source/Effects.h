#pragma once
#include <juce_dsp/juce_dsp.h>
#include "DSPUtils.h"

struct FxParams
{
    bool distOn = false; int distType = 0; float distDrive = 0.3f, distTone = 0.7f, distMix = 1.0f;
    bool eqOn = false; float eqLow = 0.0f, eqMid = 0.0f, eqMidF = 1000.0f, eqHigh = 0.0f;
    bool choOn = false; float choRate = 0.6f, choDepth = 0.35f, choMix = 0.4f;
    bool phaOn = false; float phaRate = 0.3f, phaDepth = 0.6f, phaFb = 0.4f, phaMix = 0.5f;
    bool dlyOn = false, dlySync = true; int dlyDiv = 6; float dlyTime = 350.0f, dlyFb = 0.35f; bool dlyPP = true;
    float dlyTone = 0.6f, dlyMix = 0.2f;
    bool revOn = false; float revSize = 0.6f, revDamp = 0.5f, revWidth = 1.0f, revMix = 0.2f;
    bool compOn = false; float compThresh = -18.0f, compRatio = 4.0f, compAtt = 10.0f, compRel = 120.0f, compGain = 0.0f;
    bool pumpOn = false; int pumpDiv = 2; float pumpDepth = 0.6f, pumpCurve = 0.5f;
};

struct TransportInfo
{
    double bpm = 124.0;
    bool playing = false;
    double ppq = 0.0;   // quarter-note position at the start of the block (valid if playing)
};

// ============================================================================
//  Master FX chain: Dist -> EQ -> Chorus -> Phaser -> Delay -> Reverb -> Comp -> Pump
// ============================================================================
class EffectsChain
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FxParams& p, const TransportInfo& t);

private:
    void processDistortion (float* L, float* R, int n, const FxParams& p);
    void processDelay (float* L, float* R, int n, const FxParams& p, double bpm);
    void processPump (float* L, float* R, int n, const FxParams& p, const TransportInfo& t);
    void updateEq (const FxParams& p);

    double sr = 44100.0;
    int maxBlockSize = 512;

    // distortion
    dsp::OnePole distLp[2];
    dsp::DCBlocker distDc[2];
    float crushHold[2] {}, crushCount = 0.0f;

    // EQ
    juce::dsp::IIR::Filter<float> eqLow[2], eqMid[2], eqHigh[2];
    float lastLow = 99.0f, lastMid = 99.0f, lastMidF = -1.0f, lastHigh = 99.0f;

    // modulation FX
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;
    juce::AudioBuffer<float> dryBuf;

    // delay
    std::vector<float> dlyBuf[2];
    int dlyWrite = 0;
    float dlyTimeSmoothed = -1.0f;
    dsp::OnePole dlyLp[2], dlyHp[2];

    // reverb
    juce::dsp::Reverb reverb;

    // compressor
    juce::dsp::Compressor<float> comp;

    // pump
    double pumpFreePhase = 0.0;
    float pumpGain = 1.0f;
};
