# Mathematical Foundations in Computer Graphics

This presentation outlines the core mathematical concepts driving the Car Park OpenGL scene. By mapping theoretical concepts to our `main.cpp` code, we can understand exactly how these foundations are applied in a real 3D application.

---

## 1. Core Mathematical Constants & Macros

Transformations fundamentally rely on trigonometry. Since standard C++ math functions (`sin`, `cos`, etc.) operate in radians, but humans and OpenGL functions often think in degrees, we must convert between the two.

**In `main.cpp` (Lines 50–55):**
```cpp
#define PI      3.14159265f
#define D2R(d)  ((d)*PI/180.f)
#define R2D(r)  ((r)*180.f/PI)
```
- **`D2R`**: Degrees to Radians conversion. Used every time an angle (like a car's heading or the sun's angle) must be fed into `sin()` or `cos()`.

---

## 2. Coordinate Systems

In 3D graphics, a vertex passes through multiple coordinate spaces before appearing on the screen. The two most critical spaces modeled in our scene are the **View (Camera) Space** and the **Projection (Clip) Space**.

### View Coordinate System
The `gluLookAt` function constructs the View Matrix, transforming world coordinates into camera-relative coordinates.

**In `main.cpp` (Lines 1066–1090 - `applyCamera()`):**
```cpp
float hr=D2R(camH),vr=D2R(camV);
float cx=camDist*cosf(vr)*sinf(hr);
float cy=camDist*sinf(vr)+1.f;
float cz=camDist*cosf(vr)*cosf(hr);
gluLookAt(cx,cy,cz, 0,2,0, 0,1,0);
```
- We calculate the camera's `(cx, cy, cz)` position using spherical coordinates.
- **`gluLookAt`** defines the coordinate system: where the camera is located, where it's looking (`0,2,0`), and which way is "UP" (`0,1,0`). 

### Projection Matrix
**In `main.cpp` (Lines 1095–1100 - `reshape()`):**
```cpp
gluPerspective(55.,(double)WIN_W/WIN_H,.3,600.);
```
- Maps the 3D viewing frustum to a 2D screen coordinate frame, factoring in the Field of View (55 degrees) and Aspect Ratio, providing a realistic sense of depth.

---

## 3. Vector Transformations

Vectors represent direction and magnitude. In our scene, calculating positions dynamically from angles requires resolving these 2D/3D vectors.

### Example A: The Sun's Direction Vector
We compute the Light Vector explicitly so that shadows and lighting intensities update accurately as the sun orbits.
**In `main.cpp` (Lines 173–175 - `setupLights()`):**
```cpp
float sx=cosf(D2R(sunAngle))*cosf(D2R(sunHeight*90.f));
float sy=sinf(D2R(sunHeight*90.f));
float sz=sinf(D2R(sunAngle))*cosf(D2R(sunHeight*90.f));
```
- A 3D vector calculated using a dual-angle (azimuth and elevation) spherical-to-Cartesian conversion.

### Example B: Moving the Car
The selected car moves relative to its current heading angle.
**In `main.cpp` (Lines 1280–1284 - `update()`):**
```cpp
float hr=D2R(-c.heading);
float nx=CLAMP(c.x+sinf(hr)*spd,-19.f,19.f);
float nz=CLAMP(c.z+cosf(hr)*spd,-17.f,17.f);
```
- The speed `spd` is the vector magnitude.
- `sin(hr)` and `cos(hr)` resolve the forward movement vector into `x` and `z` direction components. 

---

## 4. Affine Transformations

Affine transformations — **translation, rotation, and scaling** — preserve colinearity and ratios of distances. We manipulate the OpenGL ModelView Matrix to position objects locally.

### Hierarchical Modelling using Matrix Stacks
**In `main.cpp` (Lines 301–318 - `drawCar()`):**
```cpp
glPushMatrix(); // Save the current coordinate state
glTranslatef(c.x,.38f,c.z); // Translate (move) the car to its world X,Z
glRotatef(-c.heading,0,1,0); // Rotate the car to face its heading direction
...
drawCarGeom(c.r,c.g,c.b,c.wheelRot); // Draw the inner geometry
glPopMatrix(); // Restore the coordinate state
```
- The car geometry is drawn around the origin `(0,0,0)`. The affine transformations move that local coordinate system to the correct place in the World Coordinate System.

### Custom Shadow Transformation Matrix
A unique demonstration of affine matrix math is generating drop-shadows by squashing 3D geometry onto a 2D plane based on a light vector.
**In `main.cpp` (Lines 138–145 - `shadowMat()`):**
```cpp
m[0]=ly; m[1]=0;  m[2]=0;  m[3]=0;
m[4]=-lx;m[5]=0;  m[6]=-lz;m[7]=0;
m[8]=0;  m[9]=0;  m[10]=ly;m[11]=0;
m[12]=0; m[13]=0; m[14]=0; m[15]=ly;
```
- A custom 4x4 matrix mathematically projects shapes straight onto the `Y=0` floor plane along the directional vector of the sun `(lx, ly, lz)`. This leverages the core linear algebra foundations of 3D graphics rendering.
