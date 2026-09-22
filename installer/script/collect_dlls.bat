@rem Copyright (C) 2026 Innogrid Co., Ltd.
@rem SPDX-License-Identifier: GPL-3.0-only

@echo off
rem ===========================================================================
rem  collect_dlls.bat
rem
rem  Gathers everything NSIS has to package into installer\stage.
rem
rem  NOTE: this file is deliberately ASCII-only, unlike the rest of the project.
rem  cmd.exe parses a .bat with the active console codepage, so UTF-8 Korean
rem  text here decodes into bytes that can include | and &, which splits the
rem  line and makes cmd try to execute a comment. The Korean explanation of
rem  this script lives in README.md instead.
rem
rem  WHY THIS EXISTS
rem   Claude Code runs the status line hook with only the system PATH. If the
rem   Qt bin directory is not on PATH, ccm_probe dies with 0xC0000135
rem   (DLL not found) without printing anything and the status line just looks
rem   empty. So the Qt runtime must sit next to the executables. windeployqt
rem   decides which DLLs are needed - picking them by hand goes stale on every
rem   Qt upgrade.
rem
rem  USAGE
rem   collect_dlls.bat [build dir] [Qt bin dir]
rem
rem  Both default to the values below. Absolute paths are safer.
rem ===========================================================================
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "STAGE_DIR=%SCRIPT_DIR%..\stage"

if "%~1"=="" (
    set "BUILD_DIR=%SCRIPT_DIR%..\..\build\ccm-msvc-rel"
) else (
    set "BUILD_DIR=%~1"
)
if "%~2"=="" (
    set "QT_BIN_DIR=C:\Qt\6.8.3\msvc2022_64\bin"
) else (
    set "QT_BIN_DIR=%~2"
)

set "WINDEPLOYQT=%QT_BIN_DIR%\windeployqt.exe"

echo [1/6] Checking inputs
if not exist "%BUILD_DIR%\ClaudeCodeMonitor.exe" (
    echo   ERROR: ClaudeCodeMonitor.exe not found under %BUILD_DIR%
    echo   Build the release configuration first, or pass the build dir as arg 1.
    exit /b 2
)
if not exist "%BUILD_DIR%\ccm_probe.exe" (
    echo   ERROR: ccm_probe.exe not found under %BUILD_DIR%
    exit /b 2
)
if not exist "%WINDEPLOYQT%" (
    echo   ERROR: windeployqt.exe not found: %WINDEPLOYQT%
    echo   Pass the Qt bin directory as arg 2.
    exit /b 2
)
echo   build  : %BUILD_DIR%
echo   Qt bin : %QT_BIN_DIR%

echo [2/6] Clearing stage directory
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
mkdir "%STAGE_DIR%" || exit /b 3

echo [3/6] Copying executables
copy /y "%BUILD_DIR%\ClaudeCodeMonitor.exe" "%STAGE_DIR%\" >nul || exit /b 3
copy /y "%BUILD_DIR%\ccm_probe.exe" "%STAGE_DIR%\" >nul || exit /b 3

rem  The licence text travels with the binaries. GPLv3 section 4 requires that
rem  every copy of the program carries one, and that applies to a plain zip of
rem  the stage directory just as much as to the installer.
copy /y "%SCRIPT_DIR%..\..\LICENSE" "%STAGE_DIR%\" >nul || exit /b 3
echo [4/6] Collecting the Qt runtime with windeployqt
rem  The widget app goes first: it pulls in Charts, Network and the platform
rem  plugins.
rem
rem  --no-translations : our own strings are baked into the executables as a Qt
rem                      resource, and Qt's own .qm files are not used.
rem  --no-opengl-sw    : opengl32sw.dll is a 20 MB software OpenGL fallback.
rem                      Widgets and Qt Charts paint through the raster engine,
rem                      so it is never loaded. The CMake post-build step has
rem                      skipped it all along; this keeps the two in step.
rem  --no-system-d3d-compiler : D3Dcompiler_47.dll (4 MB) goes with the above.
rem
rem  --compiler-runtime is NOT used here: it only works inside a Visual Studio
rem  developer prompt (it needs VCINSTALLDIR) and silently does nothing
rem  otherwise, which is how the C runtime went missing before. Step 5 copies
rem  those DLLs directly instead.
"%WINDEPLOYQT%" --release --no-translations --no-opengl-sw ^
    --no-system-d3d-compiler ^
    "%STAGE_DIR%\ClaudeCodeMonitor.exe" || exit /b 4

