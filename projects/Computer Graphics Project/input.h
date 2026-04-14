#pragma once
#include "globals.h"
#include "hud.h"   // needed for Btn, btns, NBTN and the action functions

// collision helpers
bool circleAABB(float cx, float cz, float r, float x0, float x1, float z0, float z1);
bool circleCircle(float ax, float az, float ar, float bx, float bz, float br);
bool checkCollision(float cx, float cz);

// GLUT callbacks - registered in main.cpp
void update(int v);
void keyDown(unsigned char k, int x, int y);
void keyUp(unsigned char k, int x, int y);
void specDown(int k, int x, int y);
void specUp(int k, int x, int y);
void mouseBtn(int btn, int state, int x, int y);
void mouseMove(int x, int y);
