#include "Effects.h"
#include "Params.h"

using namespace dsp;

void EffectsChain::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    maxBlockSize = juce::jmax (1, maxBlock);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 2 };
    chorus.prepare (spec);
    phaser.prepare (spec);
    reverb.prepare (spec);
    comp.prepare (spec);

    juce::dsp::ProcessSpec mono { sr, (juce::uint32) maxBlockSize, 1 };
    for (int c = 0; c < 2; ++c)
    {
        eqLow[c].prepare (mono); eqMid[c].prepare (mono); eqHigh[c].prepare (mono);
        dlyBuf[c].assign ((size_t) (sr * 4.0) + (size_t) maxBlockSize + 8, 0.0f);
    }
    dryBuf.setSize (2, maxBlockSize);
    lastLow = lastMid = lastHigh = 99.0f;
    lastMidF = -1.0f;
    reset();
}

void EffectsChain::reset()
{
    for (int c = 0; c < 2; ++c)
    {
        distLp[c].reset(); distDc[c].reset(); dlyLp[c].reset(); dlyHp[c].reset();
        eqLow[c].reset(); eqMid[c].reset(); eqHigh[c].reset();
        std::fill (dlyBuf[c].begin(), dlyBuf[c].end(), 0.0f);
        crushHold[c] = 0.0f;
    }
    crushCount = 0.0f;
    dlyWrite = 0;
    dlyTimeSmoothed = -1.0f;
    chorus.reset(); phaser.reset(); reverb.reset(); comp.reset();
    pumpFreePhase = 0.0;
    pumpGain = 1.0f;
}

void EffectsChain::updateEq (const FxParams& p)
{
    using C = juce::dsp::IIR::Coefficients<float>;
    if (! juce::exactlyEqual (p.eqLow, lastLow))
    {
        auto c = C::makeLowShelf (sr, 120.0, 0.7f, juce::Decibels::decibelsToGain (p.eqLow));
        eqLow[0].coefficients = c; eqLow[1].coefficients = c; lastLow = p.eqLow;
    }
    if (! juce::exactlyEqual (p.eqMid, lastMid) || ! juce::exactlyEqual (p.eqMidF, lastMidF))
    {
        const double f = juce::jlimit (20.0, sr * 0.45, (double) p.eqMidF);
        auto c = C::makePeakFilter (sr, f, 0.9f, juce::Decibels::decibelsToGain (p.eqMid));
        eqMid[0].coefficients = c; eqMid[1].coefficients = c; lastMid = p.eqMid; lastMidF = p.eqMidF;
    }
    if (! juce::exactlyEqual (p.eqHigh, lastHigh))
    {
        const double f = juce::jmin (7000.0, sr * 0.4);
        auto c = C::makeHighShelf (sr, f, 0.7f, juce::Decibels::decibelsToGain (p.eqHigh));
        eqHigh[0].coefficients = c; eqHigh[1].coefficients = c; lastHigh = p.eqHigh;
    }
}

void EffectsChain::processDistortion (float* L, float* R, int n, const FxParams& p)
{
    const float drive = p.distDrive;
    const float g = juce::Decibels::decibelsToGain (drive * 36.0f);
    const float makeup = 1.0f / (1.0f + drive * 1.5f);
    const float fc = juce::jmin (800.0f * std::pow (25.0f, p.distTone), 0.45f * (float) sr);
    const float a = 1.0f - std::exp (-kTwoPi * fc / (float) sr);
    const float mix = p.distMix;
    const float levels = std::exp2 (16.0f - drive * 13.0f);
    const float holdLen = 1.0f + drive * 15.0f;
    float* ch[2] = { L, R };

    for (int i = 0; i < n; ++i)
    {
        bool takeSample = true;
        if (p.distType == 4)
        {
            crushCount += 1.0f;
            takeSample = crushCount >= holdLen;
            if (takeSample) crushCount -= holdLen;
        }
        for (int c = 0; c < 2; ++c)
        {
            const float x = ch[c][i];
            float y;
            switch (p.distType)
            {
                case 1:  y = juce::jlimit (-1.0f, 1.0f, x * g); break;
                case 2:  { const float v = x * g; y = distDc[c].process (fastTanh (v + 0.3f * v * v)); break; }
                case 3:  y = std::sin (x * (1.0f + drive * 8.0f) * 1.5f); break;
                case 4:
                    if (takeSample) crushHold[c] = std::round (x * levels) / levels;
                    y = crushHold[c];
                    break;
                default: y = fastTanh (x * g); break;
            }
            if (p.distType != 4 && p.distType != 3) y *= makeup;
            y = distLp[c].lp (y, a);
            ch[c][i] = x + (y - x) * mix;
        }
    }
}

