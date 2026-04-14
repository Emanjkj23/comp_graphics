// main.cpp
// entry point - initialises GLUT, registers callbacks and starts the main loop
// week 1: graphics pipeline, GLUT setup, double buffering, depth buffer
//
// the display() function below is the render loop - it calls into all the other
// modules to build up the full scene each frame
//
// HOW TO COMPILE (MinGW / g++):
//   g++ -o main main.cpp globals.cpp lighting.cpp textures.cpp objects.cpp camera.cpp hud.cpp input.cpp -lfreeglut -lopengl32 -lglu32
//
// FILE STRUCTURE:
//   globals.h/.cpp   - shared state (enums, structs, global variables)
//   lighting.h/.cpp  - phong material helper, shadow matrix, light setup
//   textures.h/.cpp  - procedural texture generation
//   objects.h/.cpp   - all 3D scene objects (cars, trees, building, ground, sky)
//   camera.h/.cpp    - camera modes including orthographic view
//   hud.h/.cpp       - 2D HUD overlay and clickable buttons
//   input.h/.cpp     - keyboard/mouse callbacks and collision detection
//   main.cpp         - this file: GLUT init and main render loop

#include "globals.h"
#include "textures.h"
#include "lighting.h"
#include "objects.h"
#include "camera.h"
#include "hud.h"
#include "input.h"

// called every frame by GLUT (via glutPostRedisplay in the update timer)
// draws everything in the correct order: sky -> shadows -> ground -> objects -> HUD
void display()
{
    // reset projection to perspective at the start of each frame
    // the ortho camera mode will override this inside applyCamera() if needed
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, (double)WIN_W / WIN_H, 0.3, 600.0);
    glMatrixMode(GL_MODELVIEW);

    if (depthMode == DM_OFF) glDisable(GL_DEPTH_TEST);
    else                     glEnable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    applyCamera();   // sets view matrix (and projection if in ortho mode)
    setupLights();

    // draw order matters - sky first (no depth write), then shadows on ground,
    // then ground, then all solid objects, then transparent HUD on top
    drawSky();
    drawSunDisc();
    drawAllShadows();
    drawGround();

    // place 8 trees around the perimeter
    float txs[] = { -17,-17,-17, 17, 17, 17,-17, 17 };
    float tzs[] = { -10,  0, 10,-10,  0, 10, -4, -4 };
    for (int t = 0; t < 8; t++) drawTree(txs[t], tzs[t]);

    drawOffice();
    drawLampPost(0,14); drawLampPost(-12,14); drawLampPost(12,14);
    drawBin(-5,12);     drawBin(5,12);
    drawBench(-6,10);   drawBench(6,10, 180.f);
    drawFlowerBed(-11,12,4,3); drawFlowerBed(11,12,4,3);
    drawFlowerBed(-11,-2,4,3); drawFlowerBed(11,-2,4,3);

    for (int i = 0; i < 4; i++) drawCar(i);

    // 2D overlay drawn last so it appears on top of everything
    drawHUD();
    drawButtons();

    glutSwapBuffers();  // swap front/back buffers (double buffering from week 1)
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    // request double buffering, RGB colour, depth buffer and MSAA anti-aliasing
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(50, 30);
    glutCreateWindow("Car Park Scene -- OpenGL Demo");

    glClearColor(.5f, .75f, .97f, 1);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);       // gouraud shading
    glEnable(GL_MULTISAMPLE);      // hardware MSAA to smooth polygon edges
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    initTextures();  // generate all procedural textures before the loop starts

    // register all GLUT callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specDown);
    glutSpecialUpFunc(specUp);
    glutMouseFunc(mouseBtn);
    glutMotionFunc(mouseMove);
    glutTimerFunc(16, update, 0);  // start the 60fps animation timer

    glutMainLoop();
    return 0;
}
