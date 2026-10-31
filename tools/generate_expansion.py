#!/usr/bin/env python3
"""
Generates Source/ExpansionPresets.inc - the v1.2 preset expansion (25 new presets per folder).

Each preset is a named recipe: the words in its name pick the sound archetype (Reese, Acid,
Wobble, Whistle, Strings, Organ ...), and each label folder has a style profile (brightness,
resonance, grit, analog drift) that shapes the parameters.  Values are deterministic per
preset name, so re-running the script always produces the same bank.

Run from the repo root:   python3 tools/generate_expansion.py
"""
import hashlib, os, random, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# folder -> (brightness multiplier, resonance offset, grit chance, analog amount)
PROFILES = {
    "Moodchild Style":        (0.85, 0.00, 0.10, 0.45),
    "Deeperfect Style":       (0.80, 0.05, 0.40, 0.40),
    "Tomango Style":          (1.15, 0.00, 0.15, 0.35),
    "Toolroom Style":         (1.20, 0.00, 0.20, 0.30),
    "Defected Style":         (1.00, -0.03, 0.05, 0.50),
    "La Pera Style":          (1.00, 0.02, 0.10, 0.35),
    "Hot Creations Style":    (1.00, 0.00, 0.10, 0.45),
    "Solid Grooves Style":    (0.90, 0.03, 0.40, 0.35),
    "Repopulate Mars Style":  (1.15, 0.00, 0.20, 0.35),
    "Dirtybird Style":        (1.00, 0.06, 0.30, 0.40),
    "Elrow Style":            (1.25, 0.00, 0.10, 0.30),
    "Snatch Style":           (1.00, 0.04, 0.70, 0.40),
    "Cuttin Headz Style":     (0.90, 0.03, 0.30, 0.40),
    "Kaluki Style":           (0.95, 0.00, 0.20, 0.50),
    "Desert Hearts Style":    (0.90, 0.02, 0.15, 0.55),
    "Tech House Pro":         (1.00, 0.00, 0.20, 0.35),
}

# 25 per folder: 8 BA, 3 LD, 2 SY, 3 PL, 2 PD, 2 KY, 2 CH, 1 AR, 1 PC, 1 FX
LAYOUT = ["BA"] * 8 + ["LD"] * 3 + ["SY"] * 2 + ["PL"] * 3 + ["PD"] * 2 + ["KY"] * 2 + ["CH"] * 2 + ["AR", "PC", "FX"]

