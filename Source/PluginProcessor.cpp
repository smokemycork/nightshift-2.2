#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const std::vector<int>& chordIntervals (int idx)
    {
        static const std::vector<int> table[] = {
            { 0 },                 // Off
            { 0, 3, 7 },           // Minor
            { 0, 4, 7 },           // Major
            { 0, 3, 7, 10 },       // Minor 7
            { 0, 4, 7, 11 },       // Major 7
            { 0, 3, 7, 10, 14 },   // Minor 9
            { 0, 5, 7 },           // Sus 4
            { 0, 7 },              // Fifth
            { 0, 12 },             // Octave
            { 0, 7, 10, 15 },      // Dub Min7 (open voicing)
            { 0, 5, 10 },          // Stab 4ths
        };
        return table[juce::jlimit (0, (int) std::size (table) - 1, idx)];
    }
}

// ----------------------------------------------------------------------------
struct NightShiftProcessor::Cache
{
    using A = std::atomic<float>*;
    explicit Cache (juce::AudioProcessorValueTreeState& s) : apvts (s)
    {
        for (int o = 0; o < 2; ++o)
        {
            auto& O = osc[o];
            O.on = get (ParamID::osc (o, "on"));   O.wt = get (ParamID::osc (o, "wt"));
            O.pos = get (ParamID::osc (o, "pos")); O.oct = get (ParamID::osc (o, "oct"));
            O.semi = get (ParamID::osc (o, "semi")); O.fine = get (ParamID::osc (o, "fine"));
            O.uni = get (ParamID::osc (o, "uni")); O.det = get (ParamID::osc (o, "det"));
            O.blend = get (ParamID::osc (o, "blend")); O.width = get (ParamID::osc (o, "width"));
            O.warp = get (ParamID::osc (o, "warp")); O.warpAmt = get (ParamID::osc (o, "warpamt"));
            O.level = get (ParamID::osc (o, "level")); O.pan = get (ParamID::osc (o, "pan"));
            O.phase = get (ParamID::osc (o, "phase")); O.rand = get (ParamID::osc (o, "rand"));
        }
        subOn = get ("sub_on"); subWave = get ("sub_wave"); subOct = get ("sub_oct"); subLevel = get ("sub_level");
        subDirect = get ("sub_direct");
        noiseOn = get ("noise_on"); noiseType = get ("noise_type"); noiseLevel = get ("noise_level");
        fOn = get ("f_on"); fType = get ("f_type"); cutoff = get ("f_cutoff"); res = get ("f_res");
        drive = get ("f_drive"); key = get ("f_key"); envAmt = get ("f_env");
        for (int e = 0; e < 3; ++e)
        {
            env[e][0] = get (ParamID::env (e, "a")); env[e][1] = get (ParamID::env (e, "d"));
            env[e][2] = get (ParamID::env (e, "s")); env[e][3] = get (ParamID::env (e, "r"));
        }
        for (int l = 0; l < 2; ++l)
        {
            lfo[l].shape = get (ParamID::lfo (l, "shape")); lfo[l].rate = get (ParamID::lfo (l, "rate"));
            lfo[l].sync = get (ParamID::lfo (l, "sync")); lfo[l].div = get (ParamID::lfo (l, "div"));
            lfo[l].mode = get (ParamID::lfo (l, "mode")); lfo[l].delay = get (ParamID::lfo (l, "delay"));
        }
        for (int m = 0; m < kNumModSlots; ++m)
        {
            slot[m][0] = get (ParamID::mod (m, "src")); slot[m][1] = get (ParamID::mod (m, "dst"));
            slot[m][2] = get (ParamID::mod (m, "amt"));
        }
        for (int m = 0; m < 4; ++m) macro[m] = get (ParamID::macro (m));
        voices = get ("voices"); mode = get ("mode"); chord = get ("chord"); glide = get ("glide");
        bend = get ("bend"); analog = get ("analog"); vel = get ("vel"); master = get ("master");

        distOn = get ("dist_on"); distType = get ("dist_type"); distDrive = get ("dist_drive");
        distTone = get ("dist_tone"); distMix = get ("dist_mix");
        eqOn = get ("eq_on"); eqLow = get ("eq_low"); eqMid = get ("eq_mid"); eqMidF = get ("eq_midf"); eqHigh = get ("eq_high");
        choOn = get ("cho_on"); choRate = get ("cho_rate"); choDepth = get ("cho_depth"); choMix = get ("cho_mix");
        phaOn = get ("pha_on"); phaRate = get ("pha_rate"); phaDepth = get ("pha_depth"); phaFb = get ("pha_fb"); phaMix = get ("pha_mix");
        dlyOn = get ("dly_on"); dlySync = get ("dly_sync"); dlyDiv = get ("dly_div"); dlyTime = get ("dly_time");
        dlyFb = get ("dly_fb"); dlyPP = get ("dly_pp"); dlyTone = get ("dly_tone"); dlyMix = get ("dly_mix");
        revOn = get ("rev_on"); revSize = get ("rev_size"); revDamp = get ("rev_damp"); revWidth = get ("rev_width"); revMix = get ("rev_mix");
        compOn = get ("comp_on"); compThresh = get ("comp_thresh"); compRatio = get ("comp_ratio");
        compAtt = get ("comp_att"); compRel = get ("comp_rel"); compGain = get ("comp_gain");
        pumpOn = get ("pump_on"); pumpDiv = get ("pump_div"); pumpDepth = get ("pump_depth"); pumpCurve = get ("pump_curve");
    }

