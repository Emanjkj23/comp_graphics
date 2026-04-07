@echo off
echo ==================================================
echo  CG GARDEN SCENE - Depth and Lighting Demo
echo  Compiler: g++ with FreeGLUT on Windows
echo ==================================================
echo.

where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] g++ not found. Install MinGW-w64.
    echo         Download: https://winlibs.com/
    echo         Extract to C:\mingw64 and add C:\mingw64\bin to PATH
    pause & exit /b 1
)

set GLUT_INC=C:\freeglut\include
set GLUT_LIB=C:\freeglut\lib\x64

if not exist "%GLUT_INC%\GL\freeglut.h" (
    echo [ERROR] freeglut not found at %GLUT_INC%
    echo         Download: https://www.transmissionzero.co.uk/software/freeglut-devel/
    echo         Extract to C:\freeglut\
    pause & exit /b 1
)

echo [INFO] Compiling GardenScene...
g++ main.cpp -o GardenScene.exe ^
    -I"%GLUT_INC%" -L"%GLUT_LIB%" ^
    -lfreeglut -lopengl32 -lglu32 -lwinmm ^
    -mwindows -O2 -std=c++11

if %errorlevel% neq 0 (
    echo [FAILED] Check errors above.
    pause & exit /b 1
)

if exist "%GLUT_LIB%\freeglut.dll" copy "%GLUT_LIB%\freeglut.dll" freeglut.dll >nul

echo.
echo [SUCCESS] GardenScene.exe ready!
echo.
echo  KEY CONTROLS - Learn by doing:
echo  -----------------------------------------------
echo  L           - Cycle lighting modes (SEE the difference!)
echo                Mode 0: Full Phong (best quality)
echo                Mode 1: No specular (remove shiny highlights)
echo                Mode 2: Ambient only (NO depth perception!)
echo                Mode 3: Lighting OFF (flat, impossible to judge depth)
echo  Z           - Toggle depth test ON/OFF
echo                OFF = objects may appear THROUGH other objects!
echo  N           - Toggle Day / Night
echo  S           - Pause/resume sun orbit
echo  -----------------------------------------------
echo  WASD/Arrows - Move the ball around the garden
echo  SPACE       - Bounce the ball
echo  R           - Reset ball position
echo  Mouse Drag  - Orbit camera around scene
echo  Scroll      - Zoom in/out
echo  ESC         - Quit
echo.
echo Run: GardenScene.exe
pause
