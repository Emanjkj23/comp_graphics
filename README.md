# Compile manually first (important test)

Inside **MinGW64 terminal**, run:

```bash
g++ "src/main.cpp" -o app -lfreeglut -lopengl32 -lglu32
g++ -v "src/triangle.cpp" -o triangle -lopengl32 -lglew32 -lfreeglut -lglu32
```

Then:

```bash
./app
```

If a window opens → everything works

---

# Build & Run
In VS Code:
  * Click **Build**
  * Then run the executable from terminal:
```bash
./build/app.exe
```

---

# If you want
```bash
g++ "src/main.cpp" -o trianglle.exe -I"C:/Users/.../AppData/Local/msys64/mingw64/include" -L"C:/Users/.../AppData/Local/msys64/mingw64/lib" -lfreeglut -lglew32 -lopengl32 -lglu32
```
