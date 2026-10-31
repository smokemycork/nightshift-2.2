# NightShift — Analog Wavetable Synthesizer (VST3 / AU / Standalone)

A tech-house focused wavetable synth with an analog heart: 2 morphing wavetable oscillators, sub + noise,
a saturating ladder filter, 3 envelopes, 2 LFOs, an 8-slot mod matrix, 4 macros, chord memory and an
8-unit FX rack (incl. a tempo-synced sidechain "Pump") — plus **677 factory presets** across 15 label-style folders + Tech House Pro, all tagged by type.

## Features

| Section | What you get |
|---|---|
| **Oscillators A & B** | 12 band-limited morphing wavetables (16 frames each, mip-mapped = no aliasing), up to 8-voice unison with detune / blend / stereo width, warp modes **Sync, Bend, Mirror, FM** (A↔B), octave/semi/fine, phase + phase randomise |
| **Sub & Noise** | Sine / triangle / square sub (−1 or −2 oct, optional filter bypass), white / pink / vinyl-crackle noise |
| **Analog engine** | Per-voice pitch drift, component-tolerance detune, filter variance and warm mixer saturation (the **ANALOG** knob) |
| **Filter** | Zero-delay-feedback **Ladder 24/12** (Moog-style, self-oscillating, tanh drive) + SVF **LP12/LP24/HP12/HP24/BP/Notch**, drive, key tracking, bipolar Env 2 amount |
| **Modulation** | Amp Env + Env 2 + Env 3 (analog exponential curves), 2 LFOs (7 shapes, tempo sync incl. triplets/dotted, free/retrig, rise time), 8-slot mod matrix with 14 sources → 20 destinations, 4 macros |
| **Voicing** | Poly (1–16 voices) / Mono / Legato, glide, pitch-bend range, velocity sensitivity, sustain pedal, mod wheel, aftertouch, **Chord Memory** (minor, major, m7, maj7, m9, sus4, fifth, octave, dub m7, 4ths) |
| **FX rack** | Distortion (soft / hard / tube / fold / bitcrush), 3-band EQ, Chorus, Phaser, Ping-pong Delay (sync), Reverb, Compressor, **Pump** (sidechain-style ducking locked to host tempo) |
| **UI** | Resizable (60–160%), wavetable / envelope / LFO displays, oscilloscope, on-screen keyboard, preset browser organised by sound type with a label filter, All Presets view and user preset saving |

## Factory presets (677)

