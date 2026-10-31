#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "Params.h"
#include "SynthVoice.h"
#include "Effects.h"
#include "Presets.h"

class NightShiftProcessor : public juce::AudioProcessor
{
public:
    NightShiftProcessor();
    ~NightShiftProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets { apvts };

    const WavetableBank& getBank() const { return bank.get(); }

    // Oscilloscope feed for the UI (lock-free ring buffer)
    static constexpr int kScopeSize = 1024;
    std::array<std::atomic<float>, kScopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };

    // Number of currently sounding voices (UI display)
    std::atomic<int> activeVoices { 0 };

    // Keyboard state so the UI keyboard can play the synth
    juce::MidiKeyboardState keyboardState;

private:
    void readParams();
    void handleMidi (const juce::MidiMessage& m);
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff (bool hard);
    SynthVoice* findVoice();

    juce::SharedResourcePointer<WavetableBank> bank;
    std::array<SynthVoice, kMaxVoices> voices;
    juce::uint64 noteCounter = 0;

    SynthParams sp;
    FxParams fx;
    GlobalState gs;
    EffectsChain effects;
    juce::SmoothedValue<float> masterGain;

    // MIDI state
    std::vector<int> monoStack;
    float lastVelocity = 1.0f;
    float lastPitch = -1.0f;
    bool sustain = false;
    std::array<bool, 128> sustained {};
    float bendNorm = 0.0f;
    int lastVoiceMode = -1;

    struct Cache;   // cached parameter pointers
    std::unique_ptr<Cache> cache;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NightShiftProcessor)
};
