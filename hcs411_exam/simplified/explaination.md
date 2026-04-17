If your teapot looks like a flat sticker, it’s usually one of two things:

1. Missing Depth Test: Without glEnable(GL_DEPTH_TEST), OpenGL draws things in the order they appear in your code. If you draw a table and then a teapot "behind" it, the teapot will still appear on top.

2. Missing Lighting/Normals: In a world without light, a 3D sphere and a 2D circle look identical because they are both just a solid "flat" color. Lighting creates the gradients that trick the eye into seeing 3D.

Function Breakdown: The "Manual" Controls
|:--Function|:--Real-World Analogy|:--What the Variables Do |
glClear(...)The EraserClears the "Chalkboard." COLOR_BUFFER wipes the background color; DEPTH_BUFFER wipes the memory of which object was "in front.
"glLoadIdentity()The Reset ButtonMoves your "drawing hand" back to $(0,0,0)$ and resets rotation to zero. Always call this before starting a new object.
gluLookAt(...)The Tripod(EyeX,Y,Z): Where the camera is.(CenterX,Y,Z): What you are looking at.(UpX,Y,Z): Which way is "up" (usually $0, 1, 0$).
glutSwapBuffers()The Page FlipIn double buffering, you draw on a "back" page while the user sees the "front." This function flips them so the user sees the finished drawing.glMatrixMode(...)The WorkstationPROJECTION: You are adjusting the lens of the camera.MODELVIEW: You are moving objects or the camera itself in the world.gluPerspective(...)The LensFOV: Zoom level ($45.0$ is standard).Aspect: Screen width/height.Near/Far: The closest and furthest things the camera can "see."