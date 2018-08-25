rem This file can not be called on its own.

if %COMPILE_TOKEN% == "" exit


rem Create portable version.

set RELEASE_FOLDER_ZIP=.\releases\%APP_NAME%

"%ZIP7%" a "%RELEASE_FOLDER_ZIP% %PLATFORM2%.zip" "%RELEASE_FOLDER%\*"


rem Create installer version.
(
	echo #define MyAppName "%APP_NAME%"
	echo #define MyAppVersion "%APP_VERSION%"
	echo #define MyAppURL "%GITHUB_LINK%"
	echo #define MyAppExeName "%APP_NAME%.exe"
	echo.
	echo [Setup]
	echo AppId={#MyAppName} "%PLATFORM2%"
	echo AppName={#MyAppName}
	echo AppVersion={#MyAppVersion}
	echo AppPublisherURL={#MyAppURL}
	echo AppSupportURL={#MyAppURL}
	echo AppUpdatesURL={#MyAppURL}
	echo AllowNoIcons=yes
	echo DefaultDirName={pf}\{#MyAppName}
	echo DisableProgramGroupPage=yes
	echo OutputDir=.\releases\
	echo OutputBaseFilename={#MyAppName} Installer %PLATFORM2%
	echo UninstallDisplayIcon={app}\{#MyAppExeName}
	echo UninstallDisplayName={#MyAppName} %APP_VERSION% %PLATFORM2%
	echo Compression=lzma
	echo SolidCompression=yes

	if "%PLATFORM2%"=="x64" (
		echo ArchitecturesInstallIn64BitMode=x64
	)

	echo.
	echo [Languages]
	echo Name: "english"; MessagesFile: "compiler:Default.isl"
	echo. 
	echo [Tasks]
	echo Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
	echo.
	echo [Files]
	echo Source: "%RELEASE_FOLDER%\%APP_NAME%.exe"; DestDir: "{app}"; Flags: ignoreversion
	echo Source: "%RELEASE_FOLDER%\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
	echo.
	echo [Icons]
	echo Name: "{commonprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
	echo Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
	echo.
	echo [Run]
	echo Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

) > setupScript.iss

"%INNO_SETUP_COMPILER%" setupScript.iss

del setupScript.iss