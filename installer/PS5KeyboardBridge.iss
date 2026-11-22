; Inno Setup 6 script. Built by scripts\build-release.ps1, which passes AppVersion, StageDir,
; OutDir, and SetupName on the command line.

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef StageDir
  #define StageDir "..\dist\stage\PS5KeyboardBridge"
#endif
#ifndef OutDir
  #define OutDir "..\dist"
#endif
#ifndef SetupName
  #define SetupName "PS5KeyboardBridge-Setup-" + AppVersion
#endif

#define AppName "PS5 Keyboard Bridge"
#define AppExe "PS5KeyboardBridge.exe"

[Setup]
; AppId must stay the same across releases so that upgrades replace the old version.
AppId={{7B1E4C2A-5D3F-4A8B-9E6C-2F1D0A4B7C93}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Projeto PS5 Keyboard Bridge (código aberto)
AppPublisherURL=https://github.com/CodyKoInABox/gta6-mousekeyboard-ps5-pc
AppSupportURL=https://github.com/CodyKoInABox/gta6-mousekeyboard-ps5-pc
AppComments=Jogue com teclado e mouse no PS5 via Remote Play, com a imagem na sua capturadora.
; Per-user install: no administrator prompt, and the app can update itself without UAC.
PrivilegesRequired=lowest
DefaultDirName={localappdata}\Programs\PS5KeyboardBridge
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
OutputDir={#OutDir}
OutputBaseFilename={#SetupName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#AppVersion}.0
VersionInfoProductName={#AppName}
VersionInfoCompany=Projeto PS5 Keyboard Bridge (código aberto)

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "Criar atalho na área de trabalho"; GroupDescription: "Atalhos:"; Flags: checkedonce

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{group}\Guia de configuração"; Filename: "{app}\guia-configuracao.md"
Name: "{group}\Desinstalar {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "Abrir o {#AppName} agora"; Flags: nowait postinstall skipifsilent

[Messages]
BeveledLabel=Projeto de código aberto (AGPL-3.0)
