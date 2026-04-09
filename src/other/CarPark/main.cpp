// ============================================================
// CAR PARK SCENE - Complete OpenGL Demonstration
// All 12 Course Weeks Covered in One Interactive Scene
//
// SCENE: A realistic car park with:
//   - Tarmac road + painted bays + zebra crossing
//   - 4 cars (red, blue, silver, yellow) with real-world colours
//   - Office building (glass windows, facade)
//   - Trees with layered foliage casting shadows
//   - Waste bin, resting benches
//   - Flower beds along paths
//   - Moving sun that casts live shadows
//   - Day / Night transition
//
// CONCEPTS DEMONSTRATED:
//   Week 1  - Graphics pipeline, GLUT init, double buffering
//   Week 2  - Perspective projection, gluLookAt, 3 camera modes
//   Week 3-4- Scene graph, hierarchical modelling, Push/Pop matrix
//   Week 5  - Affine transforms, shadow matrix derivation, trig
//   Week 6  - RGB materials, depth test, alpha blending
//   Week 7  - Phong lighting (directional sun + point lamp), Gouraud
//   Week 8-9- Keyboard, mouse, timer, clickable HUD buttons
//   Week 10 - 2D HUD overlay, procedural texture mapping, mipmaps, anisotropic
//             filtering, MSAA anti-aliasing (GL_MULTISAMPLE)
//   Week 11 - Ethics panel visible in HUD
//   Week 12 - Full mini-project integration
//
// CONTROLS  (keyboard + clickable on-screen buttons):
//   W A S D / Arrows - Move selected car  OR  walk person
//   ENTER            - Enter / Exit car (person must be within 3 units)
//   TAB              - Switch selected car (only while driving)
//   C                - Cycle camera (4 modes incl. 1st-person)
//   N                - Toggle Day / Night
//   L                - Cycle lighting mode
//   Z                - Toggle depth test
//   P                - Pause / Resume
//   S                - Toggle sun orbit
//   I K              - Move building forward / backward
//   J L              - Rotate building left / right
//   U O              - Strafe building left / right
//   Mouse Drag       - Orbit camera (overview mode)
//   Scroll           - Zoom
//   Click buttons    - Same as keyboard shortcuts
//   ESC              - Quit
// ============================================================

#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Extension / core constants that older freeglut headers may omit
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#  define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#  define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
#ifndef GL_MULTISAMPLE
#  define GL_MULTISAMPLE  0x809D
#endif

// ============================================================
// WEEK 5 - MATH FOUNDATIONS
// ============================================================
#define PI      3.14159265f
#define D2R(d)  ((d)*PI/180.f)
#define R2D(r)  ((r)*180.f/PI)
#define CLAMP(v,a,b) ((v)<(a)?(a):(v)>(b)?(b):(v))
#define ABS(v)  ((v)<0?-(v):(v))

int WIN_W=1280, WIN_H=760;

// ============================================================
// WEEK 10 - TEXTURE HANDLES
// 5 procedurally-generated textures (no external files needed)
//   0 = asphalt      (dark grey gritty surface)
//   1 = concrete     (light grey, slightly rough)
//   2 = grass        (green stippled)
//   3 = road-marking (yellow/white paint)
//   4 = tyre-mark    (dark rubber smudge decal)
// ============================================================
GLuint texID[5]={0,0,0,0,0};

// ============================================================
// SCENE STATE
// ============================================================
enum LightMode { LM_FULL=0,LM_NOSPEC=1,LM_AMBONLY=2,LM_OFF=3,LM_COUNT=4 };
enum DepthMode { DM_ON=0,DM_OFF=1,DM_COUNT=2 };
enum CamMode   { CM_FOLLOW=0,CM_OVERVIEW=1,CM_SIDE=2,CM_FIRST_PERSON=3,CM_COUNT=4 };

LightMode lightMode = LM_FULL;
DepthMode depthMode = DM_ON;
CamMode   camMode   = CM_OVERVIEW;

bool nightMode        = false;
bool paused           = false;
bool sunOrbiting      = true;
bool collisionBlocked = false;  // true this frame when selected car hit an obstacle

// ============================================================
// BUILDING STATE (movable with I/J/K/L/U/O)
// ============================================================
float bldgX       = 0.f;
float bldgZ       = 0.f;
float bldgHeading = 0.f;   // degrees Y rotation

// ============================================================
// PERSON STATE
// ============================================================
struct Person {
    float x, z;     // world position
    float heading;  // degrees Y rotation
    bool  inCar;    // true when seated in a car
    float legAnim;  // walk cycle phase (radians)
};
Person player = { 0.f, 10.f, 0.f, false, 0.f };

// Building move keys (separate from WASD)
bool bkeys[6]={};  // i,k,j,l,u,o

float sunAngle   = 60.f;   // degrees around Y axis
float sunHeight  = 0.75f;  // 0=horizon,1=zenith
float sceneTime  = 0.f;

// Sun direction (unit toward sky, used for shadow matrix)
float LX=0,LY=1,LZ=0;

// ============================================================
// WEEK 3-4 - CAR DATA (selected car moves with WASD)
// ============================================================
struct Car {
    float x,z;         // position on ground
    float heading;     // degrees Y rotation
    float r,g,b;       // real-world car colour
    const char* name;
    float wheelRot;    // rolling animation
};
Car cars[4] = {
    { 8.f, 5.f,  0.f,  0.85f,0.10f,0.10f, "Red Sedan",    0.f},
    {-8.f, 5.f,  0.f,  0.15f,0.35f,0.75f, "Blue Hatch",   0.f},
    { 8.f,-5.f,  0.f,  0.78f,0.78f,0.80f, "Silver SUV",   0.f},
    {-8.f,-5.f,  0.f,  0.90f,0.82f,0.10f, "Yellow Taxi",  0.f},
};
int selectedCar = 0;

// ============================================================
// WEEK 8-9 - INPUT STATE
// ============================================================
bool keys[256]={};
bool skeys[8]={};

// Camera
float camDist=35.f, camH=45.f, camV=30.f;
bool  mDrag=false;
int   lastMX=0,lastMY=0;

// ============================================================
// WEEK 5 - SHADOW MATRIX
// Projects geometry onto y=0 plane given directional light (lx,ly,lz)
// lx,ly,lz = direction toward the light source
// Shadow point: P' = P - (Py/ly)*L
// ============================================================
void shadowMat(float lx,float ly,float lz,float m[16])
{
    if(ly<0.01f)ly=0.01f;
    m[0]=ly; m[1]=0;  m[2]=0;  m[3]=0;
    m[4]=-lx;m[5]=0;  m[6]=-lz;m[7]=0;
    m[8]=0;  m[9]=0;  m[10]=ly;m[11]=0;
    m[12]=0; m[13]=0; m[14]=0; m[15]=ly;
}

// ============================================================
// WEEK 7 - MATERIAL HELPER  (Phong: ambient, diffuse, specular)
// ============================================================
void mat(float r,float g,float b,float sr,float sg,float sb,float sh,float a=1.f)
{
    GLfloat amb[]={r*.28f,g*.28f,b*.28f,a};
    GLfloat dif[]={r,g,b,a};
    GLfloat spc[]={sr,sg,sb,a};
    GLfloat sh2[]={sh};
    if(lightMode==LM_NOSPEC||lightMode==LM_AMBONLY){spc[0]=spc[1]=spc[2]=0;sh2[0]=0;}
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,  amb);
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,  dif);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR, spc);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,sh2);
}