    A get (const juce::String& id)
    {
        auto* p = apvts.getRawParameterValue (id);
        jassert (p != nullptr);   // every id must exist in the layout
        return p;
    }

    juce::AudioProcessorValueTreeState& apvts;
    struct Osc { A on, wt, pos, oct, semi, fine, uni, det, blend, width, warp, warpAmt, level, pan, phase, rand; } osc[2];
    A subOn, subWave, subOct, subLevel, subDirect, noiseOn, noiseType, noiseLevel;
    A fOn, fType, cutoff, res, drive, key, envAmt;
    A env[3][4];
    struct Lfo { A shape, rate, sync, div, mode, delay; } lfo[2];
    A slot[kNumModSlots][3];
    A macro[4];
    A voices, mode, chord, glide, bend, analog, vel, master;
    A distOn, distType, distDrive, distTone, distMix, eqOn, eqLow, eqMid, eqMidF, eqHigh;
    A choOn, choRate, choDepth, choMix, phaOn, phaRate, phaDepth, phaFb, phaMix;
    A dlyOn, dlySync, dlyDiv, dlyTime, dlyFb, dlyPP, dlyTone, dlyMix;
    A revOn, revSize, revDamp, revWidth, revMix;
    A compOn, compThresh, compRatio, compAtt, compRel, compGain;
    A pumpOn, pumpDiv, pumpDepth, pumpCurve;
};

// ----------------------------------------------------------------------------
NightShiftProcessor::NightShiftProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NightShiftState", createParameterLayout())
{
    cache = std::make_unique<Cache> (apvts);
    presets.setCurrentLabel ("Init", "Init");
    readParams();
}

NightShiftProcessor::~NightShiftProcessor() = default;

bool NightShiftProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NightShiftProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    gs.sampleRate = sampleRate;
    gs.bank = &bank.get();
    for (int i = 0; i < kMaxVoices; ++i)
        voices[(size_t) i].prepare (sampleRate, 0x1234567u + (uint32_t) i * 2654435761u);
    effects.prepare (sampleRate, juce::jmax (1, samplesPerBlock));
    masterGain.reset (sampleRate, 0.03);
    masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (cache->master->load()));
    monoStack.clear();
    sustained.fill (false);
    lastPitch = -1.0f;
}

