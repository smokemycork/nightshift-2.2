#include "PluginEditor.h"

namespace
{
    // Matrix row: [#] [source] [destination] [amount slider]
    struct MatrixRow : public juce::Component
    {
        MatrixRow (APVTS& s, int index)
            : src (s, ParamID::mod (index, "src"), {}), dst (s, ParamID::mod (index, "dst"), {})
        {
            num.setText (juce::String (index + 1), juce::dontSendNotification);
            num.setJustificationType (juce::Justification::centred);
            num.setFont (Theme::font (15.0f, true));
            num.setColour (juce::Label::textColourId, Theme::accent);
            amount.setSliderStyle (juce::Slider::LinearHorizontal);
            amount.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
            amount.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            amount.setDoubleClickReturnValue (true, 0.0);
            addAndMakeVisible (num); addAndMakeVisible (src); addAndMakeVisible (dst); addAndMakeVisible (amount);
            attachment = std::make_unique<APVTS::SliderAttachment> (s, ParamID::mod (index, "amt"), amount);
        }
        void paint (juce::Graphics& g) override
        {
            g.setColour (Theme::bg2.withAlpha (0.7f));
            g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 6.0f);
        }
        void resized() override
        {
            auto r = getLocalBounds().reduced (6, 4);
            num.setBounds (r.removeFromLeft (40));
            src.setBounds (r.removeFromLeft (260).reduced (6, 0));
            r.removeFromLeft (10);
            dst.setBounds (r.removeFromLeft (300).reduced (6, 0));
            r.removeFromLeft (20);
            amount.setBounds (r.reduced (6, 0));
        }
        juce::Label num;
        Choice src, dst;
        juce::Slider amount;
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };

    struct MatrixHeader : public juce::Component
    {
        void paint (juce::Graphics& g) override
        {
            g.setFont (Theme::font (11.0f, true));
            g.setColour (Theme::textDim);
            auto r = getLocalBounds().reduced (6, 0);
            r.removeFromLeft (40);
            g.drawText ("SOURCE", r.removeFromLeft (260), juce::Justification::centred);
            r.removeFromLeft (10);
            g.drawText ("DESTINATION", r.removeFromLeft (300), juce::Justification::centred);
            r.removeFromLeft (20);
            g.drawText ("AMOUNT  (double-click = 0)", r, juce::Justification::centred);
        }
    };

    struct InfoText : public juce::Component
    {
        explicit InfoText (juce::String t) : text (std::move (t)) {}
        void paint (juce::Graphics& g) override
        {
            g.setFont (Theme::font (11.5f));
            g.setColour (Theme::textDim);
            g.drawFittedText (text, getLocalBounds().reduced (4), juce::Justification::centredLeft, 4);
        }
        juce::String text;
    };
}

// ============================================================================
NightShiftEditor::NightShiftEditor (NightShiftProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    content.setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    content.onResized = [this] { layoutContent(); };

    const char* tabNames[3] = { "SYNTH", "MOD MATRIX", "EFFECTS" };
    for (int i = 0; i < 3; ++i)
    {
        tabs[i].setButtonText (tabNames[i]);
        tabs[i].setClickingTogglesState (true);
        tabs[i].setRadioGroupId (101);
        tabs[i].onClick = [this, i] { showPage (i); };
        content.addAndMakeVisible (tabs[i]);
    }
    content.addAndMakeVisible (synthPage);
    content.addChildComponent (modPage);
    content.addChildComponent (fxPage);

    buildHeader();
    buildSynthPage();
    buildModPage();
    buildFxPage();
    buildBottom();

    tabs[0].setToggleState (true, juce::dontSendNotification);
    showPage (0);
    refreshPresetBrowser();

    setResizable (true, true);
    setResizeLimits (kDesignW * 6 / 10, kDesignH * 6 / 10, kDesignW * 16 / 10, kDesignH * 16 / 10);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kDesignW / (double) kDesignH);
    setSize (kDesignW, kDesignH);
    startTimerHz (5);
}

NightShiftEditor::~NightShiftEditor()
{
    stopTimer();
    content.setLookAndFeel (nullptr);
}

void NightShiftEditor::paint (juce::Graphics& g) { g.fillAll (Theme::bg); }

