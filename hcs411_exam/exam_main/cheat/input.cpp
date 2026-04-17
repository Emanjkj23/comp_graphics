/*=============================================================================
 *  INPUT.CPP — Keyboard & Mouse Templates
 *  -------------------------------------------------------------------------
 *  Legacy OpenGL  |  GLUT only  |  No GLFW / GLAD / GLM
 *
 *  QUICK INDEX (Ctrl+F the tag to jump):
 *    [KBD]  keyboardCallback       (regular keys: WASD, ESC, L, etc.)
 *    [SPK]  specialKeysCallback    (arrow keys, F-keys)
 *    [MOU]  mouseCallback          (click)
 *    [MOT]  mouseMotionCallback    (drag)
 *    [IDL]  idleCallback           (continuous animation, optional)
 *=============================================================================*/

#include <GL/glut.h>
#include <cstdio>
#include <cstdlib>

// ──────────── EXTERNS from other files ─────────────────────────────────
// Camera state (defined in boiler.cpp)
extern float camAngleY, camAngleX, camDistance;
extern float lookX, lookY, lookZ;

// Effects toggles (defined in effects.cpp)
extern void toggleLightingMode();
extern void toggleShadeModel();
extern void enableAlphaBlending();
extern void disableAlphaBlending();
extern void enableFog(float r, float g, float b, float density);
extern void disableFog();


/* ═══════════════════════════════════════════════════════════════════════════
 *  KEYBOARD — regular keys (ASCII)                [KBD]
 *
 *  Common bindings you can remix for your exam:
 *    W/S        — zoom in/out
 *    A/D        — orbit left/right
 *    R/F        — orbit up/down
 *    L          — toggle lighting mode (ambient ↔ specular)
 *    G          — toggle shade model (flat ↔ smooth)
 *    B          — toggle alpha blending
 *    T          — toggle fog
 *    1-5        — switch scene / objects
 *    ESC        — quit
 * ═══════════════════════════════════════════════════════════════════════════*/

static bool blendingOn = false;
static bool fogOn = false;
static int  sceneMode = 0;   // you can use this to switch scenes

