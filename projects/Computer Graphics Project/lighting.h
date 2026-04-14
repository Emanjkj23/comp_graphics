#pragma once
#include "globals.h"

// builds the 4x4 shadow projection matrix for a directional light at (lx,ly,lz)
void shadowMat(float lx, float ly, float lz, float m[16]);

// applies phong material properties - ambient is auto-derived as ~28% of diffuse
void mat(float r, float g, float b,
         float sr, float sg, float sb,
         float sh, float a = 1.f);

// same as mat() but skipped during shadow pass to avoid wasted GL calls
void safemat(float r, float g, float b,
             float sr, float sg, float sb,
             float sh, float a = 1.f);

// sets up all lights: sun (GL_LIGHT0), lamp post (GL_LIGHT1), window glows (LIGHT2-4)
void setupLights();