void NightShiftEditor::resized()
{
    const float scale = (float) getWidth() / (float) kDesignW;
    content.setBounds (0, 0, kDesignW, kDesignH);
    content.setTransform (juce::AffineTransform::scale (scale));
}

void NightShiftEditor::Content::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);

    // header strip
    auto header = getLocalBounds().removeFromTop (64).toFloat();
    juce::ColourGradient grad (Theme::bg2.brighter (0.08f), 0, 0, Theme::bg, 0, header.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRect (header);
    g.setColour (Theme::panelEdge);
    g.drawHorizontalLine (64, 0.0f, (float) getWidth());

    // logo
    g.setColour (Theme::accent);
    g.setFont (Theme::font (30.0f, true));
    g.drawText ("NIGHTSHIFT", juce::Rectangle<int> (18, 6, 260, 34), juce::Justification::centredLeft, false);
    g.setColour (Theme::textDim);
    g.setFont (Theme::font (10.5f, true));
    g.drawText ("ANALOG WAVETABLE SYNTHESIZER", juce::Rectangle<int> (20, 38, 260, 16), juce::Justification::centredLeft, false);
}

// ============================================================================
void NightShiftEditor::buildHeader()
{
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &initBtn })
        content.addAndMakeVisible (b);

    prevBtn.setTooltip ("Previous preset");
    nextBtn.setTooltip ("Next preset");
    saveBtn.setTooltip ("Save current sound as a User preset");
    initBtn.setTooltip ("Reset to the Init patch");

    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    initBtn.onClick = [this] { proc.presets.loadFactory (0); refreshPresetBrowser(); };
    saveBtn.onClick = [this] { openSaveDialog(); };

    folderBox.setTextWhenNothingSelected ("Type");
    presetBox.setTextWhenNothingSelected ("-- choose preset --");
    folderBox.setTooltip ("Sound type: every Bass, Lead, Pad... from all labels in one list");
    labelBox.setTooltip ("Optional: only show sounds in one label style");
    labelBox.addItem ("All Labels", 1);
    {
        int id = 2;
        for (auto& f : proc.presets.getFolders())
            if (f != "Init" && f != PresetManager::userFolderName)
                labelBox.addItem (f.replace (" Style", ""), id++);
    }
    labelBox.setSelectedId (1, juce::dontSendNotification);
    content.addAndMakeVisible (folderBox);
    content.addAndMakeVisible (labelBox);
    content.addAndMakeVisible (presetBox);

    folderBox.onChange = [this] { viewFolder = folderBox.getText(); fillPresetBox(); };
    labelBox.onChange  = [this] { fillPresetBox(); };
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0 && id <= (int) browserItems.size())
            loadBrowserItem (browserItems[(size_t) id - 1]);
    };

    scope = std::make_unique<ScopeDisplay> (proc);
    content.addAndMakeVisible (*scope);

    masterKnob = &K ("master", "MASTER");
    content.addAndMakeVisible (*masterKnob);
}

