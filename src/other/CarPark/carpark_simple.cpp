// ============================================================
// carpark_simple.cpp — Car Park OpenGL Scene (Presentation Version)
//
// COVERS (assignment checklist):
//   [1] SCENE MODELLING       — car, tree, building, lamp, bench, bin
//   [2] ALL TRANSFORMATIONS   — translate, rotate, scale, push/pop hierarchy
//   [3] PERSPECTIVE VIEW      — gluPerspective  (cameras 1-3)
//   [4] ORTHOGRAPHIC VIEW     — glOrtho top-down (camera 4)
//   [5] PHONG LIGHTING        — ambient, diffuse, specular; sun + lamp post
//   [6] COLOUR & BLENDING     — alpha-blended glass windows
//   [7] EVENT HANDLING        — keyboard (WASD/C/N/L/P) + mouse orbit + scroll
//   [8] ANIMATION             — wheel spin, sun orbit, day/night sky
//   [9] CODE STRUCTURE        — 10 labelled sections, each ≤ 80 lines
//
// CONTROLS:
//   W / S          — drive car forward / back
//   A / D          — turn car left / right
//   C              — cycle camera  (Overview → Follow → Side → Top-Down Ortho)
//   L              — cycle lighting (Full Phong → No Specular → Ambient Only → Off)
//   N              — toggle Day / Night
//   P              — pause / resume animation
//   Mouse drag     — orbit camera (overview mode)
//   Scroll wheel   — zoom in / out
//   ESC            — quit
// ============================================================

#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstring>

// ============================================================
// SECTION 1 — CONSTANTS & GLOBAL STATE
// ============================================================
#define PI      3.14159265f
#define D2R(d)  ((d) * PI / 180.f)
#define CLAMP(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))

int WIN_W = 1100, WIN_H = 680;

// --- Lighting mode ---
enum LightMode { LM_FULL=0, LM_NOSPEC, LM_AMBONLY, LM_OFF, LM_COUNT };
LightMode lightMode = LM_FULL;

// --- Camera mode ---
enum CamMode  { CM_OVERVIEW=0, CM_FOLLOW, CM_SIDE, CM_ORTHO_TOP, CM_COUNT };
CamMode  camMode = CM_OVERVIEW;

// --- Scene flags ---
bool nightMode   = false;
bool paused      = false;
bool sunOrbiting = true;

// --- Sun ---
float sunAngle  = 60.f;   // horizontal degrees
float sunHeight = 0.75f;  // 0=horizon, 1=zenith
float LX=0, LY=1, LZ=0;  // unit sun direction (used for shadow matrix)

// --- Driven car ---
struct Car {
    float x, z;        // world XZ position
    float heading;     // Y-rotation in degrees
    float r, g, b;     // body colour
    float wheelRot;    // rolling animation (degrees)
};
Car cars[2] = {
    { 5.f,  4.f, 0.f,  0.85f, 0.12f, 0.12f, 0.f },   // Red
    {-5.f,  4.f, 0.f,  0.15f, 0.38f, 0.78f, 0.f },   // Blue
};
int  selCar = 0;       // which car WASD controls

// --- Input ---
bool keys[256] = {};
bool skeys[4]  = {};   // arrow up/down/left/right

// --- Camera orbit (overview) ---
float camDist = 38.f, camH = 45.f, camV = 28.f;
bool  mDrag   = false;
int   lastMX  = 0, lastMY = 0;

// ============================================================
// SECTION 2 — MATERIAL & LIGHTING HELPERS
// ============================================================

// Apply a Phong material.  Respects current lightMode.
void applyMat(float r, float g, float b,
              float sr, float sg, float sb, float shine, float a = 1.f)
{
    GLfloat amb[] = { r*.25f, g*.25f, b*.25f, a };
    GLfloat dif[] = { r, g, b, a };
    GLfloat spc[] = { sr, sg, sb, a };
    GLfloat sh[]  = { shine };
    if (lightMode == LM_NOSPEC || lightMode == LM_AMBONLY) {
        spc[0] = spc[1] = spc[2] = 0.f; sh[0] = 0.f;
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  spc);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, sh);
}

