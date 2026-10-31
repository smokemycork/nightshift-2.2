#include "SynthVoice.h"

using namespace dsp;

namespace
{
    constexpr float kSqrt2 = 1.41421356f;

    inline float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    inline float applyWarp (float p, int mode, float amt, float fmIn) noexcept
    {
        switch (mode)
        {
            case 1: return wrap01 (p * (1.0f + amt * 7.0f));                           // Sync
            case 2: { const float k = 0.5f - 0.49f * amt;                               // Bend (phase distortion)
                      return p < k ? 0.5f * p / k : 0.5f + 0.5f * (p - k) / (1.0f - k); }
            case 3: { const float m = p < 0.5f ? 2.0f * p : 2.0f - 2.0f * p;            // Mirror
                      return juce::jlimit (0.0f, 1.0f, p + amt * (m - p)); }
            case 4: return wrap01 (p + amt * fmIn * 0.5f);                               // FM (phase mod)
            default: return p;
        }
    }

    inline void panGains (float pan, float& l, float& r) noexcept
    {
        const float a = (juce::jlimit (-1.0f, 1.0f, pan) + 1.0f) * (kPi * 0.25f);
        l = std::cos (a) * kSqrt2;
        r = std::sin (a) * kSqrt2;
    }

    inline float pink (float w, float* b) noexcept
    {
        b[0] = 0.99886f * b[0] + w * 0.0555179f;
        b[1] = 0.99332f * b[1] + w * 0.0750759f;
        b[2] = 0.96900f * b[2] + w * 0.1538520f;
        b[3] = 0.86650f * b[3] + w * 0.3104856f;
        b[4] = 0.55000f * b[4] + w * 0.5329522f;
        b[5] = -0.7616f * b[5] - w * 0.0168980f;
        const float out = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362f;
        b[6] = w * 0.115926f;
        return out * 0.2f;
    }
}

void SynthVoice::prepare (double sampleRate, uint32_t seed)
{
    sr = sampleRate;
    rngState = seed == 0 ? 0x9E3779B9u : seed;
    driftCoef = 1.0f - std::exp (-(float) kCtrl / (0.4f * (float) sr));
    kill();
}

void SynthVoice::kill() noexcept
{
    active = false; gate = false; triggerNote = -1;
    for (auto& e : env) e.reset();
    for (int c = 0; c < 2; ++c) { ladder[c].reset(); svfA[c].reset(); svfB[c].reset(); }
    for (int i = 0; i < 7; ++i) pinkL[i] = pinkR[i] = 0.0f;
    crackle = 0.0f;
}

void SynthVoice::noteOn (int trig, float pitch, float vel, float glideFrom, bool legato,
                         const SynthParams& P, const GlobalState& G, juce::uint64 ord)
{
    triggerNote = trig;
    noteTarget = pitch;
    velocity = juce::jlimit (0.0f, 1.0f, vel);
    order = ord;
    gate = true;

    if (legato && active)
        return;   // keep envelopes running, just glide to the new pitch

    const bool wasActive = active;

    noteCurrent = (glideFrom >= 0.0f && P.glide > 0.0005f) ? glideFrom : pitch;

    auto rand01 = [this] { return noise() * 0.5f + 0.5f; };

    if (! wasActive)
    {
        for (int c = 0; c < 2; ++c) { ladder[c].reset(); svfA[c].reset(); svfB[c].reset(); }
        for (auto& e : env) e.level = 0.0f;

        for (int o = 0; o < 2; ++o)
            for (int u = 0; u < kMaxUnison; ++u)
            {
                phase[o][u] = wrap01 (P.osc[o].phase + P.osc[o].rand * rand01());
                drift[o][u] = noise();
                driftTarget[o][u] = noise();
                driftCount[o][u] = 0;
            }
        subPhase = 0.0f;
        oscMono[0] = oscMono[1] = 0.0f;
        firstControl = true;
        ampMod = 1.0f;
        panL = panR = 1.0f;
    }

    for (auto& e : env) e.noteOn();

    for (int l = 0; l < 2; ++l)
    {
        lfo[l].seed ((juce::int64) rngState + l * 7777);
        lfo[l].phase = P.lfo[l].mode == 1 ? 0.0 : G.lfoPhase[l];
    }

    samplesSinceOn = 0.0;
    noteRandom = noise();
    voiceDetune = noise() * P.analog * 0.03f;           // up to +-3 cents "component tolerance"
    cutoffOffset = noise() * P.analog * 0.12f;          // up to +-0.12 octave per voice
    velGain = 1.0f - P.velSens * (1.0f - velocity);
    active = true;
    ctrlLeft = 0;
}