NAMES = {
    "Moodchild Style": ["Velvet Roller", "Midnight Sub", "Hazy Rubber", "Soft Reese", "Mellow Acid", "Cloud Pluck Bass", "Moody Wobble", "Warm Fold Growl",
                        "Dreamy Square", "Mood Whistle", "Haze Sync", "Silk Strings", "Dusk Chop",
                        "Pillow Pluck", "Amber Pluck", "Rain Pluck", "Slow Motion Pad", "Lunar Pad", "Late Lounge Keys", "Smoke Organ",
                        "Melancholy Stab", "Deep Dub Minor", "Night Bleeps", "Soft Tick", "Mood Riser"],
    "Deeperfect Style": ["Hypnotic Roller", "Steel Sub", "Black Reese", "Acid Tunnel", "Pressure Rubber", "Driving Pluck Bass", "Iron Growl", "Concrete Mid",
                         "Strobe Lead", "Industrial Sync", "Cold Square", "Machine Hoover", "Dark Supersaw",
                         "Metallic Pluck", "Echo Pluck", "Shadow Pluck", "Void Pad", "Cathedral Drone Pad", "Dark Organ", "Steel Mallet Keys",
                         "Warehouse Stab", "Minor Pressure", "Hypno Seq", "Metal Tick", "Tunnel Riser"],
    "Tomango Style": ["Funk Roller", "Bubble Slap", "Groovy Rubber", "Disco FM", "Bounce Sub", "Party Pluck Bass", "Funky Wobble", "Wah Growl",
                      "Funk Whistle", "Vox Lead", "Bright Square", "Funk Brass", "Talk Chop",
                      "Disco Pluck", "Sunny Pluck", "Tropic Pluck", "Summer Pad", "Groove Pad", "Funk Clav", "Party Organ",
                      "Disco Stab", "Funky Seventh", "Groove Bleeps", "Conga Synth", "Party Riser"],
    "Toolroom Style": ["Classic Rubber", "Big Room Sub", "Main Stage Reese", "Acid Anthem", "Tool FM", "Peak Time Pluck Bass", "Rave Wobble", "Stadium Mid",
                       "Stadium Saw", "Peak Square", "Rave Sync", "Arena Supersaw", "Mega Hoover",
                       "Big Room Pluck", "Peak Pluck", "Euphoria Pluck", "Anthem Pad", "Hands Up Pad", "Classic M1 Organ", "Piano Anthem Keys",
                       "Rave Minor Stab", "Big Major Stab", "Festival Arp", "Stadium Zap", "Build Riser"],
    "Defected Style": ["Soul Rubber", "Deep Garage Sub", "Smooth Reese", "Gospel Organ Bass", "Jazzy FM", "Warm Pluck Bass", "Jack Wobble", "Classic Mid",
                       "Soul Whistle", "Velvet Lead", "Disco Square", "Philly Strings", "Soul Brass",
                       "Sunday Pluck", "Soul Pluck", "Glow Pluck", "Soulful Pad", "Warm Choir Pad", "Vintage Rhodes", "Church Organ",
                       "Deep Maj7 Stab", "Classic House Chord", "Soul Arp", "Shaker Synth", "Soul Sweep"],
    "La Pera Style": ["Micro Roller", "Pera Sub", "Click Pluck Bass", "Minimal Acid", "Tiny FM", "Dot Rubber", "Micro Wobble", "Tiny Growl",
                      "Tiny Whistle", "Micro Square", "Glitch Sync", "Dot Chop", "Micro Strings",
                      "Pixel Pluck", "Drop Pluck", "Woodblock Pluck", "Minimal Air Pad", "Sparse Pad", "Micro Keys", "Toy Organ",
                      "Minimal Stab", "Micro Fourths", "Micro Seq", "Click Perc", "Micro Sweep"],
    "Hot Creations Style": ["Paradise Roller", "Ibiza Rubber", "Hot Acid", "Sunset Sub", "Jones FM", "Bleep Pluck Bass", "Quirk Wobble", "Hot Growl",
                            "Paradise Whistle", "Retro Square", "Quirk Sync", "Hot Chop", "Terrace Strings",
                            "Terrace Pluck", "Sun Pluck", "Bleep Pluck", "Balearic Pad", "Island Pad", "Lounge Rhodes", "Retro Organ",
                            "Hot Minor 7", "Paradise Chord", "Hot Bleeps", "Bongo Synth", "Terrace Riser"],
    "Solid Grooves Style": ["Groove Roller", "Punch Sub", "Solid Reese", "Grit Acid", "Stomp FM", "Tight Pluck Bass", "Roll Wobble", "Raw Mid",
                            "Solid Hook", "Grit Sync", "Pulse Square", "Grid Chop", "Solid Hoover",
                            "Punch Pluck", "Rim Pluck", "Snap Pluck", "Shadow Groove Pad", "Basement Pad", "Raw Organ", "Groove Clav",
                            "Solid Dub Stab", "Rolling Minor", "Solid Seq", "Clave Synth", "Groove Downer"],
    "Repopulate Mars Style": ["Mars Roller", "Rocket Rubber", "Retro Acid", "Bounce FM", "Funk Sub", "Orbit Pluck Bass", "Space Wobble", "Retro Organ Bass",
                              "Mars Square", "Rocket Sync", "Retro Whistle", "Mars Brass", "Rave Strings",
                              "Rocket Pluck", "Retro Pluck", "Disco Ball Pluck", "Space Pad", "Orbit Pad", "Mars Piano", "Rave Organ",
                              "Mars Stab", "Retro Major", "Orbit Arp", "Space Zap", "Launch Riser"],
    "Dirtybird Style": ["Bird Roller", "Booty Rubber", "Wonk Acid", "Dirty FM", "Swamp Sub", "Chirp Pluck Bass", "Wobble Bird", "Gutter Growl",
                        "Wonky Whistle", "Freak Sync", "Squeak Square", "Freak Chop", "Goofy Hoover",
                        "Bird Pluck", "Wonk Pluck", "Squeak Pluck", "Weird Pad", "Bayou Pad", "Funky Bird Keys", "Bird Organ",
                        "Wonk Stab", "Freak Fifth", "Bird Seq", "Beak Perc", "Bird Riser"],
    "Elrow Style": ["Carnival Roller", "Samba Rubber", "Fiesta Acid", "Mariachi FM", "Party Sub", "Conga Pluck Bass", "Parade Wobble", "Festival Mid",
                    "Mariachi Lead", "Fiesta Square", "Party Whistle", "Fiesta Brass", "Carnival Strings",
                    "Salsa Pluck", "Tropical Pluck", "Steel Drum Pluck", "Fiesta Pad", "Golden Pad", "Latin Piano", "Carnival Organ",
                    "Fiesta Chord", "Samba Stab", "Carnival Arp", "Timbale Synth", "Fiesta Riser"],
    "Snatch Style": ["Jack Roller", "Grit Rubber", "Raw Acid", "Crunch FM", "Dirty Sub", "Rough Pluck Bass", "Jack Wobble", "Snatch Mid",
                     "Grit Whistle", "Raw Square", "Crunch Sync", "Raw Brass", "Grit Chop",
                     "Jack Pluck", "Grit Pluck", "Raw Pluck", "Grime Pad", "Dirty Pad", "Jack Organ", "Grit Keys",
                     "Raw Stab", "Jack Chord", "Raw Seq", "Snatch Clap", "Grit Riser"],
    "Cuttin Headz Style": ["Headz Roller", "Bronx Rubber", "Hypno Acid Roll", "Raw FM", "Deep Stomp Sub", "Hustle Pluck Bass", "Headz Wobble", "Bodega Mid",
                           "Headz Whistle", "Hustle Square", "Bronx Sync", "Headz Chop", "Midnight Brass",
                           "Street Pluck", "Hustle Pluck", "Bodega Pluck", "Late Night Pad", "Smoke Pad", "Headz Rhodes", "Bronx Organ",
                           "Headz Stab", "Hustle Minor", "Headz Seq", "Bodega Cowbell", "Headz Riser"],
    "Kaluki Style": ["Kaluki Roller", "Chunky Rubber", "Rolling Acid Line", "Warm FM", "Deep Kaluki Sub", "Chunk Pluck Bass", "Groove Wobble", "Kaluki Growl",
                     "Kaluki Square", "Warm Whistle", "Rolling Sync", "Kaluki Strings", "Chunk Chop",
                     "Chunky Pluck", "Warm Pluck", "Roll Pluck", "Kaluki Pad", "Golden Hour Pad", "Kaluki Keys", "Warm Organ",
                     "Kaluki Stab", "Chunky Minor 9", "Kaluki Arp", "Kaluki Rim", "Kaluki Riser"],
    "Desert Hearts Style": ["Desert Roller", "Sand Rubber", "Psy Acid", "Mirage FM", "Dune Sub", "Cactus Pluck Bass", "Trippy Wobble", "Desert Growl",
                            "Mirage Lead", "Desert Whistle", "Psy Sync", "Mirage Chop", "Sunrise Strings",
                            "Desert Pluck", "Mirage Pluck", "Star Pluck", "Desert Night Pad", "Mirage Pad", "Desert Keys", "Psy Organ",
                            "Desert Stab", "Mirage Minor", "Psy Arp", "Sand Perc", "Mirage Riser"],
    "Tech House Pro": ["TH Roller", "TH Punch Sub", "TH Sub Reese", "TH Acid Roll", "TH FM Pluck Bass", "TH Groove Rubber", "TH Halftime Wobble", "TH Mid Growl",
                       "TH Vox Hook", "TH Sync Lead", "TH Pulse Lead", "TH Brass Synth", "TH Strings",
                       "TH Echo Pluck", "TH Bright Pluck", "TH Soft Pluck", "TH Breakdown Pad", "TH Air Pad", "TH Organ Hit", "TH Piano Keys",
                       "TH Dub Minor", "TH Maj7 Chord", "TH Seq Bleep", "TH Hat Synth", "TH Sweep Up Riser"],
}


