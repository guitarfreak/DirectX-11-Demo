@echo off
Setlocal EnableDelayedExpansion

rem Set this to true to find latest VS version.
set ENABLE_FIND_LATEST_VC_PATH=true

rem Or set the vcvarsall.bat path manually.
set VCVARSALL_PATH=

rem Set these paths to be able to ship (as standalone or installer):
set                ZIP7=C:\Program Files\7-Zip\7z.exe
set INNO_SETUP_COMPILER=C:\Program Files (x86)\Inno Setup 5\ISCC.exe

set DUMPBIN="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.25.28610\bin\Hostx64\x64\dumpbin.exe"

set RUN_PREPROCESSOR=true
set COMPILE_PREPROCESSOR=true
set COMPILE_MAIN=true
set COMPILE_APP=true


set APP_VERSION=0.3.5
set GITHUB_LINK=www.github.com/guitarfreak/DirectX-11-Demo

rem preprocesser output -> -E, -d2cgsummary -Bt, /Qvec-report:1, /showIncludes
rem -EHa-
rem -FC 
rem -Z7
set COMPILER_OPTIONS_LIST=-nologo -Oi -wd4838 -wd4005 -wd4005 -fp:fast -fp:except- -Gm- -GR- -EHsc- -Ob2
set COMPILER_OPTIONS_LIST=%COMPILER_OPTIONS_LIST% -Zi
rem set COMPILER_OPTIONS_LIST=%COMPILER_OPTIONS_LIST% -FA

set LINKER_LIBS_LIST=user32.lib Gdi32.lib Shlwapi.lib Winmm.lib Ole32.lib D3D11.lib d3dcompiler.lib
rem Shell32.lib 

set PLATFORM=win64
if "%~1"=="-x86" set PLATFORM=win32

set INCLUDE_LIST[0]=\freetype 2.10.2\include
set     LIB_LIST[0]=\freetype 2.10.2\lib\%PLATFORM%\freetype.lib
set     DLL_LIST[0]=\freetype 2.10.2\lib\%PLATFORM%\freetype.dll
set     DLL_LIST[1]=\d3dcompiler\lib\%PLATFORM%\D3DCompiler_47.dll

rem Go to batch directory.
cd %~d0%~p0

if not %ENABLE_FIND_LATEST_VC_PATH% == true goto endFindVCPath
	for /f "usebackq tokens=*" %%i in (`"..\libs\vswhere\vswhere.exe" -latest -property installationPath`) do set VS_LATEST_PATH=%%i\VC

	rem From V2017 on the path changed so we have to check 2 paths.
	set SEARCH_PATHS[0]=%VS_LATEST_PATH%\vcvarsall.bat
	set SEARCH_PATHS[1]=%VS_LATEST_PATH%\Auxiliary\Build\vcvarsall.bat

	for /F "tokens=2 delims==" %%s in ('set SEARCH_PATHS[') do set VCVARSALL_PATH=%%s
:endFindVCPath

set C_INC=
set L_INC=

if "%VCVARSALL_PATH%" == "" goto useCustomPaths
	set ARCH=x86
	if not "%~1" == "-x86" ( set ARCH=amd64 )
	call "%VCVARSALL_PATH%" %ARCH%
	goto endSetCompiler