// ============================================================
// WEEK 7 - LIGHTING SETUP
// Two lights: sun (directional, w=0) + car park lamp (point, w=1)
// ============================================================
void setupLights()
{
    glEnable(GL_NORMALIZE);
    if(lightMode==LM_OFF){glDisable(GL_LIGHTING);return;}
    glEnable(GL_LIGHTING);

    // Sun
    float sx=cosf(D2R(sunAngle))*cosf(D2R(sunHeight*90.f));
    float sy=sinf(D2R(sunHeight*90.f));
    float sz=sinf(D2R(sunAngle))*cosf(D2R(sunHeight*90.f));
    LX=sx;LY=sy;LZ=sz;

    GLfloat sunPos[]={sx,sy,sz,0.f};   // w=0 = directional
    float dayF=CLAMP(sy,0.f,1.f);
    float rs,gs,bs;
    if(!nightMode){
        float srb=CLAMP(1.f-dayF*2.5f,0.f,1.f);
        rs=0.95f; gs=0.85f-srb*.35f; bs=0.80f-srb*.6f;
        float iv=0.25f+dayF*.75f;
        rs*=iv;gs*=iv;bs*=iv;
    } else {rs=0.08f;gs=0.09f;bs=0.22f;}

    GLfloat sD[]={rs,gs,bs,1};
    GLfloat sS[]={rs*.6f,gs*.6f,bs*.6f,1};
    // Raised ambient from .18 → .38 so ground/textures are not near-black in day
    GLfloat sA[]={nightMode?.05f:rs*.38f,nightMode?.05f:gs*.38f,nightMode?.10f:bs*.42f,1};
    if(lightMode==LM_AMBONLY){sD[0]=sD[1]=sD[2]=0;sS[0]=sS[1]=sS[2]=0;}
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0,GL_POSITION,sunPos);
    glLightfv(GL_LIGHT0,GL_DIFFUSE, sD);
    glLightfv(GL_LIGHT0,GL_SPECULAR,sS);
    glLightfv(GL_LIGHT0,GL_AMBIENT, sA);

    // Car park lamp post (point light over entrance)
    GLfloat lpPos[]={0.f,6.f,14.f,1.f};   // w=1 = positional
    float lR=nightMode?1.f:.2f, lG=nightMode?.9f:.18f, lB=nightMode?.5f:.09f;
    GLfloat lD[]={lR,lG,lB,1};
    GLfloat lA[]={0,0,0,1};
    GLfloat lSp[]={lR*.4f,lG*.4f,lB*.2f,1};
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1,GL_POSITION,lpPos);
    glLightfv(GL_LIGHT1,GL_DIFFUSE, lD);
    glLightfv(GL_LIGHT1,GL_AMBIENT, lA);
    glLightfv(GL_LIGHT1,GL_SPECULAR,lSp);
    glLightf (GL_LIGHT1,GL_CONSTANT_ATTENUATION, .5f);
    glLightf (GL_LIGHT1,GL_LINEAR_ATTENUATION,   .08f);
    glLightf (GL_LIGHT1,GL_QUADRATIC_ATTENUATION,.015f);

    // Building window glow lights (GL_LIGHT2/3/4) — three floor rows.
    // Active in night mode: warm yellow spill illuminates ground and
    // approaching cars. Placed just in front of the facade (z=-12.5).
    // In day mode they are dimmed to near-zero so they have no effect.
    float wR=nightMode?.95f:.01f, wG=nightMode?.85f:.01f, wB=nightMode?.40f:.01f;
    GLfloat wA[]={0,0,0,1};
    GLfloat wSp[]={wR*.3f,wG*.3f,wB*.1f,1};
    GLenum wLights[]={GL_LIGHT2,GL_LIGHT3,GL_LIGHT4};
    float  wHeights[]={2.f, 5.f, 8.f};  // floor 0,1,2 window Y centres (world)
    for(int fl=0;fl<3;fl++){
        GLfloat wPos[]={0.f, wHeights[fl], -12.5f, 1.f}; // just in front of glass
        GLfloat wD[]={wR,wG,wB,1};
        glEnable(wLights[fl]);
        glLightfv(wLights[fl],GL_POSITION, wPos);
        glLightfv(wLights[fl],GL_DIFFUSE,  wD);
        glLightfv(wLights[fl],GL_AMBIENT,  wA);
        glLightfv(wLights[fl],GL_SPECULAR, wSp);
        glLightf (wLights[fl],GL_CONSTANT_ATTENUATION,  1.0f);
        glLightf (wLights[fl],GL_LINEAR_ATTENUATION,    0.10f);
        glLightf (wLights[fl],GL_QUADRATIC_ATTENUATION, 0.02f);
    }
}

// ============================================================
// DRAW HELPERS: shadow state
// ============================================================
bool inShadowPass=false;  // true while drawing shadow silhouettes

// Wrapper: only applies material when NOT in shadow pass
void safemat(float r,float g,float b,float sr,float sg,float sb,float sh,float a=1.f)
{
    if(!inShadowPass) mat(r,g,b,sr,sg,sb,sh,a);
}

