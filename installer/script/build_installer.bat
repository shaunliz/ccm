@rem Copyright (C) 2026 Innogrid Co., Ltd.
@rem SPDX-License-Identifier: GPL-3.0-only

@echo off
rem ===========================================================================
rem  build_installer.bat
rem
rem  Collects the DLLs and compiles the NSIS script in one go.
rem
rem  NOTE: ASCII-only on purpose - see the comment in collect_dlls.bat.
rem  The Korean explanation lives in README.md.
rem
rem  USAGE
rem   build_installer.bat [build dir] [Qt bin dir]
rem
rem  PREREQUISITES
rem   1. The release configuration must already be built.
rem   2. NSIS must be installed (https://nsis.sourceforge.io). makensis.exe is
rem      looked up on PATH and then in the default install locations.
rem ===========================================================================
setlocal

set "SCRIPT_DIR=%~dp0"

echo === 1. Collect DLLs ===
call "%SCRIPT_DIR%collect_dlls.bat" %1 %2
if errorlevel 1 (
    echo DLL collection failed. Stopping.
    exit /b 1
)

echo.
echo === 2. Locate makensis ===
set "MAKENSIS="
where makensis.exe >nul 2>&1 && set "MAKENSIS=makensis.exe"
if not defined MAKENSIS if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" (
    set "MAKENSIS=%ProgramFiles(x86)%\NSIS\makensis.exe"
)
if not defined MAKENSIS if exist "%ProgramFiles%\NSIS\makensis.exe" (
    set "MAKENSIS=%ProgramFiles%\NSIS\makensis.exe"
)
if not defined MAKENSIS (
    echo   ERROR: makensis.exe not found.
    echo   Install NSIS or put it on PATH: https://nsis.sourceforge.io
    exit /b 2
)
echo   %MAKENSIS%

echo.
echo === 3. Build the installer ===
rem  NSIS does not create the output directory by itself.
if not exist "%SCRIPT_DIR%..\dist" mkdir "%SCRIPT_DIR%..\dist"
rem  /WX turns warnings into errors so script mistakes are not passed over.
"%MAKENSIS%" /WX "%SCRIPT_DIR%ClaudeCodeMonitor.nsi"
if errorlevel 1 (
    echo NSIS compilation failed.
    exit /b 3
)

echo.
echo Done.
dir /b "%SCRIPT_DIR%..\dist\*-setup.exe"
endlocal
exit /b 0
