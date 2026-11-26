#define MyAppName "SplitColors"
#define MyAppVersion "1.0.0"
[Setup]
AppId={{B1A55D05-87A1-4C1D-845F-305E39AD74C0}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={localappdata}\Programs\SplitColors
DefaultGroupName=SplitColors
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..
OutputBaseFilename=SplitColors-Setup-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\SplitColors.exe

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: checkedonce

[Dirs]
Name: "{app}\input"; Flags: uninsneveruninstall
Name: "{app}\output"; Flags: uninsneveruninstall

[Files]
Source: "stage\SplitColors.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "stage\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "stage\.env"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "stage\licenses\*"; DestDir: "{app}\licenses"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\SplitColors"; Filename: "{app}\SplitColors.exe"; WorkingDir: "{app}"
Name: "{group}\Input folder"; Filename: "{app}\input"
Name: "{group}\Output folder"; Filename: "{app}\output"
Name: "{autodesktop}\SplitColors"; Filename: "{app}\SplitColors.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\input"; Description: "Open the input folder"; Flags: shellexec postinstall skipifsilent
