# Texture Mapping & Procedural Generation

To add realism to object surfaces (like making the ground look like asphalt or grass), we use **Texture Mapping**. Instead of coloring a whole object with one solid color, we "wrap" it in a 2D image.

In your codebase, this logic lives primarily in `textures.cpp` and is applied in `objects.cpp`.

## 1. Procedural Texture Generation

A major standout feature of your project is that you **did not** load external image files (like `.png` or `.jpg` files). External files can make the codebase messy and require complex image-loader libraries. 

Instead, you use **Procedural Generation**. Your `makeNoise` function mathematically generates textures pixel-by-pixel using random number algorithms (`rand()`), creating convincing noise patterns!

```cpp
// math creates noise around a base RGB color
void makeNoise(unsigned char* p, int sz, unsigned char r, unsigned char g, unsigned char b, int var, unsigned seed) {
    srand(seed); // Seed ensures the random pattern is exactly the same every time
    for (int i = 0; i < sz * sz; i++) {
        int v = rand() % var - var / 2; // Math adds/subtracts variation from the base color
        p[i*3+0] = CLAMP(r + v, 0, 255);
        p[i*3+1] = CLAMP(g + v, 0, 255);
        p[i*3+2] = CLAMP(b + v, 0, 255);
    }
}
```
You build Asphalt, Grass, Concrete, and Bark entirely out of math!

## 2. Advanced Texture Properties (Mipmapping & Anisotropy)

When setting up your texturing (`glTexParameteri`), you apply high-end visual optimizations:

### Mipmapping
If you have a high-resolution 1024x1024 texture, but it's assigned to a parking spot far away (only taking up 4 pixels on the screen), OpenGL struggles to squeeze it down, causing "glittering" or "aliasing". 
Your codebase uses `gluBuild2DMipmaps()`, which automatically generates progressively smaller, pre-blurred versions of the texture. OpenGL smoothly swaps to smaller textures for objects further away!
```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
// Automatically generates the scaled-down mip-chain!
gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, SZ, SZ, GL_RGB, GL_UNSIGNED_BYTE, buf);
```

### Anisotropic Filtering
Standard textures get very blurry when viewed at a steep, shallow angle (like standing on the asphalt and looking way down the road). 
You combat this by probing the GPU for Anisotropic Filtering support:
```cpp
// Sharpens ground textures when viewed at an angle!
float maxAniso = 1.f;
glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso > 8.f ? 8.f : maxAniso);
```


## 3. Mapping Textures to Geometry (UV Mapping)

Once the textures exist in memory, you apply them to objects. The technique is called UV mapping (using `u,v` coordinates).

In `drawGround()` in `objects.cpp`, you bind the texture and specify the `glTexCoord2f(u, v)` *before* issuing each `glVertex3f(x, y, z)`.  

```cpp
glBindTexture(GL_TEXTURE_2D, texID[0]); // Bind the Asphalt procedural texture
glBegin(GL_QUADS); 
// Bottom Left corner of texture maps to bottom left corner of ground
glTexCoord2f(0,0); glVertex3f(-20, 0,-18);
// Bottom Right corner 
// NOTE: u=8. The texture repeats 8 times horizontally so it isn't overly stretched!
glTexCoord2f(8,0); glVertex3f( 20, 0,-18);
// Top Right corner (v=9, repeats 9 times vertically)
glTexCoord2f(8,9); glVertex3f( 20, 0, 18);
// Top Left corner
glTexCoord2f(0,9); glVertex3f(-20, 0, 18); 
glEnd();
```

## Summary for Presentation
For your presentation, you can heavily leverage the fact that you generated **Procedural Textures**. This demonstrates advanced computational thinking over just loading a JPEG. 
Additionally, explaining the `glTexCoord2f(U, V)` repeating mapping strategy and the inclusion of **Mipmaps** / **Anisotropic filtering** guarantees full marks for texture knowledge, as these are professional features used in major game engines.
