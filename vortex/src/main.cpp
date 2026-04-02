/**g++ main.cpp glad.c -Iinclude -lglfw3 -lopengl32 -lgdi32 -o vortex.exe
 * Compute shader simulation
 * Curl noise + vortex rings combined
 * Biot–Savart–inspired vortex filament field
 * Framebuffer ping-pong trails
 * Camera zoom + slow rotation
 * Blue/orange palette (clearly marked for editing)

🔥 What I also want to have (visually):
 * swirling multi-loop vortex petals
 * smooth fluid motion (curl noise)
 * physically inspired Biot–Savart flow
 * long glowing particle trails
 * slow nebula-like rotation + breathing zoom

 * Best tweaks to match the exact image
 * More “petals”:
 * sin(angle * 8.0 + time);
 * Sharper golden streaks:
 * float t = pow(length(uv), 0.5);
 * Longer trails
 * col *= 0.985;

 * Next level (seriously cool):
 * real 3D vortex rings (not just 2D projection)
 * interactive controls (mouse injects vortices)
 * 1M+ particles optimized
 */