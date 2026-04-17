# Lighting and Shading in OpenGL

To make a 3D scene look realistic, we need to correctly simulate how light bounces off surfaces. Your project implements the **Phong Reflection Model**, which is the standard lighting approach in legacy OpenGL.

The implementation handles both the **Light Sources** (emitters of light) and the **Materials / Shading** (how objects react to that light) in `lighting.cpp`.

---

## 1. The Three Components of Light (Phong Model)

The `mat()` function in your code sets up how every object interacts with light using three specific mathematical components:

```cpp
void mat(float r, float g, float b, float sr, float sg, float sb, float sh, float a)
{
    GLfloat amb[] = { r*.28f, g*.28f, b*.28f, a }; // Ambient
    GLfloat dif[] = { r, g, b, a };                // Diffuse
    GLfloat spc[] = { sr, sg, sb, a };             // Specular
    GLfloat sh2[] = { sh };                        // Shininess
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  spc);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, sh2);
}
```

### 1. Ambient Light (`GL_AMBIENT`)
Ambient light represents scattered light that bounces around the environment, hitting everything uniformly. It guarantees that areas not directly hit by a light source aren't pure pitch-black. 
- In your material setter, you intelligently calculate ambient reflection to be roughly 28% of the main diffuse color (`r*.28f`). 

### 2. Diffuse Light (`GL_DIFFUSE`)
Diffuse light is the core illumination on a surface. It simulates a "matte" finish. The brightness is calculated based on the angle between the surface normal (which way the surface is facing) and the incoming light direction. 
- A face pointing directly at the sun is bright; an angled face is darker. You pass exactly the `r, g, b` values into the diffuse slot.

### 3. Specular Light (`GL_SPECULAR`)
Specular light creates the mirror-like shiny highlight on objects (like the glossy sheen on a car chassis). This is highly dependent on the viewing angle (camera position) relative to the reflected light ray.
- You configure both the specular color (`sr, sg, sb`) and the `GL_SHININESS` (`sh`), which controls how tight or spread-out the shiny highlight is.

---

## 2. Light Sources

Creating materials is only half the battle; we also need the emitters! The `setupLights()` function defines the light emitters.

### Directional Lighting (The Sun)
Directional lights simulate light sources that are infinitely far away. All their light rays run in parallel.
```cpp
// sx, sy, sz calculate the sun's position. 
// A 'w' value of 0.f tells OpenGL this is a DIRECTIONAL light.
GLfloat sunPos[] = { sx, sy, sz, 0.f };  
glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
// Sun also sets the global Ambient light
glLightfv(GL_LIGHT0, GL_AMBIENT,  sA);
```

### Positional Lighting & Attenuation (The Lamp Post)
A positional (point) light originates from an exact `(X,Y,Z)` coordinate and spreads outwards in all directions. 
Because it exists within the scene, its light must get weaker as you get further away. This is called **Attenuation**.
```cpp
// w=1.f tells OpenGL this is a POSITIONAL point light
GLfloat lpPos[] = { 0.f, 6.f, 14.f, 1.f };  
glLightfv(GL_LIGHT1, GL_POSITION, lpPos);

// Calculate the light falloff (attenuation equation)
glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  .5f);
glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    .08f);
glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, .015f);
```

## Summary for Presentation
For your presentation, be sure to demonstrate how you implemented the **Phong Lighting Model**. 
- Highlight the difference between your **Sun** (Directional light casting parallel rays across the entire scene, dynamically changing color based on height/time of day) and your **Lamp Post** (Positional point light demonstrating mathematical distance attenuation).
- Show an object (like the car) and point out how it has an ambient base color, a dominant diffuse shade, and a bright specular glint when viewed from the correct angle.