void EffectsChain::processDelay (float* L, float* R, int n, const FxParams& p, double bpm)
{
    const int size = (int) dlyBuf[0].size();
    double ms = p.dlySync ? (60000.0 / juce::jmax (20.0, bpm)) * divisionToBeats (p.dlyDiv) : (double) p.dlyTime;
    const float target = (float) juce::jlimit (1.0, 3900.0, ms) * 0.001f * (float) sr;
    if (dlyTimeSmoothed < 0.0f) dlyTimeSmoothed = target;

    const float lpA = 1.0f - std::exp (-kTwoPi * juce::jmin (1000.0f * std::pow (18.0f, p.dlyTone), 0.45f * (float) sr) / (float) sr);
    const float hpA = 1.0f - std::exp (-kTwoPi * 120.0f / (float) sr);
    float* bl = dlyBuf[0].data();
    float* br = dlyBuf[1].data();

    for (int i = 0; i < n; ++i)
    {
        dlyTimeSmoothed += (target - dlyTimeSmoothed) * 0.0005f;
        float rp = (float) dlyWrite - dlyTimeSmoothed;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) % size;
        const float fr = rp - (float) i0;
        const float dL = bl[i0] + (bl[i1] - bl[i0]) * fr;
        const float dR = br[i0] + (br[i1] - br[i0]) * fr;

        // feedback conditioning (tone LP + 120 Hz HP + soft clip)
        auto cond = [&] (float x, int c)
        {
            x = dlyLp[c].lp (x, lpA);
            x = x - dlyHp[c].lp (x, hpA);
            return fastTanh (x);
        };

        const float xl = L[i], xr = R[i];
        float wl, wr;
        if (p.dlyPP)
        {
            wl = cond ((xl + xr) * 0.5f + p.dlyFb * dR, 0);
            wr = cond (p.dlyFb * dL, 1);
        }
        else
        {
            wl = cond (xl + p.dlyFb * dL, 0);
            wr = cond (xr + p.dlyFb * dR, 1);
        }
        bl[dlyWrite] = wl;
        br[dlyWrite] = wr;
        if (++dlyWrite >= size) dlyWrite = 0;

        L[i] = xl + dL * p.dlyMix;
        R[i] = xr + dR * p.dlyMix;
    }
}

void EffectsChain::processPump (float* L, float* R, int n, const FxParams& p, const TransportInfo& t)
{
    static const double beatsTable[] = { 0.25, 0.5, 1.0, 2.0 };
    const double beats = beatsTable[juce::jlimit (0, 3, p.pumpDiv)];
    const double phaseInc = (t.bpm / 60.0) / sr / beats;
    double ph = t.playing ? (t.ppq / beats - std::floor (t.ppq / beats)) : pumpFreePhase;
    const float expo = 1.0f + p.pumpCurve * 6.0f;

    for (int i = 0; i < n; ++i)
    {
        const float target = 1.0f - p.pumpDepth * std::pow (1.0f - (float) ph, expo);
        pumpGain += (target - pumpGain) * 0.03f;
        L[i] *= pumpGain;
        R[i] *= pumpGain;
        ph += phaseInc;
        if (ph >= 1.0) ph -= 1.0;
    }
    pumpFreePhase = ph;
}

void EffectsChain::process (juce::AudioBuffer<float>& buffer, const FxParams& p, const TransportInfo& t)
{
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    if (p.distOn) processDistortion (L, R, n, p);

    if (p.eqOn)
    {
        updateEq (p);
        for (int i = 0; i < n; ++i)
        {
            L[i] = eqHigh[0].processSample (eqMid[0].processSample (eqLow[0].processSample (L[i])));
            R[i] = eqHigh[1].processSample (eqMid[1].processSample (eqLow[1].processSample (R[i])));
        }
    }

    juce::dsp::AudioBlock<float> block (buffer);
    block = block.getSubsetChannelBlock (0, 2);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    if (p.choOn)
    {
        chorus.setRate (p.choRate);
        chorus.setDepth (p.choDepth);
        chorus.setCentreDelay (7.0f);
        chorus.setFeedback (0.0f);
        chorus.setMix (p.choMix);
        chorus.process (ctx);
    }

    if (p.phaOn)
    {
        phaser.setRate (p.phaRate);
        phaser.setDepth (p.phaDepth);
        phaser.setFeedback (p.phaFb);
        phaser.setCentreFrequency (900.0f);
        phaser.setMix (p.phaMix);
        phaser.process (ctx);
    }

    if (p.dlyOn) processDelay (L, R, n, p, t.bpm);

    if (p.revOn)
    {
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = p.revSize;
        rp.damping  = p.revDamp;
        rp.width    = p.revWidth;
        rp.wetLevel = p.revMix * 0.6f;
        rp.dryLevel = 1.0f - p.revMix * 0.4f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        reverb.process (ctx);
    }

    if (p.compOn)
    {
        comp.setThreshold (p.compThresh);
        comp.setRatio (p.compRatio);
        comp.setAttack (p.compAtt);
        comp.setRelease (p.compRel);
        comp.process (ctx);
        buffer.applyGain (0, 0, n, juce::Decibels::decibelsToGain (p.compGain));
        buffer.applyGain (1, 0, n, juce::Decibels::decibelsToGain (p.compGain));
    }

    if (p.pumpOn) processPump (L, R, n, p, t);
}