void NightShiftProcessor::readParams()
{
    auto& c = *cache;
    auto F = [] (std::atomic<float>* a) { return a->load (std::memory_order_relaxed); };
    auto B = [] (std::atomic<float>* a) { return a->load (std::memory_order_relaxed) > 0.5f; };
    auto I = [] (std::atomic<float>* a) { return (int) std::lround (a->load (std::memory_order_relaxed)); };

    for (int o = 0; o < 2; ++o)
    {
        auto& O = sp.osc[o]; auto& C = c.osc[o];
        O.on = B (C.on); O.wt = I (C.wt); O.pos = F (C.pos); O.oct = I (C.oct); O.semi = I (C.semi); O.fine = F (C.fine);
        O.uni = I (C.uni); O.det = F (C.det); O.blend = F (C.blend); O.width = F (C.width); O.warp = I (C.warp);
        O.warpAmt = F (C.warpAmt); O.level = F (C.level); O.pan = F (C.pan); O.phase = F (C.phase); O.rand = F (C.rand);
    }
    sp.subOn = B (c.subOn); sp.subWave = I (c.subWave); sp.subOct = I (c.subOct); sp.subLevel = F (c.subLevel);
    sp.subDirect = B (c.subDirect);
    sp.noiseOn = B (c.noiseOn); sp.noiseType = I (c.noiseType); sp.noiseLevel = F (c.noiseLevel);
    sp.fOn = B (c.fOn); sp.fType = I (c.fType); sp.cutoff = F (c.cutoff); sp.res = F (c.res); sp.drive = F (c.drive);
    sp.key = F (c.key); sp.envAmt = F (c.envAmt);
    for (int e = 0; e < 3; ++e)
        sp.env[e] = { F (c.env[e][0]), F (c.env[e][1]), F (c.env[e][2]), F (c.env[e][3]) };
    for (int l = 0; l < 2; ++l)
    {
        sp.lfo[l].shape = I (c.lfo[l].shape); sp.lfo[l].rate = F (c.lfo[l].rate); sp.lfo[l].sync = B (c.lfo[l].sync);
        sp.lfo[l].div = I (c.lfo[l].div); sp.lfo[l].mode = I (c.lfo[l].mode); sp.lfo[l].delay = F (c.lfo[l].delay);
    }
    for (int m = 0; m < kNumModSlots; ++m)
        sp.mod[m] = { I (c.slot[m][0]), I (c.slot[m][1]), F (c.slot[m][2]) };
    for (int m = 0; m < 4; ++m) sp.macro[m] = F (c.macro[m]);
    sp.voices = I (c.voices); sp.mode = I (c.mode); sp.chord = I (c.chord); sp.glide = F (c.glide);
    sp.bend = I (c.bend); sp.analog = F (c.analog); sp.velSens = F (c.vel);

    fx.distOn = B (c.distOn); fx.distType = I (c.distType); fx.distDrive = F (c.distDrive); fx.distTone = F (c.distTone);
    fx.distMix = F (c.distMix);
    fx.eqOn = B (c.eqOn); fx.eqLow = F (c.eqLow); fx.eqMid = F (c.eqMid); fx.eqMidF = F (c.eqMidF); fx.eqHigh = F (c.eqHigh);
    fx.choOn = B (c.choOn); fx.choRate = F (c.choRate); fx.choDepth = F (c.choDepth); fx.choMix = F (c.choMix);
    fx.phaOn = B (c.phaOn); fx.phaRate = F (c.phaRate); fx.phaDepth = F (c.phaDepth); fx.phaFb = F (c.phaFb); fx.phaMix = F (c.phaMix);
    fx.dlyOn = B (c.dlyOn); fx.dlySync = B (c.dlySync); fx.dlyDiv = I (c.dlyDiv); fx.dlyTime = F (c.dlyTime);
    fx.dlyFb = F (c.dlyFb); fx.dlyPP = B (c.dlyPP); fx.dlyTone = F (c.dlyTone); fx.dlyMix = F (c.dlyMix);
    fx.revOn = B (c.revOn); fx.revSize = F (c.revSize); fx.revDamp = F (c.revDamp); fx.revWidth = F (c.revWidth); fx.revMix = F (c.revMix);
    fx.compOn = B (c.compOn); fx.compThresh = F (c.compThresh); fx.compRatio = F (c.compRatio); fx.compAtt = F (c.compAtt);
    fx.compRel = F (c.compRel); fx.compGain = F (c.compGain);
    fx.pumpOn = B (c.pumpOn); fx.pumpDiv = I (c.pumpDiv); fx.pumpDepth = F (c.pumpDepth); fx.pumpCurve = F (c.pumpCurve);
}