:useCustomPaths
	rem Internally we set the compiler path manually to safe the little bit of time that vcvarsall takes.
	set WIN_SDK_PATH=C:\Program Files (x86)\Windows Kits\10
	set      VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio 12.0

	set C_INC=%C_INC% -I"%VS_PATH%\VC\include"
	set C_INC=%C_INC% -I"%WIN_SDK_PATH%\Include\10.0.19041.0\um"
	set C_INC=%C_INC% -I"%WIN_SDK_PATH%\Include\10.0.19041.0\shared"

	if "%~1"=="-x86" goto compilerSelectionX86
		set                     PATH=%VS_PATH%\VC\bin\amd64;%PATH%
		set L_INC=%L_INC% -LIBPATH:"%VS_PATH%\VC\lib\amd64"
		set L_INC=%L_INC% -LIBPATH:"%WIN_SDK_PATH%\Lib\10.0.19041.0\um\x64"
		goto compilerSelectionEnd

	:compilerSelectionX86
		set                     PATH=%VS_PATH%\VC\bin;%PATH%
		set L_INC=%L_INC% -LIBPATH:"%VS_PATH%\VC\lib"
		set L_INC=%L_INC% -LIBPATH:"%WIN_SDK_PATH%\Lib\10.0.19041.0\um\x86"
	:compilerSelectionEnd
:endSetCompiler

if exist "appName.bat" ( call appName.bat ) else ( call ..\\appName.bat )
set APP_NAME_STRING=%APP_NAME%
set APP_NAME=%APP_NAME:"=%



set BUILD_FOLDER=buildWin64
if "%~1"=="-x86" set BUILD_FOLDER=buildWin32

if not "%~4"=="-folder" goto buildSetupEnd
	set BUILD_FOLDER=releaseBuild
	if exist "..\%BUILD_FOLDER%" rmdir "..\%BUILD_FOLDER%" /S /Q
:buildSetupEnd

if not exist "..\%BUILD_FOLDER%" mkdir "..\%BUILD_FOLDER%"
pushd "..\%BUILD_FOLDER%"

rem Set includes/libs.
	set LINKER_LIBS=
	set C_INC_LIBS=
	set L_INC_LIBS=

	for %%a in (%LINKER_LIBS_LIST%)                         do set LINKER_LIBS=!LINKER_LIBS! -DEFAULTLIB:%%a
	for /F "tokens=2 delims==" %%a in ('set INCLUDE_LIST[') do set  C_INC_LIBS=!C_INC_LIBS!  -I"..\libs%%a"
	for /F "tokens=2 delims==" %%a in ('set LIB_LIST[')     do set  L_INC_LIBS=!L_INC_LIBS!  -LIBPATH:"..\libs%%~pa"
	for /F "tokens=2 delims==" %%a in ('set LIB_LIST[')     do set LINKER_LIBS=!LINKER_LIBS! -DEFAULTLIB:%%~nxa

rem Set compiler options and modes.
	set BUILD_MODE=-Od
	set RUNTIME=-MD
	set MODE_DEFINE=

	if "%~3"=="-releaseMode" (
		rem -Oy -Zo
		set BUILD_MODE=-O2
		set MODE_DEFINE=-DRELEASE_BUILD
	)
	if "%~4"=="-folder" (
		set MODE_DEFINE=%MODE_DEFINE% -DSHIPPING_MODE
		set RUNTIME=-MT
	)
	if "%~5"=="-fullOptimize" (
		set MODE_DEFINE=%MODE_DEFINE% -DFULL_OPTIMIZE
	)

	set MODE_DEFINE=%MODE_DEFINE% -DA_NAME=%APP_NAME_STRING%
	set COMPILER_OPTIONS= %RUNTIME% %BUILD_MODE% %COMPILER_OPTIONS_LIST%

rem Compile/Run preprocessor.
	set PREPROCESS_NAME=preprocess

	set PREPROCESS_OPTIONS=
	if not exist "%PREPROCESS_NAME%" (
		mkdir ".\%PREPROCESS_NAME%"
		set COMPILE_PREPROCESSOR=true
		set PREPROCESS_OPTIONS=-O2
	)

	if %COMPILE_PREPROCESSOR% == false goto afterPreprocess
		set LINKER_OPTIONS_PRE= -link -SUBSYSTEM:CONSOLE -OUT:"%PREPROCESS_NAME%.exe" -incremental:no -opt:ref

		pushd ".\%PREPROCESS_NAME%"
		cl %COMPILER_OPTIONS% %PREPROCESS_OPTIONS% ..\..\code\preprocess.cpp %MODE_DEFINE% %C_INC% %LINKER_OPTIONS_PRE% %L_INC%
		popd
	:afterPreprocess

	if not exist "..\code\generated" mkdir "..\code\generated"

	if %RUN_PREPROCESSOR% == true call ".\%PREPROCESS_NAME%\%PREPROCESS_NAME%.exe"

