// camera.cpp
// four camera modes - three perspective and one orthographic
// week 2: viewing transforms, gluLookAt, perspective vs orthographic projection
//
// modes:
//   CM_OVERVIEW  - orbiting overhead view, mouse drag to rotate, scroll to zoom
//   CM_FOLLOW    - third-person camera that follows the selected car
//   CM_SIDE      - fixed side-on view, useful for comparing object heights
//   CM_ORTHO     - top-down orthographic projection (no perspective distortion)
//                  added to meet the spec requirement for orthographic view

#include "camera.h"

// called at the start of every display frame
// sets up the view (and projection for ortho mode)
void applyCamera()
{
    Car& sel = cars[selectedCar];

    switch (camMode) {
        case CM_OVERVIEW: {
            // convert horizontal/vertical angles to a position on a sphere
            float hr = D2R(camH), vr = D2R(camV);
            float cx = camDist * cosf(vr) * sinf(hr);
            float cy = camDist * sinf(vr) + 1.f;
            float cz = camDist * cosf(vr) * cosf(hr);
            gluLookAt(cx, cy, cz,  0, 2, 0,  0, 1, 0);
            break;
        }
        case CM_FOLLOW: {
            // position the camera behind the car based on its heading angle
            float hd = D2R(-sel.heading);
            float ex = sel.x + sinf(hd)*8.f, ez = sel.z + cosf(hd)*8.f;
            gluLookAt(ex, 5.5f, ez,  sel.x, .5f, sel.z,  0, 1, 0);
            break;
        }
        case CM_SIDE: {
            // fixed side view - good for showing that objects have proper height
            gluLookAt(25, 12, 0,  0, 2, 0,  0, 1, 0);
            break;
        }
        case CM_ORTHO: {
            // orthographic top-down view - no perspective distortion so parallel lines
            // stay parallel, useful to see the whole car park layout clearly
            // need to replace the projection matrix that reshape() set up
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            float aspect = (float)WIN_W / (float)WIN_H;
            float range  = 22.0f;
            // glOrtho(left, right, bottom, top, near, far)
            glOrtho(-range*aspect, range*aspect, -range, range, 1.0, 200.0);
            glMatrixMode(GL_MODELVIEW);
            // look straight down - up vector points toward -Z (north on the map)
            gluLookAt(0, 60, 0,   0, 0, 0,   0, 0, -1);
            break;
        }
        default: break;
    }
}

// called by GLUT whenever the window is resized
// sets perspective as the default projection - ortho mode overrides this each frame
void reshape(int w, int h)
{
    WIN_W = w;
    WIN_H = (h == 0) ? 1 : h;
    glViewport(0, 0, WIN_W, WIN_H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, (double)WIN_W / WIN_H, 0.3, 600.0);
    glMatrixMode(GL_MODELVIEW);
}