// ----------------------------------------------------------------------------
SynthVoice* NightShiftProcessor::findVoice()
{
    const int limit = juce::jlimit (1, kMaxVoices, sp.voices);
    int activeCount = 0;
    for (auto& v : voices) if (v.isActive()) ++activeCount;

    if (activeCount < limit)
        for (auto& v : voices)
            if (! v.isActive())
                return &v;

    // steal: quietest released voice, otherwise the oldest note
    SynthVoice* best = nullptr;
    for (auto& v : voices)
        if (v.isActive() && ! v.isGateOn() && (best == nullptr || v.getLevel() < best->getLevel()))
            best = &v;
    if (best != nullptr) return best;

    for (auto& v : voices)
        if (v.isActive() && (best == nullptr || v.getOrder() < best->getOrder()))
            best = &v;
    return best != nullptr ? best : &voices[0];
}

void NightShiftProcessor::noteOn (int note, float velocity)
{
    lastVelocity = velocity;
    sustained[(size_t) note] = false;

    if (sp.mode == 0)   // Poly (with optional chord memory)
    {
        for (int interval : chordIntervals (sp.chord))
        {
            const float pitch = (float) (note + interval);
            if (pitch > 127.0f) continue;
            const float from = lastPitch >= 0.0f ? lastPitch + (float) interval : -1.0f;
            findVoice()->noteOn (note, pitch, velocity, from, false, sp, gs, ++noteCounter);
        }
    }
    else
    {
        monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
        monoStack.push_back (note);
        auto& v = voices[0];
        const bool legato = sp.mode == 2 && v.isActive() && v.isGateOn();
        v.noteOn (note, (float) note, velocity, lastPitch, legato, sp, gs, ++noteCounter);
    }
    lastPitch = (float) note;
}

void NightShiftProcessor::noteOff (int note)
{
    if (sustain) { sustained[(size_t) note] = true; return; }

    if (sp.mode == 0)
    {
        for (auto& v : voices)
            if (v.isActive() && v.isGateOn() && v.getTriggerNote() == note)
                v.noteOff();
        return;
    }

    monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
    auto& v = voices[0];
    if (v.getTriggerNote() != note || ! v.isGateOn())
        return;

    if (! monoStack.empty())
    {
        const int back = monoStack.back();
        v.noteOn (back, (float) back, lastVelocity, lastPitch, sp.mode == 2, sp, gs, ++noteCounter);
        lastPitch = (float) back;
    }
    else
        v.noteOff();
}

void NightShiftProcessor::allNotesOff (bool hard)
{
    for (auto& v : voices)
        hard ? v.kill() : v.noteOff();
    monoStack.clear();
    sustained.fill (false);
    sustain = false;
}

void NightShiftProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())
        bendNorm = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isChannelPressure())
        gs.aftertouch = (float) m.getChannelPressureValue() / 127.0f;
    else if (m.isAftertouch())
        gs.aftertouch = (float) m.getAfterTouchValue() / 127.0f;
    else if (m.isController())
    {
        const int cc = m.getControllerNumber(), val = m.getControllerValue();
        if (cc == 1) gs.modWheel = (float) val / 127.0f;
        else if (cc == 64)
        {
            const bool down = val >= 64;
            if (sustain && ! down)
            {
                sustain = false;
                for (int n = 0; n < 128; ++n)
                    if (sustained[(size_t) n]) { sustained[(size_t) n] = false; noteOff (n); }
            }
            sustain = down;
        }
        else if (cc == 120) allNotesOff (true);
        else if (cc == 123) allNotesOff (false);
    }
    else if (m.isAllNotesOff()) allNotesOff (false);
    else if (m.isAllSoundOff()) allNotesOff (true);

    gs.pitchBendSemis = bendNorm * (float) sp.bend;
}

