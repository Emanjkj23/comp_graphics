@echo off
echo ==================================================
echo  CAR PARK SCENE - OpenGL All-Concepts Demo
echo ==================================================
echo.
where g++ >nul 2>nul
if %errorlevel% neq 0 (echo [ERROR] g++ not found. Install MinGW-w64: https://winlibs.com/ & pause & exit /b 1)
set GLUT_INC=C:\freeglut\include
set GLUT_LIB=C:\freeglut\lib\x64
if not exist "%GLUT_INC%\GL\freeglut.h" (echo [ERROR] freeglut missing at %GLUT_INC% & pause & exit /b 1)
echo [INFO] Compiling...
g++ main.cpp -o CarPark.exe -I"%GLUT_INC%" -L"%GLUT_LIB%" -lfreeglut -lopengl32 -lglu32 -lwinmm -mwindows -O2 -std=c++11
if %errorlevel% neq 0 (echo [FAILED] & pause & exit /b 1)
if exist "%GLUT_LIB%\freeglut.dll" copy "%GLUT_LIB%\freeglut.dll" freeglut.dll >nul
echo [SUCCESS] CarPark.exe ready!
echo.
echo Controls: WASD/Arrows=Drive  TAB=Switch Car  C=Camera  L=Lighting
echo           Z=Depth  N=Night  P=Pause  S=Sun  Click HUD buttons  ESC=Quit
pause
