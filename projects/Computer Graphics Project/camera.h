#pragma once
#include "globals.h"

// sets up the view matrix for whichever camera mode is active
// also switches projection to orthographic if CM_ORTHO is selected
void applyCamera();

// GLUT reshape callback - resets the perspective projection on window resize
void reshape(int w, int h);