// ----------------------------------------------------------------------------
void NightShiftProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n == 0 || buffer.getNumChannels() < 2)
        return;

    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    readParams();

    // voice-mode switch -> clean slate
    if (sp.mode != lastVoiceMode)
    {
        if (lastVoiceMode != -1) allNotesOff (true);
        lastVoiceMode = sp.mode;
    }

    TransportInfo transport;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) transport.bpm = juce::jlimit (20.0, 999.0, *bpm);
            transport.playing = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition()) transport.ppq = *ppq; else transport.playing = false;
        }
    gs.bpm = transport.bpm;
    gs.bank = &bank.get();
    gs.pitchBendSemis = bendNorm * (float) sp.bend;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    auto renderRange = [&] (int start, int end)
    {
        if (end <= start) return;
        for (auto& v : voices)
            if (v.isActive())
                v.render (L + start, R + start, end - start, sp, gs);
    };

    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, n, meta.samplePosition);
        renderRange (pos, t);
        handleMidi (meta.getMessage());
        pos = t;
    }
    renderRange (pos, n);
    midi.clear();

    // free-running LFO phase (used to seed voices in "Free" mode)
    for (int l = 0; l < 2; ++l)
    {
        const double rate = sp.lfo[l].sync ? (gs.bpm / 60.0) / divisionToBeats (sp.lfo[l].div) : (double) sp.lfo[l].rate;
        gs.lfoPhase[l] += rate * n / gs.sampleRate;
        gs.lfoPhase[l] -= std::floor (gs.lfoPhase[l]);
    }

    buffer.applyGain (0.5f);
    effects.process (buffer, fx, transport);

    masterGain.setTargetValue (juce::Decibels::decibelsToGain (cache->master->load()));
    bool bad = false;
    int w = scopeWrite.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        const float g = masterGain.getNextValue();
        float l = L[i] * g, r = R[i] * g;
        if (! std::isfinite (l) || ! std::isfinite (r)) { bad = true; l = r = 0.0f; }
        L[i] = juce::jlimit (-2.0f, 2.0f, l);
        R[i] = juce::jlimit (-2.0f, 2.0f, r);
        scope[(size_t) w].store ((L[i] + R[i]) * 0.5f, std::memory_order_relaxed);
        w = (w + 1) & (kScopeSize - 1);
    }
    scopeWrite.store (w, std::memory_order_relaxed);

    if (bad)   // recover from any numerical blow-up
    {
        buffer.clear();
        for (auto& v : voices) v.kill();
        effects.reset();
    }

    int count = 0;
    for (auto& v : voices) if (v.isActive()) ++count;
    activeVoices.store (count, std::memory_order_relaxed);
}

// ----------------------------------------------------------------------------
int NightShiftProcessor::getNumPrograms()          { return juce::jmax (1, (int) getFactoryPresets().size()); }
int NightShiftProcessor::getCurrentProgram()       { return juce::jmax (0, presets.getCurrentFactoryIndex()); }
void NightShiftProcessor::setCurrentProgram (int i) { presets.loadFactory (i); }

const juce::String NightShiftProcessor::getProgramName (int i)
{
    const auto& list = getFactoryPresets();
    if (! juce::isPositiveAndBelow (i, (int) list.size())) return {};
    return list[(size_t) i].folder + " - " + list[(size_t) i].name;
}

void NightShiftProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetFolder", presets.getCurrentFolder(), nullptr);
    state.setProperty ("presetName", presets.getCurrentName(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void NightShiftProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType (apvts.state.getType()))
        {
            apvts.replaceState (tree);
            // Snap every parameter to a legal value and re-announce it.  Switch/choice
            // parameters can hold an un-snapped raw value set by the host, which APVTS
            // would otherwise leave untouched when the restored value is "equal".
            for (auto* prm : getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
                    rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
            presets.setCurrentLabel (tree.getProperty ("presetFolder", "Init").toString(),
                                     tree.getProperty ("presetName", "Init").toString());
        }
    }
}

juce::AudioProcessorEditor* NightShiftProcessor::createEditor() { return new NightShiftEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NightShiftProcessor(); }
