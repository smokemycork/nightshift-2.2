#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace Theme
{
    const juce::Colour bg        { 0xff121016 };
    const juce::Colour bg2       { 0xff1a1720 };
    const juce::Colour panel     { 0xff211d28 };
    const juce::Colour panelEdge { 0xff36303f };
    const juce::Colour title     { 0xff2b2533 };
    const juce::Colour text      { 0xffe9e3d8 };
    const juce::Colour textDim   { 0xff9a92a6 };
    const juce::Colour accent    { 0xffffa94d };   // warm amber (analog)
    const juce::Colour accent2   { 0xff53d6c4 };   // teal
    const juce::Colour knobBody  { 0xff2e2836 };
    const juce::Colour track     { 0xff3d3546 };

    inline juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }
}

class NSLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NSLookAndFeel()
    {
        using namespace juce;
        setColour (ResizableWindow::backgroundColourId, Theme::bg);
        setColour (ComboBox::backgroundColourId, Theme::bg2);
        setColour (ComboBox::outlineColourId, Theme::panelEdge);
        setColour (ComboBox::textColourId, Theme::text);
        setColour (ComboBox::arrowColourId, Theme::accent);
        setColour (PopupMenu::backgroundColourId, Theme::bg2);
        setColour (PopupMenu::textColourId, Theme::text);
        setColour (PopupMenu::highlightedBackgroundColourId, Theme::accent.withAlpha (0.25f));
        setColour (PopupMenu::highlightedTextColourId, Theme::accent);
        setColour (PopupMenu::headerTextColourId, Theme::accent2);
        setColour (Label::textColourId, Theme::text);
        setColour (TextButton::buttonColourId, Theme::bg2);
        setColour (TextButton::buttonOnColourId, Theme::accent.withAlpha (0.85f));
        setColour (TextButton::textColourOffId, Theme::text);
        setColour (TextButton::textColourOnId, Theme::bg);
        setColour (ToggleButton::textColourId, Theme::text);
        setColour (Slider::textBoxTextColourId, Theme::text);
        setColour (BubbleComponent::backgroundColourId, Theme::bg2);
        setColour (BubbleComponent::outlineColourId, Theme::accent);
        setColour (TooltipWindow::backgroundColourId, Theme::bg2);
        setColour (TextEditor::backgroundColourId, Theme::bg2);
        setColour (TextEditor::textColourId, Theme::text);
        setColour (TextEditor::outlineColourId, Theme::panelEdge);
        setColour (AlertWindow::backgroundColourId, Theme::panel);
        setColour (AlertWindow::textColourId, Theme::text);
        setColour (MidiKeyboardComponent::keyDownOverlayColourId, Theme::accent.withAlpha (0.7f));
        setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::accent.withAlpha (0.3f));
        setColour (MidiKeyboardComponent::whiteNoteColourId, Colour (0xffe8e2d6));
        setColour (MidiKeyboardComponent::blackNoteColourId, Colour (0xff1a1720));
        setColour (MidiKeyboardComponent::keySeparatorLineColourId, Colour (0xff8a8296));
        setColour (MidiKeyboardComponent::shadowColourId, Colours::black.withAlpha (0.3f));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override       { return Theme::font (13.0f); }
    juce::Font getPopupMenuFont() override                        { return Theme::font (14.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return Theme::font (13.0f, true); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        using namespace juce;
        const float radius = (float) jmin (w, h) * 0.5f - 3.0f;
        if (radius < 4.0f) return;
        const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
        const float angle = startAngle + pos * (endAngle - startAngle);
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float arcR = radius - 1.5f;
        const float lineW = jmax (2.0f, radius * 0.16f);

        // track
        Path track;
        track.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (Theme::track);
        g.strokePath (track, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));

        // value arc
        const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
        Path val;
        val.addCentredArc (cx, cy, arcR, arcR, 0.0f, jmin (from, angle), jmax (from, angle), true);
        const auto col = s.isEnabled() ? Theme::accent : Theme::textDim;
        g.setColour (col);
        g.strokePath (val, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));

        // body
        const float bodyR = radius - lineW - 2.5f;
        ColourGradient grad (Theme::knobBody.brighter (0.25f), cx, cy - bodyR, Theme::knobBody.darker (0.5f), cx, cy + bodyR, false);
        g.setGradientFill (grad);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        // pointer
        Path p;
        const float pl = bodyR * 0.75f;
        p.addRoundedRectangle (-1.25f, -bodyR + 2.0f, 2.5f, pl * 0.6f, 1.0f);
        g.setColour (Theme::text);
        g.fillPath (p, AffineTransform::rotation (angle).translated (cx, cy));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                           const juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        using namespace juce;
        if (style != Slider::LinearHorizontal)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, s);
            return;
        }
        const auto r = Rectangle<float> ((float) x, (float) y + (float) h * 0.5f - 4.0f, (float) w, 8.0f);
        g.setColour (Theme::track);
        g.fillRoundedRectangle (r, 4.0f);
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float zero = bipolar ? r.getCentreX() : r.getX();
        g.setColour (sliderPos >= zero ? Theme::accent : Theme::accent2);
        g.fillRoundedRectangle (Rectangle<float> (jmin (zero, sliderPos), r.getY(), std::abs (sliderPos - zero), r.getHeight()), 4.0f);
        if (bipolar)
        {
            g.setColour (Theme::textDim);
            g.fillRect (r.getCentreX() - 0.5f, r.getY() - 3.0f, 1.0f, r.getHeight() + 6.0f);
        }
        g.setColour (Theme::text);
        g.fillEllipse (sliderPos - 6.0f, r.getCentreY() - 6.0f, 12.0f, 12.0f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool) override
    {
        using namespace juce;
        auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        const float d = jmin (14.0f, r.getHeight() - 4.0f);
        const bool hasText = b.getButtonText().isNotEmpty();
        Rectangle<float> led = hasText ? Rectangle<float> (r.getX() + 2.0f, r.getCentreY() - d * 0.5f, d, d)
                                       : Rectangle<float> (d, d).withCentre (r.getCentre());
        if (on)
        {
            g.setColour (Theme::accent.withAlpha (0.35f));
            g.fillEllipse (led.expanded (3.0f));
        }
        g.setColour (on ? Theme::accent : Theme::track.brighter (highlighted ? 0.3f : 0.0f));
        g.fillEllipse (led);
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawEllipse (led, 1.0f);

        if (hasText)
        {
            g.setColour (on ? Theme::text : Theme::textDim);
            g.setFont (Theme::font (12.0f, true));
            g.drawText (b.getButtonText(), r.withTrimmedLeft (d + 7.0f), Justification::centredLeft, false);
        }
    }
};
