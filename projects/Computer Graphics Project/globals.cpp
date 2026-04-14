// globals.cpp
// definitions for all the shared global variables declared in globals.h
// keeping them here avoids multiple definition errors when linking

#include "globals.h"

int WIN_W = 1280, WIN_H = 760;

GLuint texID[5] = { 0, 0, 0, 0, 0 };

LightMode lightMode = LM_FULL;
DepthMode depthMode = DM_ON;
CamMode   camMode   = CM_OVERVIEW;

bool nightMode        = false;
bool paused           = false;
bool sunOrbiting      = true;
bool collisionBlocked = false;
bool inShadowPass     = false;

float sunAngle  = 60.f;
float sunHeight = 0.75f;
float sceneTime = 0.f;
float LX = 0.f, LY = 1.f, LZ = 0.f;

// four cars placed in parking bays at startup
Car cars[4] = {
    {  8.f,  5.f, 0.f, 0.85f, 0.10f, 0.10f, "Red Sedan",   0.f },
    { -8.f,  5.f, 0.f, 0.15f, 0.35f, 0.75f, "Blue Hatch",  0.f },
    {  8.f, -5.f, 0.f, 0.78f, 0.78f, 0.80f, "Silver SUV",  0.f },
    { -8.f, -5.f, 0.f, 0.90f, 0.82f, 0.10f, "Yellow Taxi", 0.f },
};
int selectedCar = 0;

bool keys[256] = {};
bool skeys[8]  = {};

float camDist = 35.f, camH = 45.f, camV = 30.f;
bool  mDrag   = false;
int   lastMX  = 0, lastMY = 0;