// Configure sun (GL_LIGHT0) + car-park lamp (GL_LIGHT1)
void setupLights()
{
    glEnable(GL_NORMALIZE);
    if (lightMode == LM_OFF) { glDisable(GL_LIGHTING); return; }
    glEnable(GL_LIGHTING);

    // --- Sun direction --------------------------------------------------
    float sx = cosf(D2R(sunAngle)) * cosf(D2R(sunHeight * 90.f));
    float sy = sinf(D2R(sunHeight * 90.f));
    float sz = sinf(D2R(sunAngle)) * cosf(D2R(sunHeight * 90.f));
    LX = sx; LY = sy; LZ = sz;

    float dayF  = CLAMP(sy, 0.f, 1.f);
    float iv    = 0.25f + dayF * 0.75f;
    float sR    = nightMode ? 0.05f : iv * 0.95f;
    float sG    = nightMode ? 0.05f : iv * 0.88f;
    float sB    = nightMode ? 0.15f : iv * 0.78f;

    GLfloat sunPos[] = { sx, sy, sz, 0.f };           // w=0 → directional
    GLfloat sD[]     = { sR, sG, sB, 1.f };
    GLfloat sA[]     = { sR*.35f, sG*.35f, sB*.35f, 1.f };
    GLfloat sS[]     = { sR*.5f,  sG*.5f,  sB*.5f,  1.f };
    if (lightMode == LM_AMBONLY) { sD[0]=sD[1]=sD[2]=0; sS[0]=sS[1]=sS[2]=0; }
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  sD);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  sA);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sS);

    // --- Lamp-post point light -----------------------------------------
    float lR = nightMode ? 1.0f : 0.15f;
    float lG = nightMode ? 0.9f : 0.13f;
    float lB = nightMode ? 0.5f : 0.06f;
    GLfloat lpPos[] = { 0.f, 6.f, 14.f, 1.f };       // w=1 → positional
    GLfloat lD[]    = { lR, lG, lB, 1.f };
    GLfloat lA[]    = { 0.f, 0.f, 0.f, 1.f };
    GLfloat lS[]    = { lR*.4f, lG*.4f, lB*.2f, 1.f };
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, lpPos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  lD);
    glLightfv(GL_LIGHT1, GL_AMBIENT,  lA);
    glLightfv(GL_LIGHT1, GL_SPECULAR, lS);
    glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  0.6f);
    glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.08f);
    glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.01f);
}

// ============================================================
// SECTION 3 — SCENE OBJECT DRAW FUNCTIONS
// Each function uses glPushMatrix/glPopMatrix to keep transforms
// local (hierarchical modelling).
// ============================================================

