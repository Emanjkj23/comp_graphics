# GardenScene - Complete Fix Summary

## ✅ All Requested Features Implemented

### 1. **First-Person Camera**
- **WASD Movement**: Forward/backward and strafe left/right
- **Mouse Look**: Left-click and drag to look around (yaw/pitch control)
- **Smooth Movement**: Camera can move at 0.15 units/frame
- **Collision Detection**: 
  - Won't walk through front wall (z < -6)
  - Side wall boundaries (x ∈ [-3, 3])
  - Back boundary (z < 11)

### 2. **House Improvements**
- **Proper Walls**: Individual wall faces instead of single cube
- **Interior**:
  - Wood-colored floor (RGB: 0.6, 0.45, 0.3)
  - Cream ceiling (RGB: 0.8, 0.8, 0.75)
  - Door frame structure
  - Visible from inside

### 3. **Door Animation**
- **Activation**: Press **E** to toggle
- **Smooth Animation**: Opens/closes at 4°/frame
- **Movement**: Rotates around the hinge at the left side
- **State**: Tracks `doorOpening` flag for toggle behavior

### 4. **Cooking Animation**
- **Activation**: Press **C** (only works inside house)
- **Visual Feedback**:
  - **0-50%**: Egg transitions from pale yellow-white → golden
  - **50-100%**: Deepens to full golden/yellow
- **Wobble Effect**: Rotates ±3° based on cook progress
- **Reset**: Automatically resets when toggled off

### 5. **Stove**
- **Appearance**: Dark grey metal base with burners
- **Pan**: Torus shape with flat cooking surface
- **Egg**: Sits in pan, animates during cooking
- **Visibility**: Only renders when camera is inside house

### 6. **Lighting Fixes**
- **Night Mode Fixed**: Changed from yellow (0.12, 0.13, 0.28) to cool blue (0.08, 0.10, 0.20)
- **Indoor Light (GL_LIGHT2)**:
  - Warm white: RGB(1.0, 0.95, 0.85)
  - Position: At stove height (2.8 units)
  - Activates when: camZ < -5 AND -3 < camX < 3
  - Attenuation: Proper falloff for room lighting
- **Day/Night Transitions**: Smooth progression with sunHeight parameter

## Code Architecture

### Key State Variables
```cpp
// Camera (first-person)
Vec3 camPos = {0.0f, 1.8f, 5.0f};
float yaw = -90.0f;    // left/right looking
float pitch = 0.0f;    // up/down looking
float dirX, dirY, dirZ; // direction vector

// Door and cooking
float doorAngle = 0.0f;
bool doorOpening = false;
float eggCookProgress = 0.0f;  // 0 to 1
bool isCooking = false;
```

### Key Functions

**setupLights()**
- Calculates sun/moon position
- Enables GL_LIGHT0 (directional sun/moon)
- Enables GL_LIGHT1 (outdoor garden lamp)
- Conditionally enables GL_LIGHT2 (indoor light)

**drawHouse()**
- Renders individual walls
- Interior floor and ceiling
- Door with frame
- Windows and roof

**drawStove()**
- Only renders if inside house
- Includes pan and cooking egg
- Egg color changes based on `eggCookProgress`
- Wobble animation synced to cooking

**update()**
- Door animation: Smoothly opens/closes
- Camera movement: WASD controls with collision
- Cooking progress: Advances frame-by-frame

**keyDown()**
- E: Toggle door (sets `doorOpening` flag)
- C: Toggle cooking (only inside house)
- WASD: Sets movement keys (processed in update)

### Lighting Calculations

**Night Mode Color Fix:**
```cpp
if (!nightMode) {
    // Daytime: warm yellowish
    r_s = 0.95f;  g_s = 0.85f;  b_s = 0.80f;
} else {
    // Nighttime: COOL BLUE (fixed!)
    r_s = 0.08f;  g_s = 0.10f;  b_s = 0.20f;
}
```

**Indoor Light Activation:**
```cpp
bool insideHouse = (camZ < -5.0f) && (camX > -3.0f) && (camX < 3.0f);
if (insideHouse) {
    glEnable(GL_LIGHT2);  // Activate warm white light
}
```

## How to Run

```bash
# Navigate to directory
cd "src/other/GardenScene"

# Compile
g++ -std=c++11 -o main.exe main.cpp \
    -I../../include -L../../lib \
    -lfreeglut -lopengl32 -lglu32

# Run
./main.exe
```

## Controls Reference

| Key | Action |
|-----|--------|
| **W** | Move forward |
| **A** | Strafe left |
| **S** | Move backward |
| **D** | Strafe right |
| **Mouse Drag** | Look around |
| **E** | Toggle door open/close |
| **C** | Start/stop cooking (inside only) |
| **P** | Pause/resume |
| **N** | Toggle day/night mode |
| **L** | Cycle lighting modes |
| **Z** | Toggle depth test |
| **S** | Toggle sun orbit |
| **Scroll** | Zoom (for ball/orbit camera) |
| **ESC** | Exit |

## Technical Notes

### GLM Math Library
- Used `glm::radians()` for angle conversion
- Properly handles first-person camera math
- Include: `#include <glm/glm.hpp>`

### Collision System
Simple AABB (axis-aligned bounding box):
- Prevents camera passing through front wall
- Keeps camera within side boundaries
- Restricts movement to garden area

### Lighting Performance
- GL_LIGHT0: Directional (sun/moon) - low cost
- GL_LIGHT1: Point light (garden lamp) - medium cost
- GL_LIGHT2: Point light (indoor) - only active inside
- Result: ~3 lights active simultaneously maximum

### Rendering Order
1. Sky and sun disc
2. Shadows (blended)
3. Ground and fence
4. Tree and house
5. Stove (if inside) and flowers
6. Ball (for ball gameplay)
7. HUD overlays
8. Clickable buttons

## Troubleshooting

**Yellow lighting at night**: Fixed by changing moon light RGB from (0.12, 0.13, 0.28) to (0.08, 0.10, 0.20)

**Can't see inside house**: Make sure camera is far enough inside (z < -5)

**Door won't open**: Press and hold E, or tap E to toggle state

**Egg won't cook**: Must be inside house (detection window: x∈[-3,3], z<-5)

**No indoor light**: Check that you're inside (press E to verify door opens properly)

## Future Enhancements

- Add kitchen furniture
- Multiple cooking recipes
- Smoke/steam effects
- Sound effects
- Improved collision (cylinder/sphere based)
- NPCs or characters
- Day/night cycle automation
