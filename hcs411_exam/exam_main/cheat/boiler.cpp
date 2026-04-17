/*=============================================================================
 *  BOILER.CPP — Exam Skeleton (Main, Reshape, Camera, Display Loop)
 *  -------------------------------------------------------------------------
 *  Legacy OpenGL  |  GLUT only  |  No GLFW / GLAD / GLM
 *
 *  HOW TO USE:
 *    1) Copy this file as your starting point.
 *    2) #include the other cheat files or paste the functions you need.
 *    3) Compile:
 *         g++ boiler.cpp geometry.cpp effects.cpp input.cpp -o exam \
 *             -lfreeglut -lopengl32 -lglu32       (MinGW / Windows)
 *         g++ *.cpp -o exam -lGL -lGLU -lglut     (Linux)
 *=============================================================================*/

#include <GL/glut.h>      // pulls in gl.h and glu.h automatically
#include <cmath>
#include <cstdio>

// ──────────── FORWARD DECLARATIONS (from other cheat files) ────────────
// Geometry
extern void drawGrid(int halfSize, float step);
extern void drawCoordinateAxes(float length);
extern void drawCube(float size);
extern void drawTeapot(float size);
extern void drawTable(float legHeight, float topWidth, float topDepth, float topThickness);

// Effects
extern void setupBasicLighting();
extern void toggleLightingMode();   // ambient ↔ specular

// Input
extern void keyboardCallback(unsigned char key, int x, int y);
extern void specialKeysCallback(int key, int x, int y);
extern void mouseCallback(int button, int state, int x, int y);
extern void mouseMotionCallback(int x, int y);

// ──────────── GLOBAL CAMERA STATE ──────────────────────────────────────
float camX = 0.0f, camY = 5.0f, camZ = 12.0f;   // camera position
float lookX = 0.0f, lookY = 0.0f, lookZ = 0.0f;   // look-at target
float camAngleY = 0.0f;                             // yaw   (degrees)
float camAngleX = 0.0f;                             // pitch (degrees)
float camDistance = 12.0f;                           // orbit radius
int   windowW = 800, windowH = 600;

/*─────────────────────────────────────────────────────────────────────────
 *  setupBasicCamera()  –  Perspective projection + gluLookAt
 *────────────────────────────────────────────────────────────────────────*/
void setupBasicCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // fov = 60°, near = 0.1, far = 200
    gluPerspective(60.0, (double)windowW / (double)windowH, 0.1, 200.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Orbit camera:  position computed from angles
    float radY = camAngleY * 3.14159f / 180.0f;
    float radX = camAngleX * 3.14159f / 180.0f;
    camX = camDistance * sinf(radY) * cosf(radX);
    camY = camDistance * sinf(radX) + 3.0f; // +3 so we look slightly down
    camZ = camDistance * cosf(radY) * cosf(radX);

    gluLookAt(camX, camY, camZ,    // eye
              lookX, lookY, lookZ,  // center
              0.0, 1.0, 0.0);      // up
}

/*─────────────────────────────────────────────────────────────────────────
 *  setupOrthographicCamera()  –  2D-style view (useful for HUD / 2D shapes)
 *────────────────────────────────────────────────────────────────────────*/
void setupOrthographicCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // left, right, bottom, top, near, far
    glOrtho(-10.0, 10.0, -10.0, 10.0, -10.0, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/*─────────────────────────────────────────────────────────────────────────
 *  reshape()  –  Window resize callback
 *────────────────────────────────────────────────────────────────────────*/
void reshape(int w, int h) {
    if (h == 0) h = 1;            // prevent divide-by-zero
    windowW = w;
    windowH = h;
    glViewport(0, 0, w, h);
    setupBasicCamera();           // recompute projection on resize
}

/*─────────────────────────────────────────────────────────────────────────
 *  display()  –  Main render loop  ★ EDIT THIS FOR YOUR EXAM ★
 *────────────────────────────────────────────────────────────────────────*/
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupBasicCamera();

    // ── Draw helpers ──
    drawGrid(10, 1.0f);            // floor grid  (±10 units, 1-unit spacing)
    drawCoordinateAxes(5.0f);      // RGB axes     (5 units long)

    // ── YOUR SCENE GOES HERE ──
    // Example: draw a cube on the floor
    glPushMatrix();
        glTranslatef(0.0f, 1.0f, 0.0f);  // lift it above the grid
        drawCube(2.0f);
    glPopMatrix();

    // Example: draw a teapot
    glPushMatrix();
        glTranslatef(4.0f, 0.8f, 0.0f);
        drawTeapot(1.0f);
    glPopMatrix();

    glutSwapBuffers();
}

/*─────────────────────────────────────────────────────────────────────────
 *  timer()  –  Animation timer (~60 FPS)
 *────────────────────────────────────────────────────────────────────────*/
void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // ≈ 60 fps
}

/*─────────────────────────────────────────────────────────────────────────
 *  init()  –  One-time OpenGL setup
 *────────────────────────────────────────────────────────────────────────*/
void init() {
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);   // dark grey background
    glEnable(GL_DEPTH_TEST);                     // z-buffer
    glEnable(GL_NORMALIZE);                      // auto-normalise normals after scaling
    glShadeModel(GL_SMOOTH);                     // Gouraud shading (default)

    setupBasicLighting();                        // from effects.cpp
}

/*═════════════════════════════════════════════════════════════════════════
 *  main()
 *═════════════════════════════════════════════════════════════════════════*/
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowW, windowH);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("OpenGL Exam — Cheat Bundle");

    init();

    // ── Register callbacks ──
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardCallback);       // from input.cpp
    glutSpecialFunc(specialKeysCallback);     // from input.cpp
    glutMouseFunc(mouseCallback);             // from input.cpp
    glutMotionFunc(mouseMotionCallback);      // from input.cpp
    glutTimerFunc(0, timer, 0);

    printf("=== OpenGL Exam Cheat Bundle ===\n");
    printf("Arrow keys / mouse drag : orbit camera\n");
    printf("W/S : zoom in/out\n");
    printf("L   : toggle lighting mode (ambient <-> specular)\n");
    printf("ESC : quit\n");

    glutMainLoop();
    return 0;
}
