#include "Wavetables.h"
#include "Params.h"
#include <cmath>
#include <complex>
#include <functional>

namespace
{
    constexpr int N    = WavetableBank::kSize;
    constexpr int HMAX = N / 2 - 1;   // highest usable harmonic (1023)
    constexpr double kPI = 3.14159265358979323846;

    // Harmonic spectrum of one cycle: x(t) = sum s[n] sin(n t) + c[n] cos(n t)
    struct Spectrum
    {
        double s[HMAX + 1] {}, c[HMAX + 1] {};
        void clear() { std::fill (std::begin (s), std::end (s), 0.0); std::fill (std::begin (c), std::end (c), 0.0); }
        void addScaled (const Spectrum& o, double k)
        {
            for (int n = 1; n <= HMAX; ++n) { s[n] += o.s[n] * k; c[n] += o.c[n] * k; }
        }
    };

    using Complex = std::complex<float>;

    // ------------------------------------------------------------------ basic shapes
    void sine (Spectrum& sp)  { sp.clear(); sp.s[1] = 1.0; }

    void saw (Spectrum& sp)
    {
        sp.clear();
        for (int n = 1; n <= HMAX; ++n)
            sp.s[n] = (2.0 / kPI) * ((n % 2) ? 1.0 : -1.0) / n;
    }

    void square (Spectrum& sp)
    {
        sp.clear();
        for (int n = 1; n <= HMAX; n += 2)
            sp.s[n] = (4.0 / kPI) / n;
    }

    void triangle (Spectrum& sp)
    {
        sp.clear();
        for (int n = 1; n <= HMAX; n += 2)
            sp.s[n] = (8.0 / (kPI * kPI)) * ((((n - 1) / 2) % 2) ? -1.0 : 1.0) / ((double) n * n);
    }

    // pulse = ramp(x) - ramp(x + 2*pi*w)
    void pulse (Spectrum& sp, double w)
    {
        sp.clear();
        const double phi = 2.0 * kPI * w;
        for (int n = 1; n <= HMAX; ++n)
        {
            const double k = (2.0 / kPI) * ((n % 2) ? 1.0 : -1.0) / n;
            sp.s[n] = k * (1.0 - std::cos (n * phi));
            sp.c[n] = -k * std::sin (n * phi);
        }
    }

    void morphSpectra (Spectrum& out, const std::vector<Spectrum>& keys, double t)
    {
        const double x = juce::jlimit (0.0, 1.0, t) * (double) (keys.size() - 1);
        const int i = juce::jmin ((int) x, (int) keys.size() - 2);
        const double f = x - i;
        out.clear();
        out.addScaled (keys[(size_t) i], 1.0 - f);
        out.addScaled (keys[(size_t) i + 1], f);
    }

    // Analyse one cycle of time-domain samples into a spectrum
    void analyse (const std::vector<float>& x, Spectrum& sp, juce::dsp::FFT& fft)
    {
        std::vector<Complex> in ((size_t) N), out ((size_t) N);
        for (int i = 0; i < N; ++i) in[(size_t) i] = Complex (x[(size_t) i], 0.0f);
        fft.perform (in.data(), out.data(), false);
        sp.clear();
        for (int n = 1; n <= HMAX; ++n)
        {
            sp.c[n] =  2.0 * out[(size_t) n].real() / N;
            sp.s[n] = -2.0 * out[(size_t) n].imag() / N;
        }
    }

    template <typename Fn>
    void fromTime (Spectrum& sp, juce::dsp::FFT& fft, Fn&& fn)
    {
        std::vector<float> x ((size_t) N);
        for (int i = 0; i < N; ++i)
            x[(size_t) i] = (float) fn (2.0 * kPI * (double) i / (double) N);
        analyse (x, sp, fft);
    }

    double gauss (double x, double w) { return std::exp (-(x * x) / (w * w)); }
}

