@echo off
REM Always call vcvars64 to ensure proper MSVC environment.
REM Razzle cl.cmd does not set up include paths for standalone builds.

if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto VSFOUND
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto VSFOUND
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto VSFOUND
)
if exist "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto VSFOUND
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto VSFOUND
)
echo [ERROR] Could not find Visual Studio.
exit /b 1

:VSFOUND
setlocal enabledelayedexpansion

set "ROOT=%~dp0"
set "SRC=%ROOT%src"
set "ENGINE=%SRC%\engine"
set "GAME=%SRC%\game"
set "TEST_DIR=%ROOT%test"
set "BUILD_FRE=%ROOT%build\amd64fre"
set "BUILD_CHK=%ROOT%build\amd64chk"

set "CONFIG=fre"
set "RUNTESTS=1"
set "TESTMODE=--full"
if /I "%~1"=="chk"     set "CONFIG=chk"
if /I "%~1"=="all"     set "CONFIG=all"
if /I "%~1"=="clean"   goto DO_CLEAN
if /I "%~1"=="notest"  set "RUNTESTS=0"
if /I "%~1"=="quick"   set "TESTMODE=--quick"
if /I "%~2"=="quick"   set "TESTMODE=--quick"

if not exist "%BUILD_FRE%" mkdir "%BUILD_FRE%"
if not exist "%BUILD_CHK%" mkdir "%BUILD_CHK%"

set "ENGINE_SRCS=%ENGINE%\gravity.c %ENGINE%\input.c %ENGINE%\log.c %ENGINE%\math.c %ENGINE%\physics.c %ENGINE%\renderer.c"
set "GAME_SRCS=%GAME%\game.c %GAME%\character.c %GAME%\sprite.c %GAME%\level.c"
set "TEST_SRCS=%TEST_DIR%\test_main.c %TEST_DIR%\test_helpers.c %TEST_DIR%\engine\test_math.c %TEST_DIR%\engine\test_gravity.c %TEST_DIR%\engine\test_physics.c %TEST_DIR%\game\test_character.c %TEST_DIR%\game\test_state_transitions.c %TEST_DIR%\game\test_level.c %TEST_DIR%\game\test_regression.c %TEST_DIR%\game\test_sweep.c %TEST_DIR%\game\test_replay.c %TEST_DIR%\game\test_integration.c"
set "GAME_TEST_SRCS=%GAME%\character.c %GAME%\sprite.c %GAME%\level.c"
set "LIBS=user32.lib gdi32.lib winmm.lib"

if /I "!CONFIG!"=="chk" goto BUILD_CHK
if /I "!CONFIG!"=="all" goto BUILD_ALL

:BUILD_FRE
echo.
echo === Building RELEASE (amd64fre) ===
pushd "%BUILD_FRE%"
echo --- engine.lib ---
cl /c /O2 /MT /W4 /WX /nologo /I"%ENGINE%" /I"%GAME%" !ENGINE_SRCS!
if !ERRORLEVEL! NEQ 0 ( echo FAILED: engine compile & popd & exit /b 1 )
lib /nologo /out:engine.lib gravity.obj input.obj log.obj math.obj physics.obj renderer.obj
if !ERRORLEVEL! NEQ 0 ( echo FAILED: engine lib & popd & exit /b 1 )
echo OK: engine.lib
echo --- sprites.res ---
rc /nologo /I"%GAME%" /fo sprites.res "%GAME%\sprites.rc"
if !ERRORLEVEL! NEQ 0 ( echo FAILED: resource compile & popd & exit /b 1 )
echo OK: sprites.res
echo --- parcour.exe ---
cl /O2 /MT /W4 /WX /nologo /I"%ENGINE%" /I"%GAME%" !GAME_SRCS! /link engine.lib sprites.res %LIBS% /out:parcour.exe
if !ERRORLEVEL! NEQ 0 ( echo FAILED: game build & popd & exit /b 1 )
echo OK: parcour.exe
echo --- test_runner.exe ---
cl /O2 /MT /W4 /WX /nologo /I"%ENGINE%" /I"%GAME%" /I"%TEST_DIR%" !TEST_SRCS! !GAME_TEST_SRCS! /link engine.lib %LIBS% /out:test_runner.exe
if !ERRORLEVEL! NEQ 0 ( echo FAILED: test build & popd & exit /b 1 )
echo OK: test_runner.exe
popd
if "!RUNTESTS!"=="1" (
    echo.
    echo === Running tests [amd64fre] ===
    pushd "%BUILD_FRE%"
    test_runner.exe !TESTMODE!
    if !ERRORLEVEL! NEQ 0 ( echo TESTS FAILED & popd & exit /b 1 )
    echo All tests passed
    popd
)
if /I "!CONFIG!"=="all" goto BUILD_CHK
echo.
echo Done.
exit /b 0