rem Compile app.
	if "%~4"=="-folder" goto noDLL
	if %COMPILE_APP%==false goto noDLL
		del main_*.pdb > NUL 2> NUL
		echo. 2>lock.tmp
		cl %COMPILER_OPTIONS% ..\code\app.cpp %MODE_DEFINE% -LD %C_INC% %C_INC_LIBS% -link -incremental:no -opt:ref -PDB:main_%random%.pdb -EXPORT:appMain %L_INC% %L_INC_LIBS% %LINKER_LIBS%

		if %ERRORLEVEL%==0 (
		   echo.
			echo       ###########
			echo       # SUCCES! #
			echo       ###########
			echo.
		) 

		del lock.tmp
	:noDLL

rem Compile Main.
	set MAIN_OPTIONS=
	if not exist "%APP_NAME%.exe" (
		set COMPILE_MAIN=true
		set MAIN_OPTIONS=-O2
	)

	if %COMPILE_MAIN% == true (
		cl %COMPILER_OPTIONS% %MAIN_OPTIONS% ..\code\main.cpp %MODE_DEFINE% %C_INC% %C_INC_LIBS% -link -SUBSYSTEM:WINDOWS -OUT:"%APP_NAME%.exe" -incremental:no -opt:ref %L_INC% %L_INC_LIBS% %LINKER_LIBS%
	)



SET mypath=%~dp0
rem echo %mypath:~0,-1%

rem Copy dlls if not already there.
	for /F "tokens=2 delims==" %%a in ('set DLL_LIST[') do (
		if not exist %%~nxa (
			xcopy "..\libs%%a" "..\%BUILD_FOLDER%" /Q
		)
	)

if not "%~4"=="-folder" goto packShippingFolderEnd
	cd ..

	rem Create release folder.
	mkdir ".\%BUILD_FOLDER%\data"
	xcopy ".\data" ".\%BUILD_FOLDER%\data" /E /Q

	del /s /q ".\%BUILD_FOLDER%\data\*.png"
	del /s /q ".\%BUILD_FOLDER%\data\*.jpg"

	if "%~6"=="-ship" (
		del ".\%BUILD_FOLDER%\*.pdb"
		del ".\%BUILD_FOLDER%\*.exp"
		del ".\%BUILD_FOLDER%\*.lib"
		del ".\%BUILD_FOLDER%\*.obj"
	)

	rmdir ".\%BUILD_FOLDER%\%PREPROCESS_NAME%" /S /Q

	xcopy ".\README.txt" ".\%BUILD_FOLDER%" /Q
	xcopy ".\Licenses.txt" ".\%BUILD_FOLDER%" /Q

	.\libs\seticon\seticon.exe icon.png "%BUILD_FOLDER%\%APP_NAME%.exe"

	set PLATFORM2=x64
	if "%~1"=="-x86" set PLATFORM2=x86

	set RELEASE_FOLDER=.\releases\%APP_NAME% %PLATFORM2%
	if exist "%RELEASE_FOLDER%" rmdir "%RELEASE_FOLDER%" /S /Q
	mkdir "%RELEASE_FOLDER%"

	xcopy %BUILD_FOLDER% "%RELEASE_FOLDER%" /E /Q

	rmdir ".\%BUILD_FOLDER%" /S /Q

	if not "%~6"=="-ship" goto shipEnd
		set COMPILE_TOKEN=true
		call buildCreateInstallerAndZip.bat
	:shipEnd
:packShippingFolderEnd

IF "%~2"=="-run" call "%APP_NAME%.exe"
