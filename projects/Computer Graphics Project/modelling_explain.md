# 3D Modelling in OpenGL

## 1. Concept of Modelling
In computer graphics, **modelling** refers to the process of creating mathematical representations of 3D objects. Instead of loading complex meshes created in software like Blender, the approach taken in this project explores building complex objects from basic 3D geometric **primitives** such as cubes, spheres, cylinders, and toruses.

By combining these simple shapes, we can construct recognizable objects. This technique often involves **hierarchical modelling**, where an object is broken down into a main body and its sub-parts (e.g., a car chassis has wheels, doors, headlights attached to it). To correctly position these parts relative to each other, we use a series of transformations.

## 2. Transformations
Transformations allow us to place our primitives correctly in 3D space. OpenGL uses a matrix stack to keep track of transformations.

### Key Transformation Functions:
- `glTranslatef(x, y, z)`: Moves the origin by the specified (x, y, z) offset.
- `glRotatef(angle, x, y, z)`: Rotates the local coordinate system by `angle` degrees around the vector (x, y, z).
- `glScalef(x, y, z)`: Scales the object along the X, Y, and Z axes. Used to squish or stretch default primitives (e.g., scaling a cube into a long rectangle).
- `glPushMatrix()`: Saves the current coordinate system state. Useful before modifying the system to draw a specific sub-component.
- `glPopMatrix()`: Restores the previously saved coordinate system state. Used after drawing a sub-component so its specific transformations don't affect the rest of the object.

## 3. Example: Exploring `drawCar()` and `drawCarGeom()`

Let's look at how transformations and modelling are applied to draw a car in your `objects.cpp` file.

### Step 1: Placing the Car in the World
The `drawCar` function positions the entire car in the world:

```cpp
void drawCar(int idx)
{
    Car& c = cars[idx];
    glPushMatrix(); // Save the world coordinate state
    
    // Move the entire car to its current world coordinates (x, z)
    // The Y coordinate is lifted up by 0.38 to sit perfectly on the ground
    glTranslatef(c.x, .38f, c.z);
    
    // Rotate the entire car around the Y-axis (up) to face the right direction
    glRotatef(-c.heading, 0, 1, 0);

    // Call inner function to draw the actual geometry
    drawCarGeom(c.r, c.g, c.b, c.wheelRot);
    
    glPopMatrix(); // Restore state so other objects aren't affected
}
```

### Step 2: Creating Local Pieces (`drawCarGeom`)

Inside `drawCarGeom`, everything is relative to the core of the car (established by the `glTranslatef` in `drawCar`).

#### Squishing a Cube into a Chassis
A standard `glutSolidCube(1)` creates a 1x1x1 cube. To fashion it into a car's chassis, it gets scaled and stretched:

```cpp
// Lower chassis block
safemat(cr, cg, cb, .7f, .7f, .7f, 55.f);
glPushMatrix(); 
glScalef(2.2f, .65f, 4.6f); // Stretch X, squish Y, heavily stretch Z
glutSolidCube(1); 
glPopMatrix();
```

#### Angling the Windshield
To make the windshield slanted, we take another cube and use both scaling and rotation:

```cpp
glPushMatrix(); 
// Move the windshield to upper front
glTranslatef(0, .65f, -1.52f); 
// Tip it back by 18 degrees along the X axis
glRotatef(18, 1, 0, 0); 
// Flatten it down to create a pane rather than a cube
glScalef(1.75f, .58f, .08f); 
glutSolidCube(1); 
glPopMatrix();
```

#### The Wheels (Torus + Sphere)
A wheel is fairly complex. We loop through the 4 wheels and apply `glRotatef` dynamically so that the wheels visually spin as distance increases.

```cpp
float wp[4][2] = { {-1.08f,-1.5f}, {1.08f,-1.5f}, {-1.08f,1.5f}, {1.08f,1.5f} };
for (int w = 0; w < 4; w++) {
    glPushMatrix();
    // Move to the wheel's local corner location
    glTranslatef(wp[w][0], -.02f, wp[w][1]);
    
    // Dynamic wheel spin! Rotates based on physics speed
    glRotatef(wheelRot, 1, 0, 0);   
    
    // Draw the tyre
    // Rotate the Torus 90 degrees so it stands upright
    glRotatef(90, 0, 1, 0); 
    glutSolidTorus(.20f, .36f, 10, 16);
    
    // Draw hub cap inside tyre opening
    glScalef(.22f, .22f, .09f); 
    glutSolidSphere(1, 10, 8);
    glPopMatrix();
}
```

## Summary for Presentation
For your presentation, explain that your scenes aren't pre-built meshes uploaded from 3D modeling software, but are mathematically composed in real time by scaling, rotating, and translating primary volumetric shapes (`glutSolidCube`, `glutSolidSphere`, etc.). The sequence of `glPushMatrix()` -> Transformation -> Draw Primitive -> `glPopMatrix()` is the fundamental workflow of procedural generation and rendering in legacy OpenGL.
