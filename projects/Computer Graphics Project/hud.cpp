// hud.cpp
// 2D overlay drawn in screen space using gluOrtho2D
// week 10: HUD overlay, 2D orthographic projection for UI
// week 11: ethics information panel shown in the bottom-right corner
//
// the HUD is drawn after all 3D content by switching to an orthographic
// projection, drawing quads and bitmap text, then restoring 3D state

#include "hud.h"

// scene toggle functions - shared between keyboard shortcuts and button clicks
void aLight() { lightMode = (LightMode)((lightMode + 1) % LM_COUNT); }
void aDepth() { depthMode = (DepthMode)((depthMode + 1) % DM_COUNT); }
void aNight() { nightMode    = !nightMode; }
void aPause() { paused       = !paused; }
void aSun()   { sunOrbiting  = !sunOrbiting; }
void aCam()   { camMode      = (CamMode)((camMode + 1) % CM_COUNT); }
void aTab()   { selectedCar  = (selectedCar + 1) % 4; }

// clickable on-screen buttons - position, size, label, colour and callback
Btn btns[] = {
    {  8, 200, 170, 30, "[L] Lighting",   .2f, .7f, .2f, aLight },
    {  8, 166, 170, 30, "[Z] Depth Test", .7f, .3f, .2f, aDepth },
    {  8, 132, 170, 30, "[N] Day/Night",  .2f, .4f, .8f, aNight },
    {  8,  98, 170, 30, "[P] Pause",      .6f, .2f, .7f, aPause },
    {  8,  64, 170, 30, "[S] Sun Orbit",  .7f, .6f, .1f, aSun   },
    {  8,  30, 170, 30, "[C] Camera",     .3f, .6f, .6f, aCam   },
    {184,  98, 170, 30, "[TAB] Next Car", .5f, .5f, .5f, aTab   },
};
const int NBTN = 7;

void drawButtons()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < NBTN; i++) {
        Btn& b = btns[i];
        // figure out if this button's feature is currently active
        bool on = false;
        if (i == 0) on = (lightMode != LM_FULL);
        if (i == 1) on = (depthMode == DM_OFF);
        if (i == 2) on = nightMode;
        if (i == 3) on = paused;
        if (i == 4) on = sunOrbiting;

        // drop shadow to give the button some depth
        glColor4f(0, 0, 0, .4f);
        glBegin(GL_QUADS);
        glVertex2f(b.x+3,b.y-3); glVertex2f(b.x+b.w+3,b.y-3);
        glVertex2f(b.x+b.w+3,b.y+b.h-3); glVertex2f(b.x+3,b.y+b.h-3); glEnd();

        // button body - brighter when the feature is active
        float br = on ? CLAMP(b.r*1.4f,0,1) : b.r;
        float bg = on ? CLAMP(b.g*1.4f,0,1) : b.g;
        float bb = on ? CLAMP(b.b*1.4f,0,1) : b.b;
        glColor4f(br, bg, bb, on ? .92f : .72f);
        glBegin(GL_QUADS);
        glVertex2f(b.x,b.y); glVertex2f(b.x+b.w,b.y);
        glVertex2f(b.x+b.w,b.y+b.h); glVertex2f(b.x,b.y+b.h); glEnd();

        // white border
        glColor4f(1,1,1,.7f); glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(b.x,b.y); glVertex2f(b.x+b.w,b.y);
        glVertex2f(b.x+b.w,b.y+b.h); glVertex2f(b.x,b.y+b.h); glEnd();
        glLineWidth(1);

        // label text
        glColor3f(1,1,1);
        glRasterPos2i(b.x+7, b.y+10);
        for (const char* c = b.label; *c; c++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
    }

    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}

