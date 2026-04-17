# Viewing and Projection (Cameras) in OpenGL

In 3D computer graphics, we have a 3D world but we must display it on a flat 2D monitor. The concepts of **Viewing** and **Projection** govern exactly how we mathematically translate our 3D coordinates onto the screen.

## 1. Projection (The Lens)
Projection determines what kind of "lens" the camera is using. It defines the viewing volume, determining what gets clipped out and whether objects shrink as they get further away. 

In OpenGL, this is managed by switching into the projection stack: `glMatrixMode(GL_PROJECTION)`. 

Your project implements two types of projection required by the assignment:

### Perspective Projection
This simulates human vision: objects further away appear smaller. 
In your `reshape()` function and at the start of your main `display()` loop, you set this up as the default:
```cpp
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
// field-of-view of 55 degrees, aspect ratio matching window, near plane 0.3, far plane 600
gluPerspective(55.0, (double)WIN_W / WIN_H, 0.3, 600.0);
```

### Orthographic Projection
In orthographic mode, parallel lines stay parallel, and objects do not get smaller as distance increases. This is very useful for "blueprints" or top-down maps.
In `camera.cpp`, you switch to this mode when `CM_ORTHO` is active:
```cpp
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
// glOrtho defines a rectangular box: (left, right, bottom, top, near, far)
glOrtho(-range*aspect, range*aspect, -range, range, 1.0, 200.0);
```

## 2. Viewing (The Camera Placement)
Viewing defines where the camera is standing and where it is looking. In OpenGL, the viewing transformation is logically applied to the model-view matrix (`glMatrixMode(GL_MODELVIEW)`), effectively moving the *entire world* in the opposite direction of the camera movement.

A helper function `gluLookAt` simplifies this math. It takes 9 arguments:
`gluLookAt( eyeX, eyeY, eyeZ,   centerX, centerY, centerZ,   upX, upY, upZ );`
- **Eye**: Where the camera is located.
- **Center**: The point the camera is staring at in 3D space.
- **Up**: Which way is "up" for the camera.

### Camera Examples in `applyCamera()`

#### 1. Orbiting Overview Camera (`CM_OVERVIEW`)
This camera lets the user zoom and drag the mouse to look around the center of the car park. It uses spherical coordinates (trigonometry with sine and cosine) to calculate the `eye` position:
```cpp
// hr/vr are horizontal/vertical angles from the mouse
float cx = camDist * cosf(vr) * sinf(hr);
float cy = camDist * sinf(vr) + 1.f;
float cz = camDist * cosf(vr) * cosf(hr);
// Camera looks at center point (0, 2, 0)
gluLookAt(cx, cy, cz,   0, 2, 0,   0, 1, 0);
```

#### 2. Follow Camera (`CM_FOLLOW`)
This places the camera as a "chase cam" directly behind the active car. It takes the car's current `(x, z)` position and its `heading` (angle), and calculates a spot behind the car for the eye:
```cpp
float hd = D2R(-sel.heading);
float ex = sel.x + sinf(hd)*8.f; 
float ez = sel.z + cosf(hd)*8.f;
// The eye is at (ex, 5.5, ez) and stares directly at the car's center (sel.x, 0.5, sel.z)
gluLookAt(ex, 5.5f, ez,   sel.x, .5f, sel.z,   0, 1, 0);
```

#### 3. Straight Down Orthographic (`CM_ORTHO`)
Coupled with the `glOrtho` projection mentioned earlier, this view points straight down at the ground. Notice the `UP` vector has changed! An up vector of `0,1,0` wouldn't work when looking straight down the Y axis, so the camera considers negative-Z ("North") to be "up".
```cpp
// Eye is really high in the sky (Y=60), looking at origin (0,0,0)
gluLookAt(0, 60, 0,   0, 0, 0,   0, 0, -1);
```

## Summary for Presentation
For your presentation, be sure to highlight that you have fulfilled the requirement for multiple views and camera modes. You can easily justify your design choice: 
1. **Perspective** for an immersive feeling (Follow cam, Orbit cam).
2. **Orthographic** for a tactical, map-like top-down overview. 
3. **Spherical coordinates / Trigonometry** calculated in real time to create smooth orbiting cameras and chase cameras.`
