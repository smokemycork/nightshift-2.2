#include "Presets.h"

// ============================================================================
//  Factory bank.  Each preset starts from the Init patch and overrides values.
//  Values are plain units (Hz, seconds, choice index, 0/1 for switches).
//
//  Quick index reference
//   Wavetables: 0 Analog Classic (0=sine .25=tri .5=saw .75=square 1=pulse) 1 PWM 2 Warm Saw 3 Formant
//               4 Drawbar Organ 5 Hollow Saw 6 Reso Sweep 7 FM Tines 8 Wavefold 9 Sync Sweep
//               10 Digital Grit 11 Additive Build
//   Warp: 0 Off 1 Sync 2 Bend 3 Mirror 4 FM
//   Filter: 0 Ladder24 1 Ladder12 2 LP12 3 LP24 4 HP12 5 HP24 6 BP 7 Notch
//   Mod src: 1 Env2 2 Env3 3 LFO1 4 LFO2 5 Vel 6 MW 7 AT 8 Key 9 Rnd 10-13 Macro1-4 14 AmpEnv
//   Mod dst: 1 PitchA 2 PitchB 3 PitchAll 4 PosA 5 PosB 6 LvlA 7 LvlB 8 DetA 9 DetB 10 WarpA 11 WarpB
//            12 Cutoff 13 Res 14 Drive 15 Sub 16 Noise 17 Amp 18 Pan 19 LFO1Rate 20 LFO2Rate
//   Divisions: 0 1/32 2 1/16 3 1/16D 4 1/8T 5 1/8 6 1/8D 7 1/4T 8 1/4 9 1/4D 10 1/2 11 1bar 12 2bar 13 4bar
//   Chord: 1 Min 2 Maj 3 Min7 4 Maj7 5 Min9 6 Sus4 7 Fifth 8 Oct 9 DubMin7 10 Stab4ths
//   Mode: 0 Poly 1 Mono 2 Legato     Dist: 0 Soft 1 Hard 2 Tube 3 Fold 4 Crush
// ============================================================================

namespace
{
    using V = std::vector<std::pair<juce::String, float>>;

    // Common building blocks --------------------------------------------------
    V monoBass (float glide = 0.03f) { return { { "mode", 1 }, { "glide", glide }, { "vel", 0.3f }, { "e1_a", 0.001f }, { "e1_r", 0.07f } }; }
    V legatoBass (float glide = 0.05f) { return { { "mode", 2 }, { "glide", glide }, { "vel", 0.3f }, { "e1_a", 0.001f }, { "e1_r", 0.07f } }; }
    V subOn (float lvl = 0.55f) { return { { "sub_on", 1 }, { "sub_wave", 0 }, { "sub_oct", 0 }, { "sub_level", lvl }, { "sub_direct", 1 } }; }
    V punch (int slot, float amt, float decay)
    {
        const juce::String s = "m" + juce::String (slot);
        return { { s + "_src", 2 }, { s + "_dst", 3 }, { s + "_amt", amt }, { "e3_a", 0.0005f }, { "e3_d", decay }, { "e3_s", 0.0f } };
    }
    V mod (int slot, int src, int dst, float amt)
    {
        const juce::String s = "m" + juce::String (slot);
        return { { s + "_src", (float) src }, { s + "_dst", (float) dst }, { s + "_amt", amt } };
    }
    V delay (int div, float fb, float mix, float tone = 0.55f, bool pp = true)
    {
        return { { "dly_on", 1 }, { "dly_sync", 1 }, { "dly_div", (float) div }, { "dly_fb", fb }, { "dly_mix", mix },
                 { "dly_tone", tone }, { "dly_pp", pp ? 1.0f : 0.0f } };
    }
    V reverb (float size, float mix, float damp = 0.5f) { return { { "rev_on", 1 }, { "rev_size", size }, { "rev_mix", mix }, { "rev_damp", damp } }; }
    V chorus (float mix = 0.35f, float depth = 0.35f, float rate = 0.6f) { return { { "cho_on", 1 }, { "cho_mix", mix }, { "cho_depth", depth }, { "cho_rate", rate } }; }
    V dist (int type, float drive, float mix, float tone = 0.7f) { return { { "dist_on", 1 }, { "dist_type", (float) type }, { "dist_drive", drive }, { "dist_mix", mix }, { "dist_tone", tone } }; }
    V vibrato (int slot, float amt = 0.006f, float rise = 0.45f)
    {
        auto v = mod (slot, 4, 3, amt);
        v.insert (v.end(), { { "l2_shape", 0 }, { "l2_sync", 0 }, { "l2_rate", 5.5f }, { "l2_mode", 1 }, { "l2_delay", rise } });
        return v;
    }
    V comp (float thresh = -18.0f, float ratio = 4.0f, float makeup = 3.0f)
    {
        return { { "comp_on", 1 }, { "comp_thresh", thresh }, { "comp_ratio", ratio }, { "comp_gain", makeup } };
    }

    V join (std::initializer_list<V> parts)
    {
        V out;
        for (auto& p : parts)
            for (auto& kv : p)
            {
                // later values win (allows overriding building blocks)
                auto it = std::find_if (out.begin(), out.end(), [&] (auto& e) { return e.first == kv.first; });
                if (it != out.end()) it->second = kv.second; else out.push_back (kv);
            }
        return out;
    }

    // =========================================================================
    //  Sound archetypes - reusable recipes. Convention for these presets:
    //  mod slot 2 = Macro 1 -> Cutoff ("TONE" macro), slots 4-8 left free.
    // =========================================================================
    V macroTone (float amt = 0.45f) { return mod (2, 10, 12, amt); }

