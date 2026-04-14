@echo off
echo Building Car Park Scene...

g++ main.cpp globals.cpp lighting.cpp textures.cpp objects.cpp camera.cpp hud.cpp input.cpp -o main ^
    -I"C:/Users/willi/Downloads/freeglut-MinGW-3.0.0-1.mp/freeglut/include" ^
    -L"C:/Users/willi/Downloads/freeglut-MinGW-3.0.0-1.mp/freeglut/lib/x64" ^
    -lfreeglut -lopengl32 -lglu32

if %errorlevel% == 0 (
    echo Build successful! Running...
    main.exe
) else (
    echo Build failed.
    pause
)
