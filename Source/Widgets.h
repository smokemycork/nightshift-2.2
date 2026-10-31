#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "LookAndFeel.h"
#include "PluginProcessor.h"

using APVTS = juce::AudioProcessorValueTreeState;

// ---------------------------------------------------------------- Knob
class Knob : public juce::Component
{
public:
    Knob (APVTS& s, const juce::String& id, const juce::String& label)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setPopupDisplayEnabled (true, true, nullptr, 1500);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (slider);

        name.setText (label, juce::dontSendNotification);
        name.setJustificationType (juce::Justification::centred);
        name.setFont (Theme::font (11.0f, true));
        name.setColour (juce::Label::textColourId, Theme::textDim);
        name.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (name);

        attachment = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
        if (auto* p = s.getParameter (id))
        {
            slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
            slider.setTooltip (p->getName (64) + "  (double-click to reset)");
        }
    }

    void resized() override
    {
        auto b = getLocalBounds();
        name.setBounds (b.removeFromBottom (14));
        const int d = juce::jmin (b.getWidth(), b.getHeight());
        slider.setBounds (b.withSizeKeepingCentre (d, d));
    }

    juce::Slider slider;
    juce::Label name;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

// ---------------------------------------------------------------- Choice (combo box)
class Choice : public juce::Component
{
public:
    Choice (APVTS& s, const juce::String& id, const juce::String& label)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
            box.addItemList (p->choices, 1);
        box.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (box);

        if (label.isNotEmpty())
        {
            name.setText (label, juce::dontSendNotification);
            name.setJustificationType (juce::Justification::centred);
            name.setFont (Theme::font (11.0f, true));
            name.setColour (juce::Label::textColourId, Theme::textDim);
            addAndMakeVisible (name);
        }
        attachment = std::make_unique<APVTS::ComboBoxAttachment> (s, id, box);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        if (name.getText().isNotEmpty())
            name.setBounds (b.removeFromBottom (14));
        box.setBounds (b.withSizeKeepingCentre (b.getWidth() - 4, juce::jmin (24, b.getHeight())));
    }

    juce::ComboBox box;
    juce::Label name;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

// ---------------------------------------------------------------- Toggle
class Toggle : public juce::Component
{
public:
    Toggle (APVTS& s, const juce::String& id, const juce::String& text)
    {
        button.setButtonText (text);
        addAndMakeVisible (button);
        attachment = std::make_unique<APVTS::ButtonAttachment> (s, id, button);
        if (auto* p = s.getParameter (id)) button.setTooltip (p->getName (64));
    }
    void resized() override { button.setBounds (getLocalBounds()); }

    juce::ToggleButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

// ---------------------------------------------------------------- Panel (titled grid container)
class Panel : public juce::Component
{
public:
    Panel (const juce::String& t, int columns, int rows) : title (t), cols (columns), numRows (rows) {}

    void place (juce::Component& c, int col, int row, int colSpan = 1, int rowSpan = 1)
    {
        addAndMakeVisible (c);
        cells.push_back ({ &c, col, row, colSpan, rowSpan });
    }
    void addHeader (juce::Component& c, int width) { addAndMakeVisible (c); header.push_back ({ &c, width }); }
    void setLeftArea (juce::Component& c, int width) { addAndMakeVisible (c); left = &c; leftW = width; }
    void setTopArea (juce::Component& c, int height) { addAndMakeVisible (c); top = &c; topH = height; }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (Theme::panel);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (Theme::panelEdge);
        g.drawRoundedRectangle (r, 8.0f, 1.0f);