// ============================================================
// WEEK 3-4 - MODELLING: Car (hierarchical primitives)
// One car function, called for each car with its own transform
// ============================================================
void drawCarGeom(float cr,float cg,float cb, float wheelRot)
{
    // Body lower chassis
    safemat(cr,cg,cb, .7f,.7f,.7f, 55.f);
    glPushMatrix(); glScalef(2.2f,.65f,4.6f); glutSolidCube(1); glPopMatrix();
    // Cabin
    safemat(cr*.85f,cg*.85f,cb*.85f, .6f,.6f,.6f,45.f);
    glPushMatrix(); glTranslatef(0,.62f,-.2f); glScalef(1.85f,.72f,2.5f); glutSolidCube(1); glPopMatrix();
    // Windshield glass
    safemat(.25f,.45f,.65f, .95f,.95f,1.f,120.f,.55f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glPushMatrix(); glTranslatef(0,.65f,-1.52f); glRotatef(18,1,0,0); glScalef(1.75f,.58f,.08f); glutSolidCube(1); glPopMatrix();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    // Headlights
    safemat(1.f,1.f,.85f, 1.f,1.f,.8f,120.f);
    for(int s=-1;s<=1;s+=2){
        glPushMatrix(); glTranslatef(s*.78f,.18f,-2.35f); glScalef(.38f,.22f,.10f); glutSolidSphere(1,8,6); glPopMatrix();
    }
    // Tail lights (red)
    safemat(.9f,.05f,.05f,.8f,.3f,.3f,60.f);
    for(int s=-1;s<=1;s+=2){
        glPushMatrix(); glTranslatef(s*.78f,.18f,2.35f); glutSolidSphere(.18f,8,6); glPopMatrix();
    }
    // Bumpers (grey)
    safemat(.55f,.55f,.58f,.4f,.4f,.4f,40.f);
    glPushMatrix(); glTranslatef(0,-.09f,-2.38f); glScalef(2.1f,.32f,.18f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(0,-.09f, 2.38f); glScalef(2.1f,.32f,.18f); glutSolidCube(1); glPopMatrix();
    // Wheels (4) - tyre + rim
    float wp[4][2]={{-1.08f,-1.5f},{1.08f,-1.5f},{-1.08f,1.5f},{1.08f,1.5f}};
    for(int w=0;w<4;w++){
        glPushMatrix();
        // Wheel centre must be at y=0.36 in world space (tyre outer radius=0.36).
        // Car body is translated up by 0.38, so local offset = 0.36-0.38 = -0.02.
        glTranslatef(wp[w][0],-.02f,wp[w][1]);
        glRotatef(wheelRot,1,0,0);
        // Tyre
        safemat(.12f,.12f,.12f,.08f,.08f,.08f,5.f);
        glRotatef(90,0,1,0); glutSolidTorus(.20f,.36f,10,16);
        // Rim (silver)
        safemat(.78f,.78f,.82f,.9f,.9f,.9f,90.f);
        glScalef(.22f,.22f,.09f); glutSolidSphere(1,10,8);
        glPopMatrix();
    }
    // Exhaust
    safemat(.32f,.32f,.32f,.3f,.3f,.3f,15.f);
    glPushMatrix(); glTranslatef(-.7f,-.27f,2.36f); glRotatef(90,1,0,0);
    glutSolidCylinder(.06f,.38f,8,2); glPopMatrix();
}

void drawCar(int idx)
{
    Car& c=cars[idx];
    glPushMatrix();
    glTranslatef(c.x,.38f,c.z);
    glRotatef(-c.heading,0,1,0);
    // Selection indicator: yellow = free, red = collision blocked
    if(idx==selectedCar && !inShadowPass){
        glDisable(GL_LIGHTING);
        if(collisionBlocked) glColor3f(1.f,.15f,.15f);   // red flash
        else                 glColor3f(1.f,1.f, .0f);   // yellow normal
        glPushMatrix(); glTranslatef(0,-.36f,0); glRotatef(90,1,0,0);
        glutWireTorus(.05f,1.3f,8,20); glPopMatrix();
        if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
    }
    drawCarGeom(c.r,c.g,c.b,c.wheelRot);
    glPopMatrix();
}

// ============================================================
// WEEK 3-4 - TREE (hierarchical: trunk → branches → foliage)
// Richer model: tapered root flare + bark texture + 5 foliage
// clusters with varied colour (shadow-green → sunlit yellow-green)
// ============================================================
void drawTree(float x,float z,float h=4.5f)
{
    glPushMatrix(); glTranslatef(x,0,z);

    // --- Root flare: 4 short flat ellipsoids radiating from base ----------
    mat(.32f,.20f,.08f,.06f,.04f,.02f,5.f);
    for(int r=0;r<4;r++){
        glPushMatrix();
        glRotatef(r*90.f,0,1,0);
        glTranslatef(.38f,.05f,0);
        glScalef(.55f,.12f,.22f); glutSolidSphere(1,8,4);
        glPopMatrix();
    }

    // --- Trunk: bark texture modulated on cylinder (-90 deg = stand up) ---
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID[4]);   // reuse tyre-mark slot as bark
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    mat(.42f,.27f,.11f,.12f,.08f,.04f,10.f);
    // Lower trunk (wider)
    glPushMatrix(); glRotatef(-90,1,0,0);
    glutSolidCylinder(.30f,h*.45f,14,6); glPopMatrix();
    // Upper trunk (tapers slightly)
    glPushMatrix(); glTranslatef(0,h*.45f,0); glRotatef(-90,1,0,0);
    glutSolidCylinder(.22f,h*.28f,12,4); glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    // --- 2-3 branch stubs sprouting sideways near canopy base -------------
    mat(.38f,.24f,.10f,.08f,.06f,.03f,6.f);
    for(int b=0;b<3;b++){
        glPushMatrix();
        glRotatef(b*120.f+15.f,0,1,0);
        glTranslatef(0,h*.58f,0);
        glRotatef(55.f,0,0,1);
        glutSolidCylinder(.09f,h*.20f,8,2);
        glPopMatrix();
    }

    // --- 5 foliage clusters: shadow-dark at base → sunlit at crown --------
    // L1 — broad, dark shadow-green at lower canopy
    mat(.13f,.46f,.09f,.04f,.12f,.02f,8.f);
    glPushMatrix(); glTranslatef( .15f,h*.58f, .10f); glScalef(1.35f,.85f,1.35f); glutSolidSphere(h*.29f,16,11); glPopMatrix();

    // L2 — mid-green, offset opposite
    mat(.18f,.56f,.11f,.06f,.16f,.03f,10.f);
    glPushMatrix(); glTranslatef(-.18f,h*.70f,-.08f); glScalef(1.15f,.95f,1.10f); glutSolidSphere(h*.24f,14,10); glPopMatrix();

    // L3 — slightly brighter, front
    mat(.22f,.64f,.14f,.08f,.20f,.04f,12.f);
    glPushMatrix(); glTranslatef( .08f,h*.80f, .15f); glScalef(1.05f,1.00f,1.05f); glutSolidSphere(h*.20f,13,9); glPopMatrix();

    // L4 — yellow-green sunlit, near top
    mat(.30f,.70f,.16f,.10f,.26f,.06f,16.f);
    glPushMatrix(); glTranslatef(-.06f,h*.90f, .05f); glScalef(.95f,1.00f,.90f); glutSolidSphere(h*.16f,12,8); glPopMatrix();

    // L5 — bright lime highlight at very crown
    mat(.38f,.78f,.20f,.14f,.32f,.08f,22.f);
    glPushMatrix(); glTranslatef( .02f,h*1.02f,.02f); glutSolidSphere(h*.10f,10,7); glPopMatrix();

    glPopMatrix();
}

// ============================================================
// PERSON (hierarchical — sphere head, cube torso, cylinder limbs)
// ============================================================
void drawPerson(bool shadowPass)
{
    if(player.inCar) return; // hidden inside car

    glPushMatrix();
    glTranslatef(player.x, 0.f, player.z);
    glRotatef(-player.heading, 0,1,0);

    // --- Legs (animated) -------------------------------------------
    float lSwing = sinf(player.legAnim) * 28.f; // degrees swing
    float rSwing = -lSwing;

    // Left leg
    if(!shadowPass) safemat(.18f,.28f,.72f, .3f,.3f,.6f, 20.f); // jeans blue
    glPushMatrix();
        glTranslatef(-.14f, .65f, 0);
        glRotatef(lSwing, 1,0,0);
        glTranslatef(0,-.32f, 0);
        glScalef(.16f,.65f,.16f); glutSolidCube(1);
    glPopMatrix();
    // Right leg
    glPushMatrix();
        glTranslatef( .14f, .65f, 0);
        glRotatef(rSwing, 1,0,0);
        glTranslatef(0,-.32f, 0);
        glScalef(.16f,.65f,.16f); glutSolidCube(1);
    glPopMatrix();

    // --- Shoes -------------------------------------------------------
    if(!shadowPass) safemat(.10f,.10f,.10f, .2f,.2f,.2f, 15.f);
    glPushMatrix();
        glTranslatef(-.14f, .07f, sinf(player.legAnim)*.06f-.04f);
        glScalef(.18f,.10f,.26f); glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
        glTranslatef( .14f, .07f, -sinf(player.legAnim)*.06f-.04f);
        glScalef(.18f,.10f,.26f); glutSolidCube(1);
    glPopMatrix();

    // --- Torso -------------------------------------------------------
    if(!shadowPass) safemat(.85f,.12f,.12f, .5f,.3f,.3f, 30.f); // red shirt
    glPushMatrix();
        glTranslatef(0, 1.10f, 0);
        glScalef(.38f,.55f,.22f); glutSolidCube(1);
    glPopMatrix();

    // --- Arms --------------------------------------------------------
    if(!shadowPass) safemat(.85f,.12f,.12f, .5f,.3f,.3f, 30.f);
    glPushMatrix(); // left arm
        glTranslatef(-.26f, 1.15f, 0);
        glRotatef(-lSwing*.7f, 1,0,0);
        glTranslatef(0,-.18f,0);
        glScalef(.13f,.40f,.13f); glutSolidCube(1);
    glPopMatrix();
    glPushMatrix(); // right arm
        glTranslatef( .26f, 1.15f, 0);
        glRotatef(-rSwing*.7f, 1,0,0);
        glTranslatef(0,-.18f,0);
        glScalef(.13f,.40f,.13f); glutSolidCube(1);
    glPopMatrix();

    // --- Neck --------------------------------------------------------
    if(!shadowPass) safemat(.88f,.68f,.50f, .4f,.3f,.2f, 15.f); // skin
    glPushMatrix();
        glTranslatef(0, 1.44f, 0);
        glRotatef(-90,1,0,0); glutSolidCylinder(.07f,.14f,8,2);
    glPopMatrix();

    // --- Head --------------------------------------------------------
    if(!shadowPass) safemat(.88f,.68f,.50f, .5f,.4f,.3f, 25.f);
    glPushMatrix();
        glTranslatef(0, 1.65f, 0);
        glScalef(.28f,.30f,.26f); glutSolidSphere(1,14,10);
    glPopMatrix();

    // --- Hair --------------------------------------------------------
    if(!shadowPass) safemat(.18f,.10f,.04f, .1f,.1f,.05f, 5.f);
    glPushMatrix();
        glTranslatef(0, 1.78f, -.03f);
        glScalef(.29f,.15f,.27f); glutSolidSphere(1,12,8);
    glPopMatrix();

    glPopMatrix();
}

// ============================================================
// OFFICE BUILDING with glass facade  (movable)
// ============================================================
void drawOffice()
{
    glPushMatrix();
    // Apply building movement on top of its default world position
    glTranslatef(bldgX, 0.f, bldgZ);
    glRotatef(-bldgHeading, 0,1,0);
    glTranslatef(0,0,-16.f);
    // Main concrete body  (concrete texture modulated with material colour)
    mat(.72f,.72f,.70f,.3f,.3f,.28f,18.f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID[1]);  // concrete
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glPushMatrix(); glTranslatef(0,5.5f,0);
    // Emit textured quads for front/back/sides to show realistic surface.
    // glutSolidCube doesn't emit tex-coords, so we draw the facing faces manually
    // and fall back to glutSolidCube for the core bulk (top/bottom/interior).
    // Front face of building (z=+3, XZ extents 18x11)
    glBegin(GL_QUADS); glNormal3f(0,0,1);
    glTexCoord2f(0,0); glVertex3f(-9,-5.5f, 3);
    glTexCoord2f(6,0); glVertex3f( 9,-5.5f, 3);
    glTexCoord2f(6,4); glVertex3f( 9, 5.5f, 3);
    glTexCoord2f(0,4); glVertex3f(-9, 5.5f, 3);
    glEnd();
    // Back face (z=-3)
    glBegin(GL_QUADS); glNormal3f(0,0,-1);
    glTexCoord2f(0,0); glVertex3f( 9,-5.5f,-3);
    glTexCoord2f(6,0); glVertex3f(-9,-5.5f,-3);
    glTexCoord2f(6,4); glVertex3f(-9, 5.5f,-3);
    glTexCoord2f(0,4); glVertex3f( 9, 5.5f,-3);
    glEnd();
    // Left face (x=-9)
    glBegin(GL_QUADS); glNormal3f(-1,0,0);
    glTexCoord2f(0,0); glVertex3f(-9,-5.5f,-3);
    glTexCoord2f(2,0); glVertex3f(-9,-5.5f, 3);
    glTexCoord2f(2,4); glVertex3f(-9, 5.5f, 3);
    glTexCoord2f(0,4); glVertex3f(-9, 5.5f,-3);
    glEnd();
    // Right face (x=+9)
    glBegin(GL_QUADS); glNormal3f(1,0,0);
    glTexCoord2f(0,0); glVertex3f( 9,-5.5f, 3);
    glTexCoord2f(2,0); glVertex3f( 9,-5.5f,-3);
    glTexCoord2f(2,4); glVertex3f( 9, 5.5f,-3);
    glTexCoord2f(0,4); glVertex3f( 9, 5.5f, 3);
    glEnd();
    // Top face (y=+5.5)
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-9, 5.5f,-3);
    glTexCoord2f(6,0); glVertex3f( 9, 5.5f,-3);
    glTexCoord2f(6,2); glVertex3f( 9, 5.5f, 3);
    glTexCoord2f(0,2); glVertex3f(-9, 5.5f, 3);
    glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    // Roof parapet
    mat(.58f,.58f,.56f,.25f,.25f,.22f,12.f);
    glPushMatrix(); glTranslatef(0,11.3f,0); glScalef(18.2f,.6f,6.2f); glutSolidCube(1); glPopMatrix();
    // Glass window panels (reflective blue-green)
    mat(.25f,.45f,.65f,.9f,.9f,1.f,120.f,.7f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    for(int floor=0;floor<3;floor++){
        for(int col=-3;col<=3;col++){
            glPushMatrix(); glTranslatef(col*2.5f,2.f+floor*3.f,3.05f);
            glScalef(1.9f,2.1f,.06f); glutSolidCube(1); glPopMatrix();
        }
    }
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    // Entrance doors (dark glass)
    mat(.15f,.25f,.35f,.5f,.5f,.7f,80.f,.8f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    glPushMatrix(); glTranslatef(0,.95f,3.1f); glScalef(2.f,1.9f,.06f); glutSolidCube(1); glPopMatrix();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    // Column pillars
    mat(.85f,.83f,.80f,.4f,.4f,.38f,35.f);
    for(int cp=-1;cp<=1;cp+=2){
        glPushMatrix(); glTranslatef(cp*1.2f,.95f,3.0f);
        glRotatef(-90,1,0,0); glutSolidCylinder(.12f,1.9f,10,3); glPopMatrix();
    }
    // Entrance canopy
    mat(.52f,.52f,.50f,.3f,.3f,.28f,15.f);
    glPushMatrix(); glTranslatef(0,2.1f,3.8f); glScalef(3.f,.12f,1.5f); glutSolidCube(1); glPopMatrix();
    // Night window glow
    if(nightMode){
        glDisable(GL_LIGHTING);
        for(int fl=0;fl<3;fl++) for(int cl=-3;cl<=3;cl++){
            glColor3f(1.f,.95f,.65f);
            glPushMatrix(); glTranslatef(cl*2.5f,2.f+fl*3.f,3.08f);
            glScalef(1.7f,1.9f,.04f); glutSolidCube(1); glPopMatrix();
        }
        if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
    }
    glPopMatrix();
}

// ============================================================
// WASTE BIN (dark green cylinder + lid)
// ============================================================
void drawBin(float x,float z)
{
    glPushMatrix(); glTranslatef(x,0,z);
    mat(.12f,.38f,.12f,.1f,.2f,.1f,15.f);
    glPushMatrix(); glRotatef(-90,1,0,0); glutSolidCylinder(.22f,.85f,12,4); glPopMatrix();
    // Lid
    mat(.10f,.30f,.10f,.1f,.15f,.1f,10.f);
    glPushMatrix(); glTranslatef(0,.9f,0); glScalef(1.f,.12f,1.f); glutSolidCylinder(.24f,0.f,12,1); glPopMatrix();
    glPopMatrix();
}

// ============================================================
// BENCH (wooden slats + metal frame)
// ============================================================
void drawBench(float x,float z,float ry=0.f)
{
    glPushMatrix(); glTranslatef(x,0,z); glRotatef(ry,0,1,0);
    // Seat slats (wood colour)
    mat(.55f,.35f,.15f,.15f,.10f,.08f,10.f);
    for(int s=0;s<3;s++){
        glPushMatrix(); glTranslatef(0,.48f,s*.14f-.14f); glScalef(1.6f,.06f,.09f); glutSolidCube(1); glPopMatrix();
    }
    // Back slats
    for(int s=0;s<2;s++){
        glPushMatrix(); glTranslatef(0,.75f+s*.15f,.22f); glScalef(1.6f,.06f,.06f); glutSolidCube(1); glPopMatrix();
    }
    // Metal legs
    mat(.45f,.45f,.48f,.5f,.5f,.5f,45.f);
    for(int s=-1;s<=1;s+=2){
        glPushMatrix(); glTranslatef(s*.65f,.24f,0); glScalef(.06f,.5f,.06f); glutSolidCube(1); glPopMatrix();
        // Back support
        glPushMatrix(); glTranslatef(s*.65f,.6f,.22f); glScalef(.06f,.7f,.06f); glutSolidCube(1); glPopMatrix();
    }
    glPopMatrix();
}

// ============================================================
// LAMP POST (steel pole + warm bulb)
// ============================================================
void drawLampPost(float x,float z)
{
    glPushMatrix(); glTranslatef(x,0,z);
    mat(.30f,.30f,.32f,.5f,.5f,.5f,60.f);
    glPushMatrix(); glRotatef(-90,1,0,0); glutSolidCylinder(.07f,5.5f,10,3); glPopMatrix();
    glPushMatrix(); glTranslatef(0,5.5f,0); glScalef(.5f,.3f,.5f); glutSolidCube(1); glPopMatrix();
    // Glow
    glDisable(GL_LIGHTING);
    float glow=nightMode?1.f:.25f;
    glColor3f(glow,glow*.88f,glow*.5f);
    glPushMatrix(); glTranslatef(0,5.4f,0); glutSolidSphere(.16f,10,8); glPopMatrix();
    if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
    glPopMatrix();
}

// ============================================================
// FLOWER BED patch
// ============================================================
void drawFlowerBed(float cx,float cz,float w,float d)
{
    // Soil
    glPushMatrix(); glTranslatef(cx,.01f,cz);
    mat(.42f,.28f,.15f,.05f,.04f,.02f,5.f);
    glScalef(w,.04f,d); glutSolidCube(1);
    glPopMatrix();
    // Flowers
    float fw=w*.5f,fd=d*.5f;
    float posX[]={-.8f,0,.7f,-.4f,.5f,-.6f,.2f};
    float posZ[]={-.5f,.3f,-.2f,.6f,-.7f,.1f,-.4f};
    float fcR[]={.9f,.1f,.9f,1.f,.2f,.8f,.1f};
    float fcG[]={.1f,.8f,.1f,.7f,.9f,.1f,.9f};
    float fcB[]={.1f,.1f,.8f,.0f,.1f,.9f,.1f};
    int nf=7;
    for(int i=0;i<nf;i++){
        float fx=cx+posX[i]*fw*.9f, fz2=cz+posZ[i]*fd*.9f;
        mat(.15f,.52f,.10f,.05f,.12f,.03f,5.f);
        glPushMatrix(); glTranslatef(fx,.35f,fz2); glRotatef(-90,1,0,0);
        glutSolidCylinder(.04f,.65f,5,2); glPopMatrix();
        mat(fcR[i],fcG[i],fcB[i],.4f,.3f,.3f,30.f);
        glPushMatrix(); glTranslatef(fx,.72f,fz2); glutSolidSphere(.18f,9,7); glPopMatrix();
    }
}

// ============================================================
// WEEK 10 - TEXTURE GENERATION
// Procedural textures are built in CPU memory then uploaded with
// gluBuild2DMipmaps so every mip-level is auto-generated
// (prevents aliasing when the ground is viewed at a shallow angle).
// Anisotropic filtering further sharpens oblique views.
// ============================================================
static void makeNoise(unsigned char* p,int sz,
                      unsigned char r,unsigned char g,unsigned char b,
                      int var,unsigned seed)
{
    srand(seed);
    for(int i=0;i<sz*sz;i++){
        int v=rand()%var-var/2;
        p[i*3+0]=(unsigned char)CLAMP(r+v,0,255);
        p[i*3+1]=(unsigned char)CLAMP(g+v,0,255);
        p[i*3+2]=(unsigned char)CLAMP(b+v,0,255);
    }
}

void initTextures()
{
    const int SZ=128;  // texture resolution (power-of-two)
    static unsigned char buf[SZ*SZ*3];

    glGenTextures(4,texID);

    // Helper: upload buffer as mip-mapped RGB texture
    auto upload=[&](int idx){
        glBindTexture(GL_TEXTURE_2D, texID[idx]);
        // Mipmapping: MIN uses mip chain to avoid glittering at distance
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        // Repeat across large quads
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        // Anisotropic filtering (extension present on every modern driver)
        float maxAniso=1.f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&maxAniso);
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        maxAniso>8.f?8.f:maxAniso);
        // gluBuild2DMipmaps auto-generates all mip levels
        gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,SZ,SZ,GL_RGB,GL_UNSIGNED_BYTE,buf);
    };

    // --- Texture 0: Asphalt (medium grey — brighter so GL_MODULATE doesn't
    //     make it near-black; was 52 → now 110 with tighter noise) ----------
    makeNoise(buf,SZ, 110,110,116, 20, 0xA5);
    upload(0);

    // --- Texture 1: Concrete wall (light grey, coarser variation) ----------
    makeNoise(buf,SZ,185,183,178, 22, 0xC3);
    upload(1);

    // --- Texture 2: Grass (richer green — multi-shade blade stipple) --------
    makeNoise(buf,SZ, 95,162,48, 30, 0x7F);
    // Scatter lighter sun-catch blades
    srand(0x2E);
    for(int i=0;i<SZ*SZ/5;i++){
        int px=rand()%(SZ*SZ);
        buf[px*3+0]=(unsigned char)CLAMP(buf[px*3+0]+20,0,255);
        buf[px*3+1]=(unsigned char)CLAMP(buf[px*3+1]+28,0,255);
        buf[px*3+2]=(unsigned char)CLAMP(buf[px*3+2]+ 8,0,255);
    }
    // Scatter darker shadow blades
    srand(0xB1);
    for(int i=0;i<SZ*SZ/8;i++){
        int px=rand()%(SZ*SZ);
        buf[px*3+0]=(unsigned char)CLAMP(buf[px*3+0]-30,0,255);
        buf[px*3+1]=(unsigned char)CLAMP(buf[px*3+1]-40,0,255);
        buf[px*3+2]=(unsigned char)CLAMP(buf[px*3+2]-12,0,255);
    }
    upload(2);

    // --- Texture 3: Road-marking paint (near-white with slight yellow cast) -
    makeNoise(buf,SZ,238,235,200, 10, 0x11);
    upload(3);

    // --- Texture 4: Bark / tyre-mark dual-use texture ----------------------
    // Used on tree trunks as bark (dark brown vertical streaks).
    // Also applied inverted as tyre-mark decal (very dark rubber smudge).
    glGenTextures(1,&texID[4]);  // slot 4 needs a separate glGenTextures
    makeNoise(buf,SZ, 58,36,16, 18, 0x3D);
    // Vertical dark streaks to simulate bark grain
    srand(0x5C);
    for(int col=0;col<SZ;col+=3+rand()%4){
        unsigned char dark=(unsigned char)(28+rand()%18);
        int streak_w=1+rand()%2;
        for(int row=0;row<SZ;row++){
            for(int sw=0;sw<streak_w&&col+sw<SZ;sw++){
                int px=(row*SZ+col+sw);
                buf[px*3+0]=(unsigned char)CLAMP(buf[px*3+0]-(int)dark,0,255);
                buf[px*3+1]=(unsigned char)CLAMP(buf[px*3+1]-(int)(dark*.8f),0,255);
                buf[px*3+2]=(unsigned char)CLAMP(buf[px*3+2]-(int)(dark*.4f),0,255);
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, texID[4]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,SZ,SZ,GL_RGB,GL_UNSIGNED_BYTE,buf);

    glBindTexture(GL_TEXTURE_2D,0); // unbind
}

// ============================================================
// GROUND: tarmac, bays, road markings, pavement, grass borders
// ============================================================
void drawGround()
{
    glEnable(GL_TEXTURE_2D);
    // Texture modulates with the Phong material colour (GL_MODULATE)
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

    // --- Large tarmac base (asphalt texture)
    // Raised diffuse .22→.52 so GL_MODULATE with brighter texture stays visible
    mat(.52f,.52f,.54f,.15f,.15f,.15f,10.f);
    glBindTexture(GL_TEXTURE_2D, texID[0]);   // asphalt
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0);  glVertex3f(-20,0,-18);
    glTexCoord2f(8,0);  glVertex3f( 20,0,-18);
    glTexCoord2f(8,9);  glVertex3f( 20,0, 18);
    glTexCoord2f(0,9);  glVertex3f(-20,0, 18);
    glEnd();

    // --- Grass border strips (left & right) --------------------------------
    mat(.50f,.78f,.30f,.06f,.14f,.04f,6.f);
    glBindTexture(GL_TEXTURE_2D, texID[2]);   // grass
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-20,.01f,-18);
    glTexCoord2f(2,0); glVertex3f(-15,.01f,-18);
    glTexCoord2f(2,9); glVertex3f(-15,.01f, 18);
    glTexCoord2f(0,9); glVertex3f(-20,.01f, 18);
    glEnd();
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(15,.01f,-18);
    glTexCoord2f(2,0); glVertex3f(20,.01f,-18);
    glTexCoord2f(2,9); glVertex3f(20,.01f, 18);
    glTexCoord2f(0,9); glVertex3f(15,.01f, 18);
    glEnd();

    // --- Tar road drive aisle (slightly darker than bay asphalt) -----------
    mat(.44f,.44f,.46f,.12f,.12f,.12f,10.f);
    glBindTexture(GL_TEXTURE_2D, texID[0]);   // asphalt
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-4,.01f,-18);
    glTexCoord2f(2,0); glVertex3f( 4,.01f,-18);
    glTexCoord2f(2,8); glVertex3f( 4,.01f, 16);
    glTexCoord2f(0,8); glVertex3f(-4,.01f, 16);
    glEnd();

    glDisable(GL_TEXTURE_2D);  // markings drawn flat with glColor

    // --- Parking bay lines (white) -----------------------------------------
    glDisable(GL_LIGHTING);
    glColor3f(.95f,.95f,.95f);
    // Left bay rows
    for(int b=0;b<3;b++){
        float bz=-12.f+b*8.f;
        glBegin(GL_QUADS);
        glVertex3f(-14,.02f,bz);   glVertex3f(-4,.02f,bz);
        glVertex3f(-4,.02f,bz+.1f);glVertex3f(-14,.02f,bz+.1f);
        glEnd();
        // Side lines
        for(int l=0;l<=4;l++){
            float bx=-14.f+l*2.5f;
            glBegin(GL_QUADS);
            glVertex3f(bx,.02f,bz); glVertex3f(bx+.08f,.02f,bz);
            glVertex3f(bx+.08f,.02f,bz+7.5f); glVertex3f(bx,.02f,bz+7.5f);
            glEnd();
        }
    }
    // Right bay rows
    for(int b=0;b<3;b++){
        float bz=-12.f+b*8.f;
        glBegin(GL_QUADS);
        glVertex3f(4,.02f,bz); glVertex3f(14,.02f,bz);
        glVertex3f(14,.02f,bz+.1f); glVertex3f(4,.02f,bz+.1f);
        glEnd();
        for(int l=0;l<=4;l++){
            float bx=4.f+l*2.5f;
            glBegin(GL_QUADS);
            glVertex3f(bx,.02f,bz); glVertex3f(bx+.08f,.02f,bz);
            glVertex3f(bx+.08f,.02f,bz+7.5f); glVertex3f(bx,.02f,bz+7.5f);
            glEnd();
        }
    }
    // Centre road dashed line
    glColor3f(.9f,.85f,.0f);
    for(int d=-4;d<5;d++){
        float dz=(float)d*3.5f;
        glBegin(GL_QUADS);
        glVertex3f(-.08f,.02f,dz); glVertex3f(.08f,.02f,dz);
        glVertex3f(.08f,.02f,dz+2.f); glVertex3f(-.08f,.02f,dz+2.f);
        glEnd();
    }
    // Zebra crossing at entrance
    glColor3f(.95f,.95f,.95f);
    for(int zs=0;zs<6;zs++){
        float zx=-3.5f+zs*.7f;
        glBegin(GL_QUADS);
        glVertex3f(zx,.02f,13.5f); glVertex3f(zx+.5f,.02f,13.5f);
        glVertex3f(zx+.5f,.02f,16.f); glVertex3f(zx,.02f,16.f);
        glEnd();
    }
    // Pavement strip (concrete texture)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID[1]);   // concrete
    if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
    mat(.80f,.77f,.72f,.22f,.22f,.20f,12.f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-20,.02f,14);
    glTexCoord2f(8,0); glVertex3f( 20,.02f,14);
    glTexCoord2f(8,2); glVertex3f( 20,.02f,18);
    glTexCoord2f(0,2); glVertex3f(-20,.02f,18);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// ============================================================