void NightShiftEditor::buildSynthPage()
{
    auto put = [this] (juce::Component& c, int x, int y, int w, int h)
    {
        synthPage.addAndMakeVisible (c);
        synthLayout.push_back ({ &c, { x, y, w, h } });
    };

    // ---- oscillators ----
    for (int o = 0; o < 2; ++o)
    {
        auto& p = P (o == 0 ? "OSC A" : "OSC B", 5, 3);
        p.addHeader (T (ParamID::osc (o, "on"), {}), 22);
        p.addHeader (C (ParamID::osc (o, "wt"), {}), 160);
        p.setLeftArea (make<WavetableDisplay> (proc, o), 140);
        p.place (K (ParamID::osc (o, "pos"), "WT POS"), 0, 0);
        p.place (K (ParamID::osc (o, "level"), "LEVEL"), 1, 0);
        p.place (K (ParamID::osc (o, "pan"), "PAN"), 2, 0);
        p.place (K (ParamID::osc (o, "oct"), "OCT"), 3, 0);
        p.place (K (ParamID::osc (o, "semi"), "SEMI"), 4, 0);
        p.place (K (ParamID::osc (o, "fine"), "FINE"), 0, 1);
        p.place (K (ParamID::osc (o, "uni"), "UNISON"), 1, 1);
        p.place (K (ParamID::osc (o, "det"), "DETUNE"), 2, 1);
        p.place (K (ParamID::osc (o, "blend"), "BLEND"), 3, 1);
        p.place (K (ParamID::osc (o, "width"), "WIDTH"), 4, 1);
        p.place (C (ParamID::osc (o, "warp"), "WARP MODE"), 0, 2, 2, 1);
        p.place (K (ParamID::osc (o, "warpamt"), "WARP"), 2, 2);
        p.place (K (ParamID::osc (o, "phase"), "PHASE"), 3, 2);
        p.place (K (ParamID::osc (o, "rand"), "RAND"), 4, 2);
        put (p, o * 446, 0, 440, 205);
    }

    // ---- sub / noise ----
    {
        auto& p = P ("SUB / NOISE", 4, 3);
        p.place (T ("sub_on", "SUB"), 0, 0);
        p.place (C ("sub_wave", {}), 1, 0, 2, 1);
        p.place (K ("sub_level", "SUB LVL"), 3, 0);
        p.place (C ("sub_oct", {}), 0, 1, 2, 1);
        p.place (T ("sub_direct", "NO FILTER"), 2, 1, 2, 1);
        p.place (T ("noise_on", "NOISE"), 0, 2);
        p.place (C ("noise_type", {}), 1, 2, 2, 1);
        p.place (K ("noise_level", "NOISE LVL"), 3, 2);
        put (p, 892, 0, 252, 205);
    }

    // ---- filter ----
    {
        auto& p = P ("FILTER", 5, 1);
        p.addHeader (T ("f_on", {}), 22);
        p.addHeader (C ("f_type", {}), 130);
        p.place (K ("f_cutoff", "CUTOFF"), 0, 0);
        p.place (K ("f_res", "RESO"), 1, 0);
        p.place (K ("f_drive", "DRIVE"), 2, 0);
        p.place (K ("f_key", "KEY TRK"), 3, 0);
        p.place (K ("f_env", "ENV 2 AMT"), 4, 0);
        put (p, 0, 211, 420, 155);
    }

    // ---- envelopes ----
    const char* envTitles[3] = { "AMP ENV", "ENV 2  (FILTER)", "ENV 3  (MOD)" };
    const int envX[3] = { 426, 668, 910 };
    for (int e = 0; e < 3; ++e)
    {
        auto& p = P (envTitles[e], 4, 1);
        p.setTopArea (make<EnvDisplay> (proc.apvts, e), 46);
        p.place (K (ParamID::env (e, "a"), "ATTACK"), 0, 0);
        p.place (K (ParamID::env (e, "d"), "DECAY"), 1, 0);
        p.place (K (ParamID::env (e, "s"), "SUSTAIN"), 2, 0);
        p.place (K (ParamID::env (e, "r"), "RELEASE"), 3, 0);
        put (p, envX[e], 211, e == 2 ? 234 : 236, 155);
    }

    // ---- LFOs ----
    for (int l = 0; l < 2; ++l)
    {
        auto& p = P ("LFO " + juce::String (l + 1), 5, 1);
        p.addHeader (C (ParamID::lfo (l, "shape"), {}), 120);
        p.setTopArea (make<LfoDisplay> (proc.apvts, l), 40);
        p.place (K (ParamID::lfo (l, "rate"), "RATE HZ"), 0, 0);
        p.place (T (ParamID::lfo (l, "sync"), "SYNC"), 1, 0);
        p.place (C (ParamID::lfo (l, "div"), "DIVISION"), 2, 0);
        p.place (C (ParamID::lfo (l, "mode"), "MODE"), 3, 0);
        p.place (K (ParamID::lfo (l, "delay"), "RISE"), 4, 0);
        put (p, l * 376, 372, 370, 158);
    }

    // ---- voice / global ----
    {
        auto& p = P ("VOICE", 6, 2);
        p.place (C ("mode", "MODE"), 0, 0, 2, 1);
        p.place (C ("chord", "CHORD MEMORY"), 2, 0, 2, 1);
        p.place (K ("voices", "VOICES"), 4, 0);
        p.place (K ("glide", "GLIDE"), 5, 0);
        p.place (K ("bend", "BEND"), 0, 1);
        p.place (K ("analog", "ANALOG"), 1, 1);
        p.place (K ("vel", "VELOCITY"), 2, 1);
        p.place (make<InfoText> ("ANALOG = per-voice drift, phase & filter variance + warm saturation. "
                                 "CHORD MEMORY plays a chord from one key (Poly)."), 3, 1, 3, 1);
        put (p, 752, 372, 392, 158);
    }
}

