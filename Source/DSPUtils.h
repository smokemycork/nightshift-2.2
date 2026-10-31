#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

namespace dsp
{
    constexpr float kPi    = 3.14159265358979f;
    constexpr float kTwoPi = 6.28318530717959f;

    // Smooth saturator (Pade tanh approximation, exact clamp beyond +-3)
    inline float fastTanh (float x) noexcept
    {
        if (x > 3.0f)  return 1.0f;
        if (x < -3.0f) return -1.0f;
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    inline float midiToHz (float note) noexcept { return 440.0f * std::exp2 ((note - 69.0f) * (1.0f / 12.0f)); }

    inline float wrap01 (float p) noexcept { return p - std::floor (p); }

    // ------------------------------------------------------------------ ADSR (analog-style exponential)
    struct Envelope
    {
        enum Stage { Idle, Attack, Decay, Release };
        Stage stage = Idle;
        float level = 0.0f, aC = 0.1f, dC = 0.01f, rC = 0.01f, sus = 1.0f;

        void set (float a, float d, float s, float r, float sr) noexcept
        {
            aC = 1.0f - std::exp (-1.466f / juce::jmax (1.0f, a * sr));
            dC = 1.0f - std::exp (-4.6f / juce::jmax (1.0f, d * sr));
            rC = 1.0f - std::exp (-4.6f / juce::jmax (1.0f, r * sr));
            sus = s;
        }
        void noteOn() noexcept   { stage = Attack; }
        void noteOff() noexcept  { if (stage != Idle) stage = Release; }
        void reset() noexcept    { stage = Idle; level = 0.0f; }
        bool isActive() const noexcept { return stage != Idle; }

        inline float next() noexcept
        {
            switch (stage)
            {
                case Attack:
                    level += (1.3f - level) * aC;
                    if (level >= 1.0f) { level = 1.0f; stage = Decay; }
                    break;
                case Decay:
                    level += (sus - level) * dC;
                    break;
                case Release:
                    level -= level * rC;
                    if (level < 1.0e-5f) { level = 0.0f; stage = Idle; }
                    break;
                case Idle:
                default: break;
            }
            return level;
        }
    };

    // ------------------------------------------------------------------ LFO
    struct Lfo
    {
        double phase = 0.0;
        float value = 0.0f, held = 0.0f, from = 0.0f, to = 0.0f;
        juce::Random rng;

        void seed (juce::int64 s) { rng.setSeed (s); held = rng.nextFloat() * 2.0f - 1.0f; from = held; to = rng.nextFloat() * 2.0f - 1.0f; }

        // advance by 'cycles' and return bipolar output
        float advance (double cycles, int shape) noexcept
        {
            phase += cycles;
            if (phase >= 1.0)
            {
                phase -= std::floor (phase);
                held = rng.nextFloat() * 2.0f - 1.0f;
                from = to; to = rng.nextFloat() * 2.0f - 1.0f;
            }
            const float p = (float) phase;
            switch (shape)
            {
                case 0:  value = std::sin (kTwoPi * p); break;
                case 1:  value = p < 0.25f ? 4.0f * p : (p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f); break;
                case 2:  value = 2.0f * p - 1.0f; break;
                case 3:  value = 1.0f - 2.0f * p; break;
                case 4:  value = p < 0.5f ? 1.0f : -1.0f; break;
                case 5:  value = held; break;
                default:
                {
                    const float s = p * p * (3.0f - 2.0f * p);
                    value = from + (to - from) * s;
                    break;
                }
            }
            return value;
        }
    };

    // ------------------------------------------------------------------ Filters (zero-delay feedback)
    struct SVF
    {
        float ic1 = 0.0f, ic2 = 0.0f;
        void reset() noexcept { ic1 = ic2 = 0.0f; }
        inline void tick (float x, float g, float k, float& lp, float& bp, float& hp) noexcept
        {
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1, a3 = g * a2;
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            lp = v2; bp = v1; hp = x - k * v1 - v2;
        }
    };

    struct Ladder
    {
        float s[4] {};
        void reset() noexcept { s[0] = s[1] = s[2] = s[3] = 0.0f; }

        // g = tan(pi fc / fs), k = 0..~4 resonance. Returns 24 dB (or 12 dB tap)
        inline float tick (float x, float g, float k, bool twelve) noexcept
        {
            const float G = g / (1.0f + g);
            const float b = 1.0f - G;
            const float G2 = G * G, G3 = G2 * G, G4 = G3 * G;
            const float S = G3 * b * s[0] + G2 * b * s[1] + G * b * s[2] + b * s[3];
            const float y4est = (G4 * x + S) / (1.0f + k * G4);
            const float u = fastTanh (x - k * y4est);

            float in = u, y[4];
            for (int i = 0; i < 4; ++i)
            {
                const float v = (in - s[i]) * G;
                y[i] = v + s[i];
                s[i] = y[i] + v;
                in = y[i];
            }
            const float comp = 1.0f + k * 0.45f;
            return (twelve ? y[1] : y[3]) * comp;
        }
    };

    // ------------------------------------------------------------------ simple one-pole helpers
    struct OnePole
    {
        float z = 0.0f;
        inline float lp (float x, float a) noexcept { z += a * (x - z); return z; }
        void reset() noexcept { z = 0.0f; }
    };

    struct DCBlocker
    {
        float x1 = 0.0f, y1 = 0.0f;
        inline float process (float x) noexcept { const float y = x - x1 + 0.995f * y1; x1 = x; y1 = y; return y; }
        void reset() noexcept { x1 = y1 = 0.0f; }
    };
}