// --- Ground: tarmac, parking bay lines, zebra crossing, grass ----------
void drawGround()
{
    // Tarmac base
    applyMat(.40f,.40f,.42f, .1f,.1f,.1f, 5.f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(-22,0,-20); glVertex3f(22,0,-20);
    glVertex3f( 22,0, 20); glVertex3f(-22,0, 20);
    glEnd();

    // Grass strips (left & right)
    applyMat(.30f,.65f,.20f, .05f,.10f,.03f, 5.f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(-22,.005f,-20); glVertex3f(-15,.005f,-20);
    glVertex3f(-15,.005f, 20); glVertex3f(-22,.005f, 20);
    glEnd();
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f( 15,.005f,-20); glVertex3f( 22,.005f,-20);
    glVertex3f( 22,.005f, 20); glVertex3f( 15,.005f, 20);
    glEnd();

    // Parking bay lines (white) — disable lighting for flat lines
    glDisable(GL_LIGHTING);
    glColor3f(.92f,.92f,.92f);
    for (int b = 0; b < 3; b++) {
        float bz = -12.f + b * 8.f;
        // Left row header line
        glBegin(GL_QUADS);
        glVertex3f(-14,.01f,bz);   glVertex3f(-4,.01f,bz);
        glVertex3f( -4,.01f,bz+.1f); glVertex3f(-14,.01f,bz+.1f);
        glEnd();
        // Right row header line
        glBegin(GL_QUADS);
        glVertex3f(4,.01f,bz);  glVertex3f(14,.01f,bz);
        glVertex3f(14,.01f,bz+.1f); glVertex3f(4,.01f,bz+.1f);
        glEnd();
        // Bay dividers — left
        for (int l = 0; l <= 4; l++) {
            float bx = -14.f + l * 2.5f;
            glBegin(GL_QUADS);
            glVertex3f(bx,.01f,bz);      glVertex3f(bx+.07f,.01f,bz);
            glVertex3f(bx+.07f,.01f,bz+7.4f); glVertex3f(bx,.01f,bz+7.4f);
            glEnd();
        }
        // Bay dividers — right
        for (int l = 0; l <= 4; l++) {
            float bx = 4.f + l * 2.5f;
            glBegin(GL_QUADS);
            glVertex3f(bx,.01f,bz);      glVertex3f(bx+.07f,.01f,bz);
            glVertex3f(bx+.07f,.01f,bz+7.4f); glVertex3f(bx,.01f,bz+7.4f);
            glEnd();
        }
    }
    // Centre dashed yellow line (drive aisle)
    glColor3f(.88f,.82f,.0f);
    for (int d = -5; d < 6; d++) {
        float dz = d * 3.f;
        glBegin(GL_QUADS);
        glVertex3f(-.07f,.01f,dz); glVertex3f(.07f,.01f,dz);
        glVertex3f( .07f,.01f,dz+1.9f); glVertex3f(-.07f,.01f,dz+1.9f);
        glEnd();
    }
    // Zebra crossing at entrance
    glColor3f(.94f,.94f,.94f);
    for (int zs = 0; zs < 6; zs++) {
        float zx = -3.3f + zs * 0.72f;
        glBegin(GL_QUADS);
        glVertex3f(zx,.01f,13.5f); glVertex3f(zx+.52f,.01f,13.5f);
        glVertex3f(zx+.52f,.01f,16.f); glVertex3f(zx,.01f,16.f);
        glEnd();
    }
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}

// --- Car: body + cabin + wheels (hierarchical, push/pop per sub-part) --
void drawCarGeom(float cr, float cg, float cb, float wRot)
{
    // Lower body (chassis)
    applyMat(cr, cg, cb, .65f,.65f,.65f, 50.f);
    glPushMatrix(); glScalef(2.1f, .60f, 4.4f); glutSolidCube(1); glPopMatrix();

    // Cabin
    applyMat(cr*.82f, cg*.82f, cb*.82f, .5f,.5f,.5f, 40.f);
    glPushMatrix(); glTranslatef(0, .60f, -.15f);
    glScalef(1.78f, .68f, 2.4f); glutSolidCube(1); glPopMatrix();

    // Windshield (blended glass)
    applyMat(.22f,.42f,.62f, .9f,.9f,1.f, 110.f, .5f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glPushMatrix(); glTranslatef(0,.62f,-1.48f);
    glRotatef(18,1,0,0); glScalef(1.68f,.54f,.07f); glutSolidCube(1); glPopMatrix();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);

    // Headlights
    applyMat(1.f,1.f,.85f, 1.f,1.f,.8f, 120.f);
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix(); glTranslatef(s*.74f, .18f,-2.28f);
        glScalef(.34f,.20f,.09f); glutSolidSphere(1,8,5); glPopMatrix();
    }

    // Wheels: tyre (torus) + rim (small sphere)
    float wp[4][2] = {{-1.05f,-1.45f},{1.05f,-1.45f},{-1.05f,1.45f},{1.05f,1.45f}};
    for (int w = 0; w < 4; w++) {
        glPushMatrix();
        glTranslatef(wp[w][0], -.02f, wp[w][1]);
        glRotatef(wRot, 1,0,0);
        // Tyre
        applyMat(.10f,.10f,.10f, .05f,.05f,.05f, 5.f);
        glRotatef(90,0,1,0); glutSolidTorus(.18f,.34f, 10, 16);
        // Rim
        applyMat(.76f,.76f,.80f, .9f,.9f,.9f, 90.f);
        glScalef(.20f,.20f,.08f); glutSolidSphere(1, 10, 8);
        glPopMatrix();
    }
}

void drawCar(int idx)
{
    Car& c = cars[idx];
    glPushMatrix();
    glTranslatef(c.x, .36f, c.z);
    glRotatef(-c.heading, 0,1,0);

    // Selection ring for currently driven car
    if (idx == selCar) {
        glDisable(GL_LIGHTING);
        glColor3f(1.f, 1.f, 0.f);
        glPushMatrix(); glTranslatef(0,-.34f,0);
        glRotatef(90,1,0,0); glutWireTorus(.04f,1.28f, 8, 20); glPopMatrix();
        if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
    }
    drawCarGeom(c.r, c.g, c.b, c.wheelRot);
    glPopMatrix();
}

// --- Tree: trunk cylinder + 3 foliage spheres (hierarchical) -----------
void drawTree(float x, float z, float h = 4.5f)
{
    glPushMatrix();
    glTranslatef(x, 0, z);

    // Trunk
    applyMat(.40f,.25f,.10f, .10f,.07f,.03f, 10.f);
    glPushMatrix(); glRotatef(-90,1,0,0);
    glutSolidCylinder(.25f, h*.55f, 10, 4); glPopMatrix();

    // Three foliage clusters (dark → bright layering)
    applyMat(.14f,.50f,.10f, .04f,.14f,.02f, 8.f);
    glPushMatrix(); glTranslatef( .10f, h*.60f, .08f);
    glScalef(1.3f,.82f,1.3f); glutSolidSphere(h*.28f,14,10); glPopMatrix();

    applyMat(.20f,.62f,.12f, .07f,.20f,.04f, 12.f);
    glPushMatrix(); glTranslatef(-.12f, h*.76f, -.06f);
    glScalef(1.10f,.92f,1.05f); glutSolidSphere(h*.22f,12,9); glPopMatrix();

    applyMat(.32f,.74f,.18f, .12f,.28f,.07f, 18.f);
    glPushMatrix(); glTranslatef(.04f, h*.94f, .04f);
    glutSolidSphere(h*.14f, 10, 8); glPopMatrix();

    glPopMatrix();
}

// --- Office building: concrete body + glass windows --------------------
void drawBuilding()
{
    glPushMatrix();
    glTranslatef(0, 0, -16.f);

    // Concrete walls
    applyMat(.70f,.70f,.68f, .28f,.28f,.26f, 18.f);
    glPushMatrix(); glTranslatef(0, 5.5f, 0);
    glScalef(18.f, 11.f, 6.f); glutSolidCube(1); glPopMatrix();

    // Roof parapet
    applyMat(.55f,.55f,.53f, .22f,.22f,.20f, 12.f);
    glPushMatrix(); glTranslatef(0, 11.3f, 0);
    glScalef(18.2f, .55f, 6.2f); glutSolidCube(1); glPopMatrix();

    // Glass window panels — alpha blending
    applyMat(.24f,.44f,.64f, .88f,.88f,1.f, 120.f, .65f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int fl = 0; fl < 3; fl++)
        for (int col = -3; col <= 3; col++) {
            glPushMatrix(); glTranslatef(col*2.5f, 2.f+fl*3.f, 3.05f);
            glScalef(1.85f, 2.0f, .05f); glutSolidCube(1); glPopMatrix();
        }
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);

    // Night window glow (emissive flat colour, no lighting)
    if (nightMode) {
        glDisable(GL_LIGHTING);
        glColor3f(1.f, .94f, .60f);
        for (int fl = 0; fl < 3; fl++)
            for (int col = -3; col <= 3; col++) {
                glPushMatrix(); glTranslatef(col*2.5f, 2.f+fl*3.f, 3.08f);
                glScalef(1.65f,1.85f,.03f); glutSolidCube(1); glPopMatrix();
            }
        if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
    }

    // Entrance door (dark glass strip)
    applyMat(.14f,.24f,.34f, .45f,.45f,.65f, 75.f, .78f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glPushMatrix(); glTranslatef(0, .95f, 3.1f);
    glScalef(2.f, 1.9f, .05f); glutSolidCube(1); glPopMatrix();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);

    glPopMatrix();
}

