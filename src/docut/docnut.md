{chameleon?json
{
"component": "LlmGeneratedComponent",
"props": {
"height": "800px",
"prompt": "Create an interactive 3D particle simulation using Three.js that visualizes mathematical transformations (Translate, Rotate, Scale) and Camera controls applied to an animated Torus (donut-knot) shape. The widget is designed for a Computer Science student to explore 3D concepts for a class assignment.

Objective: Visually demonstrate 3D object manipulation (Model Matrix) and camera viewing (View/Projection Matrices) using a complex torus-knot path.

Initial Values:
Torus Major Radius (A): 10
Torus Minor Radius (B): 3
Knot P: 2 (twists around the symmetry axis)
Knot Q: 3 (twists around the tube)
Particle Count: 25000
Particle Speed: 0.02
Breeze Strength: 0.2
Rotation Speed (Object): 0.5
Scale (Object): 1.0 (X, Y, Z locked)
Translate X (Object): 0
Translate Y (Object): 0
Translate Z (Object): 0
Camera FOV: 75 degrees
Camera Position X/Y/Z: (0, 0, 30)

Controls (organized by function):
1.  **Object Transformations (HCT 401 Assignment):**
    * Sliders for individual Scale X, Scale Y, Scale Z.
    * Slider for automated Rotation Speed.
    * Sliders for Translate X, Translate Y, Translate Z.
2.  **Camera & Viewing (HCT 401 Assignment):**
    * Slider for Camera FOV.
    * A set of 4 buttons (reset to standard views): 'Top (0, 30, 0)', 'Front (0, 0, 30)', 'Side (30, 0, 0)', 'Default (0, 10, 30)'.
3.  **Geometry & Environment:**
    * Sliders for Torus shape (Major A, Minor B, Knot P, Knot Q).
    * Sliders for Particle parameters (Count, Speed, Breeze Strength).
    * Checkbox for 'Enable Glow'.

Behavior:
The 3D canvas is the primary focus. A 3D orbital camera allows the user to navigate the scene independently of the camera controls.
The object (torus-knot) is rendered using thousands of animated particles/points. The particles flow along the mathematical path of the torus-knot, reusing the glowing particle swarm aesthetic.
All 'Object Transformation' parameters (Scale, Rotation Speed, Translation) must be applied mathematically to the model matrix, causing the torus shape to scale, rotate, and move in real-time.
All 'Camera & Viewing' parameters (FOV, View Buttons) must update the camera's View Matrix and Projection Matrix, altering the perspective or angle *without* moving the object itself.
Animate the flow of particles along the torus surface continuously, and apply a subtle animated 'breeze' offset to their positions.

Functional Description: This widget must clearly separate controls that manipulate the object itself from controls that manipulate the viewpoint (the camera). The user should be able to see the donut scale *and* then move it in world space using translation, and observe this from a fixed or orbital camera."
}
}
}

**The Mathematics of a Torus**
To build a torus, we cannot just type out the vertices. We use Parametric Equations inside nested `for` loops. 
We need two angles:
- `u`: The angle around the main, large ring (the "Major" radius).
- `v`: The angle around the thickness of the tube (the "Minor" radius).
- The math for any point `(x, y, z)` on the torus is: 
    - `x = (R + r \cdot \cos(v)) \cdot \cos(u)`
    - `y = (R + r \cdot \cos(v)) \cdot \sin(u)`
    - `z = r \cdot \sin(v)`
    (Where `R` is the major radius and `r` is the minor radius).

**The "T-R-S" Assignment Code (`src/donut.cpp`)**
- This code demonstrates an animation using Translate, Rotate, and Scale (TRS) applied to a mathematically generated 3D model.
- Replace your main.cpp with this. 
- Notice how we use `glm::pi` for safe `Pi` calculations and how the Model matrix in the `display()` function is now explicitly handling Scale, Rotate, and Translate in real-time.