def f(x):
    return f"{x:.3f}f".replace(".000f", ".0f")


def has(name, *words):
    return any(w.lower() in name.lower() for w in words)


def recipe(code, name, prof, rng):
    bright, resOff, grit, analog = prof
    u = rng.uniform
    c = rng.choice
    R = lambda lo, hi: min(0.92, max(0.0, u(lo, hi) + resOff))
    extra = [("analog", analog)]
    gritty = rng.random() < grit

    if code == "BA":
        if has(name, "Reese"):
            call = f"bReese ({f(u(0.3, 0.55))}, {f(u(300, 650) * bright)}, {c([11, 12, 13])}, {f(u(0.2, 0.6))})"
        elif has(name, "Acid", "Squelch"):
            call = f"bAcid ({f(u(200, 380) * bright)}, {f(R(0.65, 0.85))}, {f(u(0.45, 0.7))}, {f(u(0.04, 0.1))}, {f(c([0.5, 0.75]))})"
        elif has(name, "FM"):
            call = f"bFM ({f(u(0.3, 0.65))}, {f(u(0.08, 0.18))}, {c([0, 0, 7, 12])}, {c([0, 0, 1, -1])})"
        elif has(name, "Sub"):
            call = f"bSub ({f(u(0.05, 0.35))}, {f(u(0.02, 0.1))}, {f(u(0.05, 0.18))})"
        elif has(name, "Pluck Bass", "Slap"):
            call = f"bPluck ({c([0, 1, 7, 8])}, {f(u(0.3, 0.75))}, {f(u(280, 600) * bright)}, {f(u(0.1, 0.22))}, {f(R(0.3, 0.5))})"
        elif has(name, "Organ"):
            call = f"bOrgan ({f(u(0.2, 0.6))})"
        elif has(name, "Wobble"):
            call = f"bWobble ({c([2, 4, 5, 7])}, {f(u(250, 380) * bright)}, {f(R(0.4, 0.65))}, {c([0, 1, 3])})"
        elif has(name, "Growl", "Fold"):
            call = f"bGrowl ({c([3, 8, 10])}, {f(u(0.2, 0.6))}, {c([2, 4, 5])}, {f(u(0.15, 0.5))})"
        elif has(name, "Mid"):
            call = f"bDirty ({f(u(0.4, 0.7))}, {f(u(400, 850) * bright)}, {c([0, 8, 9])}, {f(u(0.4, 0.65))})"
        else:  # Roller / Rubber
            call = f"bRubber ({f(u(120, 260) * bright)}, {f(R(0.4, 0.7))}, {f(u(0.1, 0.22))}, {f(c([0.4, 0.5, 0.6, 0.65]))}, {c([0, 0, 2])})"
        if gritty and not has(name, "Mid", "Reese", "Acid", "Growl"):
            extra += [("dist_on", 1), ("dist_type", 2), ("dist_drive", round(u(0.2, 0.45), 3)), ("dist_mix", 0.5)]

    elif code == "LD":
        if has(name, "Whistle"):
            call = f"lWhistle ({f(u(0.04, 0.12))})"
        elif has(name, "Sync"):
            call = f"lSync ({f(u(0.2, 0.6))}, {f(u(0.3, 0.7))})"
        elif has(name, "Square", "Pulse"):
            call = f"lSquare ({f(u(0.3, 0.75))}, {f(u(2200, 3800) * bright)})"
        elif has(name, "Vox", "Vocal", "Talk"):
            call = f"lVocal ({f(u(0.2, 0.7))}, {f(u(2.0, 4.5))})"
        elif has(name, "Mariachi", "Trumpet"):
            call = f"lSaw (1, 0.0f, {f(u(2200, 3000))}, 0.03f)"
            extra += [("f_type", 2), ("e1_a", 0.03), ("f_env", 0.3), ("e2_a", 0.03), ("e2_d", 0.3), ("e2_s", 0.5)]
        else:
            call = f"lSaw ({rng.randint(3, 8)}, {f(u(0.25, 0.5))}, {f(u(3000, 6500) * bright)}, {f(u(0.02, 0.06))})"
        if gritty:
            extra += [("dist_on", 1), ("dist_type", 2), ("dist_drive", round(u(0.2, 0.4), 3)), ("dist_mix", 0.5)]

    elif code == "SY":
        if has(name, "Strings"):
            call = f"sStrings ({f(u(0.1, 0.4))}, {f(u(1800, 3000) * bright)})"
        elif has(name, "Brass"):
            call = f"sBrass ({c([0, 1, 2])}, {f(u(900, 1500) * bright)})"
        elif has(name, "Hoover"):
            call = f"sHoover ({f(u(0.4, 0.6))}, {f(u(2500, 4500) * bright)})"
        elif has(name, "Chop"):
            call = f"sChop ({c([0, 2, 3, 5])}, {c([5, 6])})"
        else:
            call = f"sSuper ({rng.randint(6, 8)}, {f(u(0.4, 0.55))}, {f(u(4000, 6500) * bright)})"

    elif code == "PL":
        call = (f"pl ({c([0, 1, 2, 5, 6, 7, 8, 11])}, {f(u(0.3, 0.7))}, {f(u(400, 2500) * bright)}, "
                f"{f(R(0.15, 0.5))}, {f(u(0.12, 0.28))}, {c([3, 5, 6, 6])})")
        if gritty:
            extra += [("dist_on", 1), ("dist_type", 2), ("dist_drive", round(u(0.2, 0.4), 3)), ("dist_mix", 0.45)]

    elif code == "PD":
        call = (f"pd ({c([0, 1, 2, 3, 10, 11])}, {f(u(0.3, 0.7))}, {rng.randint(4, 7)}, {f(u(0.2, 0.35))}, "
                f"{f(u(900, 2200) * bright)}, {f(u(0.5, 2.0))}, {f(u(1.5, 3.0))}, {f(u(0.1, 0.4))})")

    elif code == "KY":
        if has(name, "Organ"):
            call = f"kOrgan ({f(u(0.4, 0.95))}, {f(u(4000, 8000))})"
        elif has(name, "Clav"):
            call = f"kClav ({f(u(0.5, 0.7))}, {f(u(900, 1300))})"
        else:
            call = f"kFM ({f(u(0.2, 0.65))}, {f(u(0.6, 2.5))}, {f(u(0.35, 0.5))})"

    elif code == "CH":
        if has(name, "Maj7", "Seventh"):
            chord = c([4, 3])
        elif has(name, "Major"):
            chord = 2
        elif has(name, "Fifth"):
            chord = 7
        elif has(name, "Fourths"):
            chord = 10
        elif has(name, "Minor 9", "Minor 7"):
            chord = 5 if "9" in name else 3
        elif has(name, "Dub"):
            chord = 9
        else:
            chord = c([1, 3, 5, 9, 10, 4])
        call = (f"ch ({chord}, {c([0, 2, 4, 5, 6, 8])}, {f(u(0.4, 0.65))}, {f(u(700, 2500) * bright)}, "
                f"{f(u(0.2, 0.5))}, {f(u(0.15, 0.3))})")

    elif code == "AR":
        call = f"ar ({c([0, 1, 6, 9, 10, 11])}, {f(u(0.1, 0.6))}, {f(u(2000, 4500) * bright)}, {f(u(0.08, 0.16))}, {c([3, 5, 6])})"

    elif code == "PC":
        if has(name, "Clap", "Hat", "Shaker"):
            call = f"pcNoise ({f(u(1500, 6000))}, {f(u(0.05, 0.12))})"
        else:
            call = f"pc ({c([0, 1, 7, 10])}, {f(u(0.2, 0.6))}, {f(u(0.1, 1.0))}, {f(u(0.04, 0.15))}, {f(u(150, 1500))})"

    else:  # FX
        if has(name, "Riser"):
            call = f"fxRise ({f(u(5.0, 8.0))}, {c([1, 9, 10])}, {f(u(0.4, 0.7))})"
        elif has(name, "Down", "Drop", "Fall"):
            call = f"fxDown ({f(u(2.5, 4.0))})"
        elif has(name, "Impact", "Hit", "Boom"):
            call = "fxImpact()"
        else:
            call = f"fxSweep ({f(u(0.15, 0.6))})"

    ov = ", ".join("{ \"%s\", %s }" % (k, f(v) if isinstance(v, float) else f"{v}") for k, v in extra)
    return f"join ({{ {call}, {{ {ov} }} }})"