// --- Lamp post: steel pole + warm bulb sphere -------------------------
void drawLampPost(float x, float z)
{
    glPushMatrix(); glTranslatef(x, 0, z);

    applyMat(.28f,.28f,.30f, .48f,.48f,.48f, 60.f);
    glPushMatrix(); glRotatef(-90,1,0,0);
    glutSolidCylinder(.07f, 5.5f, 10, 3); glPopMatrix();
    glPushMatrix(); glTranslatef(0,5.5f,0);
    glScalef(.45f,.28f,.45f); glutSolidCube(1); glPopMatrix();

    // Bulb glow (emissive — no lighting)
    glDisable(GL_LIGHTING);
    float glow = nightMode ? 1.f : 0.22f;
    glColor3f(glow, glow*.88f, glow*.48f);
    glPushMatrix(); glTranslatef(0,5.4f,0);
    glutSolidSphere(.15f, 10, 8); glPopMatrix();
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);

    glPopMatrix();
}

// --- Bench: wooden slats + metal legs ---------------------------------
void drawBench(float x, float z, float ry = 0.f)
{
    glPushMatrix(); glTranslatef(x,0,z); glRotatef(ry,0,1,0);

    // Seat slats
    applyMat(.52f,.33f,.14f, .14f,.09f,.07f, 10.f);
    for (int s = 0; s < 3; s++) {
        glPushMatrix(); glTranslatef(0,.46f,s*.13f-.13f);
        glScalef(1.55f,.06f,.09f); glutSolidCube(1); glPopMatrix();
    }
    // Metal legs
    applyMat(.44f,.44f,.46f, .5f,.5f,.5f, 45.f);
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix(); glTranslatef(s*.62f,.22f,0);
        glScalef(.06f,.46f,.06f); glutSolidCube(1); glPopMatrix();
    }
    glPopMatrix();
}

