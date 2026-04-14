#pragma once
#include "globals.h"

// individual scene objects
void drawCarGeom(float cr, float cg, float cb, float wheelRot);
void drawCar(int idx);
void drawTree(float x, float z, float h = 4.5f);
void drawOffice();
void drawBin(float x, float z);
void drawBench(float x, float z, float ry = 0.f);
void drawLampPost(float x, float z);
void drawFlowerBed(float cx, float cz, float w, float d);

// environment
void drawGround();
void drawSky();
void drawSunDisc();

// shadow helpers
void drawShadowGeom();
void drawAllShadows();