    // ---------------- BASS ----------------
    V bRubber (float cut, float res, float dec, float pos = 0.5f, int wt = 0, float sub = 0.5f)
    {
        return join ({ monoBass (0.03f), subOn (sub), punch (1, 0.1f, 0.02f), macroTone(), {
            { "a_wt", (float) wt }, { "a_pos", pos }, { "a_rand", 0 }, { "f_type", 0 }, { "f_cutoff", cut }, { "f_res", res },
            { "f_drive", 0.4f }, { "f_env", 0.55f }, { "e2_a", 0.001f }, { "e2_d", dec }, { "e2_s", 0.05f }, { "e2_r", 0.08f },
            { "e1_d", 0.3f }, { "e1_s", 0.85f } } });
    }
    V bReese (float det, float cut, int lfoDiv, float drive = 0.3f)
    {
        return join ({ legatoBass (0.05f), subOn (0.5f), mod (1, 3, 12, 0.12f), macroTone(), dist (2, drive, 0.5f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 2 }, { "a_det", det }, { "a_blend", 1.0f }, { "a_width", 0.3f },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_uni", 2 }, { "b_det", juce::jmin (1.0f, det + 0.15f) },
            { "b_oct", -1 }, { "b_level", 0.55f }, { "b_width", 0.3f }, { "f_cutoff", cut }, { "f_res", 0.25f },
            { "f_drive", 0.4f }, { "l1_div", (float) lfoDiv }, { "l1_mode", 0 }, { "e1_s", 1.0f } } });
    }
    V bAcid (float cut, float res, float env, float glide, float pos = 0.5f)
    {
        return join ({ legatoBass (glide), mod (3, 5, 12, 0.3f), mod (2, 10, 12, 0.5f), dist (2, 0.4f, 0.6f), {
            { "a_wt", 0 }, { "a_pos", pos }, { "a_rand", 0 }, { "f_cutoff", cut }, { "f_res", res }, { "f_drive", 0.5f },
            { "f_env", env }, { "e2_d", 0.2f }, { "e2_s", 0 }, { "e1_d", 0.25f }, { "e1_s", 0.7f }, { "e1_r", 0.05f },
            { "vel", 0.4f } } });
    }
    V bFM (float amt, float dec, int bSemi = 0, int bOct = 0)
    {
        return join ({ monoBass (0.02f), subOn (0.45f), mod (3, 1, 10, 0.4f), macroTone (0.4f), {
            { "a_wt", 0 }, { "a_pos", 0.0f }, { "a_rand", 0 }, { "a_warp", 4 }, { "a_warpamt", amt },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.0f }, { "b_level", 0.0f }, { "b_rand", 0 },
            { "b_semi", (float) bSemi }, { "b_oct", (float) bOct }, { "e2_d", dec }, { "e2_s", 0 },
            { "f_cutoff", 2000 }, { "f_res", 0.1f }, { "e1_s", 0.8f }, { "e1_r", 0.06f } } });
    }
    V bSub (float warpAmt, float glide, float punchAmt = 0.1f)
    {
        return join ({ legatoBass (glide), punch (1, punchAmt, 0.03f), dist (0, 0.25f, 0.5f, 0.3f), {
            { "a_wt", 0 }, { "a_pos", 0.0f }, { "a_rand", 0 }, { "a_warp", 2 }, { "a_warpamt", warpAmt }, { "f_on", 0 },
            { "e1_a", 0.001f }, { "e1_d", 0.8f }, { "e1_s", 0.7f }, { "e1_r", 0.12f } } });
    }
    V bPluck (int wt, float pos, float cut, float dec, float res = 0.35f)
    {
        return join ({ monoBass (0.01f), subOn (0.4f), macroTone(), {
            { "a_wt", (float) wt }, { "a_pos", pos }, { "a_rand", 0 }, { "f_cutoff", cut }, { "f_res", res }, { "f_env", 0.55f },
            { "e2_d", dec }, { "e2_s", 0 }, { "e1_d", dec * 1.6f }, { "e1_s", 0.15f }, { "e1_r", 0.06f } } });
    }
    V bOrgan (float pos)
    {
        return join ({ monoBass (0.015f), subOn (0.55f), comp (-18.0f, 3.0f, 2.0f), macroTone (0.4f), {
            { "a_wt", 4 }, { "a_pos", pos }, { "a_rand", 0 }, { "f_cutoff", 900 }, { "f_res", 0.2f }, { "f_env", 0.3f },
            { "e2_d", 0.2f }, { "e1_d", 0.3f }, { "e1_s", 0.6f }, { "e1_r", 0.06f } } });
    }
    V bWobble (int div, float cut, float res, int shape = 0)
    {
        return join ({ monoBass (0.03f), subOn (0.5f), mod (1, 3, 12, 0.35f), macroTone (0.4f), {
            { "a_wt", 0 }, { "a_pos", 0.6f }, { "a_rand", 0 }, { "l1_div", (float) div }, { "l1_shape", (float) shape },
            { "f_cutoff", cut }, { "f_res", res }, { "f_drive", 0.3f }, { "e1_s", 0.9f } } });
    }
    V bGrowl (int wt, float pos, int lfoDiv, float lfoAmt)
    {
        return join ({ monoBass (0.02f), subOn (0.5f), mod (1, 3, 4, lfoAmt), macroTone (0.4f), dist (2, 0.4f, 0.6f), {
            { "a_wt", (float) wt }, { "a_pos", pos }, { "a_rand", 0 }, { "l1_div", (float) lfoDiv },
            { "f_cutoff", 900 }, { "f_res", 0.35f }, { "f_drive", 0.5f }, { "e1_s", 0.85f } } });
    }
    V bDirty (float drive, float cut, int wt = 0, float pos = 0.6f)
    {
        return join ({ monoBass (0.02f), subOn (0.5f), punch (1, 0.08f, 0.02f), macroTone (0.4f), dist (1, drive, 0.7f, 0.55f), {
            { "eq_on", 1 }, { "eq_low", -3 }, { "eq_mid", 4 }, { "eq_midf", 1200 },
            { "a_wt", (float) wt }, { "a_pos", pos }, { "a_rand", 0 }, { "f_cutoff", cut }, { "f_res", 0.3f },
            { "f_env", 0.4f }, { "e2_d", 0.18f }, { "e2_s", 0.1f }, { "e1_s", 0.8f } } });
    }

    // ---------------- LEADS ----------------
    V lSaw (int uni, float det, float cut, float glide)
    {
        return join ({ vibrato (1, 0.006f, 0.45f), delay (6, 0.35f, 0.2f), reverb (0.65f, 0.22f), macroTone (0.4f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", (float) uni }, { "a_det", det },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.75f }, { "b_oct", 1 }, { "b_uni", 2 }, { "b_det", 0.2f }, { "b_level", 0.35f },
            { "f_type", 3 }, { "f_cutoff", cut }, { "f_res", 0.15f }, { "f_key", 0.3f },
            { "e1_a", 0.004f }, { "e1_d", 0.5f }, { "e1_s", 0.8f }, { "e1_r", 0.3f }, { "mode", 2 }, { "glide", glide } } });
    }
    V lSquare (float pos, float cut)
    {
        return join ({ vibrato (1, 0.005f, 0.4f), delay (5, 0.35f, 0.18f), reverb (0.55f, 0.18f), macroTone (0.4f), {
            { "a_wt", 1 }, { "a_pos", pos }, { "f_type", 2 }, { "f_cutoff", cut }, { "f_res", 0.2f },
            { "e1_a", 0.005f }, { "e1_d", 0.4f }, { "e1_s", 0.8f }, { "e1_r", 0.25f }, { "mode", 2 }, { "glide", 0.04f } } });
    }
    V lVocal (float pos, float lfoRate)
    {
        return join ({ mod (1, 10, 4, 0.5f), mod (3, 3, 4, 0.15f), dist (2, 0.2f, 0.4f), delay (6, 0.3f, 0.2f), reverb (0.6f, 0.2f), {
            { "a_wt", 3 }, { "a_pos", pos }, { "l1_sync", 0 }, { "l1_rate", lfoRate },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_level", 0.25f },
            { "f_type", 2 }, { "f_cutoff", 4000 }, { "f_res", 0.2f }, { "mode", 2 }, { "glide", 0.05f },
            { "e1_s", 0.85f }, { "e1_r", 0.2f } } });
    }
    V lSync (float amt, float env)
    {
        return join ({ mod (1, 1, 10, env), macroTone (0.4f), dist (0, 0.2f, 0.5f), delay (6, 0.3f, 0.18f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_warp", 1 }, { "a_warpamt", amt }, { "e2_d", 0.25f }, { "e2_s", 0 },
            { "f_cutoff", 3500 }, { "f_res", 0.2f }, { "mode", 2 }, { "glide", 0.03f }, { "e1_s", 0.7f }, { "e1_r", 0.2f } } });
    }
    V lWhistle (float glide)
    {
        return join ({ vibrato (1, 0.008f, 0.3f), delay (6, 0.35f, 0.2f), reverb (0.6f, 0.25f), {
            { "a_wt", 0 }, { "a_pos", 0.12f }, { "a_rand", 0 }, { "f_on", 0 }, { "mode", 2 }, { "glide", glide },
            { "e1_a", 0.02f }, { "e1_d", 0.4f }, { "e1_s", 0.9f }, { "e1_r", 0.25f } } });
    }

    // ---------------- SYNTHS ----------------
    V sBrass (int chord, float cut)
    {
        return join ({ reverb (0.6f, 0.2f), macroTone (0.4f), {
            { "chord", (float) chord }, { "voices", 16 }, { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 4 }, { "a_det", 0.22f },
            { "b_on", 1 }, { "b_wt", 1 }, { "b_pos", 0.3f }, { "b_uni", 2 }, { "b_det", 0.15f }, { "b_level", 0.5f },
            { "f_cutoff", cut }, { "f_res", 0.2f }, { "f_env", 0.5f }, { "e2_a", 0.02f }, { "e2_d", 0.35f }, { "e2_s", 0.4f },
            { "e1_a", 0.01f }, { "e1_d", 0.4f }, { "e1_s", 0.6f }, { "e1_r", 0.25f } } });
    }
    V sHoover (float det, float cut)
    {
        return join ({ mod (1, 3, 4, 0.3f), mod (2, 2, 3, -0.1f), chorus (0.5f, 0.6f), dist (0, 0.2f, 0.5f), {
            { "a_wt", 1 }, { "a_pos", 0.55f }, { "a_uni", 7 }, { "a_det", det }, { "l1_sync", 0 }, { "l1_rate", 0.8f },
            { "e3_a", 0.0005f }, { "e3_d", 0.25f }, { "e3_s", 0 }, { "mode", 2 }, { "glide", 0.1f },
            { "f_type", 2 }, { "f_cutoff", cut }, { "f_res", 0.2f }, { "e1_s", 0.9f }, { "e1_r", 0.3f } } });
    }
    V sStrings (float att, float cut)
    {
        return join ({ vibrato (1, 0.005f, 0.3f), chorus (0.5f, 0.5f), reverb (0.7f, 0.3f), macroTone (0.35f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 6 }, { "a_det", 0.3f },
            { "b_on", 1 }, { "b_wt", 1 }, { "b_pos", 0.2f }, { "b_uni", 4 }, { "b_det", 0.2f }, { "b_level", 0.5f },
            { "f_type", 3 }, { "f_cutoff", cut }, { "f_res", 0.1f },
            { "e1_a", att }, { "e1_d", 0.5f }, { "e1_s", 0.9f }, { "e1_r", 0.9f } } });
    }
    V sSuper (int uni, float det, float cut)
    {
        return join ({ reverb (0.7f, 0.25f), delay (6, 0.3f, 0.15f), macroTone (0.4f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", (float) uni }, { "a_det", det }, { "a_blend", 0.85f },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_uni", (float) uni }, { "b_det", det * 0.8f }, { "b_oct", 1 },
            { "b_level", 0.35f }, { "f_type", 3 }, { "f_cutoff", cut }, { "f_res", 0.15f },
            { "e1_a", 0.003f }, { "e1_d", 0.6f }, { "e1_s", 0.8f }, { "e1_r", 0.45f } } });
    }
    V sChop (int lfoDiv, int shape)
    {
        return join ({ mod (1, 3, 4, 1.0f), delay (3, 0.3f, 0.2f), reverb (0.5f, 0.2f), {
            { "a_wt", 3 }, { "a_pos", 0.0f }, { "l1_shape", (float) shape }, { "l1_div", (float) lfoDiv },
            { "e1_a", 0.001f }, { "e1_d", 0.3f }, { "e1_s", 0.6f }, { "e1_r", 0.1f }, { "f_type", 2 }, { "f_cutoff", 4500 } } });
    }

    // ---------------- PLUCK / PAD / KEYS / CHORD / ARP ----------------
    V pl (int wt, float pos, float cut, float res, float dec, int dly = 6)
    {
        return join ({ delay (dly, 0.35f, 0.22f), reverb (0.55f, 0.2f), macroTone (0.4f), {
            { "a_wt", (float) wt }, { "a_pos", pos }, { "a_uni", 3 }, { "a_det", 0.15f },
            { "f_type", 0 }, { "f_cutoff", cut }, { "f_res", res }, { "f_env", 0.6f }, { "e2_d", dec }, { "e2_s", 0 },
            { "e1_d", dec * 1.7f }, { "e1_s", 0 }, { "e1_r", dec * 1.4f } } });
    }
    V pd (int wt, float pos, int uni, float det, float cut, float att, float rel, float lfoAmt)
    {
        return join ({ mod (1, 3, 4, lfoAmt), macroTone (0.35f), chorus (0.4f), reverb (0.85f, 0.35f), {
            { "a_wt", (float) wt }, { "a_pos", pos }, { "a_uni", (float) uni }, { "a_det", det },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_oct", 1 }, { "b_uni", 3 }, { "b_det", 0.2f }, { "b_level", 0.3f },
            { "f_type", 3 }, { "f_cutoff", cut }, { "f_res", 0.15f },
            { "e1_a", att }, { "e1_d", 1.0f }, { "e1_s", 0.85f }, { "e1_r", rel },
            { "l1_sync", 0 }, { "l1_rate", 0.1f }, { "l1_mode", 1 } } });
    }
    V kFM (float pos, float dec, float velAmt = 0.4f)
    {
        return join ({ mod (1, 5, 4, velAmt), chorus (0.3f, 0.25f, 1.0f), reverb (0.55f, 0.2f), macroTone (0.35f), {
            { "a_wt", 7 }, { "a_pos", pos }, { "e1_a", 0.002f }, { "e1_d", dec }, { "e1_s", 0.3f }, { "e1_r", 0.6f },
            { "f_type", 2 }, { "f_cutoff", 4500 }, { "f_key", 0.3f }, { "vel", 0.7f } } });
    }
    V kOrgan (float pos, float cut)
    {
        return join ({ chorus (0.35f, 0.4f, 0.7f), reverb (0.5f, 0.15f), comp (-18.0f, 3.0f, 2.0f), macroTone (0.35f), {
            { "a_wt", 4 }, { "a_pos", pos }, { "e1_a", 0.001f }, { "e1_d", 0.3f }, { "e1_s", 0.5f }, { "e1_r", 0.15f },
            { "f_type", 2 }, { "f_cutoff", cut } } });
    }
    V kClav (float pos, float cut)
    {
        return join ({ mod (1, 5, 12, 0.3f), macroTone (0.35f), comp (-26.0f, 4.0f, 8.0f), {
            { "a_wt", 1 }, { "a_pos", pos }, { "f_type", 6 }, { "f_cutoff", cut }, { "f_res", 0.25f }, { "f_env", 0.45f },
            { "f_drive", 0.5f }, { "e2_d", 0.12f }, { "e2_s", 0 }, { "e1_a", 0.001f }, { "e1_d", 0.25f }, { "e1_s", 0 },
            { "e1_r", 0.1f }, { "eq_on", 1 }, { "eq_mid", 4 }, { "eq_midf", 1500 }, { "vel", 0.8f },
            { "pha_on", 1 }, { "pha_rate", 0.5f }, { "pha_mix", 0.3f } } });
    }
    V ch (int chord, int wt, float pos, float cut, float dec, float dlyMix)
    {
        return join ({ delay (6, 0.4f, dlyMix, 0.45f), reverb (0.65f, 0.25f), macroTone (0.4f), {
            { "chord", (float) chord }, { "voices", 16 }, { "a_wt", (float) wt }, { "a_pos", pos }, { "a_uni", 2 }, { "a_det", 0.1f },
            { "f_cutoff", cut }, { "f_res", 0.3f }, { "f_env", 0.45f }, { "e2_d", dec * 0.7f }, { "e2_s", 0.05f },
            { "e1_d", dec }, { "e1_s", 0 }, { "e1_r", dec * 0.8f } } });
    }
    V ar (int wt, float pos, float cut, float dec, int dly = 3)
    {
        return join ({ delay (dly, 0.45f, 0.25f), reverb (0.5f, 0.15f), macroTone (0.4f), {
            { "a_wt", (float) wt }, { "a_pos", pos }, { "f_type", 2 }, { "f_cutoff", cut }, { "f_res", 0.25f },
            { "f_env", 0.3f }, { "e2_d", dec }, { "e1_a", 0.001f }, { "e1_d", dec }, { "e1_s", 0 }, { "e1_r", dec * 0.8f } } });
    }

    // ---------------- PERC / FX ----------------
    V pc (int wt, float pos, float pitchAmt, float dec, float hp)
    {
        return join ({ reverb (0.4f, 0.15f), delay (3, 0.25f, 0.12f), {
            { "a_wt", (float) wt }, { "a_pos", pos }, { "a_rand", 0 }, { "m1_src", 2 }, { "m1_dst", 1 }, { "m1_amt", pitchAmt },
            { "e3_a", 0.0005f }, { "e3_d", 0.02f }, { "e3_s", 0 }, { "e1_a", 0.001f }, { "e1_d", dec }, { "e1_s", 0 },
            { "e1_r", dec * 0.8f }, { "f_type", 4 }, { "f_cutoff", hp }, { "f_res", 0.2f } } });
    }
    V pcNoise (float hp, float dec)
    {
        return join ({ reverb (0.35f, 0.1f), {
            { "a_on", 0 }, { "noise_on", 1 }, { "noise_type", 0 }, { "noise_level", 0.8f },
            { "f_type", 5 }, { "f_cutoff", hp }, { "f_res", 0.2f },
            { "e1_a", 0.001f }, { "e1_d", dec }, { "e1_s", 0 }, { "e1_r", dec * 0.8f }, { "vel", 0.8f } } });
    }
    V fxRise (float att, int wt, float noiseLvl)
    {
        return join ({ mod (1, 1, 1, 0.5f), mod (2, 1, 4, 1.0f), reverb (0.9f, 0.4f), delay (8, 0.4f, 0.2f), {
            { "a_wt", (float) wt }, { "a_pos", 0.0f }, { "a_level", 0.5f }, { "a_uni", 4 }, { "a_det", 0.3f },
            { "noise_on", 1 }, { "noise_level", noiseLvl }, { "f_type", 6 }, { "f_cutoff", 200 }, { "f_res", 0.5f },
            { "f_env", 0.9f }, { "e2_a", att }, { "e2_s", 1.0f }, { "e2_r", 2.0f },
            { "e1_a", att * 0.75f }, { "e1_s", 1.0f }, { "e1_r", 2.0f }, { "mode", 1 } } });
    }
    V fxDown (float dec)
    {
        return join ({ mod (1, 1, 1, 0.5f), reverb (0.9f, 0.45f), {
            { "a_wt", 9 }, { "a_pos", 1.0f }, { "a_level", 0.4f }, { "a_uni", 4 }, { "a_det", 0.4f },
            { "noise_on", 1 }, { "noise_level", 0.7f }, { "f_type", 3 }, { "f_cutoff", 400 }, { "f_res", 0.3f }, { "f_env", 0.8f },
            { "e2_a", 0.001f }, { "e2_d", dec }, { "e2_s", 0 }, { "e2_r", 1.0f },
            { "e1_a", 0.01f }, { "e1_d", dec }, { "e1_s", 0 }, { "e1_r", 1.0f }, { "mode", 1 } } });
    }
    V fxImpact()
    {
        return join ({ punch (1, 1.0f, 0.4f), reverb (0.95f, 0.5f, 0.3f), dist (0, 0.4f, 0.6f, 0.4f), {
            { "a_wt", 0 }, { "a_pos", 0.0f }, { "a_rand", 0 }, { "noise_on", 1 }, { "noise_type", 1 }, { "noise_level", 0.4f },
            { "f_on", 0 }, { "e1_a", 0.001f }, { "e1_d", 2.0f }, { "e1_s", 0 }, { "e1_r", 1.5f }, { "mode", 1 } } });
    }
    V fxSweep (float rate)
    {
        return join ({ mod (1, 3, 12, 0.6f), reverb (0.85f, 0.4f), {
            { "a_on", 0 }, { "noise_on", 1 }, { "noise_type", 1 }, { "noise_level", 0.8f },
            { "f_type", 6 }, { "f_cutoff", 1200 }, { "f_res", 0.6f }, { "l1_sync", 0 }, { "l1_rate", rate }, { "l1_mode", 1 },
            { "e1_a", 0.5f }, { "e1_s", 1.0f }, { "e1_r", 2.0f } } });
    }

    std::vector<FactoryPreset> build()
    {
        std::vector<FactoryPreset> b;
        auto add = [&b] (const char* folder, const char* name, V values) { b.push_back ({ folder, name, std::move (values) }); };

        add ("Init", "Init", {});

        // =====================================================================
        //  MOODCHILD STYLE - groovy, rolling, warm deep-tech
        // =====================================================================
        const char* F = "Moodchild Style";
        add (F, "BA Rolling Sub Groove", join ({ monoBass (0.03f), subOn (0.7f), punch (1, 0.12f, 0.03f), mod (2, 10, 12, 0.5f), {
            { "a_wt", 0 }, { "a_pos", 0.62f }, { "a_rand", 0 }, { "f_type", 0 }, { "f_cutoff", 180 }, { "f_res", 0.25f },
            { "f_drive", 0.3f }, { "f_env", 0.55f }, { "e2_a", 0.001f }, { "e2_d", 0.18f }, { "e2_s", 0.1f },
            { "e1_d", 0.4f }, { "e1_s", 0.85f }, { "analog", 0.4f } } }));

        add (F, "BA Warm Rubber Bass", join ({ legatoBass (0.06f), punch (1, 0.15f, 0.025f), dist (2, 0.25f, 0.5f), {
            { "a_wt", 2 }, { "a_pos", 0.35f }, { "a_rand", 0 }, { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.75f }, { "b_oct", -1 },
            { "b_level", 0.5f }, { "b_rand", 0 }, { "f_cutoff", 220 }, { "f_res", 0.45f }, { "f_drive", 0.35f }, { "f_env", 0.6f },
            { "e2_d", 0.22f }, { "e2_s", 0.05f }, { "e1_s", 0.9f } } }));

        add (F, "PL Mood Pluck", join ({ mod (1, 1, 4, 0.5f), delay (6, 0.3f, 0.18f), reverb (0.55f, 0.18f), {
            { "a_wt", 6 }, { "a_pos", 0.3f }, { "a_uni", 3 }, { "a_det", 0.15f }, { "f_type", 3 }, { "f_cutoff", 600 },
            { "f_res", 0.3f }, { "f_env", 0.65f }, { "e2_d", 0.25f }, { "e2_s", 0 }, { "e2_r", 0.25f },
            { "e1_d", 0.45f }, { "e1_s", 0 }, { "e1_r", 0.35f } } }));

        add (F, "KY Late Night Keys", join ({ mod (1, 5, 4, 0.4f), chorus (0.35f), reverb (0.5f, 0.2f), {
            { "a_wt", 7 }, { "a_pos", 0.35f }, { "e1_a", 0.002f }, { "e1_d", 1.8f }, { "e1_s", 0.25f }, { "e1_r", 0.6f },
            { "f_type", 2 }, { "f_cutoff", 4500 }, { "f_res", 0.1f }, { "f_key", 0.3f }, { "vel", 0.7f } } }));

        add (F, "AR Hypno Bleep", join ({ mod (1, 2, 1, 0.5f), delay (5, 0.45f, 0.25f), reverb (0.5f, 0.15f), {
            { "a_wt", 0 }, { "a_pos", 0.1f }, { "a_rand", 0 }, { "e3_a", 0.0005f }, { "e3_d", 0.015f }, { "e3_s", 0 },
            { "e1_a", 0.001f }, { "e1_d", 0.12f }, { "e1_s", 0 }, { "e1_r", 0.1f }, { "f_on", 0 } } }));

        add (F, "PD Velvet Pad", join ({ mod (1, 3, 12, 0.12f), mod (2, 4, 4, 0.2f), chorus (0.4f), reverb (0.8f, 0.35f), {
            { "a_wt", 2 }, { "a_pos", 0.5f }, { "a_uni", 6 }, { "a_det", 0.35f }, { "a_blend", 0.7f },
            { "b_on", 1 }, { "b_wt", 11 }, { "b_pos", 0.6f }, { "b_uni", 4 }, { "b_det", 0.25f }, { "b_oct", 1 }, { "b_level", 0.35f },
            { "f_cutoff", 1400 }, { "f_res", 0.15f }, { "e1_a", 1.2f }, { "e1_d", 1.0f }, { "e1_s", 0.85f }, { "e1_r", 2.2f },
            { "l1_sync", 0 }, { "l1_rate", 0.15f }, { "l1_mode", 0 }, { "l2_sync", 0 }, { "l2_rate", 0.08f }, { "l2_shape", 6 },
            { "l2_mode", 0 }, { "analog", 0.5f } } }));

        add (F, "CH Swing Stab", join ({ delay (6, 0.4f, 0.25f, 0.45f), reverb (0.6f, 0.2f), {
            { "chord", 9 }, { "voices", 16 }, { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 2 }, { "a_det", 0.1f },
            { "f_cutoff", 900 }, { "f_res", 0.35f }, { "f_env", 0.5f }, { "e2_d", 0.2f }, { "e2_s", 0.1f },
            { "e1_d", 0.35f }, { "e1_s", 0 }, { "e1_r", 0.3f } } }));

        add (F, "BA Bouncy Mid Bass", join ({ monoBass (0.02f), subOn (0.55f), mod (1, 3, 4, 0.25f), {
            { "a_wt", 1 }, { "a_pos", 0.3f }, { "a_rand", 0 }, { "l1_div", 2 }, { "l1_shape", 1 },
            { "f_cutoff", 350 }, { "f_res", 0.5f }, { "f_drive", 0.4f }, { "f_env", 0.45f }, { "e2_d", 0.15f }, { "e2_s", 0.2f },
            { "e1_d", 0.3f }, { "e1_s", 0.7f } } }));

        add (F, "LD Wonky Lead", join ({ mod (1, 3, 4, 0.35f), vibrato (2, 0.006f, 0.4f), delay (7, 0.35f, 0.2f), dist (0, 0.2f, 0.6f), {
            { "a_wt", 9 }, { "a_pos", 0.3f }, { "l1_shape", 6 }, { "l1_div", 5 }, { "mode", 2 }, { "glide", 0.08f },
            { "f_type", 2 }, { "f_cutoff", 3500 }, { "f_res", 0.2f }, { "e1_s", 0.8f }, { "e1_r", 0.25f } } }));

        add (F, "CH Afterglow Chords", join ({ chorus (0.35f), delay (6, 0.35f, 0.22f), reverb (0.7f, 0.28f), {
            { "chord", 5 }, { "voices", 16 }, { "a_wt", 2 }, { "a_pos", 0.55f }, { "a_uni", 3 }, { "a_det", 0.18f },
            { "b_on", 1 }, { "b_wt", 3 }, { "b_pos", 0.4f }, { "b_level", 0.3f }, { "f_type", 3 }, { "f_cutoff", 1100 },
            { "f_res", 0.2f }, { "f_env", 0.35f }, { "e2_a", 0.01f }, { "e2_d", 0.6f }, { "e2_s", 0.3f },
            { "e1_a", 0.005f }, { "e1_d", 1.2f }, { "e1_s", 0.3f }, { "e1_r", 0.8f } } }));

        // =====================================================================
        //  DEEPERFECT STYLE - driving, dark, hypnotic
        // =====================================================================
        F = "Deeperfect Style";
        add (F, "BA Driving Reese", join ({ legatoBass (0.05f), subOn (0.6f), mod (1, 3, 12, 0.15f), dist (2, 0.3f, 0.6f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 4 }, { "a_det", 0.35f }, { "a_blend", 0.8f }, { "a_width", 0.5f },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_uni", 4 }, { "b_det", 0.45f }, { "b_level", 0.6f }, { "b_width", 0.5f },
            { "f_cutoff", 420 }, { "f_res", 0.3f }, { "f_drive", 0.5f }, { "l1_div", 11 }, { "l1_mode", 0 },
            { "e1_a", 0.005f }, { "e1_s", 1.0f }, { "e1_r", 0.1f }, { "analog", 0.5f } } }));

        add (F, "BA Rumble Low End", join ({ monoBass (0.0f), dist (0, 0.45f, 1.0f, 0.3f), reverb (0.7f, 0.25f, 0.9f), {
            { "a_wt", 0 }, { "a_pos", 0.15f }, { "a_rand", 0 }, { "sub_on", 1 }, { "sub_oct", 0 }, { "sub_level", 0.6f },
            { "f_cutoff", 120 }, { "f_res", 0.35f }, { "f_drive", 0.8f }, { "e1_a", 0.002f }, { "e1_d", 0.6f }, { "e1_s", 0.4f },
            { "e1_r", 0.4f } } }));

        add (F, "BA Acid Groove 303", join ({ legatoBass (0.07f), mod (1, 5, 12, 0.25f), mod (2, 10, 12, 0.5f), dist (2, 0.35f, 0.6f),
                                            delay (3, 0.35f, 0.15f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_rand", 0 }, { "f_cutoff", 260 }, { "f_res", 0.78f }, { "f_drive", 0.45f },
            { "f_env", 0.6f }, { "e2_a", 0.001f }, { "e2_d", 0.22f }, { "e2_s", 0 }, { "e2_r", 0.1f },
            { "e1_d", 0.3f }, { "e1_s", 0.8f }, { "e1_r", 0.05f }, { "vel", 0.4f } } }));

        add (F, "AR Hypnotic Sequence", join ({ mod (1, 3, 4, 0.3f), delay (6, 0.55f, 0.25f), reverb (0.6f, 0.2f), {
            { "a_wt", 6 }, { "a_pos", 0.45f }, { "l1_div", 13 }, { "l1_shape", 1 }, { "l1_mode", 0 },
            { "f_type", 3 }, { "f_cutoff", 1400 }, { "f_res", 0.35f }, { "f_env", 0.3f }, { "e2_d", 0.2f },
            { "e1_d", 0.25f }, { "e1_s", 0 }, { "e1_r", 0.2f } } }));

        add (F, "CH Dark Stab", join ({ reverb (0.75f, 0.3f), delay (6, 0.3f, 0.18f), {
            { "chord", 1 }, { "voices", 16 }, { "a_wt", 5 }, { "a_pos", 0.6f }, { "a_uni", 2 }, { "a_det", 0.2f },
            { "f_cutoff", 700 }, { "f_res", 0.4f }, { "f_env", 0.45f }, { "f_drive", 0.3f }, { "e2_d", 0.18f },
            { "e1_d", 0.3f }, { "e1_s", 0 }, { "e1_r", 0.25f } } }));

        add (F, "LD Warehouse Lead", join ({ mod (1, 1, 4, 0.35f), dist (0, 0.3f, 0.6f), delay (6, 0.35f, 0.2f), reverb (0.6f, 0.15f), {
            { "a_wt", 8 }, { "a_pos", 0.35f }, { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_oct", 1 }, { "b_level", 0.25f },
            { "f_type", 2 }, { "f_cutoff", 2500 }, { "f_res", 0.25f }, { "f_env", 0.3f }, { "mode", 2 }, { "glide", 0.04f },
            { "e1_s", 0.8f }, { "e1_r", 0.2f } } }));

        add (F, "FX Tension Riser", join ({ mod (1, 1, 1, 0.5f), mod (2, 1, 4, 1.0f), reverb (0.9f, 0.4f), delay (8, 0.4f, 0.2f), {
            { "a_wt", 9 }, { "a_pos", 0.0f }, { "a_level", 0.5f }, { "a_uni", 4 }, { "a_det", 0.3f },
            { "noise_on", 1 }, { "noise_type", 0 }, { "noise_level", 0.7f }, { "f_type", 6 }, { "f_cutoff", 200 },
            { "f_res", 0.5f }, { "f_env", 0.9f }, { "e2_a", 8.0f }, { "e2_s", 1.0f }, { "e2_r", 2.0f },
            { "e1_a", 6.0f }, { "e1_s", 1.0f }, { "e1_r", 2.0f }, { "mode", 1 } } }));

        add (F, "BA Deep Pulse Bass", join ({ monoBass (0.02f), subOn (0.6f), mod (1, 3, 4, 0.3f), {
            { "a_wt", 1 }, { "a_pos", 0.2f }, { "a_rand", 0 }, { "l1_sync", 0 }, { "l1_rate", 0.3f }, { "l1_mode", 0 },
            { "f_cutoff", 300 }, { "f_res", 0.35f }, { "f_env", 0.4f }, { "e2_d", 0.3f }, { "e2_s", 0.15f }, { "e1_s", 0.85f } } }));

        add (F, "PC Metal Perc Synth", join ({ punch (1, 0.3f, 0.01f), reverb (0.4f, 0.2f), delay (3, 0.3f, 0.15f), {
            { "a_wt", 10 }, { "a_pos", 0.5f }, { "a_warp", 4 }, { "a_warpamt", 0.6f },
            { "b_on", 1 }, { "b_wt", 7 }, { "b_pos", 0.7f }, { "b_oct", 1 }, { "b_semi", 7 }, { "b_level", 0.0f },
            { "e1_a", 0.001f }, { "e1_d", 0.09f }, { "e1_s", 0 }, { "e1_r", 0.08f },
            { "f_type", 4 }, { "f_cutoff", 350 }, { "f_res", 0.2f } } }));

        add (F, "PD Tunnel Pad", join ({ mod (1, 3, 4, 0.4f), reverb (0.9f, 0.4f), delay (9, 0.35f, 0.2f), {
            { "a_wt", 3 }, { "a_pos", 0.3f }, { "a_uni", 5 }, { "a_det", 0.3f }, { "l1_sync", 0 }, { "l1_rate", 0.07f },
            { "l1_mode", 1 }, { "f_type", 3 }, { "f_cutoff", 1600 }, { "f_res", 0.3f },
            { "e1_a", 2.0f }, { "e1_d", 1.0f }, { "e1_s", 0.9f }, { "e1_r", 3.0f },
            { "pha_on", 1 }, { "pha_rate", 0.1f }, { "pha_mix", 0.4f } } }));

        // =====================================================================
        //  TOMANGO STYLE - funky, vocal, playful
        // =====================================================================
        F = "Tomango Style";
        add (F, "BA Funk Slap Bass", join ({ monoBass (0.015f), subOn (0.45f), mod (1, 1, 4, 0.45f), mod (2, 5, 4, 0.2f), comp (-20.0f, 4.0f, 3.0f), {
            { "a_wt", 8 }, { "a_pos", 0.3f }, { "a_rand", 0 }, { "e2_a", 0.001f }, { "e2_d", 0.12f }, { "e2_s", 0 },
            { "f_cutoff", 600 }, { "f_res", 0.35f }, { "f_env", 0.45f }, { "e1_d", 0.35f }, { "e1_s", 0.5f }, { "vel", 0.6f } } }));

        add (F, "LD Talkbox Lead", join ({ mod (1, 3, 4, 0.6f), dist (2, 0.25f, 0.5f), chorus (0.25f), delay (6, 0.3f, 0.18f), {
            { "a_wt", 3 }, { "a_pos", 0.2f }, { "l1_div", 5 }, { "l1_shape", 1 },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_level", 0.3f }, { "b_uni", 2 }, { "b_det", 0.1f },
            { "f_type", 2 }, { "f_cutoff", 3000 }, { "f_res", 0.25f }, { "mode", 2 }, { "glide", 0.06f },
            { "e1_s", 0.85f }, { "e1_r", 0.2f } } }));

        add (F, "KY Organ Hit", join ({ chorus (0.4f, 0.4f, 0.8f), reverb (0.5f, 0.2f), {
            { "a_wt", 4 }, { "a_pos", 0.55f }, { "e1_a", 0.001f }, { "e1_d", 0.5f }, { "e1_s", 0.4f }, { "e1_r", 0.2f },
            { "f_type", 2 }, { "f_cutoff", 6000 }, { "f_res", 0.1f } } }));

        add (F, "BA Bubble Bass", join ({ legatoBass (0.05f), subOn (0.4f), mod (2, 1, 10, 0.5f), {
            { "a_wt", 0 }, { "a_pos", 0.05f }, { "a_rand", 0 }, { "a_warp", 2 }, { "a_warpamt", 0.4f },
            { "m1_src", 2 }, { "m1_dst", 1 }, { "m1_amt", 0.2f }, { "e3_a", 0.0005f }, { "e3_d", 0.06f }, { "e3_s", 0 },
            { "e2_d", 0.15f }, { "e2_s", 0 }, { "f_on", 0 }, { "e1_d", 0.4f }, { "e1_s", 0.6f }, { "e1_r", 0.1f } } }));

        add (F, "PL Groove Pluck", join ({ delay (6, 0.35f, 0.2f), reverb (0.5f, 0.15f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 3 }, { "a_det", 0.2f },
            { "f_cutoff", 500 }, { "f_res", 0.45f }, { "f_env", 0.6f }, { "e2_d", 0.18f }, { "e2_s", 0 },
            { "e1_d", 0.3f }, { "e1_s", 0 }, { "e1_r", 0.25f } } }));

        add (F, "BA Disco Rubber", join ({ monoBass (0.04f), dist (0, 0.2f, 0.5f), {
            { "a_wt", 2 }, { "a_pos", 0.7f }, { "a_rand", 0 }, { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.75f }, { "b_oct", -1 },
            { "b_level", 0.45f }, { "b_rand", 0 }, { "f_cutoff", 280 }, { "f_res", 0.55f }, { "f_env", 0.5f }, { "f_drive", 0.3f },
            { "e2_d", 0.2f }, { "e2_s", 0.1f }, { "e1_s", 0.8f } } }));

        add (F, "SY Vowel Chop", join ({ mod (1, 3, 4, 1.0f), delay (3, 0.3f, 0.2f), reverb (0.5f, 0.2f), {
            { "a_wt", 3 }, { "a_pos", 0.0f }, { "l1_shape", 5 }, { "l1_div", 2 },
            { "e1_a", 0.001f }, { "e1_d", 0.15f }, { "e1_s", 0 }, { "e1_r", 0.1f }, { "f_type", 2 }, { "f_cutoff", 4500 } } }));

        add (F, "CH Bright Brass Stab", join ({ reverb (0.6f, 0.2f), {
            { "chord", 2 }, { "voices", 16 }, { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 4 }, { "a_det", 0.22f },
            { "b_on", 1 }, { "b_wt", 1 }, { "b_pos", 0.3f }, { "b_uni", 2 }, { "b_det", 0.15f }, { "b_level", 0.5f },
            { "f_cutoff", 900 }, { "f_res", 0.2f }, { "f_env", 0.5f }, { "e2_a", 0.02f }, { "e2_d", 0.35f }, { "e2_s", 0.4f },
            { "e1_a", 0.01f }, { "e1_d", 0.4f }, { "e1_s", 0.6f }, { "e1_r", 0.25f } } }));

        add (F, "KY Sunset Keys", join ({ mod (1, 5, 4, 0.5f), reverb (0.6f, 0.25f), {
            { "a_wt", 7 }, { "a_pos", 0.25f }, { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.0f }, { "b_oct", 1 }, { "b_level", 0.2f },
            { "e1_a", 0.002f }, { "e1_d", 2.0f }, { "e1_s", 0.3f }, { "e1_r", 0.7f }, { "vel", 0.7f },
            { "pha_on", 1 }, { "pha_mix", 0.3f }, { "pha_rate", 0.2f }, { "f_type", 2 }, { "f_cutoff", 6000 } } }));

        add (F, "LD Party Lead", join ({ vibrato (1, 0.006f, 0.5f), delay (6, 0.35f, 0.22f), reverb (0.65f, 0.25f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 7 }, { "a_det", 0.4f }, { "a_blend", 0.8f },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.75f }, { "b_uni", 3 }, { "b_oct", 1 }, { "b_det", 0.25f }, { "b_level", 0.4f },
            { "f_type", 3 }, { "f_cutoff", 5000 }, { "f_res", 0.15f }, { "e1_a", 0.005f }, { "e1_d", 0.5f }, { "e1_s", 0.8f },
            { "e1_r", 0.4f }, { "mode", 2 }, { "glide", 0.03f } } }));

        // =====================================================================
        //  TOOLROOM STYLE - big tech house, organs, rubber, rave
        // =====================================================================
        F = "Toolroom Style";
        add (F, "BA Rubber Bass Classic", join ({ monoBass (0.03f), subOn (0.5f), punch (1, 0.1f, 0.02f), dist (2, 0.2f, 0.5f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_rand", 0 }, { "f_cutoff", 160 }, { "f_res", 0.62f }, { "f_drive", 0.45f },
            { "f_env", 0.55f }, { "e2_a", 0.001f }, { "e2_d", 0.16f }, { "e2_s", 0.05f }, { "e2_r", 0.08f },
            { "e1_d", 0.3f }, { "e1_s", 0.85f } } }));

        add (F, "KY Classic House Organ", join ({ chorus (0.3f, 0.35f, 0.6f), reverb (0.5f, 0.18f), comp (-18.0f, 3.0f, 2.0f), {
            { "a_wt", 4 }, { "a_pos", 0.75f }, { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.75f }, { "b_oct", 1 }, { "b_level", 0.25f },
            { "e1_a", 0.001f }, { "e1_d", 0.3f }, { "e1_s", 0.3f }, { "e1_r", 0.15f },
            { "f_type", 2 }, { "f_cutoff", 7000 }, { "f_res", 0.1f } } }));

        add (F, "CH Big Rave Stab", join ({ dist (0, 0.3f, 0.4f), reverb (0.75f, 0.3f), delay (6, 0.3f, 0.18f), {
            { "chord", 1 }, { "voices", 16 }, { "a_wt", 0 }, { "a_pos", 0.55f }, { "a_uni", 7 }, { "a_det", 0.5f }, { "a_blend", 0.85f },
            { "b_on", 1 }, { "b_wt", 9 }, { "b_pos", 0.4f }, { "b_uni", 3 }, { "b_det", 0.3f }, { "b_level", 0.45f },
            { "f_cutoff", 2500 }, { "f_res", 0.2f }, { "f_env", 0.35f }, { "e1_a", 0.001f }, { "e1_d", 0.4f }, { "e1_s", 0 },
            { "e1_r", 0.35f } } }));

        add (F, "CH Chord Memory Stab", join ({ delay (6, 0.35f, 0.22f), reverb (0.6f, 0.22f), dist (2, 0.15f, 0.4f), {
            { "chord", 3 }, { "voices", 16 }, { "a_wt", 0 }, { "a_pos", 0.62f }, { "a_uni", 2 }, { "a_det", 0.12f },
            { "f_cutoff", 1100 }, { "f_res", 0.3f }, { "f_env", 0.45f }, { "e2_d", 0.22f }, { "e2_s", 0.05f },
            { "e1_d", 0.32f }, { "e1_s", 0 }, { "e1_r", 0.28f } } }));

        add (F, "LD Main Room Lead", join ({ vibrato (1, 0.006f, 0.45f), delay (6, 0.35f, 0.2f), reverb (0.7f, 0.25f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 8 }, { "a_det", 0.45f },
            { "b_on", 1 }, { "b_wt", 2 }, { "b_pos", 0.9f }, { "b_oct", 1 }, { "b_uni", 4 }, { "b_det", 0.3f }, { "b_level", 0.45f },
            { "f_type", 3 }, { "f_cutoff", 6000 }, { "f_res", 0.15f }, { "f_key", 0.3f },
            { "e1_a", 0.004f }, { "e1_d", 0.6f }, { "e1_s", 0.75f }, { "e1_r", 0.35f }, { "mode", 2 }, { "glide", 0.03f } } }));

        add (F, "BA Pitch Drop Bass", join ({ monoBass (0.0f), subOn (0.5f), punch (1, 0.5f, 0.12f), dist (2, 0.25f, 0.5f), {
            { "a_wt", 0 }, { "a_pos", 0.62f }, { "a_rand", 0 }, { "f_cutoff", 300 }, { "f_res", 0.35f }, { "f_drive", 0.4f },
            { "f_env", 0.4f }, { "e2_d", 0.2f }, { "e1_s", 0.8f } } }));

        add (F, "PD Uplift Pad", join ({ mod (1, 1, 12, 0.55f), chorus (0.35f), reverb (0.85f, 0.35f), delay (9, 0.3f, 0.15f), {
            { "a_wt", 2 }, { "a_pos", 0.8f }, { "a_uni", 7 }, { "a_det", 0.35f },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_oct", 1 }, { "b_uni", 3 }, { "b_det", 0.2f }, { "b_level", 0.35f },
            { "f_type", 3 }, { "f_cutoff", 900 }, { "f_res", 0.2f }, { "e2_a", 3.0f }, { "e2_d", 1.0f }, { "e2_s", 1.0f }, { "e2_r", 2.0f },
            { "e1_a", 1.5f }, { "e1_d", 1.0f }, { "e1_s", 0.9f }, { "e1_r", 2.5f } } }));

        add (F, "SY Hoover Rave", join ({ mod (1, 3, 4, 0.3f), chorus (0.5f, 0.6f), dist (0, 0.2f, 0.5f), {
            { "a_wt", 1 }, { "a_pos", 0.55f }, { "a_uni", 7 }, { "a_det", 0.55f }, { "l1_sync", 0 }, { "l1_rate", 0.8f },
            { "m2_src", 2 }, { "m2_dst", 3 }, { "m2_amt", -0.1f }, { "e3_a", 0.0005f }, { "e3_d", 0.25f }, { "e3_s", 0 },
            { "mode", 2 }, { "glide", 0.1f }, { "f_type", 2 }, { "f_cutoff", 4000 }, { "f_res", 0.2f },
            { "e1_s", 0.9f }, { "e1_r", 0.3f } } }));

        add (F, "CH Piano Stab House", join ({ reverb (0.55f, 0.2f), comp (-18.0f, 3.0f, 2.0f), {
            { "chord", 3 }, { "voices", 16 }, { "a_wt", 7 }, { "a_pos", 0.6f }, { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f },
            { "b_level", 0.2f }, { "e1_a", 0.001f }, { "e1_d", 1.0f }, { "e1_s", 0 }, { "e1_r", 0.4f },
            { "f_type", 2 }, { "f_cutoff", 5000 }, { "f_key", 0.4f }, { "vel", 0.7f } } }));

        add (F, "CH Pumping Saw Chords", join ({ reverb (0.6f, 0.25f), {
            { "chord", 5 }, { "voices", 16 }, { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 5 }, { "a_det", 0.3f },
            { "f_cutoff", 2000 }, { "f_res", 0.15f }, { "e1_a", 0.005f }, { "e1_d", 0.5f }, { "e1_s", 0.8f }, { "e1_r", 0.4f },
            { "pump_on", 1 }, { "pump_div", 2 }, { "pump_depth", 0.7f }, { "pump_curve", 0.5f } } }));

        // =====================================================================
        //  DEFECTED STYLE - soulful house, deep chords, keys, strings
        // =====================================================================
        F = "Defected Style";
        add (F, "CH Deep House Chords", join ({ chorus (0.3f), delay (6, 0.3f, 0.18f), reverb (0.65f, 0.25f), {
            { "chord", 5 }, { "voices", 16 }, { "a_wt", 2 }, { "a_pos", 0.5f }, { "a_uni", 2 }, { "a_det", 0.1f },
            { "b_on", 1 }, { "b_wt", 7 }, { "b_pos", 0.3f }, { "b_level", 0.35f }, { "f_type", 3 }, { "f_cutoff", 1500 },
            { "f_res", 0.15f }, { "f_env", 0.3f }, { "e2_d", 0.5f }, { "e2_s", 0.4f },
            { "e1_a", 0.003f }, { "e1_d", 0.8f }, { "e1_s", 0.5f }, { "e1_r", 0.6f } } }));

        add (F, "BA Organ Bass", join ({ monoBass (0.01f), subOn (0.6f), comp (-18.0f, 3.0f, 2.0f), {
            { "a_wt", 4 }, { "a_pos", 0.25f }, { "a_rand", 0 }, { "e1_d", 0.4f }, { "e1_s", 0.6f }, { "e1_r", 0.08f },
            { "f_type", 2 }, { "f_cutoff", 1200 }, { "f_res", 0.1f } } }));

        add (F, "KY Soul Keys", join ({ mod (1, 5, 4, 0.35f), mod (2, 3, 18, 0.3f), chorus (0.35f, 0.25f, 1.2f), reverb (0.55f, 0.22f), {
            { "a_wt", 7 }, { "a_pos", 0.4f }, { "e1_a", 0.002f }, { "e1_d", 2.5f }, { "e1_s", 0.35f }, { "e1_r", 0.8f },
            { "f_type", 2 }, { "f_cutoff", 4000 }, { "f_key", 0.3f }, { "l1_sync", 0 }, { "l1_rate", 4.0f }, { "l1_mode", 0 },
            { "vel", 0.75f } } }));

        add (F, "SY Disco Strings", join ({ vibrato (1, 0.005f, 0.3f), chorus (0.5f, 0.5f), reverb (0.7f, 0.3f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 6 }, { "a_det", 0.3f },
            { "b_on", 1 }, { "b_wt", 1 }, { "b_pos", 0.2f }, { "b_uni", 4 }, { "b_det", 0.2f }, { "b_level", 0.5f },
            { "f_type", 3 }, { "f_cutoff", 2500 }, { "f_res", 0.1f },
            { "e1_a", 0.25f }, { "e1_d", 0.5f }, { "e1_s", 0.9f }, { "e1_r", 0.9f } } }));

        add (F, "PD Warm Analog Pad", join ({ mod (1, 3, 4, 0.3f), chorus (0.4f), reverb (0.75f, 0.3f), {
            { "a_wt", 1 }, { "a_pos", 0.3f }, { "a_uni", 4 }, { "a_det", 0.2f }, { "l1_sync", 0 }, { "l1_rate", 0.4f },
            { "l1_shape", 1 }, { "l1_mode", 0 }, { "b_on", 1 }, { "b_wt", 2 }, { "b_pos", 0.5f }, { "b_uni", 3 },
            { "b_fine", 6 }, { "b_level", 0.45f }, { "f_cutoff", 1800 }, { "f_res", 0.15f },
            { "e1_a", 0.8f }, { "e1_d", 1.0f }, { "e1_s", 0.85f }, { "e1_r", 1.8f }, { "analog", 0.6f } } }));

        add (F, "BA Garage Sub", join ({ legatoBass (0.08f), punch (1, 0.08f, 0.03f), dist (0, 0.2f, 0.5f), {
            { "a_wt", 0 }, { "a_pos", 0.0f }, { "a_rand", 0 }, { "a_warp", 2 }, { "a_warpamt", 0.25f }, { "f_on", 0 },
            { "e1_a", 0.002f }, { "e1_d", 0.5f }, { "e1_s", 0.8f }, { "e1_r", 0.1f } } }));

        add (F, "CH Jackin Stab", join ({ delay (5, 0.3f, 0.2f), reverb (0.5f, 0.2f), {
            { "chord", 3 }, { "voices", 16 }, { "a_wt", 4 }, { "a_pos", 0.6f }, { "e1_d", 0.25f }, { "e1_s", 0 }, { "e1_r", 0.2f },
            { "f_type", 2 }, { "f_cutoff", 3500 }, { "f_env", 0.3f }, { "e2_d", 0.1f } } }));

        add (F, "PL Sunrise Pluck", join ({ mod (1, 1, 4, 0.4f), delay (6, 0.4f, 0.25f), reverb (0.7f, 0.3f), {
            { "a_wt", 11 }, { "a_pos", 0.6f }, { "a_uni", 3 }, { "a_det", 0.15f }, { "e2_d", 0.3f },
            { "f_type", 3 }, { "f_cutoff", 2500 }, { "f_res", 0.2f }, { "f_env", 0.3f },
            { "e1_d", 0.6f }, { "e1_s", 0 }, { "e1_r", 0.5f } } }));

        add (F, "LD Smooth Lead", join ({ vibrato (1, 0.006f, 0.5f), delay (9, 0.3f, 0.18f), reverb (0.6f, 0.25f), {
            { "a_wt", 0 }, { "a_pos", 0.3f }, { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.1f }, { "b_oct", 1 }, { "b_level", 0.3f },
            { "f_type", 2 }, { "f_cutoff", 2800 }, { "f_res", 0.2f }, { "e1_a", 0.02f }, { "e1_d", 0.5f }, { "e1_s", 0.8f },
            { "e1_r", 0.4f }, { "mode", 2 }, { "glide", 0.1f } } }));

        add (F, "KY Funky Clav", join ({ mod (1, 5, 12, 0.3f), {
            { "a_wt", 1 }, { "a_pos", 0.6f }, { "f_type", 6 }, { "f_cutoff", 900 }, { "f_res", 0.25f }, { "f_env", 0.45f },
            { "f_drive", 0.5f }, { "eq_on", 1 }, { "eq_mid", 4 }, { "eq_midf", 1500 },
            { "comp_on", 1 }, { "comp_thresh", -26.0f }, { "comp_ratio", 4.0f }, { "comp_gain", 8.0f },
            { "e2_d", 0.12f }, { "e2_s", 0 }, { "e1_a", 0.001f }, { "e1_d", 0.25f }, { "e1_s", 0 }, { "e1_r", 0.1f },
            { "pha_on", 1 }, { "pha_rate", 0.5f }, { "pha_mix", 0.3f }, { "vel", 0.8f } } }));

        // =====================================================================
        //  LA PERA STYLE - minimal, quirky, rolling
        // =====================================================================
        F = "La Pera Style";
        add (F, "AR Minimal Bleep", join ({ delay (3, 0.5f, 0.3f), reverb (0.5f, 0.15f), {
            { "a_wt", 0 }, { "a_pos", 0.0f }, { "a_rand", 0 }, { "m1_src", 2 }, { "m1_dst", 1 }, { "m1_amt", 0.3f },
            { "e3_a", 0.0005f }, { "e3_d", 0.01f }, { "e3_s", 0 }, { "e1_a", 0.001f }, { "e1_d", 0.08f }, { "e1_s", 0 },
            { "e1_r", 0.06f }, { "f_on", 0 } } }));

        add (F, "BA Rolling Mono Bass", join ({ monoBass (0.02f), subOn (0.5f), mod (1, 3, 12, 0.08f), punch (2, 0.08f, 0.015f), {
            { "a_wt", 0 }, { "a_pos", 0.45f }, { "a_rand", 0 }, { "l1_div", 2 }, { "l1_shape", 3 },
            { "f_cutoff", 240 }, { "f_res", 0.4f }, { "f_env", 0.4f }, { "e2_d", 0.12f }, { "e2_s", 0.1f },
            { "e1_d", 0.2f }, { "e1_s", 0.5f }, { "e1_r", 0.05f } } }));

        add (F, "PC Quirky Perc Synth", join ({ mod (1, 9, 4, 0.5f), delay (3, 0.3f, 0.25f), reverb (0.45f, 0.15f), {
            { "a_wt", 9 }, { "a_pos", 0.3f }, { "m2_src", 2 }, { "m2_dst", 1 }, { "m2_amt", 0.3f },
            { "e3_a", 0.0005f }, { "e3_d", 0.02f }, { "e3_s", 0 }, { "e1_a", 0.001f }, { "e1_d", 0.1f }, { "e1_s", 0 },
            { "e1_r", 0.08f }, { "f_type", 4 }, { "f_cutoff", 300 } } }));

        add (F, "PL Glitch Pluck", join ({ mod (1, 3, 4, 0.5f), dist (4, 0.4f, 0.5f), delay (2, 0.3f, 0.2f), {
            { "a_wt", 10 }, { "a_pos", 0.2f }, { "l1_shape", 5 }, { "l1_div", 0 },
            { "e1_a", 0.001f }, { "e1_d", 0.18f }, { "e1_s", 0 }, { "e1_r", 0.12f },
            { "f_type", 2 }, { "f_cutoff", 3500 }, { "f_env", 0.2f } } }));

        add (F, "BA Wobble Groove", join ({ monoBass (0.03f), subOn (0.5f), mod (1, 3, 12, 0.35f), {
            { "a_wt", 0 }, { "a_pos", 0.6f }, { "a_rand", 0 }, { "l1_div", 4 }, { "l1_shape", 0 },
            { "f_cutoff", 300 }, { "f_res", 0.55f }, { "f_drive", 0.3f }, { "e1_s", 0.9f } } }));

        add (F, "CH Micro Stab", join ({ delay (6, 0.45f, 0.25f), reverb (0.55f, 0.2f), {
            { "chord", 7 }, { "voices", 16 }, { "a_wt", 6 }, { "a_pos", 0.55f },
            { "e1_a", 0.001f }, { "e1_d", 0.12f }, { "e1_s", 0 }, { "e1_r", 0.1f },
            { "f_type", 3 }, { "f_cutoff", 2000 }, { "f_res", 0.3f } } }));

        add (F, "PC Shaker Noise Synth", join ({ mod (1, 9, 12, 0.1f), reverb (0.35f, 0.1f), {
            { "a_on", 0 }, { "noise_on", 1 }, { "noise_type", 0 }, { "noise_level", 0.8f },
            { "f_type", 5 }, { "f_cutoff", 6000 }, { "f_res", 0.2f },
            { "e1_a", 0.001f }, { "e1_d", 0.06f }, { "e1_s", 0 }, { "e1_r", 0.05f }, { "vel", 0.8f } } }));

        add (F, "SY Bouncing Sync", join ({ mod (1, 1, 10, 0.6f), dist (0, 0.2f, 0.5f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_warp", 1 }, { "a_warpamt", 0.2f }, { "e2_d", 0.2f }, { "e2_s", 0 },
            { "f_cutoff", 2500 }, { "f_res", 0.2f }, { "e1_d", 0.25f }, { "e1_s", 0.3f }, { "e1_r", 0.1f },
            { "mode", 2 }, { "glide", 0.02f } } }));

        add (F, "KY Deep Mallet", join ({ mod (1, 1, 4, 0.4f), reverb (0.6f, 0.25f), delay (6, 0.3f, 0.18f), {
            { "a_wt", 7 }, { "a_pos", 0.5f }, { "e2_d", 0.15f }, { "e1_a", 0.001f }, { "e1_d", 0.5f }, { "e1_s", 0 }, { "e1_r", 0.4f },
            { "f_type", 2 }, { "f_cutoff", 2500 } } }));

        add (F, "BA Hypno Arp Bass", join ({ monoBass (0.0f), subOn (0.45f), mod (1, 3, 4, 0.3f), {
            { "a_wt", 1 }, { "a_pos", 0.4f }, { "a_rand", 0 }, { "l1_div", 12 }, { "l1_mode", 0 },
            { "f_cutoff", 400 }, { "f_res", 0.4f }, { "f_env", 0.35f }, { "e2_d", 0.15f },
            { "e1_d", 0.22f }, { "e1_s", 0.2f }, { "e1_r", 0.06f } } }));

        // =====================================================================
        //  HOT CREATIONS STYLE - groovy, quirky, warm, bleepy
        // =====================================================================
        F = "Hot Creations Style";
        add (F, "BA Hot Rubber",        bRubber (190, 0.55f, 0.14f, 0.55f));
        add (F, "BA Paradise Sub",      bSub (0.2f, 0.06f, 0.1f));
        add (F, "BA Jones Groove",      bPluck (0, 0.6f, 320, 0.18f, 0.4f));
        add (F, "BA Wonky Reese",       bReese (0.4f, 450, 12));
        add (F, "BA Bleep Bass",        bFM (0.35f, 0.12f));
        add (F, "BA Rolling Fold",      bGrowl (8, 0.4f, 2, 0.2f));
        add (F, "LD Quirky Whistle",    lWhistle (0.08f));
        add (F, "LD Hot Square",        lSquare (0.4f, 2600));
        add (F, "LD Vowel Talk",        lVocal (0.3f, 2.5f));
        add (F, "SY Retro Chop",        sChop (2, 5));
        add (F, "SY Sunday Strings",    sStrings (0.3f, 2200));
        add (F, "PL Ibiza Pluck",       pl (0, 0.5f, 550, 0.4f, 0.2f));
        add (F, "PL Glass Pluck",       pl (7, 0.6f, 1800, 0.2f, 0.25f));
        add (F, "PD Afterparty Pad",    pd (2, 0.6f, 6, 0.3f, 1500, 1.0f, 2.0f, 0.2f));
        add (F, "KY Warm Rhodes",       kFM (0.35f, 2.0f));
        add (F, "CH Hot Stab",          ch (3, 0, 0.55f, 900, 0.3f, 0.25f));
        add (F, "CH Deep Minor 9",      ch (5, 2, 0.5f, 1300, 0.5f, 0.2f));
        add (F, "AR Bleep Arp",         ar (0, 0.1f, 4000, 0.12f));
        add (F, "PC Wood Perc",         pc (7, 0.5f, 0.4f, 0.12f, 400));
        add (F, "FX Space Riser",       fxRise (6.0f, 9, 0.6f));

        // =====================================================================
        //  SOLID GROOVES STYLE - rolling, punchy, gritty minimal tech
        // =====================================================================
        F = "Solid Grooves Style";
        add (F, "BA Solid Roller",      bRubber (150, 0.5f, 0.12f, 0.45f));
        add (F, "BA Rumble Grit",       join ({ bDirty (0.5f, 500), { { "sub_level", 0.65f } } }));
        add (F, "BA Bounce Pulse",      bPluck (1, 0.35f, 380, 0.15f, 0.45f));
        add (F, "BA Grit Mid",          bDirty (0.65f, 800, 8, 0.45f));
        add (F, "BA Low Stomp",         bSub (0.35f, 0.02f, 0.18f));
        add (F, "BA Night Reese",       bReese (0.5f, 380, 11, 0.4f));
        add (F, "BA Rolling Acid",      bAcid (300, 0.75f, 0.55f, 0.05f));
        add (F, "LD Hook Square",       lSquare (0.55f, 3000));
        add (F, "LD Sync Cutter",       lSync (0.3f, 0.5f));
        add (F, "SY Vocal Chop Grid",   sChop (5, 5));
        add (F, "SY Hoover Groove",     sHoover (0.5f, 3500));
        add (F, "PL Tight Pluck",       pl (0, 0.5f, 450, 0.45f, 0.15f, 3));
        add (F, "PL Hollow Pluck",      pl (5, 0.5f, 700, 0.35f, 0.18f));
        add (F, "PD Dark Room Pad",     pd (10, 0.3f, 5, 0.3f, 1200, 1.5f, 2.5f, 0.3f));
        add (F, "KY Grime Organ",       kOrgan (0.6f, 4000));
        add (F, "CH Rolling Stab",      ch (9, 0, 0.5f, 750, 0.35f, 0.3f));
        add (F, "CH Solid Minor",       ch (1, 5, 0.5f, 1100, 0.25f, 0.2f));
        add (F, "AR Groove Arp",        ar (6, 0.4f, 2500, 0.15f));
        add (F, "PC Rim Synth",         pc (0, 0.5f, 0.6f, 0.06f, 600));
        add (F, "FX Grit Downer",       fxDown (3.0f));

        // =====================================================================
        //  REPOPULATE MARS STYLE - bouncy, retro rave, funky
        // =====================================================================
        F = "Repopulate Mars Style";
        add (F, "BA Mars Bounce",       bPluck (0, 0.75f, 450, 0.2f, 0.5f));
        add (F, "BA Funk Rubber",       bRubber (220, 0.6f, 0.2f, 0.65f, 2));
        add (F, "BA Retro Organ Bounce",       bOrgan (0.4f));
        add (F, "BA Disco Octave",      join ({ bRubber (260, 0.45f, 0.18f, 0.5f), { { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_oct", 1 }, { "b_level", 0.35f } } }));
        add (F, "BA Slap FM",           bFM (0.6f, 0.1f, 0, 1));
        add (F, "BA Bounce Wobble",     bWobble (5, 350, 0.5f));
        add (F, "LD Rave Square",       lSquare (0.75f, 3500));
        add (F, "LD Mars Lead",         lSaw (6, 0.4f, 5000, 0.03f));
        add (F, "LD Electro Sync",      lSync (0.5f, 0.4f));
        add (F, "SY Rave Hoover",       sHoover (0.6f, 4000));
        add (F, "SY Brass Section",     sBrass (2, 1000));
        add (F, "PL Bouncy Pluck",      pl (1, 0.4f, 600, 0.45f, 0.18f));
        add (F, "PD Retro Pad",         pd (1, 0.3f, 4, 0.25f, 1800, 0.8f, 1.8f, 0.3f));
        add (F, "KY Piano House",       kFM (0.6f, 1.0f, 0.5f));
        add (F, "KY House Organ",       kOrgan (0.75f, 7000));
        add (F, "CH Rave Stab",         join ({ ch (1, 0, 0.55f, 2500, 0.4f, 0.18f), { { "a_uni", 7 }, { "a_det", 0.5f } } }));
        add (F, "CH Major Lift",        ch (2, 0, 0.6f, 1500, 0.35f, 0.2f));
        add (F, "AR Mars Arp",          ar (1, 0.3f, 3000, 0.15f, 6));
        add (F, "PC Laser Zap",         pc (0, 0.5f, 1.0f, 0.1f, 150));
        add (F, "FX Rave Riser",        fxRise (7.0f, 1, 0.5f));

        // =====================================================================
        //  DIRTYBIRD STYLE - wobbly, bassy, quirky, weird
        // =====================================================================
        F = "Dirtybird Style";
        add (F, "BA Bird Wobble",       bWobble (4, 280, 0.6f));
        add (F, "BA Ghetto Growl",      bGrowl (3, 0.3f, 5, 0.5f));
        add (F, "BA Wonk Bass",         join ({ bAcid (350, 0.6f, 0.5f, 0.12f, 0.75f), { { "m4_src", 3 }, { "m4_dst", 3 }, { "m4_amt", 0.01f }, { "l1_div", 2 } } }));
        add (F, "BA Dirty Saw",         bDirty (0.7f, 600));
        add (F, "BA Booty Sub",         bSub (0.3f, 0.1f, 0.15f));
        add (F, "BA Squelch",           bAcid (200, 0.85f, 0.7f, 0.06f));
        add (F, "BA Fold Monster",      bGrowl (8, 0.6f, 4, 0.4f));
        add (F, "LD Bird Whistle",      lWhistle (0.12f));
        add (F, "LD Weird Sync",        lSync (0.6f, 0.7f));
        add (F, "LD Talkbox Bird",      lVocal (0.6f, 4.0f));
        add (F, "SY Squish Chop",       sChop (3, 6));
        add (F, "SY Laser Synth",       join ({ sHoover (0.4f, 3000), { { "m2_amt", -0.3f } } }));
        add (F, "PL Bounce Pluck",      pl (8, 0.4f, 700, 0.4f, 0.15f));
        add (F, "PD Swamp Pad",         pd (3, 0.5f, 5, 0.35f, 1000, 1.5f, 2.5f, 0.4f));
        add (F, "KY Wonky Keys",        join ({ kFM (0.5f, 1.5f), { { "m4_src", 3 }, { "m4_dst", 3 }, { "m4_amt", 0.004f }, { "l1_sync", 0 }, { "l1_rate", 3.0f } } }));
        add (F, "CH Bird Stab",         ch (7, 8, 0.4f, 1200, 0.25f, 0.25f));
        add (F, "AR Quirk Arp",         join ({ ar (9, 0.3f, 3000, 0.12f), { { "m4_src", 9 }, { "m4_dst", 4 }, { "m4_amt", 0.5f } } }));
        add (F, "PC Bird Chirp",        pc (0, 0.2f, 1.0f, 0.05f, 500));
        add (F, "PC Snap Synth",        pcNoise (3000, 0.08f));
        add (F, "FX Bird Siren",        join ({ fxSweep (0.5f), { { "a_on", 1 }, { "a_wt", 0 }, { "a_pos", 0.1f }, { "m3_src", 3 }, { "m3_dst", 1 }, { "m3_amt", 0.3f } } }));

        // =====================================================================
        //  ELROW STYLE - festive, latin, big and colourful
        // =====================================================================
        F = "Elrow Style";
        add (F, "BA Fiesta Bass",       bRubber (240, 0.5f, 0.2f, 0.6f));
        add (F, "BA Carnival Pluck Bass",    bPluck (7, 0.4f, 600, 0.2f, 0.3f));
        add (F, "BA Party Rubber",      bRubber (180, 0.65f, 0.17f, 0.5f, 2));
        add (F, "BA Latin Sub",         bSub (0.15f, 0.04f, 0.08f));
        add (F, "BA Big Room Reese",    bReese (0.45f, 600, 11, 0.35f));
        add (F, "LD Trumpet Lead",      join ({ lSaw (1, 0.0f, 2500, 0.03f), { { "f_type", 2 }, { "e1_a", 0.03f }, { "f_env", 0.3f },
                                                                                 { "e2_a", 0.03f }, { "e2_d", 0.3f }, { "e2_s", 0.5f } } }));
        add (F, "LD Fiesta Saw",        lSaw (8, 0.45f, 6000, 0.02f));
        add (F, "LD Carnival Whistle",  lWhistle (0.05f));
        add (F, "SY Horn Section",      sBrass (1, 1200));
        add (F, "SY Party Strings",     sStrings (0.15f, 3000));
        add (F, "SY Fiesta Supersaw",   sSuper (8, 0.5f, 5000));
        add (F, "PL Marimba Pluck",     pl (7, 0.5f, 2500, 0.15f, 0.2f));
        add (F, "PL Steel Pluck",       pl (11, 0.7f, 2000, 0.2f, 0.22f));
        add (F, "PD Sunrise Pad",       pd (2, 0.7f, 7, 0.35f, 2000, 1.2f, 2.5f, 0.2f));
        add (F, "KY Salsa Piano",       kFM (0.65f, 1.2f, 0.5f));
        add (F, "KY Fiesta Organ",      kOrgan (0.9f, 8000));
        add (F, "CH Big Fiesta Stab",   ch (2, 0, 0.6f, 2500, 0.35f, 0.2f));
        add (F, "CH Brass Stab",        join ({ sBrass (2, 1400), { { "e1_s", 0.0f }, { "e1_d", 0.3f }, { "e1_r", 0.2f } } }));
        add (F, "PC Cowbell Synth",     pc (1, 0.5f, 0.1f, 0.15f, 700));
        add (F, "FX Confetti Riser",    fxRise (8.0f, 10, 0.4f));

        // =====================================================================
        //  SNATCH! STYLE - rough, jackin', distorted
        // =====================================================================
        F = "Snatch Style";
        add (F, "BA Snatch Jack",       bDirty (0.55f, 450));
        add (F, "BA Rough Rubber",      join ({ bRubber (200, 0.6f, 0.15f), dist (1, 0.4f, 0.6f) }));
        add (F, "BA Jackin Organ Bass",      bOrgan (0.55f));
        add (F, "BA Crushed Sub",       join ({ bSub (0.3f, 0.03f), { { "dist_type", 4 }, { "dist_drive", 0.5f }, { "dist_mix", 0.4f } } }));
        add (F, "BA Rough Reese",       bReese (0.4f, 500, 12, 0.6f));
        add (F, "LD Crunch Lead",       join ({ lSquare (0.6f, 3000), dist (1, 0.4f, 0.6f) }));
        add (F, "LD Raw Saw",           join ({ lSaw (3, 0.3f, 4000, 0.02f), dist (2, 0.4f, 0.6f) }));
        add (F, "SY Jack Chop",         sChop (2, 5));
        add (F, "SY Grit Hoover",       join ({ sHoover (0.55f, 3000), dist (1, 0.3f, 0.5f) }));
        add (F, "PL Rough Pluck",       join ({ pl (0, 0.6f, 600, 0.5f, 0.15f), dist (2, 0.4f, 0.5f) }));
        add (F, "KY Dirty Organ",       join ({ kOrgan (0.7f, 5000), dist (2, 0.3f, 0.5f) }));
        add (F, "CH Jack Stab",         ch (3, 4, 0.6f, 3000, 0.25f, 0.2f));
        add (F, "CH Crushed Chord",     join ({ ch (1, 0, 0.5f, 1500, 0.3f, 0.2f), dist (4, 0.45f, 0.5f) }));
        add (F, "AR Grit Arp",          join ({ ar (0, 0.6f, 2000, 0.12f), dist (1, 0.35f, 0.5f) }));
        add (F, "PC Clap Synth",        pcNoise (1500, 0.12f));
        add (F, "FX Noise Blast",       join ({ fxImpact(), { { "noise_level", 0.7f } } }));

        // =====================================================================
        //  TECH HOUSE PRO - production-ready workhorses
        // =====================================================================
        F = "Tech House Pro";
        add (F, "BA TH Rubber Sub", join ({ monoBass (0.02f), subOn (0.6f), punch (1, 0.1f, 0.02f), mod (2, 10, 12, 0.4f), {
            { "a_wt", 0 }, { "a_pos", 0.4f }, { "a_rand", 0 }, { "f_cutoff", 140 }, { "f_res", 0.55f }, { "f_drive", 0.35f },
            { "f_env", 0.45f }, { "e2_d", 0.13f }, { "e2_s", 0.05f } } }));

        add (F, "BA TH Gritty Mid Bass", join ({ monoBass (0.02f), subOn (0.5f), dist (2, 0.45f, 0.7f), {
            { "a_wt", 8 }, { "a_pos", 0.45f }, { "a_rand", 0 }, { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_oct", -1 },
            { "b_level", 0.4f }, { "b_rand", 0 }, { "f_cutoff", 700 }, { "f_res", 0.3f }, { "f_drive", 0.6f }, { "f_env", 0.35f },
            { "e1_s", 0.8f }, { "eq_on", 1 }, { "eq_low", -2 }, { "eq_mid", 3 }, { "eq_midf", 900 } } }));

        add (F, "BA TH FM Bass", join ({ monoBass (0.02f), subOn (0.45f), mod (1, 1, 10, 0.4f), {
            { "a_wt", 0 }, { "a_pos", 0.0f }, { "a_rand", 0 }, { "a_warp", 4 }, { "a_warpamt", 0.5f },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.0f }, { "b_level", 0.0f }, { "b_rand", 0 },
            { "e2_d", 0.15f }, { "e2_s", 0 }, { "f_cutoff", 2000 }, { "f_res", 0.1f }, { "e1_s", 0.8f }, { "e1_r", 0.06f } } }));

        add (F, "PL TH Pluck Classic", join ({ delay (6, 0.35f, 0.22f), reverb (0.55f, 0.2f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 3 }, { "a_det", 0.18f },
            { "f_cutoff", 450 }, { "f_res", 0.4f }, { "f_env", 0.6f }, { "e2_d", 0.2f }, { "e2_s", 0 },
            { "e1_d", 0.35f }, { "e1_s", 0 }, { "e1_r", 0.3f } } }));

        add (F, "CH TH Dub Chord", join ({ delay (6, 0.55f, 0.35f, 0.35f), reverb (0.75f, 0.3f, 0.7f), {
            { "chord", 9 }, { "voices", 16 }, { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 2 }, { "a_det", 0.08f },
            { "f_cutoff", 650 }, { "f_res", 0.35f }, { "f_env", 0.4f }, { "e2_d", 0.25f }, { "e2_s", 0.1f },
            { "e1_d", 0.45f }, { "e1_s", 0 }, { "e1_r", 0.35f }, { "analog", 0.5f } } }));

        add (F, "CH TH Stab Minor7", join ({ reverb (0.6f, 0.25f), delay (6, 0.3f, 0.2f), {
            { "chord", 3 }, { "voices", 16 }, { "a_wt", 5 }, { "a_pos", 0.45f }, { "a_uni", 2 }, { "a_det", 0.12f },
            { "f_type", 3 }, { "f_cutoff", 1600 }, { "f_res", 0.25f }, { "f_env", 0.35f }, { "e2_d", 0.15f },
            { "e1_d", 0.22f }, { "e1_s", 0 }, { "e1_r", 0.2f } } }));

        add (F, "LD TH Vocal Lead", join ({ mod (1, 10, 4, 0.5f), mod (2, 3, 4, 0.15f), dist (2, 0.2f, 0.4f), delay (6, 0.3f, 0.2f),
                                          reverb (0.6f, 0.2f), {
            { "a_wt", 3 }, { "a_pos", 0.5f }, { "l1_sync", 0 }, { "l1_rate", 3.0f },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_level", 0.25f },
            { "f_type", 2 }, { "f_cutoff", 4000 }, { "f_res", 0.2f }, { "mode", 2 }, { "glide", 0.05f },
            { "e1_s", 0.85f }, { "e1_r", 0.2f } } }));

        add (F, "BA TH Acid Line", join ({ legatoBass (0.06f), mod (1, 5, 12, 0.3f), mod (2, 10, 12, 0.5f), dist (2, 0.4f, 0.6f),
                                         delay (3, 0.3f, 0.15f), {
            { "a_wt", 0 }, { "a_pos", 0.75f }, { "a_rand", 0 }, { "f_cutoff", 320 }, { "f_res", 0.82f }, { "f_drive", 0.5f },
            { "f_env", 0.55f }, { "e2_d", 0.18f }, { "e2_s", 0 }, { "e1_d", 0.25f }, { "e1_s", 0.7f }, { "e1_r", 0.05f },
            { "vel", 0.4f } } }));

        add (F, "BA TH Reese", join ({ legatoBass (0.05f), subOn (0.5f), mod (1, 3, 12, 0.1f), dist (2, 0.3f, 0.5f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_uni", 2 }, { "a_det", 0.55f }, { "a_blend", 1.0f }, { "a_width", 0.3f },
            { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.5f }, { "b_uni", 2 }, { "b_det", 0.7f }, { "b_oct", -1 }, { "b_level", 0.55f },
            { "b_width", 0.3f }, { "f_cutoff", 500 }, { "f_res", 0.25f }, { "f_drive", 0.4f }, { "l1_div", 12 }, { "l1_mode", 0 },
            { "e1_s", 1.0f } } }));

        add (F, "KY TH Organ Stab Bright", join ({ chorus (0.35f, 0.4f, 0.7f), comp (-18.0f, 3.0f, 2.0f), reverb (0.5f, 0.15f), {
            { "a_wt", 4 }, { "a_pos", 1.0f }, { "e1_a", 0.001f }, { "e1_d", 0.3f }, { "e1_s", 0.2f }, { "e1_r", 0.15f },
            { "f_type", 2 }, { "f_cutoff", 8000 } } }));

        add (F, "FX TH Noise Riser", join ({ mod (1, 3, 18, 0.5f), reverb (0.9f, 0.45f), delay (6, 0.35f, 0.2f), {
            { "a_on", 0 }, { "noise_on", 1 }, { "noise_type", 0 }, { "noise_level", 0.9f }, { "l1_sync", 0 }, { "l1_rate", 0.4f },
            { "f_type", 3 }, { "f_cutoff", 300 }, { "f_res", 0.45f }, { "f_env", 0.9f }, { "e2_a", 8.0f }, { "e2_s", 1.0f },
            { "e2_r", 3.0f }, { "e1_a", 7.0f }, { "e1_s", 1.0f }, { "e1_r", 3.0f }, { "mode", 1 } } }));

        add (F, "FX TH Downlifter", join ({ mod (1, 1, 1, 0.5f), reverb (0.9f, 0.45f), {
            { "a_wt", 9 }, { "a_pos", 1.0f }, { "a_level", 0.4f }, { "a_uni", 4 }, { "a_det", 0.4f },
            { "noise_on", 1 }, { "noise_level", 0.7f }, { "f_type", 3 }, { "f_cutoff", 400 }, { "f_res", 0.3f }, { "f_env", 0.8f },
            { "e2_a", 0.001f }, { "e2_d", 4.0f }, { "e2_s", 0 }, { "e2_r", 1.0f },
            { "e1_a", 0.01f }, { "e1_d", 4.0f }, { "e1_s", 0 }, { "e1_r", 1.0f }, { "mode", 1 } } }));

        add (F, "PD TH Atmos Pad", join ({ mod (1, 3, 4, 0.4f), reverb (0.95f, 0.45f), delay (9, 0.35f, 0.2f), {
            { "a_wt", 10 }, { "a_pos", 0.3f }, { "a_uni", 6 }, { "a_det", 0.3f }, { "l1_sync", 0 }, { "l1_rate", 0.05f }, { "l1_mode", 0 },
            { "b_on", 1 }, { "b_wt", 3 }, { "b_pos", 0.6f }, { "b_uni", 4 }, { "b_level", 0.4f }, { "b_det", 0.3f },
            { "f_type", 3 }, { "f_cutoff", 1800 }, { "f_res", 0.2f }, { "e1_a", 2.5f }, { "e1_d", 2.0f }, { "e1_s", 0.9f },
            { "e1_r", 4.0f }, { "noise_on", 1 }, { "noise_type", 1 }, { "noise_level", 0.15f },
            { "pha_on", 1 }, { "pha_rate", 0.08f }, { "pha_mix", 0.35f } } }));

        add (F, "LD TH Bell Lead", join ({ delay (6, 0.4f, 0.25f), reverb (0.65f, 0.3f), {
            { "a_wt", 7 }, { "a_pos", 0.85f }, { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.0f }, { "b_oct", 2 }, { "b_level", 0.15f },
            { "e1_a", 0.001f }, { "e1_d", 1.2f }, { "e1_s", 0.1f }, { "e1_r", 0.6f }, { "f_type", 2 }, { "f_cutoff", 6000 },
            { "f_key", 0.3f } } }));

        add (F, "BA TH Sub 808 Glide", join ({ legatoBass (0.12f), punch (1, 0.12f, 0.04f), dist (0, 0.3f, 0.5f, 0.3f), {
            { "a_wt", 0 }, { "a_pos", 0.0f }, { "a_rand", 0 }, { "a_warp", 2 }, { "a_warpamt", 0.15f }, { "f_on", 0 },
            { "e1_a", 0.001f }, { "e1_d", 1.2f }, { "e1_s", 0.5f }, { "e1_r", 0.15f } } }));

        add (F, "PC TH Percussive Zap", join ({ reverb (0.4f, 0.15f), delay (2, 0.3f, 0.15f), {
            { "a_wt", 0 }, { "a_pos", 0.5f }, { "a_rand", 0 }, { "m1_src", 2 }, { "m1_dst", 1 }, { "m1_amt", 1.0f },
            { "e3_a", 0.0005f }, { "e3_d", 0.03f }, { "e3_s", 0 }, { "e1_a", 0.001f }, { "e1_d", 0.1f }, { "e1_s", 0 },
            { "e1_r", 0.08f }, { "f_type", 4 }, { "f_cutoff", 150 } } }));

        add (F, "BA TH Organ Bass", join ({ monoBass (0.015f), subOn (0.55f), comp (-18.0f, 3.0f, 2.0f), {
            { "a_wt", 4 }, { "a_pos", 0.35f }, { "a_rand", 0 }, { "f_cutoff", 900 }, { "f_res", 0.2f }, { "f_env", 0.3f },
            { "e2_d", 0.2f }, { "e1_d", 0.3f }, { "e1_s", 0.6f }, { "e1_r", 0.06f } } }));

        add (F, "AR TH Bleep Sequence", join ({ mod (1, 9, 4, 0.4f), delay (3, 0.5f, 0.3f), reverb (0.5f, 0.2f), {
            { "a_wt", 11 }, { "a_pos", 0.2f }, { "e1_a", 0.001f }, { "e1_d", 0.12f }, { "e1_s", 0 }, { "e1_r", 0.1f },
            { "f_type", 2 }, { "f_cutoff", 5000 } } }));

        add (F, "PD TH Pumping Pad", join ({ reverb (0.7f, 0.3f), {
            { "chord", 3 }, { "voices", 16 }, { "a_wt", 2 }, { "a_pos", 0.6f }, { "a_uni", 5 }, { "a_det", 0.3f },
            { "f_type", 3 }, { "f_cutoff", 2200 }, { "e1_a", 0.4f }, { "e1_s", 0.9f }, { "e1_r", 1.0f },
            { "pump_on", 1 }, { "pump_div", 2 }, { "pump_depth", 0.85f }, { "pump_curve", 0.6f } } }));

        // =====================================================================
        //  Expansion: more sounds for the original label folders
        // =====================================================================
        F = "Moodchild Style";
        add (F, "BA Mood Sub Roll",     bSub (0.2f, 0.04f, 0.12f));
        add (F, "BA Warm Reese",        bReese (0.3f, 350, 12));
        add (F, "BA Groove Fold",       bGrowl (8, 0.3f, 5, 0.15f));
        add (F, "LD Moody Square",      lSquare (0.3f, 2200));
        add (F, "SY Mood Strings",      sStrings (0.4f, 1800));
        add (F, "PL Soft Pluck",        pl (2, 0.4f, 500, 0.3f, 0.25f));
        add (F, "PD Night Drive Pad",   pd (2, 0.4f, 5, 0.3f, 1100, 1.5f, 2.5f, 0.15f));
        add (F, "KY Dusk Keys",         kFM (0.25f, 2.2f));
        add (F, "AR Mood Arp",          ar (11, 0.4f, 3000, 0.15f, 6));
        add (F, "FX Mood Sweep",        fxSweep (0.2f));

        F = "Deeperfect Style";
        add (F, "BA Tunnel Rubber",     bRubber (140, 0.5f, 0.12f, 0.5f));
        add (F, "BA Hypno Acid",        bAcid (240, 0.8f, 0.6f, 0.06f));
        add (F, "BA Dark FM",           bFM (0.45f, 0.15f, 0, -1));
        add (F, "BA Driving Mid",       bDirty (0.45f, 650));
        add (F, "LD Dark Sync",         lSync (0.4f, 0.5f));
        add (F, "SY Warehouse Hoover",  sHoover (0.5f, 2500));
        add (F, "PL Dark Pluck",        pl (5, 0.6f, 450, 0.45f, 0.15f));
        add (F, "PD Abyss Pad",         pd (10, 0.6f, 6, 0.35f, 900, 2.0f, 3.0f, 0.3f));
        add (F, "CH Techno Stab",       ch (10, 0, 0.5f, 800, 0.3f, 0.3f));
        add (F, "FX Deep Impact",       fxImpact());

        F = "Tomango Style";
        add (F, "BA Groove Slap",       bPluck (8, 0.35f, 500, 0.15f, 0.35f));
        add (F, "BA Funky Octave",      join ({ bRubber (250, 0.5f, 0.18f, 0.6f), { { "b_on", 1 }, { "b_wt", 0 }, { "b_pos", 0.6f }, { "b_oct", 1 }, { "b_level", 0.3f } } }));
        add (F, "BA Tomango FM",        bFM (0.5f, 0.12f, 7, 0));
        add (F, "LD Funk Square",       lSquare (0.65f, 3200));
        add (F, "LD Disco Vox",         lVocal (0.5f, 3.0f));
        add (F, "SY Disco Brass",       sBrass (2, 1300));
        add (F, "PL Funk Pluck",        pl (1, 0.6f, 650, 0.45f, 0.16f));
        add (F, "KY Disco Clav",        kClav (0.6f, 1000));
        add (F, "CH Funk Chord",        ch (4, 0, 0.55f, 1600, 0.35f, 0.2f));
        add (F, "AR Party Arp",         ar (0, 0.5f, 3500, 0.12f, 6));

        F = "Toolroom Style";
        add (F, "BA Main Room Rubber",  bRubber (170, 0.6f, 0.16f, 0.55f));
        add (F, "BA Big Reese",         bReese (0.45f, 500, 11, 0.35f));
        add (F, "BA House Organ Bass",  bOrgan (0.3f));
        add (F, "LD Anthem Saw",        lSaw (8, 0.45f, 6500, 0.03f));
        add (F, "SY Big Supersaw",      sSuper (8, 0.5f, 6000));
        add (F, "SY Classic Hoover",    sHoover (0.55f, 4000));
        add (F, "PL Main Room Pluck",   pl (0, 0.5f, 600, 0.4f, 0.2f));
        add (F, "PD Big Room Pad",      pd (0, 0.5f, 7, 0.35f, 2000, 1.0f, 2.5f, 0.2f));
        add (F, "KY 90s House Organ",   kOrgan (0.85f, 6500));
        add (F, "CH Anthem Stab",       join ({ ch (1, 0, 0.55f, 2200, 0.4f, 0.2f), { { "a_uni", 6 }, { "a_det", 0.4f } } }));

        F = "Defected Style";
        add (F, "BA Soulful Sub",       bSub (0.1f, 0.06f, 0.06f));
        add (F, "BA Deep Organ Bass",   bOrgan (0.2f));
        add (F, "BA Garage Rubber",     bRubber (210, 0.45f, 0.2f, 0.45f));
        add (F, "LD Soul Sine",         lWhistle (0.1f));
        add (F, "SY Lush Strings",      sStrings (0.35f, 2600));
        add (F, "PL Deep Pluck",        pl (2, 0.5f, 700, 0.3f, 0.25f));
        add (F, "PD Gospel Pad",        pd (4, 0.6f, 4, 0.25f, 2000, 0.6f, 2.0f, 0.1f));
        add (F, "KY Classic Rhodes",    kFM (0.4f, 2.5f, 0.45f));
        add (F, "KY Gospel Organ",      kOrgan (0.65f, 5500));
        add (F, "CH Soulful Maj7",      ch (4, 2, 0.5f, 1800, 0.6f, 0.2f));

        F = "La Pera Style";
        add (F, "BA Minimal Roll",      bPluck (0, 0.45f, 300, 0.12f, 0.4f));
        add (F, "BA Micro Acid",        bAcid (280, 0.7f, 0.5f, 0.04f, 0.75f));
        add (F, "BA Pera Wobble",       bWobble (2, 320, 0.45f, 3));
        add (F, "LD Micro Whistle",     lWhistle (0.04f));
        add (F, "SY Micro Chop",        sChop (0, 5));
        add (F, "PL Click Pluck",       pl (10, 0.3f, 2500, 0.2f, 0.1f, 3));
        add (F, "KY Minimal Mallet",    kFM (0.55f, 0.6f));
        add (F, "CH Minimal Fifth",     ch (7, 6, 0.5f, 1500, 0.15f, 0.3f));
        add (F, "AR Pera Arp",          ar (9, 0.2f, 3000, 0.1f));
        add (F, "PC Tick Perc",         pc (10, 0.4f, 0.5f, 0.04f, 1500));

        F = "Tech House Pro";
        add (F, "BA TH Bounce Sub",     bSub (0.25f, 0.03f, 0.15f));
        add (F, "BA TH Rolling Pluck Bass",  bPluck (0, 0.55f, 350, 0.14f, 0.45f));
        add (F, "BA TH Wobble",         bWobble (5, 300, 0.55f));
        add (F, "BA TH Growl",          bGrowl (3, 0.4f, 4, 0.4f));
        add (F, "BA TH Dirty Mid",      bDirty (0.6f, 700));
        add (F, "BA TH Organ Pluck Bass",    bOrgan (0.5f));
        add (F, "LD TH Saw Lead",       lSaw (5, 0.35f, 5000, 0.03f));
        add (F, "LD TH Square Hook",    lSquare (0.5f, 3000));
        add (F, "LD TH Whistle",        lWhistle (0.07f));
        add (F, "SY TH Supersaw",       sSuper (7, 0.45f, 5000));
        add (F, "SY TH Hoover",         sHoover (0.55f, 3500));
        add (F, "SY TH Vox Chop",       sChop (2, 5));
        add (F, "PL TH Short Pluck",    pl (0, 0.5f, 400, 0.45f, 0.12f, 3));
        add (F, "PL TH FM Pluck",       pl (7, 0.6f, 2500, 0.2f, 0.2f));
        add (F, "PD TH Warm Pad",       pd (2, 0.5f, 6, 0.3f, 1500, 1.0f, 2.0f, 0.2f));
        add (F, "KY TH Rhodes",         kFM (0.4f, 1.8f));
        add (F, "CH TH Minor 9 Stab",   ch (5, 0, 0.5f, 1200, 0.3f, 0.25f));
        add (F, "AR TH Arp Seq",        ar (0, 0.5f, 2500, 0.12f));
        add (F, "PC TH Rim Shot",       pc (0, 0.5f, 0.7f, 0.05f, 800));
        add (F, "FX TH Riser 8 Bar",    fxRise (8.0f, 9, 0.6f));
        add (F, "FX TH Impact",         fxImpact());

        // v1.2 expansion (generated by tools/generate_expansion.py)
       #include "ExpansionPresets.inc"

        // Level calibration (per folder + name), generated by Tests/RenderTest --calibrate:
        // master volume per preset so everything sits at a similar loudness, peaks < -1 dBFS.
        static const struct { const char* folder; const char* name; float db; } levels[] = {
           #include "PresetLevels.inc"
            { "", "", 0.0f }
        };
        for (auto& preset : b)
            for (auto& L : levels)
                if (preset.folder == L.folder && preset.name == L.name)
                    preset.values.push_back ({ "master", L.db });

        return b;
    }
}

const juce::StringArray& getPresetCategories()
{
    static const juce::StringArray cats { "Bass", "Lead", "Synth", "Pluck", "Pad", "Keys", "Chord", "Arp", "Perc", "FX" };
    return cats;
}

juce::String categoryForPresetName (const juce::String& name)
{
    static const juce::StringArray codes { "BA", "LD", "SY", "PL", "PD", "KY", "CH", "AR", "PC", "FX" };
    const int idx = name.length() > 3 && name[2] == ' ' ? codes.indexOf (name.substring (0, 2)) : -1;
    return idx >= 0 ? getPresetCategories()[idx] : juce::String ("Other");
}

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> bank = []
    {
        auto list = build();
        for (auto& p : list)
            p.category = categoryForPresetName (p.name);
        return list;
    }();
    return bank;
}
