# OpenGL Exam Cheat Bundle — Walkthrough

## Structure

All files are in the `cheat/` subfolder to keep them separate from your group project:

```
cheat/
├── boiler.cpp      ← Main skeleton, camera, reshape, display loop
├── geometry.cpp    ← 15+ shape functions (2D & 3D, all with normals)
├── effects.cpp     ← Lighting, materials, shadows, blending, fog
├── input.cpp       ← Keyboard, mouse, scene switching
├── build.bat       ← One-click compile script
└── exam.exe        ← Compiled binary (ready to run)
```

---

## File Breakdown

### [boiler.cpp](file:///c:/Users/collet.mutangiri1/Downloads/stuf/dev/comp_graphics/projects/Computer%20Graphics%20Project/cheat/boiler.cpp)
| Function | Purpose |
|---|---|
| `setupBasicCamera()` | Perspective projection + orbit camera via `gluLookAt` |
| `setupOrthographicCamera()` | 2D-style view for HUD or flat shapes |
| `reshape()` | Window resize handler |
| `display()` | **★ Edit this for your exam ★** — main render loop |
| `init()` | One-time GL state setup (depth test, shading, lighting) |
| `main()` | GLUT init + callback registration |

---

### [geometry.cpp](file:///c:/Users/collet.mutangiri1/Downloads/stuf/dev/comp_graphics/projects/Computer%20Graphics%20Project/cheat/geometry.cpp)

**2D Shapes** — drawn on XY plane at z=0:
| Function | Notes |
|---|---|
| `drawTriangle2D()` | Equilateral, centred at origin |
| `drawQuad2D(w, h)` | Rectangle |
| `drawCircle2D(radius, segments)` | Filled circle |
| `drawStar2D(outerR, innerR, points)` | N-pointed star |

**3D Shapes** — all include `glNormal3f` for proper lighting:
| Function | Notes |
|---|---|
| `drawTriangle()` | Flat triangle in XY plane |
| `drawCube(size)` | 6 faces, per-face normals |
| `drawSphere(radius, slices, stacks)` | UV-sphere, per-vertex normals |
| `drawCylinder(radius, height, slices)` | With top + bottom caps |
| `drawCone(radius, height, slices)` | With base cap |
| `drawPyramid(base, height)` | Square base |
| `drawTorus(innerR, outerR, sides, rings)` | Donut shape |
| `drawDisk(innerR, outerR, slices)` | Flat ring on XZ plane |
| `drawPrism(base, height, depth)` | Triangular prism |
| `drawTeapot(size)` | GLUT's Utah teapot |

**Composite Objects:**
| Function | Notes |
|---|---|
| `drawTable(legH, topW, topD, topThick)` | 4 cylinder legs + cube top |
| `drawChair(seatH, seatW, seatD, backH)` | 4 legs + seat + backrest |

**Debug Helpers:**
| Function | Notes |
|---|---|
| `drawGrid(halfSize, step)` | Grid lines on XZ plane (floor) |
| `drawCoordinateAxes(length)` | RGB = XYZ colour-coded axes |

---

### [effects.cpp](file:///c:/Users/collet.mutangiri1/Downloads/stuf/dev/comp_graphics/projects/Computer%20Graphics%20Project/cheat/effects.cpp)

**Lighting:**
| Function | Notes |
|---|---|
| `setupBasicLighting()` | Call once in `init()` — sets up GL_LIGHT0 |
| `toggleLightingMode()` | **Press `L`** — switches Ambient ↔ Specular |
| `setLightPosition(x,y,z)` | Move the light dynamically |
| `setDirectionalLight(dx,dy,dz)` | Infinite directional light (w=0) |
| `enableSecondLight(...)` | GL_LIGHT1 as a coloured fill light |

**Materials (presets):**
`setMaterialGold()`, `setMaterialSilver()`, `setMaterialChrome()`, `setMaterialEmerald()`, `setMaterialRubber(r,g,b)`, `setMaterialDefault()`

**Shading & Shadows:**
| Function | Notes |
|---|---|
| `toggleShadeModel()` | **Press `G`** — Flat ↔ Smooth (Gouraud) |
| `beginPlanarShadow(lx,ly,lz)` | Start drawing shadow pass |
| `endPlanarShadow()` | End shadow pass |
| `beginStencilShadow(...)` | Cleaner version (requires GLUT_STENCIL) |

**Blending & Fog:**
| Function | Notes |
|---|---|
| `enableAlphaBlending()` | Standard transparency |
| `disableAlphaBlending()` | Restore opaque rendering |
| `setTransparency(alpha)` | Set current colour alpha |
| `enableFog(r,g,b,density)` | Exponential fog |
| `enableLinearFog(r,g,b,start,end)` | Distance-based fog |

---

### [input.cpp](file:///c:/Users/collet.mutangiri1/Downloads/stuf/dev/comp_graphics/projects/Computer%20Graphics%20Project/cheat/input.cpp)

| Key | Action |
|---|---|
| `W` / `S` | Zoom in / out |
| `A` / `D` | Orbit left / right |
| `R` / `F` | Orbit up / down |
| `Arrow keys` | Orbit camera (larger steps) |
| `L` (uppercase) | Toggle lighting mode |
| `G` | Toggle shade model |
| `B` | Toggle alpha blending |
| `T` | Toggle fog |
| `1`–`5` | Switch scene mode |
| `0` | Reset camera |
| `F1` | Print help |
| `Mouse drag` | Orbit camera |
| `Scroll wheel` | Zoom |
| `ESC` | Quit |

---

## Build & Run

```bat
cd cheat
build.bat
exam.exe
```

> [!TIP]
> Copy `freeglut.dll` into the `cheat/` folder if the exe doesn't find it at runtime.

## Compilation Verified
✅ **Compiled with zero errors and zero warnings** using `g++` with `-lfreeglut -lopengl32 -lglu32`.
