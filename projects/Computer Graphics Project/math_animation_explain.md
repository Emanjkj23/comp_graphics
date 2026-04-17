# Mathematical Foundations & Animation

To make the car park simulation feel interactive and "alive", OpenGL relies on a mathematical framework heavily leaning on Trigonometry and basic Kinematics.

## 1. Animation (The Update Loop)

Animation in OpenGL isn't like a video file; it's a constant loop of calculating new states and re-drawing the scene. 

In `input.cpp`, you have an `update(int v)` function attached to a `glutTimerFunc(16, ...)`. This tells OpenGL to run the `update()` math approximately every 16 milliseconds (~60 Frames Per Second).

### Car Physics (Kinematics)
To move the car, the code doesn't just "jump" its X and Z coordinates. It calculates forward momentum using angles:

```cpp
// 1. Calculate heading from steering input
c.heading += turnR * (ABS(spd) > 0.01f ? 1.f : 0.3f);

// 2. Convert degrees to radians (math functions require radians)
float hr = D2R(-c.heading);

// 3. Trigonometry! Calculate the new X and Z based on angle and speed.
// sin() gives the horizontal offset, cos() gives the depth offset.
float nx = c.x + sinf(hr) * spd;
float nz = c.z + cosf(hr) * spd;
```

### Wheel Rotation (Arc Length Math)
When the car moves forward, the wheels shouldn't just slide; they need to roll accurately based on how far the car moved. 
The arc length formula states $s = r \cdot \theta$ (distance = radius * angle). 
We spin the wheel by calculating $\theta$ based on the wheel's radius (`0.36f`):
```cpp
// Wheel rot = distance travelled * (1 / wheel radius), converted to degrees
c.wheelRot += spd * R2D(1.f / 0.36f);
```


## 2. Collision Detection 

To stop cars from driving through buildings and trees, mathematical collision tests run before moving the car.

### Circle-to-Circle Collision (Car vs Trees/Bins)
Most objects are treated as circles (cylinders from a top-down view). We use the **Pythagorean Theorem** ($a^2 + b^2 = c^2$) to calculate the distance between the two circles' centers. If the square of the distance is less than the square of their combined radii, they have collided!

```cpp
bool circleCircle(float ax, float az, float ar, float bx, float bz, float br) {
    float dx = ax - bx;
    float dz = az - bz;
    float combinedRadius = ar + br;
    // Calculate a^2 + b^2 < c^2
    return (dx*dx + dz*dz) < combinedRadius * combinedRadius;
}
```

### Circle-to-AABB (Car vs Office Building)
The office building is an Axis-Aligned Bounding Box (AABB) — a rectangle. To check if a circle hits it:
1. We take the circle's center point.
2. We `CLAMP` that point to the nearest bounds of the rectangle (finding the closest spot on the building to the car).
3. We check the Pythagorean distance from the car's center to that closest spot!

```cpp
bool circleAABB(float cx, float cz, float r, float x0, float x1, float z0, float z1) {
    float nx = CLAMP(cx, x0, x1);
    float nz = CLAMP(cz, z0, z1);
    float dx = cx - nx; 
    float dz = cz - nz;
    return (dx*dx + dz*dz) < r*r;
}
```

## Summary for Presentation
When presenting Mathematical Foundations:
- Emphasize how **Trigonometry** (`sinf`, `cosf`) transforms a 1D "speed" variable and an angle into 2D (X, Z) coordinate movement.
- Highlight the **Pythagorean Theorem** being the heart of your collision detection logic. Avoiding the heavy `sqrt()` function and comparing squared distances (`dx*dx + dz*dz < rad*rad`) is a classic, hyper-optimized game development technique!
