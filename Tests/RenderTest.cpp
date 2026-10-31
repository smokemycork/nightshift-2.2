// Headless test: validates the factory bank and renders every preset, checking for
// NaNs, silence, clipping and CPU cost.  Exit code != 0 on failure.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <chrono>
#include <iostream>

struct Result { float peak = 0, rms = 0, maxWindowRms = 0; bool nan = false; double rtFactor = 0; };

static Result renderPreset (NightShiftProcessor& p, double sr, int block, bool monoStyle, int chordMode,
                            juce::AudioBuffer<float>* capture = nullptr, bool longNote = false)
{
    Result res;
    const double seconds = longNote ? 10.0 : 3.0;
    const int total = (int) (seconds * sr);
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    double sumSq = 0.0; long count = 0;
    std::vector<float> sq; sq.reserve ((size_t) total);

    struct Ev { int sample; juce::MidiMessage msg; };
    std::vector<Ev> events;
    auto on  = [&] (double t, int n, float v) { events.push_back ({ (int) (t * sr), juce::MidiMessage::noteOn (1, n, v) }); };
    auto off = [&] (double t, int n)          { events.push_back ({ (int) (t * sr), juce::MidiMessage::noteOff (1, n) }); };

    if (longNote)
    {   on (0.0, 60, 0.9f); off (8.5, 60); }
    else if (monoStyle)
    {   // bass line: C1 G1 (overlapping -> legato) A#1, then release
        on (0.0, 36, 0.9f); on (0.45, 43, 0.7f); off (0.5, 36); on (0.9, 46, 1.0f); off (0.95, 43); off (1.6, 46);
    }
    else if (chordMode != 0)
    {   on (0.0, 57, 0.9f); off (0.4, 57); on (0.5, 60, 0.8f); off (1.6, 60); }
    else
    {   on (0.0, 60, 0.9f); on (0.0, 64, 0.8f); on (0.0, 67, 0.85f); on (0.0, 71, 0.7f);
        off (1.6, 60); off (1.6, 64); off (1.6, 67); off (1.6, 71); }

    const auto t0 = std::chrono::high_resolution_clock::now();
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        buf.setSize (2, n, false, false, true);
        midi.clear();
        for (auto& e : events)
            if (e.sample >= pos && e.sample < pos + n)
                midi.addEvent (e.msg, e.sample - pos);
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
        {
            const float* d = buf.getReadPointer (c);
            for (int i = 0; i < n; ++i)
            {
                if (! std::isfinite (d[i])) res.nan = true;
                res.peak = std::max (res.peak, std::abs (d[i]));
                if (pos + i < (int) (1.6 * sr)) { sumSq += (double) d[i] * d[i]; ++count; }
            }
        }
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (buf.getSample (0, i) + buf.getSample (1, i));
            sq.push_back (m * m);
        }
        if (capture != nullptr)
            for (int c = 0; c < 2; ++c)
                capture->copyFrom (c, pos, buf, c, 0, n);
    }
    const auto t1 = std::chrono::high_resolution_clock::now();
    const double cpu = std::chrono::duration<double> (t1 - t0).count();
    res.rtFactor = seconds / cpu;
    res.rms = (float) std::sqrt (sumSq / std::max (1L, count));
    // loudest 300 ms window (short-term loudness proxy)
    const int win = (int) (0.3 * sr);
    double acc = 0.0, best = 0.0;
    for (size_t i = 0; i < sq.size(); ++i)
    {
        acc += sq[i];
        if (i >= (size_t) win) acc -= sq[i - (size_t) win];
        best = std::max (best, acc);
    }
    res.maxWindowRms = (float) std::sqrt (best / win);
    return res;
}

