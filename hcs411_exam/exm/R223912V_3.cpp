#include <GL/glut.h>
#include <cmath>
#include <cstdio>

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


/* ═══════════════════════════════════════════════════════════════════════════
 *  UTILITY / DEBUG HELPERS                         [UTL]
 * ═══════════════════════════════════════════════════════════════════════════*/

/*─── drawGrid(halfSize, step)  –  flat grid on the XZ plane at y=0  ────
 *  Draws from -halfSize to +halfSize with the given step.                 */
void drawGrid(int halfSize, float step) {
    glDisable(GL_LIGHTING);         // draw grid without lighting
    glColor3f(0.35f, 0.35f, 0.35f); // subtle grey
    glBegin(GL_LINES);
        for (float i = -halfSize; i <= halfSize; i += step) {
            // lines parallel to Z (varying X)
            glVertex3f(i, 0.0f, (float)-halfSize);
            glVertex3f(i, 0.0f, (float) halfSize);
            // lines parallel to X (varying Z)
            glVertex3f((float)-halfSize, 0.0f, i);
            glVertex3f((float) halfSize, 0.0f, i);
        }
    glEnd();
    glEnable(GL_LIGHTING);          // restore lighting
}

/*─── drawCoordinateAxes(length)  –  RGB = XYZ  ─────────────────────────
 *  Red = +X,  Green = +Y,  Blue = +Z                                     */
void drawCoordinateAxes(float length) {
    glDisable(GL_LIGHTING);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
        // X axis – Red
        glColor3f(1, 0, 0);  glVertex3f(0, 0, 0);  glVertex3f(length, 0, 0);
        // Y axis – Green
        glColor3f(0, 1, 0);  glVertex3f(0, 0, 0);  glVertex3f(0, length, 0);
        // Z axis – Blue
        glColor3f(0, 0, 1);  glVertex3f(0, 0, 0);  glVertex3f(0, 0, length);
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

/*  drawQuad2D(w, h)  –  rectangle centred at origin                       */
void square(float w, float h) {
    float hw = w * 0.5f, hh = h * 0.5f;
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex2f(-hw, -hh);
        glVertex2f( hw, -hh);
        glVertex2f( hw,  hh);
        glVertex2f(-hw,  hh);
    glEnd();
}

// thos code draws 6 concentric squares that get progressively smaller and
// first outer square is red and the inner 5 squares are white
void display(void) {
    int i;
    float s;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Basic Camera
    gluLookAt(0.0, 0.0, 10.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0);

    // ── Draw helpers ──
    drawGrid(10, 1.0f);            // floor grid  (±10 units, 1-unit spacing)
    drawCoordinateAxes(10.0f);      // RGB axes     (5 units long)

    for(i=0;i<6;i++) {
        glPushMatrix();

        s=(6.0 - (float)i)/6.0 ;

        glScalef(s, s, s);
        // COde segmaents required
        glRotatef(45.0, 0.0, 0.0, 1.0);
        glTranslatef(-5.0, -0.75, -0.4);

        if (i=0)
            glColor3f(1.0, 0.0, 0.0);  // draws red
        else
            glColor3f(1.0, 1.0, 1.0);  // draws white
        square(1, 1);
        glPopMatrix();
    }
    
    glutSwapBuffers();
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
}

/*═════════════════════════════════════════════════════════════════════════
 *  main()
 *═════════════════════════════════════════════════════════════════════════*/
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowW, windowH);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("HCS411 Exam - Question 1");

    init();

    // ── Register callbacks ──
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}