:BUILD_CHK
echo.
echo === Building DEBUG (amd64chk) ===
pushd "%BUILD_CHK%"
echo --- engine.lib ---
cl /c /Od /Zi /MTd /W4 /WX /nologo /D_DEBUG /I"%ENGINE%" /I"%GAME%" !ENGINE_SRCS!
if !ERRORLEVEL! NEQ 0 ( echo FAILED: engine compile & popd & exit /b 1 )
lib /nologo /out:engine.lib gravity.obj input.obj log.obj math.obj physics.obj renderer.obj
if !ERRORLEVEL! NEQ 0 ( echo FAILED: engine lib & popd & exit /b 1 )
echo OK: engine.lib
echo --- sprites.res ---
rc /nologo /I"%GAME%" /fo sprites.res "%GAME%\sprites.rc"
if !ERRORLEVEL! NEQ 0 ( echo FAILED: resource compile & popd & exit /b 1 )
echo OK: sprites.res
echo --- parcour.exe ---
cl /Od /Zi /MTd /W4 /WX /nologo /D_DEBUG /I"%ENGINE%" /I"%GAME%" !GAME_SRCS! /link engine.lib sprites.res %LIBS% /DEBUG /out:parcour.exe
if !ERRORLEVEL! NEQ 0 ( echo FAILED: game build & popd & exit /b 1 )
echo OK: parcour.exe
echo --- test_runner.exe ---
cl /Od /Zi /MTd /W4 /WX /nologo /D_DEBUG /I"%ENGINE%" /I"%GAME%" /I"%TEST_DIR%" !TEST_SRCS! !GAME_TEST_SRCS! /link engine.lib %LIBS% /DEBUG /out:test_runner.exe
if !ERRORLEVEL! NEQ 0 ( echo FAILED: test build & popd & exit /b 1 )
echo OK: test_runner.exe
popd
if "!RUNTESTS!"=="1" (
    echo.
    echo === Running tests [amd64chk] ===
    pushd "%BUILD_CHK%"
    test_runner.exe !TESTMODE!
    if !ERRORLEVEL! NEQ 0 ( echo TESTS FAILED & popd & exit /b 1 )
    echo All tests passed
    popd
)
echo.
echo Done.
exit /b 0

:BUILD_ALL
goto BUILD_FRE

:DO_CLEAN
echo Cleaning...
del /q "%BUILD_FRE%\*.exe" "%BUILD_FRE%\*.obj" "%BUILD_FRE%\*.lib" "%BUILD_FRE%\*.res" "%BUILD_FRE%\*.pdb" "%BUILD_FRE%\*.log" "%BUILD_FRE%\*.ilk" 2>nul
del /q "%BUILD_CHK%\*.exe" "%BUILD_CHK%\*.obj" "%BUILD_CHK%\*.lib" "%BUILD_CHK%\*.res" "%BUILD_CHK%\*.pdb" "%BUILD_CHK%\*.log" "%BUILD_CHK%\*.ilk" 2>nul
echo Done.
exit /b 0