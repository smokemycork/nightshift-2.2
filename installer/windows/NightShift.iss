; NightShift Windows installer (Inno Setup 6)
; Built automatically by .github/workflows/build.yml
; Manual build:  ISCC.exe /DAppVersion=1.0.0 /DBuildDir=<repo>\build\NightShift_artefacts\Release /DOutputDir=<repo>\dist NightShift.iss

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build\NightShift_artefacts\Release"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

[Setup]
AppId={{7E3B6D2A-4C1F-4B8E-9A55-2F6C0D9E8B41}
AppName=NightShift
AppVersion={#AppVersion}
AppVerName=NightShift {#AppVersion}
AppPublisher=NightShift Audio
DefaultDirName={commonpf64}\NightShift Audio\NightShift
DefaultGroupName=NightShift
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=NightShift-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=NightShift Synthesizer
UninstallDisplayIcon={app}\NightShift.exe

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in (Ableton Live, FL Studio, Cubase, Bitwig, Reaper...)"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "Standalone application"; Types: full

[Files]
Source: "{#BuildDir}\VST3\NightShift.vst3\*"; DestDir: "{commoncf64}\VST3\NightShift.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\NightShift.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\NightShift"; Filename: "{app}\NightShift.exe"; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\NightShift.vst3"

[Messages]
FinishedLabel=NightShift has been installed.%n%nThe VST3 plug-in is in C:\Program Files\Common Files\VST3. Rescan plug-ins in your DAW to find it.
