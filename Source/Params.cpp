#include "Params.h"

using APF   = juce::AudioParameterFloat;
using APC   = juce::AudioParameterChoice;
using API   = juce::AudioParameterInt;
using APB   = juce::AudioParameterBool;
using Range = juce::NormalisableRange<float>;

namespace
{
    struct Builder
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        void flt (const juce::String& id, const juce::String& name, float lo, float hi, float def,
                  float centre = -1.0f, const juce::String& label = {})
        {
            Range r (lo, hi);
            if (centre > lo && centre < hi)
                r.setSkewForCentre (centre);
            auto toText = [label] (float v, int)
            {
                if (label == "Hz")  return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                                                        : juce::String (v, v < 10.0f ? 2 : 0) + " Hz";
                if (label == "s")   return v < 1.0f ? juce::String (v * 1000.0f, v < 0.01f ? 1 : 0) + " ms"
                                                    : juce::String (v, 2) + " s";
                if (label == "ms")  return juce::String (v, v < 10.0f ? 1 : 0) + " ms";
                if (label == "dB")  return juce::String (v, 1) + " dB";
                if (label == "ct")  return juce::String (juce::roundToInt (v)) + " ct";
                return juce::String (v, 2);
            };
            params.push_back (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def,
                                                     juce::AudioParameterFloatAttributes().withLabel (label)
                                                                                          .withStringFromValueFunction (toText)));
        }
        void integer (const juce::String& id, const juce::String& name, int lo, int hi, int def)
        {
            params.push_back (std::make_unique<API> (juce::ParameterID { id, 1 }, name, lo, hi, def));
        }
        void choice (const juce::String& id, const juce::String& name, const juce::StringArray& c, int def)
        {
            params.push_back (std::make_unique<APC> (juce::ParameterID { id, 1 }, name, c, def));
        }
        void toggle (const juce::String& id, const juce::String& name, bool def)
        {
            params.push_back (std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def));
        }
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    Builder b;

    // ---------------- Oscillators ----------------
    for (int o = 0; o < 2; ++o)
    {
        const juce::String n = o == 0 ? "Osc A " : "Osc B ";
        b.toggle  (ParamID::osc (o, "on"),    n + "On", o == 0);
        b.choice  (ParamID::osc (o, "wt"),    n + "Wavetable", Choices::wavetables, 0);
        b.flt     (ParamID::osc (o, "pos"),   n + "WT Position", 0.0f, 1.0f, 0.5f);
        b.integer (ParamID::osc (o, "oct"),   n + "Octave", -3, 3, 0);
        b.integer (ParamID::osc (o, "semi"),  n + "Semi", -12, 12, 0);
        b.flt     (ParamID::osc (o, "fine"),  n + "Fine", -100.0f, 100.0f, 0.0f, -1.0f, "ct");
        b.integer (ParamID::osc (o, "uni"),   n + "Unison", 1, kMaxUnison, 1);
        b.flt     (ParamID::osc (o, "det"),   n + "Detune", 0.0f, 1.0f, 0.25f);
        b.flt     (ParamID::osc (o, "blend"), n + "Blend", 0.0f, 1.0f, 0.75f);
        b.flt     (ParamID::osc (o, "width"), n + "Width", 0.0f, 1.0f, 0.8f);
        b.choice  (ParamID::osc (o, "warp"),  n + "Warp Mode", Choices::warp, 0);
        b.flt     (ParamID::osc (o, "warpamt"), n + "Warp", 0.0f, 1.0f, 0.0f);
        b.flt     (ParamID::osc (o, "level"), n + "Level", 0.0f, 1.0f, 0.75f);
        b.flt     (ParamID::osc (o, "pan"),   n + "Pan", -1.0f, 1.0f, 0.0f);
        b.flt     (ParamID::osc (o, "phase"), n + "Phase", 0.0f, 1.0f, 0.0f);
        b.flt     (ParamID::osc (o, "rand"),  n + "Phase Rand", 0.0f, 1.0f, 1.0f);
    }

    // ---------------- Sub / Noise ----------------
    b.toggle ("sub_on", "Sub On", false);
    b.choice ("sub_wave", "Sub Wave", Choices::subWave, 0);
    b.choice ("sub_oct", "Sub Octave", Choices::subOct, 0);
    b.flt    ("sub_level", "Sub Level", 0.0f, 1.0f, 0.6f);
    b.toggle ("sub_direct", "Sub Bypass Filter", true);

    b.toggle ("noise_on", "Noise On", false);
    b.choice ("noise_type", "Noise Type", Choices::noiseType, 0);
    b.flt    ("noise_level", "Noise Level", 0.0f, 1.0f, 0.3f);

    // ---------------- Filter ----------------
    b.toggle ("f_on", "Filter On", true);
    b.choice ("f_type", "Filter Type", Choices::filterType, 0);
    b.flt    ("f_cutoff", "Cutoff", 20.0f, 20000.0f, 20000.0f, 1000.0f, "Hz");
    b.flt    ("f_res", "Resonance", 0.0f, 1.0f, 0.1f);
    b.flt    ("f_drive", "Filter Drive", 0.0f, 1.0f, 0.0f);
    b.flt    ("f_key", "Key Track", 0.0f, 1.0f, 0.0f);
    b.flt    ("f_env", "Filter Env Amt", -1.0f, 1.0f, 0.0f);

    // ---------------- Envelopes ----------------
    const float envDef[3][4] = { { 0.002f, 0.3f, 1.0f, 0.15f },
                                 { 0.002f, 0.4f, 0.0f, 0.3f },
                                 { 0.002f, 0.15f, 0.0f, 0.2f } };
    const char* envNames[3] = { "Amp Env ", "Env 2 ", "Env 3 " };
    for (int e = 0; e < 3; ++e)
    {
        const juce::String n = envNames[e];
        b.flt (ParamID::env (e, "a"), n + "Attack",  0.0005f, 10.0f, envDef[e][0], 0.5f, "s");
        b.flt (ParamID::env (e, "d"), n + "Decay",   0.001f,  10.0f, envDef[e][1], 0.7f, "s");
        b.flt (ParamID::env (e, "s"), n + "Sustain", 0.0f,    1.0f,  envDef[e][2]);
        b.flt (ParamID::env (e, "r"), n + "Release", 0.001f,  15.0f, envDef[e][3], 0.8f, "s");
    }

    // ---------------- LFOs ----------------
    for (int l = 0; l < 2; ++l)
    {
        const juce::String n = "LFO " + juce::String (l + 1) + " ";
        b.choice (ParamID::lfo (l, "shape"), n + "Shape", Choices::lfoShape, 0);
        b.flt    (ParamID::lfo (l, "rate"),  n + "Rate", 0.01f, 30.0f, 2.0f, 2.0f, "Hz");
        b.toggle (ParamID::lfo (l, "sync"),  n + "Sync", true);
        b.choice (ParamID::lfo (l, "div"),   n + "Division", Choices::divisions, 8);
        b.choice (ParamID::lfo (l, "mode"),  n + "Mode", Choices::lfoMode, 1);
        b.flt    (ParamID::lfo (l, "delay"), n + "Rise", 0.0f, 5.0f, 0.0f, 1.0f, "s");
    }

    // ---------------- Mod matrix ----------------
    for (int m = 0; m < kNumModSlots; ++m)
    {
        const juce::String n = "Mod " + juce::String (m + 1) + " ";
        b.choice (ParamID::mod (m, "src"), n + "Source", Choices::modSrc, 0);
        b.choice (ParamID::mod (m, "dst"), n + "Dest", Choices::modDst, 0);
        b.flt    (ParamID::mod (m, "amt"), n + "Amount", -1.0f, 1.0f, 0.0f);
    }
    for (int m = 0; m < 4; ++m)
        b.flt (ParamID::macro (m), "Macro " + juce::String (m + 1), 0.0f, 1.0f, 0.0f);

    // ---------------- Global ----------------
    b.integer ("voices", "Voices", 1, kMaxVoices, 8);
    b.choice  ("mode", "Voice Mode", Choices::voiceMode, 0);
    b.choice  ("chord", "Chord", Choices::chord, 0);
    b.flt     ("glide", "Glide", 0.0f, 2.0f, 0.0f, 0.25f, "s");
    b.integer ("bend", "Bend Range", 0, 24, 2);
    b.flt     ("analog", "Analog", 0.0f, 1.0f, 0.35f);
    b.flt     ("vel", "Velocity Sens", 0.0f, 1.0f, 0.5f);
    b.flt     ("master", "Master", -36.0f, 6.0f, -6.0f, -1.0f, "dB");

    // ---------------- FX ----------------
    b.toggle ("dist_on", "Dist On", false);
    b.choice ("dist_type", "Dist Type", Choices::distType, 0);
    b.flt    ("dist_drive", "Dist Drive", 0.0f, 1.0f, 0.3f);
    b.flt    ("dist_tone", "Dist Tone", 0.0f, 1.0f, 0.7f);
    b.flt    ("dist_mix", "Dist Mix", 0.0f, 1.0f, 1.0f);

    b.toggle ("eq_on", "EQ On", false);
    b.flt    ("eq_low", "EQ Low", -12.0f, 12.0f, 0.0f, -1.0f, "dB");
    b.flt    ("eq_mid", "EQ Mid", -12.0f, 12.0f, 0.0f, -1.0f, "dB");
    b.flt    ("eq_midf", "EQ Mid Freq", 200.0f, 8000.0f, 1000.0f, 1000.0f, "Hz");
    b.flt    ("eq_high", "EQ High", -12.0f, 12.0f, 0.0f, -1.0f, "dB");

    b.toggle ("cho_on", "Chorus On", false);
    b.flt    ("cho_rate", "Chorus Rate", 0.05f, 5.0f, 0.6f, 1.0f, "Hz");
    b.flt    ("cho_depth", "Chorus Depth", 0.0f, 1.0f, 0.35f);
    b.flt    ("cho_mix", "Chorus Mix", 0.0f, 1.0f, 0.4f);

    b.toggle ("pha_on", "Phaser On", false);
    b.flt    ("pha_rate", "Phaser Rate", 0.02f, 5.0f, 0.3f, 0.8f, "Hz");
    b.flt    ("pha_depth", "Phaser Depth", 0.0f, 1.0f, 0.6f);
    b.flt    ("pha_fb", "Phaser Feedback", -0.95f, 0.95f, 0.4f);
    b.flt    ("pha_mix", "Phaser Mix", 0.0f, 1.0f, 0.5f);

    b.toggle ("dly_on", "Delay On", false);
    b.toggle ("dly_sync", "Delay Sync", true);
    b.choice ("dly_div", "Delay Division", Choices::divisions, 6);
    b.flt    ("dly_time", "Delay Time", 1.0f, 2000.0f, 350.0f, 300.0f, "ms");
    b.flt    ("dly_fb", "Delay Feedback", 0.0f, 0.95f, 0.35f);
    b.toggle ("dly_pp", "Delay Ping Pong", true);
    b.flt    ("dly_tone", "Delay Tone", 0.0f, 1.0f, 0.6f);
    b.flt    ("dly_mix", "Delay Mix", 0.0f, 1.0f, 0.2f);

    b.toggle ("rev_on", "Reverb On", false);
    b.flt    ("rev_size", "Reverb Size", 0.0f, 1.0f, 0.6f);
    b.flt    ("rev_damp", "Reverb Damp", 0.0f, 1.0f, 0.5f);
    b.flt    ("rev_width", "Reverb Width", 0.0f, 1.0f, 1.0f);
    b.flt    ("rev_mix", "Reverb Mix", 0.0f, 1.0f, 0.2f);

    b.toggle ("comp_on", "Comp On", false);
    b.flt    ("comp_thresh", "Comp Threshold", -48.0f, 0.0f, -18.0f, -1.0f, "dB");
    b.flt    ("comp_ratio", "Comp Ratio", 1.0f, 20.0f, 4.0f, 4.0f);
    b.flt    ("comp_att", "Comp Attack", 0.1f, 100.0f, 10.0f, 10.0f, "ms");
    b.flt    ("comp_rel", "Comp Release", 10.0f, 1000.0f, 120.0f, 150.0f, "ms");
    b.flt    ("comp_gain", "Comp Makeup", 0.0f, 24.0f, 0.0f, -1.0f, "dB");

    b.toggle ("pump_on", "Pump On", false);
    b.choice ("pump_div", "Pump Rate", Choices::pumpDiv, 2);
    b.flt    ("pump_depth", "Pump Depth", 0.0f, 1.0f, 0.6f);
    b.flt    ("pump_curve", "Pump Curve", 0.0f, 1.0f, 0.5f);

    return { b.params.begin(), b.params.end() };
}