// --- Bin: cylinder body + lid -----------------------------------------
void drawBin(float x, float z)
{
    glPushMatrix(); glTranslatef(x,0,z);
    applyMat(.12f,.36f,.12f, .08f,.18f,.08f, 15.f);
    glPushMatrix(); glRotatef(-90,1,0,0);
    glutSolidCylinder(.20f,.82f,12,4); glPopMatrix();
    applyMat(.08f,.28f,.08f, .06f,.12f,.06f, 10.f);
    glPushMatrix(); glTranslatef(0,.88f,0);
    glScalef(1.f,.1f,1.f); glutSolidCylinder(.22f,0.f,12,1); glPopMatrix();
    glPopMatrix();
}

// ============================================================
// SECTION 4 — FLAT SHADOW PROJECTION
// Derivation: P' = P - (Py / Ly) * L  (project onto y=0 plane)
// ============================================================
void shadowMatrix(float lx, float ly, float lz, float m[16])
{
    if (ly < 0.01f) ly = 0.01f;
    // Column-major layout for OpenGL
    m[0]=ly; m[4]=-lx; m[8] =0;  m[12]=0;
    m[1]=0;  m[5]=0;   m[9] =-lz;m[13]=0;
    m[2]=0;  m[6]=0;   m[10]=ly; m[14]=0;
    m[3]=0;  m[7]=0;   m[11]=0;  m[15]=ly;
}

void drawAllShadows()
{
    if (LY < 0.06f) return;
    float alpha = CLAMP(LY * .55f, .08f, .55f);
    if (nightMode) alpha = .08f;

    float sm[16]; shadowMatrix(LX, LY, LZ, sm);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(.03f,.04f,.02f, alpha);

    glPushMatrix();
    glTranslatef(0, .012f, 0);
    glMultMatrixf(sm);
    // Shadow geometry (same shapes, no materials needed)
    for (int i = 0; i < 2; i++) drawCar(i);
    drawBuilding();
    drawLampPost(0,14);
    float txs[]={-16,-16,16,16}, tzs[]={-8,8,-8,8};
    for (int t = 0; t < 4; t++) drawTree(txs[t],tzs[t]);
    glPopMatrix();

    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}

