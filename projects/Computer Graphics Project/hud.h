#pragma once
#include "globals.h"

// toggle functions - called by both keyboard shortcuts and button clicks
void aLight();
void aDepth();
void aNight();
void aPause();
void aSun();
void aCam();
void aTab();

// on-screen button definition
struct Btn {
    int x, y, w, h;
    const char* label;
    float r, g, b;
    void (*fn)();   // action to run on click
};

extern Btn      btns[];
extern const int NBTN;

// draws the HUD info panel at the top of the screen
void drawHUD();

// draws the clickable control buttons in the bottom-left corner
void drawButtons();