void SynthVoice::noteOff() noexcept
{
    gate = false;
    for (auto& e : env) e.noteOff();
}

void SynthVoice::render (float* L, float* R, int numSamples, const SynthParams& P, const GlobalState& G) noexcept
{
    if (! active)
        return;

    const float fsr = (float) sr;
    for (int e = 0; e < 3; ++e)
        env[e].set (P.env[e].a, P.env[e].d, P.env[e].s, P.env[e].r, fsr);

    int i = 0;
    while (i < numSamples && active)
    {
        if (ctrlLeft <= 0)
        {
            updateControl (P, G);
            ctrlLeft = kCtrl;
        }
        const int chunk = juce::jmin (numSamples - i, ctrlLeft);
        renderChunk (L + i, R + i, chunk, P, G);
        i += chunk;
        ctrlLeft -= chunk;
    }
}

void SynthVoice::updateControl (const SynthParams& P, const GlobalState& G) noexcept
{
    const float fsr = (float) sr;

    // ---------------- LFOs ----------------
    for (int l = 0; l < 2; ++l)
    {
        const auto& LP = P.lfo[l];
        double rate = LP.sync ? (G.bpm / 60.0) / divisionToBeats (LP.div) : (double) LP.rate;
        rate *= std::exp2 ((double) dstVal[l == 0 ? DstLfo1Rate : DstLfo2Rate] * 3.0);
        lfo[l].advance (rate * kCtrl / sr, LP.shape);
        const float fade = LP.delay > 0.001f ? (float) juce::jmin (1.0, samplesSinceOn / (LP.delay * sr)) : 1.0f;
        srcVal[l == 0 ? SrcLfo1 : SrcLfo2] = lfo[l].value * fade;
    }
    samplesSinceOn += kCtrl;

    // ---------------- sources ----------------
    srcVal[SrcNone]       = 0.0f;
    srcVal[SrcEnv2]       = env[1].level;
    srcVal[SrcEnv3]       = env[2].level;
    srcVal[SrcVelocity]   = velocity;
    srcVal[SrcModWheel]   = G.modWheel;
    srcVal[SrcAftertouch] = G.aftertouch;
    srcVal[SrcKeyTrack]   = (noteTarget - 60.0f) / 60.0f;
    srcVal[SrcNoteRandom] = noteRandom;
    for (int m = 0; m < 4; ++m) srcVal[SrcMacro1 + m] = P.macro[m];
    srcVal[SrcAmpEnv]     = env[0].level;

    // ---------------- matrix ----------------
    for (auto& d : dstVal) d = 0.0f;
    for (const auto& s : P.mod)
        if (s.src > 0 && s.src < NumModSrc && s.dst > 0 && s.dst < NumModDst)
            dstVal[s.dst] += s.amt * srcVal[s.src];

    // ---------------- glide ----------------
    if (P.glide <= 0.0005f)
        noteCurrent = noteTarget;
    else
        noteCurrent += (noteTarget - noteCurrent) * (1.0f - std::exp (-(float) kCtrl / (P.glide * fsr * 0.25f)));

    // ---------------- oscillators ----------------
    for (int o = 0; o < 2; ++o)
    {
        const auto& O = P.osc[o];
        if (! O.on) continue;

        const int N = juce::jlimit (1, kMaxUnison, O.uni);
        curUni[o] = N;

        const float pitchBase = noteCurrent + (float) (O.oct * 12 + O.semi) + O.fine * 0.01f
                              + G.pitchBendSemis
                              + (dstVal[o == 0 ? DstPitchA : DstPitchB] + dstVal[DstPitchAll]) * 24.0f
                              + voiceDetune;

        const float det = juce::jlimit (0.0f, 1.0f, O.det + dstVal[DstDetuneA + o]);
        const float spreadCents = 60.0f * det * det;
        const float level = juce::jlimit (0.0f, 1.0f, O.level + dstVal[DstLevelA + o]);

        float gains[kMaxUnison];
        float sumSq = 0.0f;
        for (int u = 0; u < N; ++u)
        {
            const bool centre = (N == 1) || ((N & 1) && u == N / 2) || (! (N & 1) && (u == N / 2 - 1 || u == N / 2));
            gains[u] = centre ? 1.0f : O.blend;
            sumSq += gains[u] * gains[u];
        }
        const float norm = sumSq > 1.0e-6f ? 1.0f / std::sqrt (sumSq) : 0.0f;

        for (int u = 0; u < N; ++u)
        {
            const float off = N == 1 ? 0.0f : (2.0f * (float) u / (float) (N - 1) - 1.0f);

            if (--driftCount[o][u] <= 0)
            {
                driftTarget[o][u] = noise();
                driftCount[o][u] = (int) ((0.15f + 0.5f * (noise() * 0.5f + 0.5f)) * fsr / kCtrl);
            }
            drift[o][u] += (driftTarget[o][u] - drift[o][u]) * driftCoef;

            const float cents = off * spreadCents + drift[o][u] * P.analog * 7.0f;
            const float hz = midiToHz (pitchBase + cents * 0.01f);
            const float in = juce::jlimit (0.0f, 0.49f, hz / fsr);
            inc[o][u] = in;
            mip[o][u] = WavetableBank::mipForIncrement (in);

            float pl, pr;
            panGains (off * O.width + O.pan, pl, pr);
            const float gu = gains[u] * norm;
            gainL[o][u] = gu * level * pl;
            gainR[o][u] = gu * level * pr;
            gainM[o][u] = gu;
        }

        curPos[o]  = juce::jlimit (0.0f, 1.0f, O.pos + dstVal[DstPosA + o]);
        curWarp[o] = juce::jlimit (0.0f, 1.0f, O.warpAmt + dstVal[DstWarpA + o]);
    }

    // ---------------- sub / noise ----------------
    if (P.subOn)
    {
        const float subNote = noteCurrent + G.pitchBendSemis + dstVal[DstPitchAll] * 24.0f - 12.0f * (float) (P.subOct + 1);
        subInc = juce::jlimit (0.0f, 0.49f, midiToHz (subNote) / fsr);
        subGain = juce::jlimit (0.0f, 1.0f, P.subLevel + dstVal[DstSub]);
    }
    else subGain = 0.0f;

    noiseGain = P.noiseOn ? juce::jlimit (0.0f, 1.0f, P.noiseLevel + dstVal[DstNoise]) : 0.0f;

    // ---------------- filter ----------------
    res   = juce::jlimit (0.0f, 1.0f, P.res + dstVal[DstRes]);
    drive = juce::jlimit (0.0f, 1.0f, P.drive + dstVal[DstDrive]);
    if (P.fOn)
    {
        const float octs = P.envAmt * env[1].level * 7.0f + P.key * (noteCurrent - 60.0f) / 12.0f
                         + dstVal[DstCutoff] * 6.0f + cutoffOffset;
        const float fc = juce::jlimit (16.0f, juce::jmin (20000.0f, 0.45f * fsr), P.cutoff * std::exp2 (octs));
        gTarget = std::tan (kPi * fc / fsr);
    }
    if (firstControl) { g = gTarget; firstControl = false; }
    gStep = (gTarget - g) / (float) kCtrl;

    // ---------------- amp / pan ----------------
    ampModTarget = juce::jlimit (0.0f, 2.0f, 1.0f + dstVal[DstAmp]);
    panGains (dstVal[DstPan], panLT, panRT);
}