rem  The hook executable only needs Qt Core. Same directory, so this is
rem  usually a no-op, but it makes windeployqt verify nothing is missing.
"%WINDEPLOYQT%" --release --no-translations --no-opengl-sw ^
    --no-system-d3d-compiler ^
    "%STAGE_DIR%\ccm_probe.exe" || exit /b 4

echo [5/6] Copying the Visual C++ runtime
rem  The executables and every Qt DLL import MSVCP140 / VCRUNTIME140. The
rem  api-ms-win-crt-* imports are part of Windows 10 and later, so those need
rem  no action. msvcp140_2.dll is easy to miss: nothing of ours imports it, but
rem  Qt6Gui.dll does, and without it the app dies on a machine that has no
rem  Visual C++ runtime installed.
rem
rem  App-local copies are used rather than running vc_redist.x64.exe: this is a
rem  per-user install (RequestExecutionLevel user) and the redistributable
rem  installer needs administrator rights, which would pop UAC in the middle of
rem  a user-level setup. Microsoft's redistributable terms allow shipping these
rem  DLLs alongside the application.
rem
rem  %ProgramFiles(x86)% cannot be expanded inside a parenthesised block - the
rem  closing paren of (x86) ends the block early - so it is captured first.
set "PF_X86=%ProgramFiles(x86)%"
set "PF_X64=%ProgramFiles%"

set "CRT_DIR="
if defined VCToolsRedistDir (
    if exist "%VCToolsRedistDir%x64\Microsoft.VC143.CRT\msvcp140.dll" (
        set "CRT_DIR=%VCToolsRedistDir%x64\Microsoft.VC143.CRT"
    )
)
if not defined CRT_DIR (
    for %%E in (BuildTools Community Professional Enterprise) do (
        for /d %%V in ("%PF_X86%\Microsoft Visual Studio\2022\%%E\VC\Redist\MSVC\14.*") do (
            if exist "%%V\x64\Microsoft.VC143.CRT\msvcp140.dll" set "CRT_DIR=%%V\x64\Microsoft.VC143.CRT"
        )
        for /d %%V in ("%PF_X64%\Microsoft Visual Studio\2022\%%E\VC\Redist\MSVC\14.*") do (
            if exist "%%V\x64\Microsoft.VC143.CRT\msvcp140.dll" set "CRT_DIR=%%V\x64\Microsoft.VC143.CRT"
        )
    )
)

if not defined CRT_DIR (
    echo   ERROR: the Visual C++ runtime redistributable files were not found.
    echo   Looked for Microsoft.VC143.CRT under the Visual Studio 2022 redist
    echo   directory. Install the "C++ Redistributable MSMs" component, or run
    echo   this script from a Visual Studio developer prompt.
    exit /b 6
)
echo   from: !CRT_DIR!
for %%F in (msvcp140.dll msvcp140_1.dll msvcp140_2.dll vcruntime140.dll vcruntime140_1.dll) do (
    copy /y "!CRT_DIR!\%%F" "%STAGE_DIR%\" >nul || exit /b 6
    echo   + %%F
)

echo [6/6] Verifying result
set "MISSING="
for %%F in (Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Charts.dll Qt6Network.dll ^
            msvcp140.dll msvcp140_1.dll msvcp140_2.dll ^
            vcruntime140.dll vcruntime140_1.dll) do (
    if not exist "%STAGE_DIR%\%%F" set "MISSING=!MISSING! %%F"
)
if not exist "%STAGE_DIR%\platforms\qwindows.dll" set "MISSING=!MISSING! platforms\qwindows.dll"
if not exist "%STAGE_DIR%\LICENSE" set "MISSING=!MISSING! LICENSE"

if not "!MISSING!"=="" (
    echo   ERROR: these files were not collected:!MISSING!
    echo   Check that your Qt installation has those modules.
    exit /b 5
)

echo.
echo Done. stage: %STAGE_DIR%
dir /b "%STAGE_DIR%"
echo.
echo Next: run build_installer.bat to produce the setup executable.
endlocal
exit /b 0
