#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

struct FactoryPreset
{
    juce::String folder;
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;   // parameter ID -> plain (un-normalised) value
    juce::String category {};                                // derived from the 2-letter name prefix
};

// Sound types, matching the 2-letter preset name prefixes (BA, LD, SY, PL, PD, KY, CH, AR, PC, FX)
const juce::StringArray& getPresetCategories();
juce::String categoryForPresetName (const juce::String& name);

const std::vector<FactoryPreset>& getFactoryPresets();

class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    static juce::File getUserPresetDirectory();
    static constexpr const char* userFolderName = "User";
    static constexpr const char* fileExtension = ".nspreset";

    juce::StringArray getFolders() const;
    juce::StringArray getPresetsInFolder (const juce::String& folder) const;

    void initPatch();
    bool loadFactory (int index);
    bool load (const juce::String& folder, const juce::String& name);
    bool saveUserPreset (const juce::String& name);
    void step (int delta);   // next / previous preset across all folders

    juce::String getCurrentFolder() const { return currentFolder; }
    juce::String getCurrentName() const   { return currentName; }
    int getCurrentFactoryIndex() const    { return currentFactory; }
    void setCurrentLabel (const juce::String& folder, const juce::String& name) { currentFolder = folder; currentName = name; }

    // Development check: returns a list of problems in the factory bank (empty == OK)
    juce::StringArray validateFactoryPresets() const;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String currentFolder { "Init" }, currentName { "Init" };
    int currentFactory = 0;
};