        auto t = r.removeFromTop (24.0f);
        juce::Path tp;
        tp.addRoundedRectangle (t.getX(), t.getY(), t.getWidth(), t.getHeight(), 8.0f, 8.0f, true, true, false, false);
        g.setColour (Theme::title);
        g.fillPath (tp);
        g.setColour (Theme::accent);
        g.fillRect (t.getX() + 10.0f, t.getBottom() - 2.0f, 18.0f, 2.0f);
        g.setFont (Theme::font (12.5f, true));
        g.setColour (Theme::text);
        g.drawText (title, t.withTrimmedLeft (10.0f).toNearestInt(), juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto t = r.removeFromTop (24).reduced (4, 2);
        for (auto it = header.rbegin(); it != header.rend(); ++it)
        {
            it->comp->setBounds (t.removeFromRight (it->width));
            t.removeFromRight (4);
        }

        r.reduce (6, 4);
        if (left != nullptr) { left->setBounds (r.removeFromLeft (leftW).reduced (2)); r.removeFromLeft (4); }
        if (top != nullptr)  { top->setBounds (r.removeFromTop (topH).reduced (2)); r.removeFromTop (2); }

        const float cw = (float) r.getWidth() / (float) cols;
        const float ch = (float) r.getHeight() / (float) numRows;
        for (auto& c : cells)
            c.comp->setBounds (juce::Rectangle<float> ((float) r.getX() + cw * (float) c.col, (float) r.getY() + ch * (float) c.row,
                                                       cw * (float) c.cs, ch * (float) c.rs).toNearestInt().reduced (2));
    }

private:
    struct Cell { juce::Component* comp; int col, row, cs, rs; };
    struct Head { juce::Component* comp; int width; };
    juce::String title;
    int cols, numRows;
    std::vector<Cell> cells;
    std::vector<Head> header;
    juce::Component* left = nullptr;
    juce::Component* top = nullptr;
    int leftW = 0, topH = 0;
};

// ---------------------------------------------------------------- Displays
class ParamWatcher : public juce::Timer
{
public:
    ParamWatcher (APVTS& s, juce::StringArray ids, std::function<void()> onChange)
        : state (s), callback (std::move (onChange))
    {
        for (auto& id : ids) if (auto* p = s.getRawParameterValue (id)) params.push_back (p);
        last.assign (params.size(), -12345.0f);
        startTimerHz (20);
    }
    void timerCallback() override
    {
        bool changed = false;
        for (size_t i = 0; i < params.size(); ++i)
        {
            const float v = params[i]->load();
            if (! juce::exactlyEqual (v, last[i])) { last[i] = v; changed = true; }
        }
        if (changed && callback) callback();
    }
    float get (size_t i) const { return i < params.size() ? params[i]->load() : 0.0f; }

private:
    APVTS& state;
    std::vector<std::atomic<float>*> params;
    std::vector<float> last;
    std::function<void()> callback;
};

class WavetableDisplay : public juce::Component
{
public:
    WavetableDisplay (NightShiftProcessor& p, int oscIndex)
        : proc (p), watcher (p.apvts, { ParamID::osc (oscIndex, "wt"), ParamID::osc (oscIndex, "pos"),
                                        ParamID::osc (oscIndex, "on"), ParamID::osc (oscIndex, "warp"),
                                        ParamID::osc (oscIndex, "warpamt") }, [this] { repaint(); }) {}

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Theme::bg);
        g.fillRoundedRectangle (r, 6.0f);
        const auto& bank = proc.getBank();
        const int table = juce::jlimit (0, bank.getNumTables() - 1, (int) std::lround (watcher.get (0)));
        const float pos = watcher.get (1);
        const bool on = watcher.get (2) > 0.5f;

        auto area = r.reduced (8.0f, 10.0f);
        const int frames = 8;
        const float dx = area.getWidth() * 0.18f / frames, dy = area.getHeight() * 0.35f / frames;
        const float w = area.getWidth() * 0.82f, h = area.getHeight() * 0.65f;

        for (int f = frames - 1; f >= 0; --f)
        {
            const float fp = (float) f / (float) (frames - 1);
            const float ox = area.getX() + dx * (float) f, oy = area.getY() + area.getHeight() * 0.35f - dy * (float) f;
            juce::Path path;
            for (int i = 0; i <= 96; ++i)
            {
                const float ph = (float) i / 96.0f;
                const float v = bank.read (table, fp, 4, juce::jmin (ph, 0.9999f));
                const float px = ox + ph * w, py = oy + h * 0.5f - v * h * 0.45f;
                i == 0 ? path.startNewSubPath (px, py) : path.lineTo (px, py);
            }
            g.setColour (Theme::accent2.withAlpha (on ? 0.12f : 0.06f));
            g.strokePath (path, juce::PathStrokeType (1.0f));
        }

        // current position (drawn on the front layer, interpolated)
        const float ox = area.getX() + dx * pos * (frames - 1), oy = area.getY() + area.getHeight() * 0.35f - dy * pos * (frames - 1);
        juce::Path cur;
        for (int i = 0; i <= 160; ++i)
        {
            const float ph = (float) i / 160.0f;
            const float v = bank.read (table, pos, 2, juce::jmin (ph, 0.9999f));
            const float px = ox + ph * w, py = oy + h * 0.5f - v * h * 0.45f;
            i == 0 ? cur.startNewSubPath (px, py) : cur.lineTo (px, py);
        }
        g.setColour ((on ? Theme::accent : Theme::textDim).withAlpha (0.25f));
        g.strokePath (cur, juce::PathStrokeType (5.0f));
        g.setColour (on ? Theme::accent : Theme::textDim);
        g.strokePath (cur, juce::PathStrokeType (1.8f));

