// input.cpp
// keyboard, mouse and timer callbacks - week 8/9 content
// also handles collision detection to stop cars driving through objects
//
// collision approach: car = circle on XZ plane, building = AABB rectangle,
// everything else = circles. simple but good enough for this scene.

#include "input.h"

// returns true if a circle overlaps an axis-aligned bounding box
// used for the building since it's rectangular
bool circleAABB(float cx, float cz, float r,
                float x0, float x1, float z0, float z1)
{
    // find the closest point on the rectangle to the circle centre
    float nx = CLAMP(cx, x0, x1), nz = CLAMP(cz, z0, z1);
    float dx = cx - nx, dz = cz - nz;
    return (dx*dx + dz*dz) < r*r;
}

// returns true if two circles overlap
bool circleCircle(float ax, float az, float ar,
                  float bx, float bz, float br)
{
    float dx = ax-bx, dz = az-bz, d = ar+br;
    return (dx*dx + dz*dz) < d*d;
}

static const float CAR_R = 2.6f;  // car footprint radius (roughly half the diagonal of the body)

// tests whether moving the selected car to (cx,cz) would collide with anything
bool checkCollision(float cx, float cz)
{
    // office building: world footprint X[-9..9], Z[-19..-12.5]
    if (circleAABB(cx, cz, CAR_R, -9.f,9.f, -19.f,-12.5f)) return true;

    // trees
    const float txs[] = {-17.f,-17.f,-17.f, 17.f, 17.f, 17.f,-17.f,17.f};
    const float tzs[] = {-10.f,  0.f, 10.f,-10.f,  0.f, 10.f, -4.f,-4.f};
    for (int t = 0; t < 8; t++)
        if (circleCircle(cx,cz,CAR_R, txs[t],tzs[t], 1.6f)) return true;

    // other (non-selected) cars
    for (int i = 0; i < 4; i++) {
        if (i == selectedCar) continue;
        if (circleCircle(cx,cz,CAR_R, cars[i].x,cars[i].z, CAR_R)) return true;
    }

    // waste bins
    const float bins[][2] = {{-5.f,12.f},{5.f,12.f}};
    for (auto& b : bins)
        if (circleCircle(cx,cz,CAR_R, b[0],b[1], 0.6f)) return true;

    // benches
    const float benches[][2] = {{-6.f,10.f},{6.f,10.f}};
    for (auto& b : benches)
        if (circleCircle(cx,cz,CAR_R, b[0],b[1], 1.4f)) return true;

    // lamp posts
    const float lamps[][2] = {{0.f,14.f},{-12.f,14.f},{12.f,14.f}};
    for (auto& l : lamps)
        if (circleCircle(cx,cz,CAR_R, l[0],l[1], 0.55f)) return true;

    return false;
}

// animation timer - fires every 16ms (~60fps)
// updates sun position and moves the selected car if a key is held
void update(int v)
{
    if (!paused) {
        sceneTime += .016f;

        // orbit the sun slowly around the scene
        if (sunOrbiting && !nightMode) {
            sunAngle  = fmodf(sunAngle + .20f, 360.f);
            sunHeight = sinf(D2R(sunAngle)) * .78f + .22f;
            sunHeight = CLAMP(sunHeight, 0.f, 1.f);
        }

        // move selected car from held keys
        Car& c = cars[selectedCar];
        float spd = 0.f, turnR = 0.f;
        if (keys['w']||keys['W']||skeys[0]) spd   =  .12f;
        if (keys['x']||keys['X']||skeys[1]) spd   = -.08f;
        if (keys['a']||keys['A']||skeys[2]) turnR = -2.f;
        if (keys['d']||keys['D']||skeys[3]) turnR =  2.f;

        // turning is slower when the car is stationary (more realistic)
        c.heading += turnR * (ABS(spd) > 0.01f ? 1.f : 0.3f);

        float hr = D2R(-c.heading);
        float nx = CLAMP(c.x + sinf(hr)*spd, -19.f, 19.f);
        float nz = CLAMP(c.z + cosf(hr)*spd, -17.f, 17.f);

        if (!checkCollision(nx, nz)) {
            c.x = nx; c.z = nz;
            c.wheelRot += spd * R2D(1.f / 0.36f);  // rotate wheel by the right amount for its radius
            collisionBlocked = false;
        } else {
            // only show the red ring indicator when actually trying to move forward/back
            collisionBlocked = (ABS(spd) > 0.01f);
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// keyboard press
void keyDown(unsigned char k, int x, int y)
{
    keys[k] = true;
    switch (k) {
        case 27:    exit(0);
        case 'l': case 'L': aLight(); break;
        case 'z': case 'Z': aDepth(); break;
        case 'n': case 'N': aNight(); break;
        case 'p': case 'P': aPause(); break;
        case 's': case 'S': aSun();   break;
        case 'c': case 'C': aCam();   break;
        case '\t':           aTab();   break;
        case 'r': case 'R':
            // reset selected car back to centre
            cars[selectedCar].x = 0;
            cars[selectedCar].z = 0;
            break;
    }
}

void keyUp(unsigned char k, int x, int y) { keys[k] = false; }

// arrow keys (special keys in GLUT)
void specDown(int k, int x, int y)
{
    if (k == GLUT_KEY_UP)    skeys[0] = true;
    if (k == GLUT_KEY_DOWN)  skeys[1] = true;
    if (k == GLUT_KEY_LEFT)  skeys[2] = true;
    if (k == GLUT_KEY_RIGHT) skeys[3] = true;
}

void specUp(int k, int x, int y)
{
    if (k == GLUT_KEY_UP)    skeys[0] = false;
    if (k == GLUT_KEY_DOWN)  skeys[1] = false;
    if (k == GLUT_KEY_LEFT)  skeys[2] = false;
    if (k == GLUT_KEY_RIGHT) skeys[3] = false;
}

// mouse button handler
// left click: check for button hits first, then start camera drag
// scroll wheel: zoom in/out
void mouseBtn(int btn, int state, int x, int y)
{
    if (btn == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mDrag = true; lastMX = x; lastMY = y;
            int sy2 = WIN_H - y;  // flip Y because OpenGL origin is bottom-left
            for (int i = 0; i < NBTN; i++) {
                Btn& b = btns[i];
                if (x>=b.x && x<=b.x+b.w && sy2>=b.y && sy2<=b.y+b.h) {
                    b.fn(); mDrag = false; break;
                }
            }
        } else {
            mDrag = false;
        }
    }
    if (btn == 3) { camDist -= .8f; if (camDist < 5)  camDist = 5;  }  // scroll up = zoom in
    if (btn == 4) { camDist += .8f; if (camDist > 80) camDist = 80; }  // scroll down = zoom out
}

// mouse drag - rotate camera while left button is held
void mouseMove(int x, int y)
{
    if (mDrag) {
        camH += (x - lastMX) * .5f;
        camV -= (y - lastMY) * .4f;
        camV = CLAMP(camV, -5.f, 88.f);  // clamp vertical so camera doesn't flip
    }
    lastMX = x; lastMY = y;
}