def main():
    src = open(os.path.join(ROOT, "Source", "FactoryPresets.cpp")).read()
    existing = set()
    cur = None
    for m in re.finditer(r'(?:const char\* )?F = "([^"]+)";|add \((F|"Init"), "([^"]+)"', src):
        if m.group(1):
            cur = m.group(1)
            continue
        existing.add((("Init" if m.group(2) == '"Init"' else cur), m.group(3)))

    out = ["// GENERATED by tools/generate_expansion.py - do not edit by hand.",
           "// v1.2 expansion: 25 new presets per folder, incl. 3 new label-style folders.", ""]
    total = 0
    for folder, names in NAMES.items():
        assert len(names) == len(LAYOUT) == 25, (folder, len(names))
        assert len(set(names)) == 25, folder
        out.append(f'        F = "{folder}";')
        for code, name in zip(LAYOUT, names):
            full = f"{code} {name}"
            assert (folder, full) not in existing, f"duplicate: {folder}/{full}"
            seed = int(hashlib.md5(f"{folder}/{full}".encode()).hexdigest()[:8], 16)
            rng = random.Random(seed)
            out.append(f'        add (F, "{full}", {recipe(code, name, PROFILES[folder], rng)});')
            total += 1
        out.append("")
    open(os.path.join(ROOT, "Source", "ExpansionPresets.inc"), "w").write("\n".join(out) + "\n")
    print(f"wrote {total} presets")


if __name__ == "__main__":
    main()