// ============================================================
// SECTION 5 — SKY GRADIENT & SUN DISC
// ============================================================
void drawSky()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    float dayF = CLAMP(LY, 0.f, 1.f);
    float tR, tG, tB, hR, hG, hB;
    if (!nightMode) {
        tR=.25f+dayF*.12f; tG=.45f+dayF*.10f; tB=.88f;
        hR=.68f; hG=.78f; hB=.92f;
    } else {
        tR=.01f; tG=.01f; tB=.06f;
        hR=.03f; hG=.03f; hB=.10f;
    }
    glClearColor(hR,hG,hB,1);
    float W=200, H=90;
    // Draw sky box faces with gradient (horizon colour → top colour)
    float faces[4][4][3] = {
        {{-W,0,-W},{W,0,-W},{W,H,-W},{-W,H,-W}},
        {{-W,0, W},{W,0, W},{W,H, W},{-W,H, W}},
        {{-W,0,-W},{-W,0,W},{-W,H,W},{-W,H,-W}},
        {{ W,0,-W},{ W,0,W},{ W,H,W},{ W,H,-W}},
    };
    for (int f = 0; f < 4; f++) {
        glBegin(GL_QUADS);
        glColor3f(hR,hG,hB); glVertex3fv(faces[f][0]);
        glColor3f(hR,hG,hB); glVertex3fv(faces[f][1]);
        glColor3f(tR,tG,tB); glVertex3fv(faces[f][2]);
        glColor3f(tR,tG,tB); glVertex3fv(faces[f][3]);
        glEnd();
    }
    glBegin(GL_QUADS); glColor3f(tR,tG,tB);
    glVertex3f(-W,H,-W); glVertex3f(W,H,-W);
    glVertex3f( W,H, W); glVertex3f(-W,H, W); glEnd();

    // Night stars
    if (nightMode) {
        glPointSize(1.6f); srand(99999);
        glBegin(GL_POINTS);
        for (int i = 0; i < 500; i++) {
            float a1=((float)rand()/RAND_MAX)*2*PI;
            float a2=((float)rand()/RAND_MAX)*PI*.45f;
            float b=.35f+((float)rand()/RAND_MAX)*.65f;
            glColor3f(b,b,b*.92f);
            glVertex3f(90*cosf(a2)*cosf(a1), 90*sinf(a2)+5, 90*cosf(a2)*sinf(a1));
        }
        glEnd();
    }
    glEnable(GL_DEPTH_TEST);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}

void drawSunDisc()
{
    float sx=cosf(D2R(sunAngle))*16.f, sy=LY*16.f, sz=sinf(D2R(sunAngle))*16.f;
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    if (!nightMode) {
        glColor3f(1.f, .94f, .45f);
        glPushMatrix(); glTranslatef(sx,sy,sz); glutSolidSphere(1.3f,16,12); glPopMatrix();
    } else {
        glColor3f(.86f,.86f,.78f);
        glPushMatrix(); glTranslatef(sx*.6f,sy*.6f,sz*.6f); glutSolidSphere(1.0f,14,10); glPopMatrix();
    }
    glEnable(GL_DEPTH_TEST);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}

// ============================================================
// SECTION 6 — CAMERA / PROJECTION
// 4 modes:
//   CM_OVERVIEW    — perspective, free orbit with mouse
//   CM_FOLLOW      — perspective, behind the selected car
//   CM_SIDE        — perspective, fixed side angle
//   CM_ORTHO_TOP   — ORTHOGRAPHIC top-down (satisfies the requirement)
// ============================================================
void applyCamera()
{
    Car& sel = cars[selCar];
    switch (camMode) {
        case CM_OVERVIEW: {
            float hr = D2R(camH), vr = D2R(camV);
            float cx = camDist*cosf(vr)*sinf(hr);
            float cy = camDist*sinf(vr) + 1.f;
            float cz = camDist*cosf(vr)*cosf(hr);
            gluLookAt(cx,cy,cz, 0,2,0, 0,1,0);
            break;
        }
        case CM_FOLLOW: {
            float hd = D2R(-sel.heading);
            float ex = sel.x + sinf(hd)*9.f;
            float ez = sel.z + cosf(hd)*9.f;
            gluLookAt(ex, 5.f, ez, sel.x, .5f, sel.z, 0,1,0);
            break;
        }
        case CM_SIDE:
            gluLookAt(28,12,0, 0,2,0, 0,1,0);
            break;
        case CM_ORTHO_TOP: {
            // Orthographic: switch projection here, look straight down
            glMatrixMode(GL_PROJECTION); glLoadIdentity();
            float hs  = 25.f;             // half-size of ortho viewport
            float asp = (float)WIN_W/WIN_H;
            glOrtho(-hs*asp, hs*asp, -hs, hs, 0.1, 200.0);
            glMatrixMode(GL_MODELVIEW);
            gluLookAt(0,50,0, 0,0,0, 0,0,-1);  // eye above, looking down
            break;
        }
        default: break;
    }
}