The browser is organised **by sound type**: the first menu picks **Bass, Lead, Synth, Pluck, Pad, Keys, Chord, Arp,
Perc or FX** — each folder holds that type from *every* label, with the label shown in brackets
(e.g. `Rolling Sub Groove [Moodchild]`), grouped by label and sorted A–Z. The second menu optionally narrows the list to a
single label. **All Presets** shows everything, **User** shows your saved sounds, and **< >** step through the current list.
(Internally names keep a type code — `BA` Bass, `LD` Lead, `SY` Synth, `PL` Pluck, `PD` Pad, `KY` Keys, `CH` Chord,
`AR` Arp, `PC` Perc, `FX` — which is what your DAW's own preset list shows.)

By type: Bass 213, Lead 82, Pluck 70, Chord 62, Synth 59, Keys 55, Pad 48, Arp 29, FX 29, Perc 29.

| Folder | Presets |
|---|---|
| Moodchild Style | 45 |
| Deeperfect Style | 45 |
| Tomango Style | 45 |
| Toolroom Style | 45 |
| Defected Style | 45 |
| La Pera Style | 45 |
| Hot Creations Style | 45 |
| Solid Grooves Style | 45 |
| Repopulate Mars Style | 45 |
| Dirtybird Style | 45 |
| Elrow Style | 45 |
| Snatch Style | 41 |
| Cuttin Headz Style | 25 |
| Kaluki Style | 25 |
| Desert Hearts Style | 25 |
| Tech House Pro | 65 |

<details><summary><b>Moodchild Style</b> (45)</summary>

BA Rolling Sub Groove, BA Warm Rubber Bass, PL Mood Pluck, KY Late Night Keys, AR Hypno Bleep, PD Velvet Pad, CH Swing Stab, BA Bouncy Mid Bass, LD Wonky Lead, CH Afterglow Chords, BA Mood Sub Roll, BA Warm Reese, BA Groove Fold, LD Moody Square, SY Mood Strings, PL Soft Pluck, PD Night Drive Pad, KY Dusk Keys, AR Mood Arp, FX Mood Sweep, BA Velvet Roller, BA Midnight Sub, BA Hazy Rubber, BA Soft Reese, BA Mellow Acid, BA Cloud Pluck Bass, BA Moody Wobble, BA Warm Fold Growl, LD Dreamy Square, LD Mood Whistle, LD Haze Sync, SY Silk Strings, SY Dusk Chop, PL Pillow Pluck, PL Amber Pluck, PL Rain Pluck, PD Slow Motion Pad, PD Lunar Pad, KY Late Lounge Keys, KY Smoke Organ, CH Melancholy Stab, CH Deep Dub Minor, AR Night Bleeps, PC Soft Tick, FX Mood Riser

</details>

<details><summary><b>Deeperfect Style</b> (45)</summary>

BA Driving Reese, BA Rumble Low End, BA Acid Groove 303, AR Hypnotic Sequence, CH Dark Stab, LD Warehouse Lead, FX Tension Riser, BA Deep Pulse Bass, PC Metal Perc Synth, PD Tunnel Pad, BA Tunnel Rubber, BA Hypno Acid, BA Dark FM, BA Driving Mid, LD Dark Sync, SY Warehouse Hoover, PL Dark Pluck, PD Abyss Pad, CH Techno Stab, FX Deep Impact, BA Hypnotic Roller, BA Steel Sub, BA Black Reese, BA Acid Tunnel, BA Pressure Rubber, BA Driving Pluck Bass, BA Iron Growl, BA Concrete Mid, LD Strobe Lead, LD Industrial Sync, LD Cold Square, SY Machine Hoover, SY Dark Supersaw, PL Metallic Pluck, PL Echo Pluck, PL Shadow Pluck, PD Void Pad, PD Cathedral Drone Pad, KY Dark Organ, KY Steel Mallet Keys, CH Warehouse Stab, CH Minor Pressure, AR Hypno Seq, PC Metal Tick, FX Tunnel Riser

</details>

<details><summary><b>Tomango Style</b> (45)</summary>

BA Funk Slap Bass, LD Talkbox Lead, KY Organ Hit, BA Bubble Bass, PL Groove Pluck, BA Disco Rubber, SY Vowel Chop, CH Bright Brass Stab, KY Sunset Keys, LD Party Lead, BA Groove Slap, BA Funky Octave, BA Tomango FM, LD Funk Square, LD Disco Vox, SY Disco Brass, PL Funk Pluck, KY Disco Clav, CH Funk Chord, AR Party Arp, BA Funk Roller, BA Bubble Slap, BA Groovy Rubber, BA Disco FM, BA Bounce Sub, BA Party Pluck Bass, BA Funky Wobble, BA Wah Growl, LD Funk Whistle, LD Vox Lead, LD Bright Square, SY Funk Brass, SY Talk Chop, PL Disco Pluck, PL Sunny Pluck, PL Tropic Pluck, PD Summer Pad, PD Groove Pad, KY Funk Clav, KY Party Organ, CH Disco Stab, CH Funky Seventh, AR Groove Bleeps, PC Conga Synth, FX Party Riser

</details>

<details><summary><b>Toolroom Style</b> (45)</summary>

BA Rubber Bass Classic, KY Classic House Organ, CH Big Rave Stab, CH Chord Memory Stab, LD Main Room Lead, BA Pitch Drop Bass, PD Uplift Pad, SY Hoover Rave, CH Piano Stab House, CH Pumping Saw Chords, BA Main Room Rubber, BA Big Reese, BA House Organ Bass, LD Anthem Saw, SY Big Supersaw, SY Classic Hoover, PL Main Room Pluck, PD Big Room Pad, KY 90s House Organ, CH Anthem Stab, BA Classic Rubber, BA Big Room Sub, BA Main Stage Reese, BA Acid Anthem, BA Tool FM, BA Peak Time Pluck Bass, BA Rave Wobble, BA Stadium Mid, LD Stadium Saw, LD Peak Square, LD Rave Sync, SY Arena Supersaw, SY Mega Hoover, PL Big Room Pluck, PL Peak Pluck, PL Euphoria Pluck, PD Anthem Pad, PD Hands Up Pad, KY Classic M1 Organ, KY Piano Anthem Keys, CH Rave Minor Stab, CH Big Major Stab, AR Festival Arp, PC Stadium Zap, FX Build Riser

</details>

<details><summary><b>Defected Style</b> (45)</summary>

CH Deep House Chords, BA Organ Bass, KY Soul Keys, SY Disco Strings, PD Warm Analog Pad, BA Garage Sub, CH Jackin Stab, PL Sunrise Pluck, LD Smooth Lead, KY Funky Clav, BA Soulful Sub, BA Deep Organ Bass, BA Garage Rubber, LD Soul Sine, SY Lush Strings, PL Deep Pluck, PD Gospel Pad, KY Classic Rhodes, KY Gospel Organ, CH Soulful Maj7, BA Soul Rubber, BA Deep Garage Sub, BA Smooth Reese, BA Gospel Organ Bass, BA Jazzy FM, BA Warm Pluck Bass, BA Jack Wobble, BA Classic Mid, LD Soul Whistle, LD Velvet Lead, LD Disco Square, SY Philly Strings, SY Soul Brass, PL Sunday Pluck, PL Soul Pluck, PL Glow Pluck, PD Soulful Pad, PD Warm Choir Pad, KY Vintage Rhodes, KY Church Organ, CH Deep Maj7 Stab, CH Classic House Chord, AR Soul Arp, PC Shaker Synth, FX Soul Sweep

</details>

<details><summary><b>La Pera Style</b> (45)</summary>

AR Minimal Bleep, BA Rolling Mono Bass, PC Quirky Perc Synth, PL Glitch Pluck, BA Wobble Groove, CH Micro Stab, PC Shaker Noise Synth, SY Bouncing Sync, KY Deep Mallet, BA Hypno Arp Bass, BA Minimal Roll, BA Micro Acid, BA Pera Wobble, LD Micro Whistle, SY Micro Chop, PL Click Pluck, KY Minimal Mallet, CH Minimal Fifth, AR Pera Arp, PC Tick Perc, BA Micro Roller, BA Pera Sub, BA Click Pluck Bass, BA Minimal Acid, BA Tiny FM, BA Dot Rubber, BA Micro Wobble, BA Tiny Growl, LD Tiny Whistle, LD Micro Square, LD Glitch Sync, SY Dot Chop, SY Micro Strings, PL Pixel Pluck, PL Drop Pluck, PL Woodblock Pluck, PD Minimal Air Pad, PD Sparse Pad, KY Micro Keys, KY Toy Organ, CH Minimal Stab, CH Micro Fourths, AR Micro Seq, PC Click Perc, FX Micro Sweep

</details>

<details><summary><b>Hot Creations Style</b> (45)</summary>

BA Hot Rubber, BA Paradise Sub, BA Jones Groove, BA Wonky Reese, BA Bleep Bass, BA Rolling Fold, LD Quirky Whistle, LD Hot Square, LD Vowel Talk, SY Retro Chop, SY Sunday Strings, PL Ibiza Pluck, PL Glass Pluck, PD Afterparty Pad, KY Warm Rhodes, CH Hot Stab, CH Deep Minor 9, AR Bleep Arp, PC Wood Perc, FX Space Riser, BA Paradise Roller, BA Ibiza Rubber, BA Hot Acid, BA Sunset Sub, BA Jones FM, BA Bleep Pluck Bass, BA Quirk Wobble, BA Hot Growl, LD Paradise Whistle, LD Retro Square, LD Quirk Sync, SY Hot Chop, SY Terrace Strings, PL Terrace Pluck, PL Sun Pluck, PL Bleep Pluck, PD Balearic Pad, PD Island Pad, KY Lounge Rhodes, KY Retro Organ, CH Hot Minor 7, CH Paradise Chord, AR Hot Bleeps, PC Bongo Synth, FX Terrace Riser

</details>

<details><summary><b>Solid Grooves Style</b> (45)</summary>

BA Solid Roller, BA Rumble Grit, BA Bounce Pulse, BA Grit Mid, BA Low Stomp, BA Night Reese, BA Rolling Acid, LD Hook Square, LD Sync Cutter, SY Vocal Chop Grid, SY Hoover Groove, PL Tight Pluck, PL Hollow Pluck, PD Dark Room Pad, KY Grime Organ, CH Rolling Stab, CH Solid Minor, AR Groove Arp, PC Rim Synth, FX Grit Downer, BA Groove Roller, BA Punch Sub, BA Solid Reese, BA Grit Acid, BA Stomp FM, BA Tight Pluck Bass, BA Roll Wobble, BA Raw Mid, LD Solid Hook, LD Grit Sync, LD Pulse Square, SY Grid Chop, SY Solid Hoover, PL Punch Pluck, PL Rim Pluck, PL Snap Pluck, PD Shadow Groove Pad, PD Basement Pad, KY Raw Organ, KY Groove Clav, CH Solid Dub Stab, CH Rolling Minor, AR Solid Seq, PC Clave Synth, FX Groove Downer

</details>

<details><summary><b>Repopulate Mars Style</b> (45)</summary>

BA Mars Bounce, BA Funk Rubber, BA Retro Organ, BA Disco Octave, BA Slap FM, BA Bounce Wobble, LD Rave Square, LD Mars Lead, LD Electro Sync, SY Rave Hoover, SY Brass Section, PL Bouncy Pluck, PD Retro Pad, KY Piano House, KY House Organ, CH Rave Stab, CH Major Lift, AR Mars Arp, PC Laser Zap, FX Rave Riser, BA Mars Roller, BA Rocket Rubber, BA Retro Acid, BA Bounce FM, BA Funk Sub, BA Orbit Pluck Bass, BA Space Wobble, BA Retro Organ Bass, LD Mars Square, LD Rocket Sync, LD Retro Whistle, SY Mars Brass, SY Rave Strings, PL Rocket Pluck, PL Retro Pluck, PL Disco Ball Pluck, PD Space Pad, PD Orbit Pad, KY Mars Piano, KY Rave Organ, CH Mars Stab, CH Retro Major, AR Orbit Arp, PC Space Zap, FX Launch Riser

</details>

<details><summary><b>Dirtybird Style</b> (45)</summary>

BA Bird Wobble, BA Ghetto Growl, BA Wonk Bass, BA Dirty Saw, BA Booty Sub, BA Squelch, BA Fold Monster, LD Bird Whistle, LD Weird Sync, LD Talkbox Bird, SY Squish Chop, SY Laser Synth, PL Bounce Pluck, PD Swamp Pad, KY Wonky Keys, CH Bird Stab, AR Quirk Arp, PC Bird Chirp, PC Snap Synth, FX Bird Siren, BA Bird Roller, BA Booty Rubber, BA Wonk Acid, BA Dirty FM, BA Swamp Sub, BA Chirp Pluck Bass, BA Wobble Bird, BA Gutter Growl, LD Wonky Whistle, LD Freak Sync, LD Squeak Square, SY Freak Chop, SY Goofy Hoover, PL Bird Pluck, PL Wonk Pluck, PL Squeak Pluck, PD Weird Pad, PD Bayou Pad, KY Funky Bird Keys, KY Bird Organ, CH Wonk Stab, CH Freak Fifth, AR Bird Seq, PC Beak Perc, FX Bird Riser

</details>

<details><summary><b>Elrow Style</b> (45)</summary>

BA Fiesta Bass, BA Carnival Pluck, BA Party Rubber, BA Latin Sub, BA Big Room Reese, LD Trumpet Lead, LD Fiesta Saw, LD Carnival Whistle, SY Horn Section, SY Party Strings, SY Fiesta Supersaw, PL Marimba Pluck, PL Steel Pluck, PD Sunrise Pad, KY Salsa Piano, KY Fiesta Organ, CH Big Fiesta Stab, CH Brass Stab, PC Cowbell Synth, FX Confetti Riser, BA Carnival Roller, BA Samba Rubber, BA Fiesta Acid, BA Mariachi FM, BA Party Sub, BA Conga Pluck Bass, BA Parade Wobble, BA Festival Mid, LD Mariachi Lead, LD Fiesta Square, LD Party Whistle, SY Fiesta Brass, SY Carnival Strings, PL Salsa Pluck, PL Tropical Pluck, PL Steel Drum Pluck, PD Fiesta Pad, PD Golden Pad, KY Latin Piano, KY Carnival Organ, CH Fiesta Chord, CH Samba Stab, AR Carnival Arp, PC Timbale Synth, FX Fiesta Riser

</details>

<details><summary><b>Snatch Style</b> (41)</summary>

BA Snatch Jack, BA Rough Rubber, BA Jackin Organ, BA Crushed Sub, BA Rough Reese, LD Crunch Lead, LD Raw Saw, SY Jack Chop, SY Grit Hoover, PL Rough Pluck, KY Dirty Organ, CH Jack Stab, CH Crushed Chord, AR Grit Arp, PC Clap Synth, FX Noise Blast, BA Jack Roller, BA Grit Rubber, BA Raw Acid, BA Crunch FM, BA Dirty Sub, BA Rough Pluck Bass, BA Jack Wobble, BA Snatch Mid, LD Grit Whistle, LD Raw Square, LD Crunch Sync, SY Raw Brass, SY Grit Chop, PL Jack Pluck, PL Grit Pluck, PL Raw Pluck, PD Grime Pad, PD Dirty Pad, KY Jack Organ, KY Grit Keys, CH Raw Stab, CH Jack Chord, AR Raw Seq, PC Snatch Clap, FX Grit Riser

</details>

<details><summary><b>Cuttin Headz Style</b> (25)</summary>

BA Headz Roller, BA Bronx Rubber, BA Hypno Acid Roll, BA Raw FM, BA Deep Stomp Sub, BA Hustle Pluck Bass, BA Headz Wobble, BA Bodega Mid, LD Headz Whistle, LD Hustle Square, LD Bronx Sync, SY Headz Chop, SY Midnight Brass, PL Street Pluck, PL Hustle Pluck, PL Bodega Pluck, PD Late Night Pad, PD Smoke Pad, KY Headz Rhodes, KY Bronx Organ, CH Headz Stab, CH Hustle Minor, AR Headz Seq, PC Bodega Cowbell, FX Headz Riser

</details>

<details><summary><b>Kaluki Style</b> (25)</summary>

BA Kaluki Roller, BA Chunky Rubber, BA Rolling Acid Line, BA Warm FM, BA Deep Kaluki Sub, BA Chunk Pluck Bass, BA Groove Wobble, BA Kaluki Growl, LD Kaluki Square, LD Warm Whistle, LD Rolling Sync, SY Kaluki Strings, SY Chunk Chop, PL Chunky Pluck, PL Warm Pluck, PL Roll Pluck, PD Kaluki Pad, PD Golden Hour Pad, KY Kaluki Keys, KY Warm Organ, CH Kaluki Stab, CH Chunky Minor 9, AR Kaluki Arp, PC Kaluki Rim, FX Kaluki Riser

</details>

<details><summary><b>Desert Hearts Style</b> (25)</summary>

BA Desert Roller, BA Sand Rubber, BA Psy Acid, BA Mirage FM, BA Dune Sub, BA Cactus Pluck Bass, BA Trippy Wobble, BA Desert Growl, LD Mirage Lead, LD Desert Whistle, LD Psy Sync, SY Mirage Chop, SY Sunrise Strings, PL Desert Pluck, PL Mirage Pluck, PL Star Pluck, PD Desert Night Pad, PD Mirage Pad, KY Desert Keys, KY Psy Organ, CH Desert Stab, CH Mirage Minor, AR Psy Arp, PC Sand Perc, FX Mirage Riser

</details>

<details><summary><b>Tech House Pro</b> (65)</summary>

BA TH Rubber Sub, BA TH Gritty Mid Bass, BA TH FM Bass, PL TH Pluck Classic, CH TH Dub Chord, CH TH Stab Minor7, LD TH Vocal Lead, BA TH Acid Line, BA TH Reese, KY TH Organ Stab Bright, FX TH Noise Riser, FX TH Downlifter, PD TH Atmos Pad, LD TH Bell Lead, BA TH Sub 808 Glide, PC TH Percussive Zap, BA TH Organ Bass, AR TH Bleep Sequence, PD TH Pumping Pad, BA TH Bounce Sub, BA TH Rolling Pluck, BA TH Wobble, BA TH Growl, BA TH Dirty Mid, BA TH Organ Pluck, LD TH Saw Lead, LD TH Square Hook, LD TH Whistle, SY TH Supersaw, SY TH Hoover, SY TH Vox Chop, PL TH Short Pluck, PL TH FM Pluck, PD TH Warm Pad, KY TH Rhodes, CH TH Minor 9 Stab, AR TH Arp Seq, PC TH Rim Shot, FX TH Riser 8 Bar, FX TH Impact, BA TH Roller, BA TH Punch Sub, BA TH Sub Reese, BA TH Acid Roll, BA TH FM Pluck Bass, BA TH Groove Rubber, BA TH Halftime Wobble, BA TH Mid Growl, LD TH Vox Hook, LD TH Sync Lead, LD TH Pulse Lead, SY TH Brass Synth, SY TH Strings, PL TH Echo Pluck, PL TH Bright Pluck, PL TH Soft Pluck, PD TH Breakdown Pad, PD TH Air Pad, KY TH Organ Hit, KY TH Piano Keys, CH TH Dub Minor, CH TH Maj7 Chord, AR TH Seq Bleep, PC TH Hat Synth, FX TH Sweep Up Riser

</details>

On the archetype-based presets, **Macro 1 = Tone** (filter cutoff), so you can open up any sound with one knob.

> Preset folders are named after the *styles* these labels are known for, as a sound-design reference.
> NightShift is not affiliated with or endorsed by any of these labels.

All presets are loudness-matched (measured automatically by the test suite) so you can browse without volume jumps.
Your own presets save to **Documents/NightShift/User Presets** and appear in the **User** folder.

---

## Build the installers on GitHub (no coding tools needed on your computer)

GitHub's free build servers compile the synth for **Windows and Mac** and hand you ready-to-run installers.

### 1. Create the repository
1. Sign in at **github.com** (free account is fine).
2. Click **+ → New repository**, name it `NightShift`, choose **Public**
   (public repos get unlimited free build minutes; private repos are limited and Mac minutes count 10×), click **Create repository**.

### 2. Upload the project
1. Unzip `NightShift.zip` on your computer.
2. On the new repo page click **uploading an existing file**, then drag **everything inside** the `NightShift` folder
   (`Source`, `Tests`, `installer`, `.github`, `CMakeLists.txt`, `README.md`, `.gitignore`) into the browser.
3. Click **Commit changes**.

> ⚠️ **The `.github` folder is hidden on Mac/Windows by default** and is easy to miss. Check that your repo shows
> `.github/workflows/build.yml`. If it doesn't: in the repo click **Add file → Create new file**, type
> `.github/workflows/build.yml` as the name, paste the contents of that file from the zip, and commit.
> (Mac: press **Cmd + Shift + .** in Finder to show hidden files.)

### 3. Let it build
1. Open the **Actions** tab. A run called **Build NightShift Installers** starts automatically
   (if Actions asks to be enabled, click the green button). You can also start it with **Run workflow**.
2. Wait ~10–20 minutes until all jobs are green ✅.
3. Click the finished run and scroll to **Artifacts**:
   - `NightShift-Windows-Installer` → `NightShift-1.3.0-Windows-Setup.exe`
   - `NightShift-macOS-Installer` → `NightShift-1.3.0-macOS.pkg` (Universal: Apple Silicon + Intel)
   - `NightShift-Linux-VST3` (bonus)

### 4. (Optional) Publish a proper Release page
Create a tag starting with `v`: **Releases → Draft a new release → Choose a tag → type `v1.0.0` → Create new tag → Publish**.
The workflow builds again and attaches both installers to that release automatically.

---

## Installing

**Upgrading from an earlier version?** Just run the new installer — no need to uninstall. It replaces the plugin in place, your projects keep working, and your saved User presets are untouched.


**Windows:** run the `Setup.exe`. If SmartScreen appears (the installer isn't code-signed), click **More info → Run anyway**.
The VST3 goes to `C:\Program Files\Common Files\VST3`.

**macOS:** double-click the `.pkg`. Because it isn't notarized by Apple, macOS may block it the first time:
open **System Settings → Privacy & Security** and click **Open Anyway** (or right-click the pkg → **Open**).
Installs VST3, AU and the standalone app.

**Ableton Live:** Preferences → **Plug-ins** → turn on **Use VST3 Plug-in System Folders** (Mac: also *Use Audio Units v2*)
→ **Rescan**. NightShift appears under *Plug-ins → NightShift Audio*.

## Quick tips
- **Macro 1** is the TONE (filter) knob on all the newer presets and several originals — automate it for builds, or map any macro yourself in the MOD MATRIX tab.
- **Pump** (FX page) gives instant sidechain ducking locked to your project tempo; works best while Live is playing.
- **Chord Memory** + **Delay 1/8D** + **Reverb** = instant dub-chord stabs.
- Double-click any knob to reset it. Drag the window corner to resize.

## Building locally (developers)
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
JUCE 8 is downloaded automatically. Run the headless test suite with `-DNS_BUILD_TESTS=ON` then
`build/NightShiftTests_artefacts/Release/NightShiftTests` (renders every preset and checks for NaNs, silence,
level, CPU, random-parameter fuzz and state save/restore). Add `--wav` to render every preset to WAV files.

## License note
NightShift is built with the [JUCE framework](https://juce.com). JUCE is dual-licensed (AGPLv3 or a commercial licence,
with a free tier for small developers). Personal use is fine; if you plan to **sell or distribute** NightShift,
check JUCE's licence terms first.