// WEEK 5 - SHADOW PASS: all shadow-casting geometry
// Called inside shadow matrix transform
// ============================================================
void drawShadowGeom()
{
    inShadowPass=true;

    // Cars
    for(int i=0;i<4;i++) drawCar(i);

    // Trees
    float txs[]={-17,-17,-17, 17, 17, 17,-17,17};
    float tzs[]={-10,  0, 10,-10,  0, 10, -4,-4};
    for(int t=0;t<8;t++) drawTree(txs[t],tzs[t]);

    // Office
    drawOffice();

    // Lamp posts
    drawLampPost(0,14);
    drawLampPost(-12,14);
    drawLampPost(12,14);

    // Bins and benches
    drawBin(-5,12);
    drawBin(5,12);
    drawBench(-6,10);
    drawBench(6,10,180);

    // Person shadow
    drawPerson(true);

    inShadowPass=false;
}

// ============================================================
// DRAW ALL SHADOWS ON GROUND
// ============================================================
void drawAllShadows()
{
    if(LY<0.06f) return;
    float alpha=CLAMP(LY*.65f,.08f,.60f);
    if(nightMode) alpha=.10f;

    float sm[16]; shadowMat(LX,LY,LZ,sm);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if(!nightMode) glColor4f(.04f,.05f,.03f,alpha);
    else           glColor4f(.04f,.04f,.14f,alpha);

    glPushMatrix();
    glTranslatef(0,.015f,0);
    glMultMatrixf(sm);
    drawShadowGeom();
    glPopMatrix();

    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// ============================================================
// SKY
// ============================================================
void drawSky()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    float dayF=CLAMP(sinf(D2R(sunHeight*90.f)),0.f,1.f);
    float tR,tG,tB,hR,hG,hB;
    if(!nightMode){
        float srb=CLAMP(1.f-dayF*2.5f,0.f,1.f);
        tR=.28f+dayF*.1f; tG=.48f+dayF*.1f; tB=.88f;
        hR=.72f+srb*.25f; hG=.72f-srb*.22f; hB=.82f-srb*.42f;
    } else {tR=.01f;tG=.01f;tB=.06f;hR=.03f;hG=.03f;hB=.10f;}
    glClearColor(hR,hG,hB,1);
    float W=250,H=100;
    auto face=[&](float x0,float y0,float z0,float x1,float y1,float z1,
                  float x2,float y2,float z2,float x3,float y3,float z3){
        glBegin(GL_QUADS);
        glColor3f(hR,hG,hB); glVertex3f(x0,y0,z0); glVertex3f(x1,y1,z1);
        glColor3f(tR,tG,tB); glVertex3f(x2,y2,z2); glVertex3f(x3,y3,z3);
        glEnd();
    };
    face(-W,0,-W,W,0,-W,W,H,-W,-W,H,-W);
    face(-W,0,W,W,0,W,W,H,W,-W,H,W);
    face(-W,0,-W,-W,0,W,-W,H,W,-W,H,-W);
    face(W,0,-W,W,0,W,W,H,W,W,H,-W);
    glBegin(GL_QUADS); glColor3f(tR,tG,tB);
    glVertex3f(-W,H,-W);glVertex3f(W,H,-W);glVertex3f(W,H,W);glVertex3f(-W,H,W);glEnd();
    if(nightMode){
        glPointSize(1.8f); srand(11111);
        glBegin(GL_POINTS);
        for(int i=0;i<700;i++){
            float a1=((float)rand()/RAND_MAX)*2*PI,a2=((float)rand()/RAND_MAX)*PI*.48f;
            float b=.3f+((float)rand()/RAND_MAX)*.7f;
            glColor3f(b,b,b*.9f);
            glVertex3f(100*cosf(a2)*cosf(a1),100*sinf(a2)+5,100*cosf(a2)*sinf(a1));
        }
        glEnd();
    }
    glEnable(GL_DEPTH_TEST);
    if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// ============================================================
// SUN / MOON DISC
// ============================================================
void drawSunDisc()
{
    float sx=cosf(D2R(sunAngle))*18.f,sy=sinf(D2R(sunHeight*90.f))*18.f,sz=sinf(D2R(sunAngle))*18.f;
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    if(!nightMode){
        float srb=CLAMP(1.f-(sy/18.f)*2.5f,0.f,1.f);
        glColor3f(1.f,.95f-srb*.3f,.4f-srb*.3f);
        glPushMatrix(); glTranslatef(sx,sy,sz); glutSolidSphere(1.4f,16,12); glPopMatrix();
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.f,.9f,.5f,.15f);
        glPushMatrix(); glTranslatef(sx,sy,sz); glutSolidSphere(2.2f,12,9); glPopMatrix();
        glDisable(GL_BLEND);
    } else {
        glColor3f(.88f,.88f,.80f);
        glPushMatrix(); glTranslatef(sx*.65f,sy*.65f,sz*.65f); glutSolidSphere(1.1f,14,10); glPopMatrix();
    }
    glEnable(GL_DEPTH_TEST);
    if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// ============================================================
// WEEK 8-9 - ON-SCREEN BUTTONS (clickable)
// ============================================================
struct Btn {int x,y,w,h; const char* label; float r,g,b; void(*fn)();};

void aLight()  {lightMode=(LightMode)((lightMode+1)%LM_COUNT);}
void aDepth()  {depthMode=(DepthMode)((depthMode+1)%DM_COUNT);}
void aNight()  {nightMode=!nightMode;}
void aPause()  {paused=!paused;}
void aSun()    {sunOrbiting=!sunOrbiting;}
void aCam()    {camMode=(CamMode)((camMode+1)%CM_COUNT);}
void aTab()    {if(player.inCar) selectedCar=(selectedCar+1)%4;}

Btn btns[]={
    {8,200,170,30,"[L] Lighting",    .2f,.7f,.2f, aLight},
    {8,166,170,30,"[Z] Depth Test",  .7f,.3f,.2f, aDepth},
    {8,132,170,30,"[N] Day/Night",   .2f,.4f,.8f, aNight},
    {8, 98,170,30,"[P] Pause",       .6f,.2f,.7f, aPause},
    {8, 64,170,30,"[S] Sun Orbit",   .7f,.6f,.1f, aSun  },
    {8, 30,170,30,"[C] Camera",      .3f,.6f,.6f, aCam  },
    {184,98,170,30,"[TAB] Next Car", .5f,.5f,.5f, aTab  },
};
const int NBTN=7;

void drawButtons()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    for(int i=0;i<NBTN;i++){
        Btn& b=btns[i];
        bool on=false;
        if(i==0)on=(lightMode!=LM_FULL);
        if(i==1)on=(depthMode==DM_OFF);
        if(i==2)on=nightMode;
        if(i==3)on=paused;
        if(i==4)on=sunOrbiting;

        // Shadow
        glColor4f(0,0,0,.4f);
        glBegin(GL_QUADS);
        glVertex2f(b.x+3,b.y-3);glVertex2f(b.x+b.w+3,b.y-3);
        glVertex2f(b.x+b.w+3,b.y+b.h-3);glVertex2f(b.x+3,b.y+b.h-3);glEnd();
        // Body
        float br=on?CLAMP(b.r*1.4f,0,1):b.r;
        float bg=on?CLAMP(b.g*1.4f,0,1):b.g;
        float bb=on?CLAMP(b.b*1.4f,0,1):b.b;
        glColor4f(br,bg,bb,on?.92f:.72f);
        glBegin(GL_QUADS);
        glVertex2f(b.x,b.y);glVertex2f(b.x+b.w,b.y);
        glVertex2f(b.x+b.w,b.y+b.h);glVertex2f(b.x,b.y+b.h);glEnd();
        // Border
        glColor4f(1,1,1,.7f); glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(b.x,b.y);glVertex2f(b.x+b.w,b.y);
        glVertex2f(b.x+b.w,b.y+b.h);glVertex2f(b.x,b.y+b.h);glEnd();
        glLineWidth(1);
        // Label
        glColor3f(1,1,1);
        glRasterPos2i(b.x+7,b.y+10);
        for(const char*c=b.label;*c;c++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*c);
    }
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);glPopMatrix();
    glMatrixMode(GL_MODELVIEW);glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// ============================================================
