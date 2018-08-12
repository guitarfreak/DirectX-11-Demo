@echo off

rem cd code
rem call ..\\appName.bat

rem call ".\buildWin.bat" -x64 -noRun -release -ship
call ".\code\buildWin.bat" -x64 -noRun -release -ship
rem call ".\code\buildWin.bat" -x86 -noRun -release -ship
rem PAUSE