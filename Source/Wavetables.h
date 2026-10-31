#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>

// ============================================================================
//  Band-limited, mip-mapped morphing wavetables (generated procedurally)
// ============================================================================
class WavetableBank
{
public:
    static constexpr int kSize   = 2048;   // samples per cycle
    static constexpr int kFrames = 16;     // morph frames per table
    static constexpr int kMips   = 11;     // harmonic limits 1024, 512, ... 1

    WavetableBank();

    int getNumTables() const noexcept { return numTables; }

    // Returns a pointer to kSize + 1 samples (last sample == first, for interpolation)
    const float* get (int table, int frame, int mip) const noexcept
    {
        return data.data() + (((size_t) table * kFrames + (size_t) frame) * kMips + (size_t) mip) * (kSize + 1);
    }

    // inc = frequency / sampleRate (cycles per sample)
    static inline int mipForIncrement (float inc) noexcept
    {
        const float maxHarm = 0.5f / juce::jmax (inc, 1.0e-9f);
        int limit = 1024;
        for (int m = 0; m < kMips; ++m, limit >>= 1)
            if ((float) limit <= maxHarm)
                return m;
        return kMips - 1;
    }

    // Morphing read (frame position 0..1, phase 0..1)
    inline float read (int table, float pos, int mip, float phase) const noexcept
    {
        const float fpos = juce::jlimit (0.0f, 1.0f, pos) * (float) (kFrames - 1);
        const int f0 = juce::jmin ((int) fpos, kFrames - 2);
        const float ff = fpos - (float) f0;
        const float idx = phase * (float) kSize;
        int i0 = (int) idx;
        i0 = juce::jlimit (0, kSize - 1, i0);
        const float fr = idx - (float) i0;
        const float* a = get (table, f0, mip);
        const float* b = get (table, f0 + 1, mip);
        const float sa = a[i0] + (a[i0 + 1] - a[i0]) * fr;
        const float sb = b[i0] + (b[i0 + 1] - b[i0]) * fr;
        return sa + (sb - sa) * ff;
    }

private:
    int numTables = 0;
    std::vector<float> data;
};