WavetableBank::WavetableBank()
{
    numTables = Choices::wavetables.size();
    data.assign ((size_t) numTables * kFrames * kMips * (kSize + 1), 0.0f);

    juce::dsp::FFT fft (11);  // 2048
    jassert (fft.getSize() == N);

    std::vector<Spectrum> keys (5);
    Spectrum tmp;
    juce::Random rng (0x4E534654);   // fixed seed -> identical tables every load

    // Pre-generate random key spectra for "Digital Grit"
    std::vector<Spectrum> grit (5);
    for (auto& g : grit)
        for (int n = 1; n <= HMAX; ++n)
        {
            const double amp = std::pow ((double) n, -0.85) * (0.2 + 0.8 * rng.nextDouble());
            const double ph = rng.nextDouble() * 2.0 * kPI;
            g.s[n] = amp * std::cos (ph);
            g.c[n] = amp * std::sin (ph);
        }

    std::vector<Complex> bins ((size_t) N), timeBuf ((size_t) N);
    std::vector<float> mip0 ((size_t) N);

    for (int table = 0; table < numTables; ++table)
    {
        for (int frame = 0; frame < kFrames; ++frame)
        {
            const double t = (double) frame / (double) (kFrames - 1);
            Spectrum sp;

            switch (table)
            {
                case 0: // Analog Classic: sine -> tri -> saw -> square -> pulse
                {
                    sine (keys[0]); triangle (keys[1]); saw (keys[2]); square (keys[3]); pulse (keys[4], 0.22);
                    morphSpectra (sp, keys, t);
                    break;
                }
                case 1: // PWM Pulse 50% -> 4%
                    pulse (sp, 0.5 - t * 0.46);
                    break;

                case 2: // Warm Saw (dark -> bright)
                {
                    saw (sp);
                    const double k = (1.0 - t) * 0.32;
                    for (int n = 1; n <= HMAX; ++n) sp.s[n] *= std::exp (-(n - 1) * k);
                    break;
                }
                case 3: // Formant Vowel A-E-I-O-U
                {
                    static const double F[5][3] = { { 800, 1150, 2900 }, { 400, 1600, 2700 }, { 350, 1900, 2800 },
                                                    { 450, 800, 2830 },  { 325, 700, 2530 } };
                    const double x = t * 4.0;
                    const int i = juce::jmin ((int) x, 3);
                    const double f = x - i;
                    double fm[3];
                    for (int j = 0; j < 3; ++j) fm[j] = F[i][j] * (1.0 - f) + F[i + 1][j] * f;
                    const double f0 = 110.0, bw[3] = { 90, 110, 160 }, g[3] = { 1.0, 0.55, 0.28 };
                    for (int n = 1; n <= HMAX; ++n)
                    {
                        const double hz = n * f0;
                        double a = 0.0;
                        for (int j = 0; j < 3; ++j) a += g[j] * gauss (hz - fm[j], bw[j]);
                        a = a * std::pow ((double) n, -0.25) + (n == 1 ? 0.25 : 0.0);
                        sp.s[n] = a * ((n % 2) ? 1.0 : -1.0);
                    }
                    break;
                }
                case 4: // Drawbar organ (adds drawbars as frames progress)
                {
                    static const int harm[8] = { 1, 2, 3, 4, 6, 8, 10, 12 };
                    static const double lvl[8] = { 1.0, 0.85, 0.7, 0.6, 0.5, 0.42, 0.3, 0.25 };
                    for (int i = 0; i < 8; ++i)
                    {
                        const double on = juce::jlimit (0.0, 1.0, t * 8.0 - i + 1.0);
                        sp.s[harm[i]] += lvl[i] * (i == 0 ? 1.0 : on);
                    }
                    break;
                }
                case 5: // Hollow Saw: saw with a sweeping spectral notch
                {
                    saw (sp);
                    const double centre = 2.0 + std::pow (t, 1.5) * 40.0;
                    for (int n = 1; n <= HMAX; ++n)
                        sp.s[n] *= 1.0 - 0.97 * gauss (n - centre, 1.5 + centre * 0.25);
                    break;
                }
                case 6: // Reso Sweep: saw + resonant harmonic peak moving up
                {
                    saw (sp);
                    const double centre = std::pow (48.0, t);
                    const double w = 0.8 + centre * 0.1;
                    for (int n = 1; n <= HMAX; ++n)
                    {
                        const double rolloff = (n <= centre) ? 1.0 : std::exp (-(n - centre) * 0.08);
                        sp.s[n] = sp.s[n] * 0.45 * rolloff + ((n % 2) ? 1.0 : -1.0) * 0.6 * gauss (n - centre, w);
                    }
                    break;
                }
                case 7: // FM Tines (electric piano / bell)
                {
                    const double i1 = t * 2.6, i2 = t * 1.8;
                    fromTime (sp, fft, [=] (double p)
                    {
                        return 0.8 * std::sin (p + i1 * std::sin (p)) + 0.22 * std::sin (4.0 * p + i2 * std::sin (p));
                    });
                    break;
                }
                case 8: // Wavefold
                {
                    const double g = 1.0 + t * 8.0;
                    fromTime (sp, fft, [=] (double p) { return std::sin (0.5 * kPI * g * std::sin (p)); });
                    break;
                }
                case 9: // Sync Sweep (hard-synced saw)
                {
                    const double ratio = 1.0 + t * 7.0;
                    fromTime (sp, fft, [=] (double p)
                    {
                        const double ph = std::fmod (p / (2.0 * kPI) * ratio, 1.0);
                        const double win = 0.5 - 0.5 * std::cos (p);       // soften the reset
                        return (2.0 * ph - 1.0) * (0.35 + 0.65 * win);
                    });
                    break;
                }
                case 10: // Digital Grit (morphing random spectra)
                    morphSpectra (sp, grit, t);
                    break;

                case 11: // Additive Build (1 -> 32 harmonics)
                default:
                {
                    const double K = 1.0 + t * 31.0;
                    for (int n = 1; n <= 33 && n <= HMAX; ++n)
                    {
                        const double w = juce::jlimit (0.0, 1.0, K - n + 1.0);
                        sp.s[n] = w / n;
                    }
                    break;
                }
            }

            // ---- synthesise mip levels via inverse FFT ----
            float scale = 1.0f;
            for (int m = 0; m < kMips; ++m)
            {
                const int limit = juce::jmin (HMAX, 1024 >> m);
                std::fill (bins.begin(), bins.end(), Complex (0.0f, 0.0f));
                for (int n = 1; n <= limit; ++n)
                {
                    const Complex v ((float) (0.5 * sp.c[n]), (float) (-0.5 * sp.s[n]));
                    bins[(size_t) n] = v;
                    bins[(size_t) (N - n)] = std::conj (v);
                }
                fft.perform (bins.data(), timeBuf.data(), true);

                float* dst = const_cast<float*> (get (table, frame, m));
                for (int i = 0; i < N; ++i) dst[i] = timeBuf[(size_t) i].real();

                if (m == 0)
                {
                    float peak = 0.0f;
                    for (int i = 0; i < N; ++i) peak = juce::jmax (peak, std::abs (dst[i]));
                    scale = peak > 1.0e-9f ? 1.0f / peak : 0.0f;
                }
                for (int i = 0; i < N; ++i) dst[i] *= scale;
                dst[N] = dst[0];
            }
        }
    }
}