void keyboardCallback(unsigned char key, int x, int y) {
    switch (key) {
        // ── Camera zoom ──
        case 'w': case 'W':
            camDistance -= 0.5f;
            if (camDistance < 1.0f) camDistance = 1.0f;
            break;
        case 's': case 'S':
            camDistance += 0.5f;
            break;

        // ── Camera orbit (horizontal) ──
        case 'a': case 'A':
            camAngleY -= 3.0f;
            break;
        case 'd': case 'D':
            camAngleY += 3.0f;
            break;

        // ── Camera orbit (vertical) ──
        case 'r': case 'R':
            camAngleX += 2.0f;
            if (camAngleX > 89.0f) camAngleX = 89.0f;
            break;
        case 'f': case 'F':
            camAngleX -= 2.0f;
            if (camAngleX < -89.0f) camAngleX = -89.0f;
            break;

        // ── Look-at target movement (move what you're looking at) ──
        case 'j': case 'J':  lookX -= 0.3f; break;
        case 'l': // lowercase L conflicts with lighting, using ';' or skip
            break;
        case ';':             lookX += 0.3f; break;
        case 'i': case 'I':  lookZ -= 0.3f; break;
        case 'k': case 'K':  lookZ += 0.3f; break;

        // ── Toggle lighting mode ──
        case 'L':
            toggleLightingMode();
            break;

        // ── Toggle shade model ──
        case 'g': case 'G':
            toggleShadeModel();
            break;

        // ── Toggle blending ──
        case 'b': case 'B':
            blendingOn = !blendingOn;
            if (blendingOn) enableAlphaBlending();
            else            disableAlphaBlending();
            break;

        // ── Toggle fog ──
        case 't': case 'T':
            fogOn = !fogOn;
            if (fogOn) enableFog(0.5f, 0.5f, 0.5f, 0.04f);
            else       disableFog();
            break;

        // ── Scene switching (useful in exams to show different demos) ──
        case '1': sceneMode = 0; printf("Scene: 0\n"); break;
        case '2': sceneMode = 1; printf("Scene: 1\n"); break;
        case '3': sceneMode = 2; printf("Scene: 2\n"); break;
        case '4': sceneMode = 3; printf("Scene: 3\n"); break;
        case '5': sceneMode = 4; printf("Scene: 4\n"); break;

        // ── Reset camera ──
        case '0':
            camAngleY = 0; camAngleX = 0; camDistance = 12.0f;
            lookX = 0; lookY = 0; lookZ = 0;
            printf("Camera reset.\n");
            break;

        // ── Quit ──
        case 27:  // ESC
            printf("Bye!\n");
            exit(0);
            break;

        default:
            printf("Key pressed: '%c' (ASCII %d)\n", key, (int)key);
            break;
    }

    glutPostRedisplay();
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  SPECIAL KEYS — arrow keys, F1-F12, etc.         [SPK]
 * ═══════════════════════════════════════════════════════════════════════════*/

void specialKeysCallback(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:
            camAngleY -= 5.0f;
            break;
        case GLUT_KEY_RIGHT:
            camAngleY += 5.0f;
            break;
        case GLUT_KEY_UP:
            camAngleX += 3.0f;
            if (camAngleX > 89.0f) camAngleX = 89.0f;
            break;
        case GLUT_KEY_DOWN:
            camAngleX -= 3.0f;
            if (camAngleX < -89.0f) camAngleX = -89.0f;
            break;

        // F-keys for quick toggles
        case GLUT_KEY_F1:
            printf("=== HELP ===\n");
            printf("Arrows/WASD : orbit/zoom\n");
            printf("L           : toggle lighting\n");
            printf("G           : toggle shading\n");
            printf("B           : toggle blending\n");
            printf("T           : toggle fog\n");
            printf("1-5         : switch scene\n");
            printf("0           : reset camera\n");
            printf("ESC         : quit\n");
            break;
        case GLUT_KEY_F2:
            toggleLightingMode();
            break;
        case GLUT_KEY_F3:
            toggleShadeModel();
            break;

        default:
            printf("Special key: %d\n", key);
            break;
    }

    glutPostRedisplay();
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  MOUSE — click callback                         [MOU]
 * ═══════════════════════════════════════════════════════════════════════════*/

static int mouseLastX = 0, mouseLastY = 0;
static bool mouseDragging = false;

void mouseCallback(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseDragging = true;
            mouseLastX = x;
            mouseLastY = y;
        } else {
            mouseDragging = false;
        }
    }

    // ── Scroll wheel zoom (freeglut extension) ──
    if (button == 3) {  // scroll up
        camDistance -= 1.0f;
        if (camDistance < 1.0f) camDistance = 1.0f;
        glutPostRedisplay();
    }
    if (button == 4) {  // scroll down
        camDistance += 1.0f;
        glutPostRedisplay();
    }
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  MOUSE MOTION — drag to orbit                   [MOT]
 * ═══════════════════════════════════════════════════════════════════════════*/

void mouseMotionCallback(int x, int y) {
    if (mouseDragging) {
        int dx = x - mouseLastX;
        int dy = y - mouseLastY;

        camAngleY += dx * 0.5f;    // horizontal drag → yaw
        camAngleX += dy * 0.3f;    // vertical drag   → pitch

        // Clamp pitch
        if (camAngleX >  89.0f) camAngleX =  89.0f;
        if (camAngleX < -89.0f) camAngleX = -89.0f;

        mouseLastX = x;
        mouseLastY = y;
        glutPostRedisplay();
    }
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  IDLE CALLBACK (optional)                       [IDL]
 *
 *  Use this instead of glutTimerFunc if you need continuous animation.
 *  Register with: glutIdleFunc(idleCallback);
 * ═══════════════════════════════════════════════════════════════════════════*/

static float animAngle = 0.0f;  // shared rotation angle for animations

void idleCallback() {
    animAngle += 0.5f;
    if (animAngle > 360.0f) animAngle -= 360.0f;
    glutPostRedisplay();
}

// Getter so display() can use the animation angle
float getAnimAngle() { return animAngle; }

// Getter for scene mode so display() can switch scenes
int getSceneMode() { return sceneMode; }
