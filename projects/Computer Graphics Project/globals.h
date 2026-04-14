#pragma once

#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// some older freeglut headers don't define these so adding them manually
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#  define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#  define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
#ifndef GL_MULTISAMPLE
#  define GL_MULTISAMPLE 0x809D
#endif

// handy math macros used all over the place
#define PI           3.14159265f
#define D2R(d)       ((d) * PI / 180.f)
#define R2D(r)       ((r) * 180.f / PI)
#define CLAMP(v,a,b) ((v)<(a)?(a):(v)>(b)?(b):(v))
#define ABS(v)       ((v)<0?-(v):(v))

// rendering/scene modes
enum LightMode { LM_FULL=0, LM_NOSPEC=1, LM_AMBONLY=2, LM_OFF=3, LM_COUNT=4 };
enum DepthMode { DM_ON=0, DM_OFF=1, DM_COUNT=2 };
// CM_ORTHO added to satisfy the orthographic projection requirement from the spec
enum CamMode   { CM_OVERVIEW=0, CM_FOLLOW=1, CM_SIDE=2, CM_ORTHO=3, CM_COUNT=4 };

// car holds position, rotation, colour and wheel animation angle
struct Car {
    float x, z;
    float heading;    // degrees around Y axis
    float r, g, b;
    const char* name;
    float wheelRot;   // accumulated rotation for the wheel spin animation
};

// window dimensions - updated in reshape callback
extern int WIN_W, WIN_H;

// 5 procedurally generated texture handles (asphalt, concrete, grass, markings, bark)
extern GLuint texID[5];

// current scene state
extern LightMode lightMode;
extern DepthMode depthMode;
extern CamMode   camMode;

extern bool nightMode;
extern bool paused;
extern bool sunOrbiting;
extern bool collisionBlocked;  // set when the selected car hits something
extern bool inShadowPass;      // stops material calls firing during shadow rendering

// sun position
extern float sunAngle;
extern float sunHeight;
extern float sceneTime;
extern float LX, LY, LZ;  // current sun direction unit vector

// the four cars in the scene
extern Car  cars[4];
extern int  selectedCar;

// keyboard state - true while a key is held down
extern bool keys[256];
extern bool skeys[8];   // arrow keys

// camera orbit state controlled by mouse
extern float camDist, camH, camV;
extern bool  mDrag;
extern int   lastMX, lastMY;
