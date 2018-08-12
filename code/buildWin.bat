@echo off

set SET_ICON_PATH=C:\Standalone\SetIcon\seticon.exe
set      ZIP_PATH=C:\Program Files\7-Zip\7z.exe
set  WIN_SDK_PATH=C:\Program Files (x86)\Windows Kits\10
set       VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio 12.0

if exist "appName.bat" (
	call appName.bat
) else (
	call ..\\appName.bat
)

set scriptpath=%~d0%~p0
cd %scriptpath%

set PLATFORM=win64
if "%~1"=="-x86" set PLATFORM=win32

set PLATFORM2=x64
if "%~1"=="-x86" set PLATFORM2=x86

set BUILD_FOLDER=buildWin64
if "%~1"=="-x86" set BUILD_FOLDER=buildWin32

if NOT "%~4"=="-ship" goto buildSetupEnd
	set BUILD_FOLDER=releaseBuild
	if exist "..\%BUILD_FOLDER%" rmdir "..\%BUILD_FOLDER%" /S /Q
:buildSetupEnd

if not exist "..\%BUILD_FOLDER%" mkdir "..\%BUILD_FOLDER%"
pushd "..\%BUILD_FOLDER%"

set INC=
set LINC=

rem -DEFAULTLIB:Shell32.lib 
set LINKER_LIBS= -DEFAULTLIB:user32.lib -DEFAULTLIB:Gdi32.lib -DEFAULTLIB:Shlwapi.lib -DEFAULTLIB:Winmm.lib -DEFAULTLIB:Ole32.lib -DEFAULTLIB:D3D11.lib -DEFAULTLIB:d3dcompiler.lib

set          INC=%INC% -I"%VS_PATH%\VC\include"
set          INC=%INC% -I"%WIN_SDK_PATH%\Include\10.0.17134.0\um"
set          INC=%INC% -I"%WIN_SDK_PATH%\Include\10.0.17134.0\shared"

if "%~1"=="-x86" goto compilerSelectionX86

set                  PATH=%VS_PATH%\VC\bin\amd64;%PATH%
set LINC=%LINC% -LIBPATH:"%VS_PATH%\VC\lib\amd64"
set LINC=%LINC% -LIBPATH:"%WIN_SDK_PATH%\Lib\10.0.17134.0\um\x64"

goto compilerSelectionEnd
:compilerSelectionX86

set                  PATH=%VS_PATH%\VC\bin;%PATH%
set LINC=%LINC% -LIBPATH:"%VS_PATH%\VC\lib"
set LINC=%LINC% -LIBPATH:"%WIN_SDK_PATH%\Lib\10.0.17134.0\um\x86"

:compilerSelectionEnd

set          INC=%INC% -I"..\libs\freetype 2.9\include"
set LINC=%LINC% -LIBPATH:"..\libs\freetype 2.9\lib\%PLATFORM%"
set LINKER_LIBS=%LINKER_LIBS% -DEFAULTLIB:freetype.lib

set BUILD_MODE=-Od
set MODE_DEFINE=
if "%~3"=="-release" (
	rem -Oy -Zo
	set BUILD_MODE=-O2
	set MODE_DEFINE=-DRELEASE_BUILD
)

set RUNTIME=-MD

if "%~4"=="-ship" (
	set MODE_DEFINE=%MODE_DEFINE% -DSHIPPING_MODE
	set RUNTIME=-MT
)

if "%~5"=="-fullOptimize" (
	set MODE_DEFINE=%MODE_DEFINE% -DFULL_OPTIMIZE
)

set MODE_DEFINE=%MODE_DEFINE% -DA_NAME=%APP_NAME%

rem preprocesser output -> -E
rem -d2cgsummary -Bt
set COMPILER_OPTIONS= %RUNTIME% %BUILD_MODE% -nologo -Oi -FC -wd4838 -wd4005 -fp:fast -fp:except- -Gm- -GR- -EHa- -Z7
set LINKER_OPTIONS= -link -SUBSYSTEM:WINDOWS -OUT:"%APP_NAME:"=%.exe" -incremental:no -opt:ref


if "%~4"=="-ship" goto noDLL

del main_*.pdb > NUL 2> NUL
echo. 2>lock.tmp
cl %COMPILER_OPTIONS% ..\code\app.cpp %MODE_DEFINE% -LD %INC% -link -incremental:no -opt:ref -PDB:main_%random%.pdb -EXPORT:appMain %LINC% %LINKER_LIBS%

if %ERRORLEVEL%==0 (
   echo.
	echo       ###########
	echo       # SUCCES! #
	echo       ###########
	echo.
) 

del lock.tmp

:noDLL

cl %COMPILER_OPTIONS% ..\code\main.cpp %MODE_DEFINE% %INC% %LINKER_OPTIONS% %LINC% %LINKER_LIBS%



if NOT "%~4"=="-ship" goto packShippingFolderEnd

	cd ..

	mkdir ".\%BUILD_FOLDER%\data"
	xcopy ".\data" ".\%BUILD_FOLDER%\data" /E /Q

	if "%~3"=="" goto nodelete
		del ".\%BUILD_FOLDER%\*.pdb"
		del ".\%BUILD_FOLDER%\*.exp"
		del ".\%BUILD_FOLDER%\*.lib"
		del ".\%BUILD_FOLDER%\*.obj"
	:nodelete

	xcopy ".\libs\freetype 2.9\lib\%PLATFORM%\*.dll" ".\%BUILD_FOLDER%" /Q
	xcopy ".\libs\d3dcompiler\lib\%PLATFORM%\*.dll" ".\%BUILD_FOLDER%" /Q

	xcopy ".\README.txt" ".\%BUILD_FOLDER%" /Q
	xcopy ".\Licenses.txt" ".\%BUILD_FOLDER%" /Q

	"%SET_ICON_PATH%" %cd%"\icon.png" %cd%"\%BUILD_FOLDER%\%APP_NAME:"=%.exe"

	pause

	set RELEASE_FOLDER=.\releases\%PLATFORM%\%APP_NAME:"=%
	if exist "%RELEASE_FOLDER%" rmdir "%RELEASE_FOLDER%" /S /Q
	mkdir "%RELEASE_FOLDER%"

	xcopy %BUILD_FOLDER% "%RELEASE_FOLDER%" /E /Q

	rmdir ".\%BUILD_FOLDER%" /S /Q

	"%ZIP_PATH%" a "%RELEASE_FOLDER% %PLATFORM2%.zip" "%RELEASE_FOLDER%"

:packShippingFolderEnd

IF "%~2"=="-run" call "%APP_NAME:"=%.exe"