void NightShiftEditor::buildModPage()
{
    auto& p = P ("MOD MATRIX  -  8 SLOTS", 1, 10);
    p.place (make<MatrixHeader>(), 0, 0);
    for (int m = 0; m < kNumModSlots; ++m)
        p.place (make<MatrixRow> (proc.apvts, m), 0, m + 1);
    p.place (make<InfoText> ("Sources: Env 2/3, LFO 1/2, Velocity, Mod Wheel, Aftertouch, Key Track, Note Random, "
                             "Macro 1-4 and the Amp Env.  Pitch amount 1.0 = 24 semitones, Cutoff 1.0 = 6 octaves.  "
                             "Map the MACRO knobs (bottom left) here to build performance controls."), 0, 9);
    modPage.addAndMakeVisible (p);
    modLayout.push_back ({ &p, { 0, 0, 1144, 530 } });
}

void NightShiftEditor::buildFxPage()
{
    auto put = [this] (Panel& p, int col, int row)
    {
        fxPage.addAndMakeVisible (p);
        const int w = (1144 - 3 * 6) / 4;
        fxLayout.push_back ({ &p, { col * (w + 6), row * 268, w, 262 } });
    };

    { auto& p = P ("DISTORTION", 2, 2); p.addHeader (T ("dist_on", {}), 22);
      p.place (C ("dist_type", "TYPE"), 0, 0); p.place (K ("dist_drive", "DRIVE"), 1, 0);
      p.place (K ("dist_tone", "TONE"), 0, 1); p.place (K ("dist_mix", "MIX"), 1, 1); put (p, 0, 0); }

    { auto& p = P ("EQUALIZER", 2, 2); p.addHeader (T ("eq_on", {}), 22);
      p.place (K ("eq_low", "LOW"), 0, 0); p.place (K ("eq_high", "HIGH"), 1, 0);
      p.place (K ("eq_mid", "MID"), 0, 1); p.place (K ("eq_midf", "MID FREQ"), 1, 1); put (p, 1, 0); }

    { auto& p = P ("CHORUS", 2, 2); p.addHeader (T ("cho_on", {}), 22);
      p.place (K ("cho_rate", "RATE"), 0, 0); p.place (K ("cho_depth", "DEPTH"), 1, 0);
      p.place (K ("cho_mix", "MIX"), 0, 1, 2, 1); put (p, 2, 0); }

    { auto& p = P ("PHASER", 2, 2); p.addHeader (T ("pha_on", {}), 22);
      p.place (K ("pha_rate", "RATE"), 0, 0); p.place (K ("pha_depth", "DEPTH"), 1, 0);
      p.place (K ("pha_fb", "FEEDBACK"), 0, 1); p.place (K ("pha_mix", "MIX"), 1, 1); put (p, 3, 0); }

    { auto& p = P ("DELAY", 4, 2); p.addHeader (T ("dly_on", {}), 22);
      p.place (T ("dly_sync", "SYNC"), 0, 0); p.place (C ("dly_div", "DIVISION"), 1, 0, 2, 1); p.place (K ("dly_time", "TIME MS"), 3, 0);
      p.place (K ("dly_fb", "FEEDBK"), 0, 1); p.place (K ("dly_tone", "TONE"), 1, 1); p.place (K ("dly_mix", "MIX"), 2, 1);
      p.place (T ("dly_pp", "P.PONG"), 3, 1); put (p, 0, 1); }

    { auto& p = P ("REVERB", 2, 2); p.addHeader (T ("rev_on", {}), 22);
      p.place (K ("rev_size", "SIZE"), 0, 0); p.place (K ("rev_damp", "DAMP"), 1, 0);
      p.place (K ("rev_width", "WIDTH"), 0, 1); p.place (K ("rev_mix", "MIX"), 1, 1); put (p, 1, 1); }

    { auto& p = P ("COMPRESSOR", 3, 2); p.addHeader (T ("comp_on", {}), 22);
      p.place (K ("comp_thresh", "THRESH"), 0, 0); p.place (K ("comp_ratio", "RATIO"), 1, 0); p.place (K ("comp_gain", "MAKEUP"), 2, 0);
      p.place (K ("comp_att", "ATTACK"), 0, 1); p.place (K ("comp_rel", "RELEASE"), 1, 1); put (p, 2, 1); }

    { auto& p = P ("PUMP  (SIDECHAIN)", 2, 2); p.addHeader (T ("pump_on", {}), 22);
      p.place (C ("pump_div", "RATE"), 0, 0, 2, 1); p.place (K ("pump_depth", "DEPTH"), 0, 1);
      p.place (K ("pump_curve", "CURVE"), 1, 1); put (p, 3, 1); }
}