static void silence (NightShiftProcessor& p, int block)
{
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::allSoundOff (1), 0);
    juce::AudioBuffer<float> buf (2, block);
    p.processBlock (buf, midi);
    midi.clear();
    for (int i = 0; i < 40; ++i) p.processBlock (buf, midi);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const bool writeWavs = argc > 1 && juce::String (argv[1]) == "--wav";

    if (argc > 1 && juce::String (argv[1]) == "--snapshot")
    {
        NightShiftProcessor p;
        p.prepareToPlay (48000.0, 256);
        p.presets.loadFactory (argc > 2 ? juce::String (argv[2]).getIntValue() : 1);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (NightShiftEditor::kDesignW, NightShiftEditor::kDesignH);
        // play a chord so the scope shows something
        juce::AudioBuffer<float> buf (2, 256); juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 48, 0.9f), 0);
        for (int i = 0; i < 20; ++i) { p.processBlock (buf, midi); midi.clear(); }
        auto findButtons = [&] (juce::Component* root) {
            juce::Array<juce::TextButton*> tabs;
            std::function<void (juce::Component*)> walk = [&] (juce::Component* c) {
                if (auto* b = dynamic_cast<juce::TextButton*> (c)) if (b->getRadioGroupId() == 101) tabs.add (b);
                for (auto* ch : c->getChildren()) walk (ch);
            };
            walk (root);
            return tabs;
        };
        {   // print the preset browser contents (type folder + list) for checking labels
            juce::Array<juce::ComboBox*> boxes;
            std::function<void (juce::Component*)> walkC = [&] (juce::Component* c) {
                if (auto* cb = dynamic_cast<juce::ComboBox*> (c)) boxes.add (cb);
                for (auto* ch : c->getChildren()) walkC (ch);
            };
            walkC (ed.get());
            for (auto* cb : boxes)
                if (cb->getNumItems() > 20 || cb->getTooltip().startsWith ("Sound type") || cb->getTooltip().startsWith ("Optional"))
                {
                    std::cout << "BOX [" << cb->getText() << "] items=" << cb->getNumItems() << "\n";
                    for (int k = 0; k < juce::jmin (cb->getNumItems(), 400); ++k)
                        std::cout << "   " << cb->getItemText (k) << "\n";
                }
        }
        auto tabs = findButtons (ed.get());
        for (int t = 0; t < tabs.size(); ++t)
        {
            tabs[t]->setToggleState (true, juce::dontSendNotification);
            if (tabs[t]->onClick) tabs[t]->onClick();
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::File f (juce::File::getCurrentWorkingDirectory().getChildFile ("ui_page" + juce::String (t) + ".png"));
            f.deleteFile();
            juce::FileOutputStream os (f);
            juce::PNGImageFormat png;
            png.writeImageToStream (img, os);
            std::cout << "wrote " << f.getFullPathName() << "\n";
        }
        return 0;
    }
    int failures = 0;

    auto proc = std::make_unique<NightShiftProcessor>();

    // ---------------- static validation ----------------
    const auto errors = proc->presets.validateFactoryPresets();
    for (auto& e : errors) std::cout << "PRESET ERROR: " << e << "\n";
    failures += errors.size();
    std::cout << "Parameters: " << proc->getParameters().size()
              << "   Factory presets: " << getFactoryPresets().size() << "\n";

    // ---------------- render every preset ----------------
    const double sr = 48000.0;
    const int block = 256;
    proc->setPlayConfigDetails (0, 2, sr, block);
    proc->prepareToPlay (sr, block);

    const auto& bank = getFactoryPresets();
    double worstRt = 1e9;
    std::printf ("%-20s %-24s %8s %8s %8s %8s\n", "FOLDER", "PRESET", "PEAK", "RMS dB", "LOUD dB", "xRT");
    juce::StringArray calib;
    for (int i = 0; i < (int) bank.size(); ++i)
    {
        proc->presets.loadFactory (i);
        silence (*proc, block);
        const int mode = (int) proc->apvts.getRawParameterValue ("mode")->load();
        const int chord = (int) proc->apvts.getRawParameterValue ("chord")->load();
        const bool isRiser = bank[(size_t) i].name.contains ("Riser");
        juce::AudioBuffer<float> cap (2, (int) ((isRiser ? 10.0 : 3.0) * sr));
        cap.clear();
        auto r = renderPreset (*proc, sr, block, mode != 0, chord, writeWavs ? &cap : nullptr, isRiser);
        const float rmsDb = juce::Decibels::gainToDecibels (r.rms, -120.0f);
        const float loudDb = juce::Decibels::gainToDecibels (r.maxWindowRms, -120.0f);
        {
            const float master = proc->apvts.getRawParameterValue ("master")->load();
            float trim = -15.0f - loudDb;                                              // target short-term loudness
            trim = std::min (trim, juce::Decibels::gainToDecibels (0.89f / std::max (r.peak, 1.0e-6f)));  // keep peaks < -1 dBFS
            const float suggested = juce::jlimit (-36.0f, 6.0f, master + trim);
            calib.add ("{ \"" + bank[(size_t) i].folder + "\", \"" + bank[(size_t) i].name + "\", " + juce::String (suggested, 1) + "f },");
        }
        std::printf ("%-20s %-24s %8.3f %8.1f %8.1f %8.0f%s\n", bank[(size_t) i].folder.toRawUTF8(), bank[(size_t) i].name.toRawUTF8(),
                     r.peak, rmsDb, loudDb, r.rtFactor,
                     r.nan ? "  <-- NaN!" : (r.rms < 1.0e-4f ? "  <-- SILENT!" : (r.peak > 1.5f ? "  <-- HOT" : "")));
        if (r.nan || r.rms < 1.0e-4f) ++failures;
        worstRt = std::min (worstRt, r.rtFactor);

        if (writeWavs)
        {
            auto dir = juce::File::getCurrentWorkingDirectory().getChildFile ("renders");
            dir.createDirectory();
            auto f = dir.getChildFile (bank[(size_t) i].folder + " - " + bank[(size_t) i].name + ".wav");
            f.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
            if (w) w->writeFromAudioSampleBuffer (cap, 0, cap.getNumSamples());
        }
    }
    std::printf ("Worst-case real-time factor: %.0fx\n", worstRt);
    if (argc > 1 && juce::String (argv[1]) == "--calibrate")
        for (auto& c : calib) std::cout << "CALIB " << c << "\n";

    // ---------------- other sample rates / block sizes ----------------
    for (double rate : { 44100.0, 96000.0, 192000.0 })
        for (int bs : { 1, 64, 1024 })
        {
            proc->prepareToPlay (rate, bs);
            for (int i : { 1, 12, 23, 50, (int) bank.size() - 1 })
            {
                if (i >= (int) bank.size()) continue;
                proc->presets.loadFactory (i);
                silence (*proc, bs);
                auto r = renderPreset (*proc, rate, bs, false, 0);
                if (r.nan || r.rms < 1.0e-5f)
                {
                    std::printf ("FAIL at %.0f Hz / block %d preset %d (nan=%d rms=%g)\n", rate, bs, i, (int) r.nan, r.rms);
                    ++failures;
                }
            }
        }
    std::printf ("Sample-rate / block-size sweep done.\n");

    // ---------------- stress: 16 voices x 8 unison x 2 osc ----------------
    {
        proc->prepareToPlay (48000.0, 512);
        proc->presets.loadFactory (0);
        auto set = [&] (const char* id, float v) { auto* prm = proc->apvts.getParameter (id); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); };
        set ("voices", 16); set ("a_uni", 8); set ("b_on", 1); set ("b_uni", 8); set ("chord", 5);
        set ("dly_on", 1); set ("rev_on", 1); set ("cho_on", 1); set ("pha_on", 1); set ("dist_on", 1); set ("comp_on", 1);
        set ("f_type", 3); set ("f_res", 1.0f); set ("f_drive", 1.0f);
        auto r = renderPreset (*proc, 48000.0, 512, false, 0);
        std::printf ("Stress test (16 voices, 2x8 unison, all FX): peak %.2f  xRT %.1f %s\n", r.peak, r.rtFactor, r.nan ? "NaN!" : "");
        if (r.nan) ++failures;
    }

    // ---------------- extreme parameter fuzz ----------------
    {
        juce::Random rng (1234);
        proc->prepareToPlay (48000.0, 256);
        for (int trial = 0; trial < 60; ++trial)
        {
            for (auto* prm : proc->getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
                    if (rp->getParameterID() != "master")
                        rp->setValueNotifyingHost (rng.nextFloat());
            auto* m = proc->apvts.getParameter ("master"); m->setValueNotifyingHost (m->convertTo0to1 (-6.0f));
            auto r = renderPreset (*proc, 48000.0, 256, trial % 2 == 0, 0);
            if (r.nan) { std::printf ("Fuzz trial %d produced NaN\n", trial); ++failures; }
            silence (*proc, 256);
        }
        std::printf ("Random-parameter fuzz: 60 trials done.\n");
    }

    // ---------------- state round trip ----------------
    {
        proc->presets.loadFactory (10);
        juce::MemoryBlock mb;
        proc->getStateInformation (mb);
        const float before = proc->apvts.getRawParameterValue ("f_cutoff")->load();
        proc->presets.loadFactory (0);
        proc->setStateInformation (mb.getData(), (int) mb.getSize());
        const float after = proc->apvts.getRawParameterValue ("f_cutoff")->load();
        const bool ok = std::abs (before - after) < 0.01f && proc->presets.getCurrentName() == bank[10].name;
        std::printf ("State save/restore: %s\n", ok ? "OK" : "FAILED");
        if (! ok) ++failures;
    }

    std::printf ("\n%s (%d failures)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED", failures);
    return failures == 0 ? 0 : 1;
}