void drawHUD()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // dark semi-transparent background panel at top
    glColor4f(0, 0, 0, .62f);
    glBegin(GL_QUADS);
    glVertex2f(6,WIN_H-6); glVertex2f(WIN_W-6,WIN_H-6);
    glVertex2f(WIN_W-6,WIN_H-120); glVertex2f(6,WIN_H-120); glEnd();
    glDisable(GL_BLEND);

    // text helpers
    auto t9 = [&](int x, int y, const char* s, float r, float g, float b) {
        glColor3f(r,g,b); glRasterPos2i(x,y);
        for (const char* c=s; *c; c++) glutBitmapCharacter(GLUT_BITMAP_9_BY_15,*c);
    };
    auto t8 = [&](int x, int y, const char* s, float r, float g, float b) {
        glColor3f(r,g,b); glRasterPos2i(x,y);
        for (const char* c=s; *c; c++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*c);
    };

    char buf[256];
    t9(14, WIN_H-24, "CAR PARK SCENE  --  OpenGL Concepts Demo", 0.3f,1.f,0.4f);

    const char* lmN[] = { "FULL PHONG","NO SPECULAR","AMBIENT ONLY","LIGHTS OFF" };
    // CM_ORTHO shows as TOP-DOWN ORTHO to make the projection type clear
    const char* camN[] = { "OVERVIEW","FOLLOW CAR","SIDE VIEW","TOP-DOWN ORTHO" };

    snprintf(buf, 256, "Lighting: %-16s | Depth: %-5s | Camera: %-18s | %s",
        lmN[lightMode], depthMode==DM_ON?"ON":"OFF!",
        camN[camMode], nightMode?"NIGHT":"DAY");

    float sr = lightMode==LM_FULL?.3f : lightMode==LM_NOSPEC?1.f : lightMode==LM_AMBONLY?1.f:.7f;
    float sg = lightMode==LM_FULL?1.f : lightMode==LM_NOSPEC?.8f : lightMode==LM_AMBONLY?.5f:.7f;
    float sb = lightMode==LM_FULL?.3f:.2f;
    t8(14, WIN_H-46, buf, sr,sg,sb);

    snprintf(buf, 256, "Selected: %s  (TAB=next)  |  Sun: %.0f deg  |  %s",
        cars[selectedCar].name, sunAngle, paused?"*** PAUSED ***":"Animating");
    t8(14, WIN_H-64, buf, 1.f,1.f,.5f);

    const char* lmExp[] = {
        "Full Phong: ambient + diffuse + specular -- best depth perception",
        "No Specular: highlights removed -- surfaces look flatter",
        "Ambient Only: no directional light cues -- hard to judge depth",
        "Lights Off: flat colour only -- no 3D depth perception at all"
    };
    t8(14, WIN_H-82, lmExp[lightMode], 1.f,.95f,.65f);
    t8(14, WIN_H-100, "WASD/Arrows=Drive  TAB=Switch Car  C=Camera  N=Night  L=Lighting  P=Pause",
        .65f,.65f,.65f);
    t8(14, WIN_H-116, "Wk1:Pipeline  Wk2:Camera+Projection  Wk3-4:Modelling  Wk5:Shadows  Wk6:Colour",
        .5f,.75f,.5f);
    t8(WIN_W-390, WIN_H-100, "Wk7:Phong Lighting  Wk8-9:Keyboard+Mouse  Wk10:HUD+Textures  Wk11:Ethics  Wk12:Project",
        .5f,.75f,.5f);

    // ethics panel bottom-right (week 11)
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,.05f,.15f,.65f);
    glBegin(GL_QUADS);
    glVertex2f(WIN_W-360,6); glVertex2f(WIN_W-6,6);
    glVertex2f(WIN_W-6,90); glVertex2f(WIN_W-360,90); glEnd();
    glDisable(GL_BLEND);

    t8(WIN_W-355, 74, "Week 11 -- Ethics & Standards",          .4f,.9f,1.f);
    t8(WIN_W-355, 56, "OpenGL = open standard (Khronos Group)", .9f,.9f,.7f);
    t8(WIN_W-355, 40, "Accessibility: colour contrast for all", .9f,.9f,.7f);
    t8(WIN_W-355, 24, "IP: 3D models are copyrightable works",  .9f,.9f,.7f);

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}