void reshape(int w, int h)
{
    WIN_W = w; WIN_H = (h == 0) ? 1 : h;
    glViewport(0, 0, WIN_W, WIN_H);
    // Default perspective — overridden per-frame when ortho mode is active
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(55.0, (double)WIN_W/WIN_H, 0.3, 500.0);
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
// SECTION 7 — 2D HUD OVERLAY
// Drawn in orthographic screen-space (gluOrtho2D) so it is not
// affected by the 3D camera or projection.
// ============================================================
void drawHUD()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Semi-transparent top bar
    glColor4f(0.f, 0.f, 0.f, .60f);
    glBegin(GL_QUADS);
    glVertex2f(5,WIN_H-5); glVertex2f(WIN_W-5,WIN_H-5);
    glVertex2f(WIN_W-5,WIN_H-96); glVertex2f(5,WIN_H-96);
    glEnd();
    glDisable(GL_BLEND);

    auto text = [&](int x, int y, const char* s, float r, float g, float b){
        glColor3f(r,g,b); glRasterPos2i(x,y);
        for (const char* c = s; *c; c++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
    };

    const char* camNames[] = {"Overview (Persp)", "Follow Car (Persp)", "Side (Persp)", "Top-Down (ORTHO)"};
    const char* lmNames[]  = {"FULL PHONG", "NO SPECULAR", "AMBIENT ONLY", "LIGHTS OFF"};

    text(12, WIN_H-22, "CAR PARK — OpenGL Scene (Simplified Presentation)", 0.3f,1.f,0.4f);

    char buf[256];
    snprintf(buf,256,"Camera: %-20s | Lighting: %-14s | %s",
        camNames[camMode], lmNames[lightMode], nightMode?"NIGHT":"DAY");
    text(12, WIN_H-42, buf, 1.f,1.f,.5f);

    snprintf(buf,256,"Car[%s] x=%.1f z=%.1f heading=%.0f deg  |  Sun angle=%.0f deg",
        selCar==0?"Red":"Blue", cars[selCar].x, cars[selCar].z,
        cars[selCar].heading, sunAngle);
    text(12, WIN_H-60, buf, .80f,.88f,1.f);

    text(12, WIN_H-78, "WASD=Drive  C=Camera  L=Lighting  N=Day/Night  P=Pause  Tab=Switch  ESC=Quit", .65f,.65f,.65f);

    // Concept map footer
    text(12, WIN_H-94, "Wk1:Pipeline  Wk2:Camera/Ortho  Wk3-4:Modelling  Wk5:Transforms/Shadows  Wk6:Colour  Wk7:Phong  Wk8-9:Events  Wk10:HUD", .5f,.78f,.5f);

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}

// ============================================================
// SECTION 8 — DISPLAY (full render pipeline, Week 1)
// ============================================================
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Restore default perspective each frame (ortho mode overrides inside applyCamera)
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(55.0, (double)WIN_W/WIN_H, 0.3, 500.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    applyCamera();
    setupLights();

    drawSky();
    drawSunDisc();
    drawAllShadows();   // shadows BEFORE ground (stencil-free)
    drawGround();

    // Scene objects
    drawBuilding();
    drawLampPost(0,14); drawLampPost(-11,14); drawLampPost(11,14);
    float txs[]={-16,-16,16,16}, tzs[]={-8,8,-8,8};
    for (int t = 0; t < 4; t++) drawTree(txs[t],tzs[t]);
    drawBench(-6,11); drawBench(6,11,180);
    drawBin(-4,13);   drawBin(4,13);

    for (int i = 0; i < 2; i++) drawCar(i);

    drawHUD();
    glutSwapBuffers();
}

// ============================================================
// SECTION 9 — INPUT HANDLERS
// ============================================================
void keyDown(unsigned char k, int, int)
{
    keys[k] = true;
    switch (k) {
        case 27:  exit(0);
        case 'c': case 'C': camMode   = (CamMode)  ((camMode  +1)%CM_COUNT);   break;
        case 'l': case 'L': lightMode = (LightMode)((lightMode+1)%LM_COUNT);   break;
        case 'n': case 'N': nightMode = !nightMode;                              break;
        case 'p': case 'P': paused    = !paused;                                 break;
        case '\t':           selCar   = (selCar + 1) % 2;                       break;
    }
}
void keyUp(unsigned char k, int, int) { keys[k] = false; }

void specDown(int k, int, int)
{
    if (k==GLUT_KEY_UP)    skeys[0]=true;
    if (k==GLUT_KEY_DOWN)  skeys[1]=true;
    if (k==GLUT_KEY_LEFT)  skeys[2]=true;
    if (k==GLUT_KEY_RIGHT) skeys[3]=true;
}
void specUp(int k, int, int)
{
    if (k==GLUT_KEY_UP)    skeys[0]=false;
    if (k==GLUT_KEY_DOWN)  skeys[1]=false;
    if (k==GLUT_KEY_LEFT)  skeys[2]=false;
    if (k==GLUT_KEY_RIGHT) skeys[3]=false;
}

void mouseBtn(int btn, int state, int x, int y)
{
    if (btn == GLUT_LEFT_BUTTON) {
        mDrag = (state == GLUT_DOWN);
        lastMX = x; lastMY = y;
    }
    if (btn == 3) { camDist -= .9f; if (camDist<5)  camDist=5;  }
    if (btn == 4) { camDist += .9f; if (camDist>80) camDist=80; }
}
void mouseMove(int x, int y)
{
    if (mDrag) {
        camH += (x - lastMX) * .5f;
        camV -= (y - lastMY) * .4f;
        camV  = CLAMP(camV, -5.f, 88.f);
    }
    lastMX = x; lastMY = y;
}

// ============================================================
// SECTION 10 — ANIMATION TIMER + MAIN
// ============================================================
void update(int)
{
    if (!paused) {
        // Sun orbit
        if (sunOrbiting && !nightMode) {
            sunAngle  = fmodf(sunAngle + .20f, 360.f);
            sunHeight = sinf(D2R(sunAngle))*.78f + .22f;
            sunHeight = CLAMP(sunHeight, 0.f, 1.f);
        }
        LX = cosf(D2R(sunAngle))*cosf(D2R(sunHeight*90.f));
        LY = sinf(D2R(sunHeight*90.f));
        LZ = sinf(D2R(sunAngle))*cosf(D2R(sunHeight*90.f));

        // Drive selected car
        Car& c = cars[selCar];
        float spd = 0.f, turn = 0.f;
        if (keys['w']||keys['W']||skeys[0]) spd =  .11f;
        if (keys['s']||keys['S']||skeys[1]) spd = -.07f;
        if (keys['a']||keys['A']||skeys[2]) turn = -2.2f;
        if (keys['d']||keys['D']||skeys[3]) turn =  2.2f;
        if (spd != 0.f) c.heading += turn;
        float hr = D2R(-c.heading);
        c.x = CLAMP(c.x + sinf(hr)*spd, -18.f, 18.f);
        c.z = CLAMP(c.z + cosf(hr)*spd, -16.f, 16.f);
        // Wheel rotation proportional to distance travelled
        float wheelRadius = 0.34f;
        c.wheelRot += spd * (180.f / (PI * wheelRadius));
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(60, 40);
    glutCreateWindow("Car Park Scene — Simplified OpenGL Demo");

    glClearColor(.55f, .78f, .96f, 1.f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);   // Gouraud interpolation across faces

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specDown);
    glutSpecialUpFunc(specUp);
    glutMouseFunc(mouseBtn);
    glutMotionFunc(mouseMove);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}