void NightShiftEditor::buildBottom()
{
    auto& p = P ("MACROS", 4, 1);
    for (int m = 0; m < 4; ++m)
        p.place (K (ParamID::macro (m), "MACRO " + juce::String (m + 1)), m, 0);
    macroPanel = &p;
    content.addAndMakeVisible (p);

    keyboard.setAvailableRange (24, 96);    // C0..C5 (Ableton naming: middle C = C3)
    keyboard.setLowestVisibleKey (24);
    keyboard.setScrollButtonsVisible (false);
    content.addAndMakeVisible (keyboard);
}

void NightShiftEditor::layoutContent()
{
    // header
    int x = 246;
    prevBtn.setBounds (x, 18, 28, 28);   x += 32;
    folderBox.setBounds (x, 18, 150, 28); x += 154;
    labelBox.setBounds (x, 18, 124, 28);  x += 128;
    presetBox.setBounds (x, 18, 226, 28); x += 230;
    nextBtn.setBounds (x, 18, 28, 28);   x += 34;
    saveBtn.setBounds (x, 18, 54, 28);   x += 58;
    initBtn.setBounds (x, 18, 46, 28);
    scope->setBounds (958, 8, 108, 48);
    masterKnob->setBounds (1074, 2, 76, 60);

    // tabs
    for (int i = 0; i < 3; ++i)
        tabs[i].setBounds (8 + i * 128, 70, 124, 24);

    const juce::Rectangle<int> page (8, 100, 1144, 530);
    for (auto* pg : { &synthPage, &modPage, &fxPage })
        pg->setBounds (page);
    for (auto* lay : { &synthLayout, &modLayout, &fxLayout })
        for (auto& pl : *lay)
            pl.comp->setBounds (pl.bounds);

    macroPanel->setBounds (8, 636, 380, 118);
    keyboard.setBounds (396, 640, 756, 112);
    keyboard.setKeyWidth (756.0f / 36.0f);   // 36 white keys in range
}

void NightShiftEditor::showPage (int index)
{
    synthPage.setVisible (index == 0);
    modPage.setVisible (index == 1);
    fxPage.setVisible (index == 2);
}

// ============================================================================
void NightShiftEditor::fillPresetBox()
{
    browserItems.clear();
    presetBox.clear (juce::dontSendNotification);
    const auto& bank = getFactoryPresets();

    if (viewFolder == PresetManager::userFolderName)
    {
        for (auto& name : proc.presets.getPresetsInFolder (viewFolder))
            browserItems.push_back ({ -1, name });
    }
    else
    {
        // folder = sound type (or All); label box narrows to one label style
        const bool allTypes = viewFolder == kAllFolders;
        const juce::String label = labelBox.getSelectedId() > 1 ? labelBox.getText() : juce::String();
        for (int i = 1; i < (int) bank.size(); ++i)   // 0 = Init (INIT button)
        {
            const auto& p = bank[(size_t) i];
            if ((allTypes || p.category == viewFolder)
                && (label.isEmpty() || p.folder.replace (" Style", "") == label))
                browserItems.push_back ({ i, {} });
        }

        // group by label (in folder order), then alphabetical by name
        const auto folderOrder = proc.presets.getFolders();
        std::stable_sort (browserItems.begin(), browserItems.end(), [&] (const BrowserItem& a, const BrowserItem& b)
        {
            const auto& pa = bank[(size_t) a.factoryIndex];
            const auto& pb = bank[(size_t) b.factoryIndex];
            const int fa = folderOrder.indexOf (pa.folder), fb = folderOrder.indexOf (pb.folder);
            if (fa != fb) return fa < fb;
            return pa.name.substring (3).compareNatural (pb.name.substring (3)) < 0;
        });
    }

    for (size_t i = 0; i < browserItems.size(); ++i)
    {
        const auto& it = browserItems[i];
        juce::String text;
        if (it.factoryIndex >= 0)
        {
            const auto& p = bank[(size_t) it.factoryIndex];
            // inside a type folder the BA/LD/... prefix is redundant; show the label instead
            text = (viewFolder == kAllFolders ? p.name : p.name.substring (3))
                 + "   [" + p.folder.replace (" Style", "") + "]";
        }
        else text = it.userName;
        presetBox.addItem (text, (int) i + 1);
    }

    const int cur = indexOfCurrentInBrowser();
    if (cur >= 0) presetBox.setSelectedId (cur + 1, juce::dontSendNotification);
}