void SynthVoice::renderChunk (float* L, float* R, int n, const SynthParams& P, const GlobalState& G) noexcept
{
    const WavetableBank& bank = *G.bank;
    const int numTables = bank.getNumTables();

    const float pre  = 1.0f + drive * 6.0f;
    const float comp = 1.0f / (0.7f * std::sqrt (pre));
    const float kLad = res * 3.9f;
    const float k12  = 1.414f - 1.374f * res;
    const float k24a = 1.848f;
    const float k24b = 0.765f - 0.725f * res;
    const int   ftype = P.fType;
    const float subDt = subInc;

    for (int i = 0; i < n; ++i)
    {
        const float ampEnv = env[0].next();
        env[1].next();
        env[2].next();

        g = juce::jmax (1.0e-4f, g + gStep);
        ampMod += (ampModTarget - ampMod) * 0.02f;
        panL   += (panLT - panL) * 0.02f;
        panR   += (panRT - panR) * 0.02f;

        float oL = 0.0f, oR = 0.0f;

        // Osc B first so Osc A can FM from its current sample (B FMs from A's previous one)
        for (int o = 1; o >= 0; --o)
        {
            const auto& O = P.osc[o];
            if (! O.on) { oscMono[o] = 0.0f; continue; }

            const int table = juce::jlimit (0, numTables - 1, O.wt);
            const float fmIn = oscMono[1 - o];
            const int N = curUni[o];
            float sl = 0.0f, sr_ = 0.0f, sm = 0.0f;

            for (int u = 0; u < N; ++u)
            {
                float p = phase[o][u];
                const float s = bank.read (table, curPos[o], mip[o][u], applyWarp (p, O.warp, curWarp[o], fmIn));
                sl  += s * gainL[o][u];
                sr_ += s * gainR[o][u];
                sm  += s * gainM[o][u];
                p += inc[o][u];
                if (p >= 1.0f) p -= 1.0f;
                phase[o][u] = p;
            }
            oscMono[o] = sm;
            oL += sl;
            oR += sr_;
        }

        // noise
        if (noiseGain > 0.0f)
        {
            const float wl = noise(), wr = noise();
            float nl, nr;
            switch (P.noiseType)
            {
                case 1:  nl = pink (wl, pinkL); nr = pink (wr, pinkR); break;
                case 2:
                    if (noise() > 0.9985f) crackle = noise();
                    crackle *= 0.85f;
                    nl = crackle + wl * 0.04f; nr = crackle * 0.8f + wr * 0.04f;
                    break;
                default: nl = wl; nr = wr; break;
            }
            oL += nl * noiseGain * 0.6f;
            oR += nr * noiseGain * 0.6f;
        }

        // sub
        float sub = 0.0f;
        if (subGain > 0.0f)
        {
            subPhase += subInc;
            if (subPhase >= 1.0f) subPhase -= 1.0f;
            switch (P.subWave)
            {
                case 1:  sub = 4.0f * std::abs (subPhase - 0.5f) - 1.0f; break;
                case 2:  sub = (subPhase < 0.5f ? 1.0f : -1.0f) + polyBlep (subPhase, subDt)
                             - polyBlep (wrap01 (subPhase + 0.5f), subDt);
                         sub *= 0.7f; break;
                default: sub = std::sin (kTwoPi * subPhase); break;
            }
            sub *= subGain;
            if (! P.subDirect) { oL += sub; oR += sub; }
        }

        // analog mixer saturation (+ filter drive)
        float xL = fastTanh (oL * pre * 0.7f) * comp;
        float xR = fastTanh (oR * pre * 0.7f) * comp;

        if (P.fOn)
        {
            float lp, bp, hp, lp2, bp2, hp2;
            float* chans[2] = { &xL, &xR };
            for (int c = 0; c < 2; ++c)
            {
                float& x = *chans[c];
                switch (ftype)
                {
                    case 0: x = ladder[c].tick (x, g, kLad, false); break;
                    case 1: x = ladder[c].tick (x, g, kLad, true);  break;
                    case 2: svfA[c].tick (x, g, k12, lp, bp, hp); x = lp; break;
                    case 3: svfA[c].tick (x, g, k24a, lp, bp, hp); svfB[c].tick (lp, g, k24b, lp2, bp2, hp2); x = lp2; break;
                    case 4: svfA[c].tick (x, g, k12, lp, bp, hp); x = hp; break;
                    case 5: svfA[c].tick (x, g, k24a, lp, bp, hp); svfB[c].tick (hp, g, k24b, lp2, bp2, hp2); x = hp2; break;
                    case 6: svfA[c].tick (x, g, k12, lp, bp, hp); x = bp * k12 * 1.4f; break;
                    default: svfA[c].tick (x, g, k12, lp, bp, hp); x = lp + hp; break;
                }
            }
        }

        if (P.subDirect) { xL += sub; xR += sub; }

        const float a = ampEnv * velGain * ampMod;
        L[i] += xL * a * panL;
        R[i] += xR * a * panR;
    }

    if (! env[0].isActive())
    {
        active = false;
        gate = false;
        triggerNote = -1;
    }
}
