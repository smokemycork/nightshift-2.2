#!/usr/bin/env bash
# Builds a macOS installer package containing the VST3, AU and Standalone app.
# Usage: build_pkg.sh <version> <artefacts dir (…/NightShift_artefacts/Release)> <output dir>
set -euo pipefail

VERSION="$1"
ART="$2"
OUT="$3"
ID="com.nightshiftaudio.nightshift"
HERE="$(cd "$(dirname "$0")" && pwd)"

mkdir -p "$OUT"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

make_component () {
    local name="$1" src="$2" dest="$3" scripts="${4:-}"
    mkdir -p "$STAGE/root-$name"
    cp -R "$src" "$STAGE/root-$name/"
    # Ad-hoc sign so the binaries load on Apple Silicon (no paid developer ID needed)
    codesign --force --deep --sign - "$STAGE/root-$name/$(basename "$src")"

    # Stop the installer from "relocating" the bundle to some other copy on disk
    pkgbuild --analyze --root "$STAGE/root-$name" "$STAGE/$name.plist"
    /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$STAGE/$name.plist"

    local extra=()
    if [[ -n "$scripts" ]]; then extra=(--scripts "$scripts"); fi
    pkgbuild --root "$STAGE/root-$name" \
             --component-plist "$STAGE/$name.plist" \
             --identifier "$ID.$name" \
             --version "$VERSION" \
             --install-location "$dest" \
             ${extra[@]+"${extra[@]}"} \
             "$STAGE/NightShift-$name.pkg"
}

make_component vst3 "$ART/VST3/NightShift.vst3"      "/Library/Audio/Plug-Ins/VST3"
make_component au   "$ART/AU/NightShift.component"   "/Library/Audio/Plug-Ins/Components" "$HERE/scripts"
make_component app  "$ART/Standalone/NightShift.app" "/Applications"

cat > "$STAGE/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>NightShift ${VERSION}</title>
    <welcome file="welcome.html"/>
    <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <domains enable_localSystem="true"/>
    <choices-outline>
        <line choice="vst3"/>
        <line choice="au"/>
        <line choice="app"/>
    </choices-outline>
    <choice id="vst3" title="VST3 Plug-in" description="For Ableton Live, Bitwig, Cubase, FL Studio, Reaper…">
        <pkg-ref id="${ID}.vst3"/>
    </choice>
    <choice id="au" title="Audio Unit" description="For Logic Pro, GarageBand and Ableton Live.">
        <pkg-ref id="${ID}.au"/>
    </choice>
    <choice id="app" title="Standalone App" description="Play NightShift without a DAW.">
        <pkg-ref id="${ID}.app"/>
    </choice>
    <pkg-ref id="${ID}.vst3" version="${VERSION}">NightShift-vst3.pkg</pkg-ref>
    <pkg-ref id="${ID}.au" version="${VERSION}">NightShift-au.pkg</pkg-ref>
    <pkg-ref id="${ID}.app" version="${VERSION}">NightShift-app.pkg</pkg-ref>
</installer-gui-script>
EOF

productbuild --distribution "$STAGE/distribution.xml" \
             --package-path "$STAGE" \
             --resources "$HERE/resources" \
             "$OUT/NightShift-${VERSION}-macOS.pkg"

echo "Created $OUT/NightShift-${VERSION}-macOS.pkg"