int NightShiftEditor::indexOfCurrentInBrowser() const
{
    const int fi = proc.presets.getCurrentFactoryIndex();
    for (size_t i = 0; i < browserItems.size(); ++i)
    {
        const auto& it = browserItems[i];
        if (fi >= 0 ? it.factoryIndex == fi
                    : (it.factoryIndex < 0 && proc.presets.getCurrentFolder() == PresetManager::userFolderName
                       && it.userName == proc.presets.getCurrentName()))
            return (int) i;
    }
    return -1;
}

void NightShiftEditor::loadBrowserItem (const BrowserItem& item)
{
    if (item.factoryIndex >= 0) proc.presets.loadFactory (item.factoryIndex);
    else proc.presets.load (PresetManager::userFolderName, item.userName);
    refreshPresetBrowser();
}

void NightShiftEditor::stepPreset (int delta)
{
    if (browserItems.empty()) { proc.presets.step (delta); refreshPresetBrowser(); return; }
    const int n = (int) browserItems.size();
    const int cur = indexOfCurrentInBrowser();
    const int next = cur < 0 ? (delta > 0 ? 0 : n - 1) : ((cur + delta) % n + n) % n;
    loadBrowserItem (browserItems[(size_t) next]);
}

void NightShiftEditor::refreshPresetBrowser()
{
    juce::StringArray folders;
    folders.addArray (getPresetCategories());   // Bass, Lead, Synth, Pluck, Pad, Keys, Chord, Arp, Perc, FX
    folders.add (kAllFolders);
    folders.add (PresetManager::userFolderName);
    folderBox.clear (juce::dontSendNotification);
    for (int i = 0; i < folders.size(); ++i)
        folderBox.addItem (folders[i], i + 1);

    // follow the loaded preset's type, unless the user is browsing "All Presets"
    const int fi = proc.presets.getCurrentFactoryIndex();
    if (fi > 0)
    {
        if (viewFolder != kAllFolders)
            viewFolder = getFactoryPresets()[(size_t) fi].category;
    }
    else if (proc.presets.getCurrentFolder() == PresetManager::userFolderName)
        viewFolder = PresetManager::userFolderName;
    else if (! folders.contains (viewFolder))
        viewFolder = "Bass";

    const int f = folders.indexOf (viewFolder);
    if (f >= 0) folderBox.setSelectedId (f + 1, juce::dontSendNotification);
    fillPresetBox();

    shownFolder = proc.presets.getCurrentFolder();
    shownName = proc.presets.getCurrentName();
}

void NightShiftEditor::timerCallback()
{
    if (proc.presets.getCurrentFolder() != shownFolder || proc.presets.getCurrentName() != shownName)
        refreshPresetBrowser();
}

void NightShiftEditor::openSaveDialog()
{
    auto* w = new juce::AlertWindow ("Save Preset", "Name your preset (saved to Documents/NightShift/User Presets):",
                                     juce::MessageBoxIconType::NoIcon);
    const auto current = proc.presets.getCurrentFolder() == PresetManager::userFolderName ? proc.presets.getCurrentName()
                                                                                          : juce::String ("My Preset");
    w->addTextEditor ("name", current);
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<NightShiftEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result == 1 && safe != nullptr)
        {
            if (safe->proc.presets.saveUserPreset (w->getTextEditorContents ("name")))
                safe->refreshPresetBrowser();
        }
    }), true);
}