        g.setFont (Theme::font (10.0f));
        g.setColour (Theme::textDim);
        g.drawText ("FRAME " + juce::String (1 + (int) std::lround (pos * 15.0f)) + "/16", r.reduced (6, 3),
                    juce::Justification::bottomRight, false);
    }

private:
    NightShiftProcessor& proc;
    ParamWatcher watcher;
};

class EnvDisplay : public juce::Component
{
public:
    EnvDisplay (APVTS& s, int e)
        : watcher (s, { ParamID::env (e, "a"), ParamID::env (e, "d"), ParamID::env (e, "s"), ParamID::env (e, "r") },
                   [this] { repaint(); }) {}

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Theme::bg);
        g.fillRoundedRectangle (r, 5.0f);
        r = r.reduced (6.0f, 6.0f);
        auto t = [] (float sec) { return std::cbrt (juce::jmax (0.0f, sec)); };   // perceptual time axis
        const float a = t (watcher.get (0)), d = t (watcher.get (1)), s = watcher.get (2), rel = t (watcher.get (3));
        const float hold = 0.6f;
        const float total = a + d + hold + rel;
        const float sx = r.getWidth() / juce::jmax (0.001f, total);
        auto Y = [&] (float lvl) { return r.getBottom() - lvl * r.getHeight(); };

        juce::Path p;
        float x = r.getX();
        p.startNewSubPath (x, Y (0));
        x += a * sx;   p.quadraticTo (x - a * sx * 0.6f, Y (0.9f), x, Y (1));
        x += d * sx;   p.quadraticTo (x - d * sx * 0.8f, Y (s), x, Y (s));
        x += hold * sx; p.lineTo (x, Y (s));
        x += rel * sx; p.quadraticTo (x - rel * sx * 0.8f, Y (0), x, Y (0));

        juce::Path fill (p);
        fill.lineTo (r.getX(), Y (0));
        fill.closeSubPath();
        g.setColour (Theme::accent.withAlpha (0.12f));
        g.fillPath (fill);
        g.setColour (Theme::accent);
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }

private:
    ParamWatcher watcher;
};

class LfoDisplay : public juce::Component
{
public:
    LfoDisplay (APVTS& s, int l) : watcher (s, { ParamID::lfo (l, "shape") }, [this] { repaint(); }) {}

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Theme::bg);
        g.fillRoundedRectangle (r, 5.0f);
        r = r.reduced (6.0f, 6.0f);
        dsp::Lfo lfo;
        lfo.seed (42);
        const int shape = (int) std::lround (watcher.get (0));
        juce::Path p;
        const int N = 200;
        for (int i = 0; i <= N; ++i)
        {
            const float v = lfo.advance (i == 0 ? 0.0 : 2.0 / N, shape);
            const float px = r.getX() + r.getWidth() * (float) i / (float) N;
            const float py = r.getCentreY() - v * r.getHeight() * 0.45f;
            i == 0 ? p.startNewSubPath (px, py) : p.lineTo (px, py);
        }
        g.setColour (Theme::accent2);
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }

private:
    ParamWatcher watcher;
};

class ScopeDisplay : public juce::Component, private juce::Timer
{
public:
    explicit ScopeDisplay (NightShiftProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Theme::bg);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (Theme::panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

        constexpr int S = NightShiftProcessor::kScopeSize;
        float buf[S];
        const int w = proc.scopeWrite.load();
        for (int i = 0; i < S; ++i) buf[i] = proc.scope[(size_t) ((w + i) & (S - 1))].load();

        // find a rising zero crossing in the first half for a stable trace
        int start = 0;
        for (int i = 1; i < S / 2; ++i)
            if (buf[i - 1] < 0.0f && buf[i] >= 0.0f) { start = i; break; }

        const int len = S / 2;
        juce::Path p;
        r = r.reduced (4.0f, 4.0f);
        for (int i = 0; i < len; ++i)
        {
            const float v = juce::jlimit (-1.0f, 1.0f, buf[start + i] * 1.2f);
            const float px = r.getX() + r.getWidth() * (float) i / (float) (len - 1);
            const float py = r.getCentreY() - v * r.getHeight() * 0.5f;
            i == 0 ? p.startNewSubPath (px, py) : p.lineTo (px, py);
        }
        g.setColour (Theme::accent2);
        g.strokePath (p, juce::PathStrokeType (1.4f));

        g.setFont (Theme::font (10.0f));
        g.setColour (Theme::textDim);
        g.drawText ("VOICES " + juce::String (proc.activeVoices.load()), getLocalBounds().reduced (6, 2),
                    juce::Justification::bottomRight, false);
    }

private:
    void timerCallback() override { repaint(); }
    NightShiftProcessor& proc;
};