// WEEK 10 - HUD OVERLAY (2D orthographic, info panel)
// ============================================================
void drawHUD()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Top info panel
    glColor4f(0,0,0,.62f);
    glBegin(GL_QUADS);
    glVertex2f(6,WIN_H-6);glVertex2f(WIN_W-6,WIN_H-6);
    glVertex2f(WIN_W-6,WIN_H-120);glVertex2f(6,WIN_H-120);glEnd();
    glDisable(GL_BLEND);

    auto t9=[&](int x,int y,const char*s,float r,float g,float b){
        glColor3f(r,g,b); glRasterPos2i(x,y);
        for(const char*c=s;*c;c++) glutBitmapCharacter(GLUT_BITMAP_9_BY_15,*c);
    };
    auto t8=[&](int x,int y,const char*s,float r,float g,float b){
        glColor3f(r,g,b); glRasterPos2i(x,y);
        for(const char*c=s;*c;c++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*c);
    };

    char buf[256];
    t9(14,WIN_H-24,"CAR PARK SCENE — OpenGL All-Concepts Demo",0.3f,1.f,0.4f);

    const char* lmN[]={"FULL PHONG","NO SPECULAR","AMBIENT ONLY","LIGHTS OFF"};
    const char* camN[]={"OVERVIEW","FOLLOW CAR","SIDE VIEW","FIRST PERSON"};
    snprintf(buf,256,"Lighting: %-16s | Depth: %-12s | Camera: %-12s | %s",
        lmN[lightMode], depthMode==DM_ON?"ON":"OFF!",
        camN[camMode], nightMode?"NIGHT":"DAY");
    float sr=lightMode==LM_FULL?.3f:lightMode==LM_NOSPEC?1.f:lightMode==LM_AMBONLY?1.f:.7f;
    float sg=lightMode==LM_FULL?1.f:lightMode==LM_NOSPEC?.8f:lightMode==LM_AMBONLY?.5f:.7f;
    float sb=lightMode==LM_FULL?.3f:.2f;
    t8(14,WIN_H-46,buf,sr,sg,sb);

    if(player.inCar)
        snprintf(buf,256,"Driving: %s  (ENTER=exit)  |  Sun: %.0f deg  |  %s",
            cars[selectedCar].name, sunAngle, paused?"*** PAUSED ***":"Animating");
    else
        snprintf(buf,256,"Walking  (ENTER near car to enter)  |  Sun: %.0f deg  |  %s",
            sunAngle, paused?"*** PAUSED ***":"Animating");
    t8(14,WIN_H-64,buf,1.f,1.f,.5f);

    const char* lmExp[]={"Full Phong: all 3 lighting components — best depth perception",
                         "No Specular: remove highlights — surfaces look flat/dull",
                         "Ambient Only: NO directional cues — depth perception lost!",
                         "Lights Off: pure flat colours — impossible to judge 3D depth"};
    t8(14,WIN_H-82,lmExp[lightMode],1.f,.95f,.65f);

    // Show different control hints depending on person state
    if(!player.inCar){
        t8(14,WIN_H-100,"WASD=Walk  ENTER=Enter Car (get close!)  C=Camera  R=Reset  I/K/J/L=Move Building",
            .65f,.65f,.65f);
    } else {
        snprintf(buf,256,"WASD=Drive [%s]  ENTER=Exit Car  TAB=Switch Car  C=Camera  I/K/J/L=Building",
            cars[selectedCar].name);
        t8(14,WIN_H-100,buf,.65f,.65f,.65f);
    }
    t8(14,WIN_H-116,"Wk1:Pipeline  Wk2:Camera  Wk3-4:Modelling  Wk5:Math/Shadows  Wk6:Colour",
        .5f,.75f,.5f);
    // Right side: week panel
    t8(WIN_W-380,WIN_H-100,"Wk7:Lighting  Wk8-9:Events  Wk10:HUD  Wk11:Ethics  Wk12:Project",.5f,.75f,.5f);

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    // Ethics blurb (bottom right)
    glColor4f(0,.05f,.15f,.65f);
    glBegin(GL_QUADS);
    glVertex2f(WIN_W-360,6);glVertex2f(WIN_W-6,6);
    glVertex2f(WIN_W-6,90);glVertex2f(WIN_W-360,90);glEnd();
    glDisable(GL_BLEND);

    t8(WIN_W-355,74,"Week 11 — Ethics & Standards",.4f,.9f,1.f);
    t8(WIN_W-355,56,"OpenGL = open standard (Khronos Group)",.9f,.9f,.7f);
    t8(WIN_W-355,40,"Accessibility: colour contrast for all users",.9f,.9f,.7f);
    t8(WIN_W-355,24,"IP: 3D models are copyrightable works",.9f,.9f,.7f);

    glMatrixMode(GL_PROJECTION);glPopMatrix();
    glMatrixMode(GL_MODELVIEW);glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    if(lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// ============================================================
// WEEK 2 - VIEWING: 4 camera modes (incl. First-Person)
// ============================================================
void applyCamera()
{
    Car& sel=cars[selectedCar];
    switch(camMode){
        case CM_OVERVIEW: {
            float hr=D2R(camH),vr=D2R(camV);
            float cx=camDist*cosf(vr)*sinf(hr);
            float cy=camDist*sinf(vr)+1.f;
            float cz=camDist*cosf(vr)*cosf(hr);
            gluLookAt(cx,cy,cz, 0,2,0, 0,1,0);
            break;
        }
        case CM_FOLLOW: {
            float hd=D2R(-sel.heading);
            float ex=sel.x+sinf(hd)*8.f, ez=sel.z+cosf(hd)*8.f;
            gluLookAt(ex,5.5f,ez, sel.x,.5f,sel.z, 0,1,0);
            break;
        }
        case CM_SIDE: {
            gluLookAt(25,12,0, 0,2,0, 0,1,0);
            break;
        }
        case CM_FIRST_PERSON: {
            // Eye position: person's head (or driver seat)
            float eyeX, eyeY, eyeZ, fwdX, fwdZ, heading;
            if(player.inCar){
                // Driving: view from driver seat (slightly left of centre)
                heading = D2R(-sel.heading);
                eyeX = sel.x - sinf(heading)*0.4f;
                eyeY = 1.2f;  // seat height
                eyeZ = sel.z - cosf(heading)*0.4f;
            } else {
                // Walking: head level
                heading = D2R(-player.heading);
                eyeX = player.x;
                eyeY = 1.65f;  // eye height
                eyeZ = player.z;
            }
            fwdX = eyeX - sinf(heading)*4.f;
            fwdZ = eyeZ - cosf(heading)*4.f;
            gluLookAt(eyeX, eyeY, eyeZ,
                      fwdX, eyeY, fwdZ,
                      0,1,0);
            break;
        }
        default: break;
    }
}

// ============================================================
// RESHAPE (Week 2 - projection)
// ============================================================
void reshape(int w,int h){
    WIN_W=w; WIN_H=(h==0)?1:h;
    glViewport(0,0,WIN_W,WIN_H);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(55.,(double)WIN_W/WIN_H,.3,600.);
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
// DISPLAY — full render pipeline (Week 1)
// ============================================================
void display()
{
    if(depthMode==DM_OFF) glDisable(GL_DEPTH_TEST);
    else glEnable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    applyCamera();
    setupLights();

    // Render order
    drawSky();
    drawSunDisc();
    drawAllShadows();     // shadows on ground BEFORE ground drawn
    drawGround();

    // All 3D objects
    float txs[]={-17,-17,-17,17,17,17,-17,17};
    float tzs[]={-10, 0,  10,-10,0,10, -4,-4};
    for(int t=0;t<8;t++) drawTree(txs[t],tzs[t]);

    drawOffice();
    drawLampPost(0,14); drawLampPost(-12,14); drawLampPost(12,14);
    drawBin(-5,12); drawBin(5,12);
    drawBench(-6,10); drawBench(6,10,180);
    drawFlowerBed(-11,12,4,3);
    drawFlowerBed( 11,12,4,3);
    drawFlowerBed(-11,-2,4,3);
    drawFlowerBed( 11,-2,4,3);

    for(int i=0;i<4;i++) drawCar(i);
    drawPerson(false);  // draw person after cars

    drawHUD();
    drawButtons();
    glutSwapBuffers();
}

// ============================================================
// BASIC COLLISION DETECTION
// The selected car is modelled as a circle (radius CAR_R) on the
// XZ plane.  We test it against:
//   - Office building  : AABB (axis-aligned rectangle)
//   - Trees            : circle vs circle
//   - Other 3 cars     : circle vs circle
//   - Bins / benches / lamp posts : circle vs circle
// ============================================================
static const float CAR_R = 2.6f;  // car footprint radius (half-diagonal of body)

// True if circle (cx,cz,r) overlaps the rectangle [x0..x1] x [z0..z1]
bool circleAABB(float cx,float cz,float r,
                float x0,float x1,float z0,float z1)
{
    float nx=CLAMP(cx,x0,x1), nz=CLAMP(cz,z0,z1);
    float dx=cx-nx, dz=cz-nz;
    return (dx*dx+dz*dz) < r*r;
}

// True if two circles on the XZ plane overlap
bool circleCircle(float ax,float az,float ar,
                  float bx,float bz,float br)
{
    float dx=ax-bx, dz=az-bz, d=ar+br;
    return (dx*dx+dz*dz) < d*d;
}

// Returns true if placing the selected car at (cx,cz) would cause a collision
bool checkCollision(float cx,float cz)
{
    // --- Office building --------------------------------------------------
    // Building parent translated to z=-16; concrete body scale 18x11x6
    // World footprint: X[-9..9], Z[-19..-13]  (add tiny pad for car bumper)
    if(circleAABB(cx,cz,CAR_R, -9.f,9.f, -19.f,-12.5f)) return true;

    // --- Trees (8 positions, foliage radius ~1.6) --------------------------
    const float txs[]={-17.f,-17.f,-17.f, 17.f, 17.f, 17.f,-17.f,17.f};
    const float tzs[]={-10.f,  0.f, 10.f,-10.f,  0.f, 10.f, -4.f,-4.f};
    for(int t=0;t<8;t++)
        if(circleCircle(cx,cz,CAR_R, txs[t],tzs[t], 1.6f)) return true;

    // --- Other (non-selected) cars ----------------------------------------
    for(int i=0;i<4;i++){
        if(i==selectedCar) continue;
        if(circleCircle(cx,cz,CAR_R, cars[i].x,cars[i].z, CAR_R)) return true;
    }

    // --- Waste bins -------------------------------------------------------
    const float bins[][2]={{-5.f,12.f},{5.f,12.f}};
    for(auto& b:bins)
        if(circleCircle(cx,cz,CAR_R, b[0],b[1], 0.6f)) return true;

    // --- Benches ----------------------------------------------------------
    const float benches[][2]={{-6.f,10.f},{6.f,10.f}};
    for(auto& b:benches)
        if(circleCircle(cx,cz,CAR_R, b[0],b[1], 1.4f)) return true;

    // --- Lamp posts -------------------------------------------------------
    const float lamps[][2]={{0.f,14.f},{-12.f,14.f},{12.f,14.f}};
    for(auto& l:lamps)
        if(circleCircle(cx,cz,CAR_R, l[0],l[1], 0.55f)) return true;

    return false;
}

// ============================================================
// UPDATE TIMER (Week 8-9 animation)
// ============================================================

// Person footprint radius (small, like a person)
static const float PERSON_R = 0.35f;

// True if placing the person at (px,pz) would cause a collision
bool checkPersonCollision(float px, float pz)
{
    // Office building (world footprint applies bldg offset too)
    // We use a simplified fixed AABB for now (building starts at z=-16,
    // depth 6 each side, width 18 each side)
    float bfx0 = bldgX - 9.f, bfx1 = bldgX + 9.f;
    float bfz0 = bldgZ - 19.f, bfz1 = bldgZ - 13.f;
    if(circleAABB(px,pz,PERSON_R, bfx0,bfx1, bfz0,bfz1)) return true;

    // Trees
    const float txs[]={-17.f,-17.f,-17.f,17.f,17.f,17.f,-17.f,17.f};
    const float tzs[]={-10.f,  0.f, 10.f,-10.f, 0.f,10.f, -4.f,-4.f};
    for(int t=0;t<8;t++)
        if(circleCircle(px,pz,PERSON_R, txs[t],tzs[t], 0.50f)) return true;

    return false;
}

void update(int v)
{
    if(!paused){
        sceneTime+=.016f;
        if(sunOrbiting&&!nightMode){
            sunAngle=fmodf(sunAngle+.20f,360.f);
            sunHeight=sinf(D2R(sunAngle))*.78f+.22f;
            sunHeight=CLAMP(sunHeight,0.f,1.f);
        }

        // ---- Building movement (I/J/K/L/U/O) ----------------------
        {
            float bspd=0.f, bturn=0.f, bstrafe=0.f;
            if(bkeys[0]) bspd=   .12f;   // I  forward
            if(bkeys[1]) bspd=  -.10f;   // K  backward
            if(bkeys[2]) bturn= -1.5f;   // J  rotate left
            if(bkeys[3]) bturn=  1.5f;   // L  rotate right
            if(bkeys[4]) bstrafe=-0.10f; // U  strafe left
            if(bkeys[5]) bstrafe= 0.10f; // O  strafe right
            bldgHeading += bturn;
            float bhr = D2R(-bldgHeading);
            bldgX += sinf(bhr)*bspd - cosf(bhr)*bstrafe;
            bldgZ += cosf(bhr)*bspd + sinf(bhr)*bstrafe;
        }

        // ---- Person walking (WASD when not in car) ----------------
        if(!player.inCar){
            float pspd=0.f, pturn=0.f;
            if(keys['w']||keys['W']||skeys[0]) pspd=  .07f;
            if(keys['x']||keys['X']||skeys[1]) pspd= -.05f;
            if(keys['a']||keys['A']||skeys[2]) pturn= -2.5f;
            if(keys['d']||keys['D']||skeys[3]) pturn=  2.5f;
            player.heading += pturn;
            float phr = D2R(-player.heading);
            float npx = CLAMP(player.x + sinf(phr)*pspd, -19.f, 19.f);
            float npz = CLAMP(player.z + cosf(phr)*pspd, -17.f, 17.f);
            if(!checkPersonCollision(npx, npz)){
                player.x = npx;
                player.z = npz;
            }
            // Leg animation: only cycle when moving
            if(ABS(pspd) > 0.005f)
                player.legAnim = fmodf(player.legAnim + ABS(pspd)*8.f, 2.f*PI);

        } else {
            // ---- Driving selected car (WASD when in car) -----------
            Car& c=cars[selectedCar];
            float spd=.0f,turnR=0.f;
            if(keys['w']||keys['W']||skeys[0]) spd=.12f;
            if(keys['x']||keys['X']||skeys[1]) spd=-.08f;
            if(keys['a']||keys['A']||skeys[2]) turnR=-2.f;
            if(keys['d']||keys['D']||skeys[3]) turnR= 2.f;
            c.heading+=turnR*(ABS(spd)>0.01f?1.f:0.3f);
            float hr=D2R(-c.heading);
            float nx=CLAMP(c.x+sinf(hr)*spd,-19.f,19.f);
            float nz=CLAMP(c.z+cosf(hr)*spd,-17.f,17.f);
            if(!checkCollision(nx,nz)){
                c.x=nx; c.z=nz;
                c.wheelRot+=spd*R2D(1.f/0.36f);
                collisionBlocked=false;
            } else {
                collisionBlocked=(ABS(spd)>0.01f);
            }
            // Keep person glued to car while driving
            player.x = c.x;
            player.z = c.z;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16,update,0);
}

// ============================================================
// KEYBOARD (Week 8-9)
// ============================================================
void keyDown(unsigned char k,int x,int y){
    keys[k]=true;
    // Building movement key state
    if(k=='i'||k=='I') bkeys[0]=true;
    if(k=='k'||k=='K') bkeys[1]=true;
    if(k=='j'||k=='J') bkeys[2]=true;
    if(k=='l'||k=='L') bkeys[3]=true;
    if(k=='u'||k=='U') bkeys[4]=true;
    if(k=='o'||k=='O') bkeys[5]=true;
    switch(k){
        case 27: exit(0);
        // Note: L is now overloaded — click HUD button for lighting, 
        // change later if want L function key:-> case 'l':case'L': aLight(); break;
        // or use keyboard which moves the building (bkeys[3]).
        // We keep aLight() on the button only.
        case 'z':case'Z': aDepth(); break;
        case 'n':case'N': aNight(); break;
        case 'p':case'P': aPause(); break;
        case 's':case'S': aSun();   break;
        case 'c':case'C': aCam();   break;
        case '\t':         if(player.inCar) aTab(); break; // tab only while driving
        case 'r':case'R':
            if(player.inCar){
                cars[selectedCar].x=0; cars[selectedCar].z=0;
            } else {
                player.x=0; player.z=10;
            }
            break;
        case '\r':case'\n': { // ENTER key: enter / exit car
            if(!player.inCar){
                // Find nearest car within interaction distance
                float bestDist = 3.2f; // max reach
                int bestCar = -1;
                for(int i=0;i<4;i++){
                    float dx=player.x-cars[i].x, dz=player.z-cars[i].z;
                    float dist=sqrtf(dx*dx+dz*dz);
                    if(dist < bestDist){ bestDist=dist; bestCar=i; }
                }
                if(bestCar>=0){
                    player.inCar   = true;
                    selectedCar    = bestCar;
                }
            } else {
                // Exit car: drop person to left of car (driver door side)
                Car& c = cars[selectedCar];
                float hr = D2R(-c.heading);
                float exitX = c.x - cosf(hr)*1.8f;
                float exitZ = c.z + sinf(hr)*1.8f;
                player.x       = exitX;
                player.z       = exitZ;
                player.heading = c.heading;
                player.inCar   = false;
                collisionBlocked = false;
            }
            break;
        }
    }
}
void keyUp(unsigned char k,int x,int y){
    keys[k]=false;
    // Release building movement keys
    if(k=='i'||k=='I') bkeys[0]=false;
    if(k=='k'||k=='K') bkeys[1]=false;
    if(k=='j'||k=='J') bkeys[2]=false;
    if(k=='l'||k=='L') bkeys[3]=false;
    if(k=='u'||k=='U') bkeys[4]=false;
    if(k=='o'||k=='O') bkeys[5]=false;
}
void specDown(int k,int x,int y){
    if(k==GLUT_KEY_UP)    skeys[0]=true;
    if(k==GLUT_KEY_DOWN)  skeys[1]=true;
    if(k==GLUT_KEY_LEFT)  skeys[2]=true;
    if(k==GLUT_KEY_RIGHT) skeys[3]=true;
}
void specUp(int k,int x,int y){
    if(k==GLUT_KEY_UP)    skeys[0]=false;
    if(k==GLUT_KEY_DOWN)  skeys[1]=false;
    if(k==GLUT_KEY_LEFT)  skeys[2]=false;
    if(k==GLUT_KEY_RIGHT) skeys[3]=false;
}

// ============================================================
// MOUSE (Week 8-9 - orbit camera + button clicks)
// ============================================================
void mouseBtn(int btn,int state,int x,int y){
    if(btn==GLUT_LEFT_BUTTON){
        if(state==GLUT_DOWN){
            mDrag=true; lastMX=x; lastMY=y;
            int sy2=WIN_H-y;
            for(int i=0;i<NBTN;i++){
                Btn& b=btns[i];
                if(x>=b.x&&x<=b.x+b.w&&sy2>=b.y&&sy2<=b.y+b.h){
                    b.fn(); mDrag=false; break;
                }
            }
        } else mDrag=false;
    }
    if(btn==3){camDist-=.8f;if(camDist<5)camDist=5;}
    if(btn==4){camDist+=.8f;if(camDist>80)camDist=80;}
}
void mouseMove(int x,int y){
    if(mDrag){camH+=(x-lastMX)*.5f;camV-=(y-lastMY)*.4f;camV=CLAMP(camV,-5.f,88.f);}
    lastMX=x;lastMY=y;
}

// ============================================================
// MAIN (Week 1 - API init)
// ============================================================
int main(int argc,char** argv)
{
    glutInit(&argc,argv);
    // GLUT_MULTISAMPLE requests hardware MSAA (multi-sample anti-aliasing)
    // — smooths polygon edges without any per-pixel cost at render time.
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH|GLUT_MULTISAMPLE);
    glutInitWindowSize(WIN_W,WIN_H);
    glutInitWindowPosition(50,30);
    glutCreateWindow("Car Park Scene — OpenGL All-Weeks Demo");

    glClearColor(.5f,.75f,.97f,1);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    // MSAA: enable hardware multi-sample anti-aliasing
    glEnable(GL_MULTISAMPLE);
    // Line smoothing for wire overlays (selection ring, HUD lines)
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    // Use explicit glMaterial calls instead of GL_COLOR_MATERIAL
    // (GL_COLOR_MATERIAL was causing glColor calls to override
    //  material settings and produced incorrect object colours)
    // glEnable(GL_COLOR_MATERIAL);
    // glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);

    // Build procedural textures (asphalt, concrete, grass)
    initTextures();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specDown);
    glutSpecialUpFunc(specUp);
    glutMouseFunc(mouseBtn);
    glutMotionFunc(mouseMove);
    glutTimerFunc(16,update,0);
    glutMainLoop();
    return 0;
}
