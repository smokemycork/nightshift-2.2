#pragma once
#include "Widgets.h"

class NightShiftEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int kDesignW = 1160;
    static constexpr int kDesignH = 760;

    explicit NightShiftEditor (NightShiftProcessor&);
    ~NightShiftEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // design-size container that gets scaled with the window
    struct Content : public juce::Component
    {
        void paint (juce::Graphics&) override;
        void resized() override { if (onResized) onResized(); }
        std::function<void()> onResized;
    };

    void timerCallback() override;
    void buildHeader();
    void buildSynthPage();
    void buildModPage();
    void buildFxPage();
    void buildBottom();
    void layoutContent();
    void showPage (int index);
    void refreshPresetBrowser();
    void fillPresetBox();
    void stepPreset (int delta);
    void openSaveDialog();

    template <typename T, typename... Args>
    T& make (Args&&... args)
    {
        auto p = std::make_unique<T> (std::forward<Args> (args)...);
        auto& ref = *p;
        owned.push_back (std::move (p));
        return ref;
    }
    Knob&   K (const juce::String& id, const juce::String& label) { return make<Knob> (proc.apvts, id, label); }
    Choice& C (const juce::String& id, const juce::String& label) { return make<Choice> (proc.apvts, id, label); }
    Toggle& T (const juce::String& id, const juce::String& text)  { return make<Toggle> (proc.apvts, id, text); }
    Panel&  P (const juce::String& title, int cols, int rows)     { return make<Panel> (title, cols, rows); }

    NightShiftProcessor& proc;
    NSLookAndFeel lnf;
    Content content;
    juce::TooltipWindow tooltips { &content, 600 };

    juce::Component synthPage, modPage, fxPage;
    juce::TextButton tabs[3];

    // header
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "SAVE" }, initBtn { "INIT" };
    juce::ComboBox folderBox, labelBox, presetBox;   // folder = sound type, label = optional style filter
    struct BrowserItem { int factoryIndex; juce::String userName; };
    std::vector<BrowserItem> browserItems;
    juce::String viewFolder;
    static constexpr const char* kAllFolders = "All Presets";
    void loadBrowserItem (const BrowserItem& item);
    int indexOfCurrentInBrowser() const;
    std::unique_ptr<ScopeDisplay> scope;
    Knob* masterKnob = nullptr;

    // bottom
    Panel* macroPanel = nullptr;
    juce::MidiKeyboardComponent keyboard;

    // page layout records
    struct Placement { juce::Component* comp; juce::Rectangle<int> bounds; };
    std::vector<Placement> synthLayout, modLayout, fxLayout;

    std::vector<std::unique_ptr<juce::Component>> owned;
    juce::String shownFolder, shownName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NightShiftEditor)
};
