# Basic Collision
Implemented basic collison detection using AABB; (LATER: implement advanced collision detection using OBB)

# Texture Maps
1. Essential Surface Textures (PBR Workflow)
For concrete/asphalt floors and walls, use the following five maps:
Albedo/Diffuse Map (RGB): The raw color of the asphalt or concrete without lighting or shadows.
Normal Map (RGB): Adds surface detail (small cracks, aggregate texture) without extra geometry, crucial for realistic lighting.
Roughness Map (Grayscale): Defines how rough or smooth the surface is. Pavements should have high roughness; wet concrete should have low roughness (white/black respectively).
Ambient Occlusion (AO) Map (Grayscale): Simulates self-shadowing in crevices (e.g., cracks in the concrete).
Metallic Map (Grayscale): Identifies non-metallic surfaces (concrete/asphalt) to prevent incorrect reflection behavior. 
Asphalt/Concrete Tileable Textures: Essential for large, repeating surfaces.
Tire/Oil Stain Decals (RGBA): Added to enhance realism, especially in parking stalls.

2. Vegetation Textures
Trees and bushes require specific maps to look realistic:
Albedo/Diffuse Map (RGB): The base color of the leaves and bark.
Alpha Map (Grayscale): Defines transparency for leaves, allowing light to pass through edges and creating a natural, non-solid look.
Normal Map (RGB): Adds fine leaf and bark detail.
Roughness Map (Grayscale): Controls the shininess of leaves (glossy) and bark (rough).

3. Building Textures
For glass windows and concrete walls:
Glass Maps:
Albedo/Diffuse Map (RGB): Base color of the glass.
Opacity Map (Grayscale): Controls transparency (black = opaque, white = transparent).
Normal Map (RGB): Adds subtle imperfections to the glass surface.
Roughness Map (Grayscale): Defines how reflective the glass is.
Concrete Wall Maps:
Albedo/Diffuse Map (RGB): The base color of the concrete.
Normal Map (RGB): Adds surface detail like cracks and pores.
Roughness Map (Grayscale): Controls the roughness of the concrete.
Metallic Map (Grayscale): Identifies non-metallic surfaces (concrete) to prevent incorrect reflection behavior.

4. Car Textures
Each car requires a set of PBR maps for realistic paint:
Albedo/Diffuse Map (RGB): The base color of the car paint.
Normal Map (RGB): Adds fine surface details like orange peel or scratches.
Roughness Map (Grayscale): Controls the glossiness of the paint (high roughness = matte, low roughness = glossy).
Metallic Map (Grayscale): Identifies metallic paints (silver, gold) to enable metallic reflections.

5. Optimization and Advanced Techniques
To ensure high-quality rendering without performance issues:
Texture Atlases: Combine multiple textures into a single larger texture to reduce draw calls.
Mipmapping: Generate lower-resolution versions of textures for objects far from the camera to prevent aliasing.
Texture Compression: Use formats like DXT/BC to reduce memory usage and improve loading times.
Anisotropic Filtering: Improve the quality of textures viewed at oblique angles (e.g., roads stretching into the distance). 
BC7: A high-quality, lossy compression format suitable for PBR textures.
BC5: A high-performance, lossy compression format suitable for normal maps.
BC4: A high-performance, lossy compression format suitable for grayscale maps (AO, Roughness).  

# Reducing LINES of Code
1. Lines 740–754 — Duplicate Parking Bay Lines: In drawGround(), the "Right bay rows" rendering logic is an exact copy of the "Left bay rows" (Lines 724–739), except for the starting horizontal offset (bx = 4.f instead of bx = -14.f).

    - Fix: You can wrap the logic in a simple outer loop for(float baseX : {-14.f, 4.f}) and remove the entire second block. (Saves ~14 lines)
2. Lines 1243–1256 — Repetitive Collision Code: In checkCollision(), you are defining separate arrays and separate for loops for bins (0.6f radius), benches (1.4f radius), and lamp posts (0.55f radius).

    - Fix: These can be squashed together into a single const float obstacles[][3] = { {x, z, radius}, ... } structure and a single loop. (Saves ~10 lines)
3. Lines 1129–1135 — Repetitive Object Placements: In display(), the function calls to drawLampPost, drawBin, drawBench, and especially drawFlowerBed are written out individually for identical symmetrical locations.

    - Fix: The drawFlowerBed calls can be condensed using a nested 1-liner loop: for(int x:{-11,11}) for(int z:{12,-2}) drawFlowerBed(x,z,4,3); (Saves ~5 lines)
4. Lines 702–707 — Redundant GL_QUADS Block: For the grass border strips in drawGround(), the left strip and right strip each have their own glBegin(GL_QUADS); ... glEnd(); wrappers.

    - Fix: Since they share the same material and texture, the right strip's four vertices can just be moved inside the left strip's glBegin/glEnd block. (Saves 2 lines)
5. Lines 278–280 — Car Bumpers: In drawCarGeom(), the front bumper and rear bumper definitions are exactly the same except for a -2.38f vs +2.38f offset.

    - Fix: It can be placed inside a 1-liner loop for(int s=-1;s<=1;s+=2) just like the headlights. (Saves 1-2 lines)

In total, condensing these sections would save roughly **32 to 35 lines of code** while achieving the exact same visual and mechanical results. Let me know if you would like me to implement these optimizations!