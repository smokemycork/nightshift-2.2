#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// ============================================================================
//  NightShift parameter definitions (single source of truth)
// ============================================================================
namespace Choices
{
    inline const juce::StringArray wavetables { "Analog Classic", "PWM Pulse", "Warm Saw", "Formant Vowel",
                                                "Drawbar Organ", "Hollow Saw", "Reso Sweep", "FM Tines",
                                                "Wavefold", "Sync Sweep", "Digital Grit", "Additive Build" };
    inline const juce::StringArray warp       { "Off", "Sync", "Bend", "Mirror", "FM" };
    inline const juce::StringArray subWave    { "Sine", "Triangle", "Square" };
    inline const juce::StringArray subOct     { "-1 Oct", "-2 Oct" };
    inline const juce::StringArray noiseType  { "White", "Pink", "Crackle" };
    inline const juce::StringArray filterType { "Ladder 24", "Ladder 12", "LP 12", "LP 24",
                                                "HP 12", "HP 24", "Band Pass", "Notch" };
    inline const juce::StringArray lfoShape   { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Smooth Rnd" };
    inline const juce::StringArray lfoMode    { "Free", "Retrig" };
    inline const juce::StringArray divisions  { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D",
                                                "1/4T", "1/4", "1/4D", "1/2", "1 Bar", "2 Bars", "4 Bars", "8 Bars" };
    inline const juce::StringArray voiceMode  { "Poly", "Mono", "Legato" };
    inline const juce::StringArray chord      { "Off", "Minor", "Major", "Minor 7", "Major 7", "Minor 9",
                                                "Sus 4", "Fifth", "Octave", "Dub Min7", "Stab 4ths" };
    inline const juce::StringArray distType   { "Soft", "Hard", "Tube", "Fold", "Crush" };
    inline const juce::StringArray pumpDiv    { "1/16", "1/8", "1/4", "1/2" };
    inline const juce::StringArray modSrc     { "None", "Env 2", "Env 3", "LFO 1", "LFO 2", "Velocity", "Mod Wheel",
                                                "Aftertouch", "Key Track", "Note Random", "Macro 1", "Macro 2",
                                                "Macro 3", "Macro 4", "Amp Env" };
    inline const juce::StringArray modDst     { "None", "Osc A Pitch", "Osc B Pitch", "Global Pitch", "Osc A WT Pos",
                                                "Osc B WT Pos", "Osc A Level", "Osc B Level", "Osc A Detune",
                                                "Osc B Detune", "Osc A Warp", "Osc B Warp", "Filter Cutoff",
                                                "Filter Res", "Filter Drive", "Sub Level", "Noise Level", "Amp",
                                                "Pan", "LFO 1 Rate", "LFO 2 Rate" };
}

// Beats per cycle for each entry of Choices::divisions
inline double divisionToBeats (int idx)
{
    static const double beats[] = { 0.125, 0.25 * 2.0 / 3.0, 0.25, 0.375, 0.5 * 2.0 / 3.0, 0.5, 0.75,
                                    2.0 / 3.0, 1.0, 1.5, 2.0, 4.0, 8.0, 16.0, 32.0 };
    return beats[juce::jlimit (0, 14, idx)];
}

enum ModSrc { SrcNone = 0, SrcEnv2, SrcEnv3, SrcLfo1, SrcLfo2, SrcVelocity, SrcModWheel, SrcAftertouch,
              SrcKeyTrack, SrcNoteRandom, SrcMacro1, SrcMacro2, SrcMacro3, SrcMacro4, SrcAmpEnv, NumModSrc };

enum ModDst { DstNone = 0, DstPitchA, DstPitchB, DstPitchAll, DstPosA, DstPosB, DstLevelA, DstLevelB,
              DstDetuneA, DstDetuneB, DstWarpA, DstWarpB, DstCutoff, DstRes, DstDrive, DstSub, DstNoise,
              DstAmp, DstPan, DstLfo1Rate, DstLfo2Rate, NumModDst };

constexpr int kNumModSlots = 8;
constexpr int kMaxVoices   = 16;
constexpr int kMaxUnison   = 8;

namespace ParamID
{
    inline juce::String osc (int o, const char* name) { return juce::String (o == 0 ? "a_" : "b_") + name; }
    inline juce::String env (int e, const char* name) { return "e" + juce::String (e + 1) + "_" + name; }
    inline juce::String lfo (int l, const char* name) { return "l" + juce::String (l + 1) + "_" + name; }
    inline juce::String mod (int m, const char* name) { return "m" + juce::String (m + 1) + "_" + name; }
    inline juce::String macro (int m)                 { return "macro" + juce::String (m + 1); }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
