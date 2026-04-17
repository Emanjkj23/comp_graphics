@echo off
echo === Building Exam Cheat Bundle ===
g++ boiler.cpp geometry.cpp effects.cpp input.cpp -o exam.exe -lfreeglut -lopengl32 -lglu32 -static-libgcc -static-libstdc++
if %errorlevel% equ 0 (
    echo BUILD OK — run exam.exe
) else (
    echo BUILD FAILED — check errors above
)
pause
