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
//   Week 10 - 2D HUD overlay, texture concept demonstration
//   Week 11 - Ethics panel visible in HUD
//   Week 12 - Full mini-project integration
//
// CONTROLS  (keyboard + clickable on-screen buttons):
//   W A S D / Arrows - Move selected car
//   TAB              - Switch selected car
//   C                - Cycle camera (3 modes)
//   N                - Toggle Day / Night
//   L                - Cycle lighting mode
//   Z                - Toggle depth test
//   P                - Pause / Resume
//   S                - Toggle sun orbit
//   Mouse Drag       - Orbit camera (follow mode)
//   Scroll           - Zoom
//   Click buttons    - Same as keyboard shortcuts
//   ESC              - Quit
// ============================================================

#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
// SCENE STATE
// ============================================================
enum LightMode { LM_FULL=0,LM_NOSPEC=1,LM_AMBONLY=2,LM_OFF=3,LM_COUNT=4 };
enum DepthMode { DM_ON=0,DM_OFF=1,DM_COUNT=2 };
enum CamMode   { CM_FOLLOW=0,CM_OVERVIEW=1,CM_SIDE=2,CM_COUNT=3 };

LightMode lightMode = LM_FULL;
DepthMode depthMode = DM_ON;
CamMode   camMode   = CM_OVERVIEW;

bool nightMode        = false;
bool paused           = false;
bool sunOrbiting      = true;
bool collisionBlocked = false;  // true this frame when selected car hit an obstacle

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
    GLfloat sA[]={nightMode?.05f:rs*.18f,nightMode?.05f:gs*.18f,nightMode?.10f:bs*.25f,1};
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
// ============================================================
void drawTree(float x,float z,float h=4.5f)
{
    glPushMatrix(); glTranslatef(x,0,z);
    // Trunk (bark brown)
    mat(.40f,.26f,.12f,.10f,.08f,.05f,8.f);
    glPushMatrix(); glRotatef(-90,1,0,0); glutSolidCylinder(.28f,h*.7f,12,5); glPopMatrix();
    // Foliage L1 - darkest
    mat(.15f,.52f,.10f,.05f,.15f,.03f,8.f);
    glPushMatrix(); glTranslatef(.1f,h*.6f,.1f); glScalef(1.3f,.9f,1.3f); glutSolidSphere(h*.28f,14,10); glPopMatrix();
    // L2
    mat(.20f,.62f,.13f,.07f,.20f,.04f,12.f);
    glPushMatrix(); glTranslatef(-.1f,h*.78f,.0f); glScalef(1.1f,1.f,1.1f); glutSolidSphere(h*.22f,12,9); glPopMatrix();
    // L3 top
    mat(.26f,.72f,.17f,.09f,.25f,.06f,16.f);
    glPushMatrix(); glTranslatef(0,h*.94f,0); glutSolidSphere(h*.14f,10,7); glPopMatrix();
    glPopMatrix();
}

// ============================================================
// OFFICE BUILDING with glass facade
// ============================================================
void drawOffice()
{
    glPushMatrix(); glTranslatef(0,0,-16.f);
    // Main concrete body
    mat(.72f,.72f,.70f,.3f,.3f,.28f,18.f);
    glPushMatrix(); glTranslatef(0,5.5f,0); glScalef(18.f,11.f,6.f); glutSolidCube(1); glPopMatrix();
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
// GROUND: tarmac, bays, road markings, pavement, grass borders
// ============================================================
void drawGround()
{
    // Large tarmac base
    mat(.22f,.22f,.24f,.12f,.12f,.12f,8.f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(-20,0,-18); glVertex3f(20,0,-18);
    glVertex3f(20,0,18);   glVertex3f(-20,0,18);
    glEnd();

    // Grass border strip (left & right of car park)
    mat(.35f,.62f,.20f,.05f,.10f,.03f,5.f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(-20,.01f,-18); glVertex3f(-15,.01f,-18);
    glVertex3f(-15,.01f,18);  glVertex3f(-20,.01f,18);
    glEnd();
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(15,.01f,-18); glVertex3f(20,.01f,-18);
    glVertex3f(20,.01f,18);  glVertex3f(15,.01f,18);
    glEnd();

    // Tar road (main drive aisle, centre)
    mat(.18f,.18f,.20f,.10f,.10f,.10f,10.f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(-4,.01f,-18); glVertex3f(4,.01f,-18);
    glVertex3f(4,.01f,16);   glVertex3f(-4,.01f,16);
    glEnd();

    // Parking bay lines (white)
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
    // Pavement strip (front entrance)
    glColor3f(.75f,.72f,.68f);
    glBegin(GL_QUADS);
    glVertex3f(-20,.02f,14); glVertex3f(20,.02f,14);
    glVertex3f(20,.02f,18);  glVertex3f(-20,.02f,18);
    glEnd();
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
void aTab()    {selectedCar=(selectedCar+1)%4;}

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
    const char* camN[]={"OVERVIEW","FOLLOW CAR","SIDE VIEW"};
    snprintf(buf,256,"Lighting: %-16s | Depth: %-12s | Camera: %-12s | %s",
        lmN[lightMode], depthMode==DM_ON?"ON":"OFF!",
        camN[camMode], nightMode?"NIGHT":"DAY");
    float sr=lightMode==LM_FULL?.3f:lightMode==LM_NOSPEC?1.f:lightMode==LM_AMBONLY?1.f:.7f;
    float sg=lightMode==LM_FULL?1.f:lightMode==LM_NOSPEC?.8f:lightMode==LM_AMBONLY?.5f:.7f;
    float sb=lightMode==LM_FULL?.3f:.2f;
    t8(14,WIN_H-46,buf,sr,sg,sb);

    snprintf(buf,256,"Selected: %s  (TAB=next car)  |  Sun: %.0f deg  |  %s",
        cars[selectedCar].name, sunAngle, paused?"*** PAUSED ***":"Animating");
    t8(14,WIN_H-64,buf,1.f,1.f,.5f);

    const char* lmExp[]={"Full Phong: all 3 lighting components — best depth perception",
                         "No Specular: remove highlights — surfaces look flat/dull",
                         "Ambient Only: NO directional cues — depth perception lost!",
                         "Lights Off: pure flat colours — impossible to judge 3D depth"};
    t8(14,WIN_H-82,lmExp[lightMode],1.f,.95f,.65f);

    t8(14,WIN_H-100,"WASD/Arrows=Drive  SPACE=Brake  TAB=Switch Car  C=Camera  Click buttons below",
        .65f,.65f,.65f);
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
// WEEK 2 - VIEWING: 3 camera modes
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
void update(int v)
{
    if(!paused){
        sceneTime+=.016f;
        if(sunOrbiting&&!nightMode){
            sunAngle=fmodf(sunAngle+.20f,360.f);
            sunHeight=sinf(D2R(sunAngle))*.78f+.22f;
            sunHeight=CLAMP(sunHeight,0.f,1.f);
        }
        // Drive selected car  (with collision detection)
        Car& c=cars[selectedCar];
        float spd=.0f,turnR=0.f;
        if(keys['w']||keys['W']||skeys[0]) spd=.12f;
        if(keys['x']||keys['X']||skeys[1]) spd=-.08f;
        if(keys['a']||keys['A']||skeys[2]) turnR=-2.f;
        if(keys['d']||keys['D']||skeys[3]) turnR= 2.f;
        c.heading+=turnR*(ABS(spd)>0.01f?1.f:0.3f);
        float hr=D2R(-c.heading);
        // Compute candidate position then test for collisions
        float nx=CLAMP(c.x+sinf(hr)*spd,-19.f,19.f);
        float nz=CLAMP(c.z+cosf(hr)*spd,-17.f,17.f);
        if(!checkCollision(nx,nz)){
            c.x=nx; c.z=nz;              // accept move
            c.wheelRot+=spd*R2D(1.f/0.36f);
            collisionBlocked=false;
        } else {
            // Blocked: keep old position; flag only when car is actually
            // trying to move forward/back (not just turning on the spot)
            collisionBlocked=(ABS(spd)>0.01f);
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
    switch(k){
        case 27: exit(0);
        case 'l':case'L': aLight(); break;
        case 'z':case'Z': aDepth(); break;
        case 'n':case'N': aNight(); break;
        case 'p':case'P': aPause(); break;
        case 's':case'S': aSun();   break;
        case 'c':case'C': aCam();   break;
        case '\t':         aTab();   break;
        case 'r':case'R':
            cars[selectedCar].x=0; cars[selectedCar].z=0; break;
    }
}
void keyUp(unsigned char k,int x,int y){keys[k]=false;}
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
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(WIN_W,WIN_H);
    glutInitWindowPosition(50,30);
    glutCreateWindow("Car Park Scene — OpenGL All-Weeks Demo");

    glClearColor(.5f,.75f,.97f,1);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    // Use explicit glMaterial calls instead of GL_COLOR_MATERIAL
    // (GL_COLOR_MATERIAL was causing glColor calls to override
    //  material settings and produced incorrect object colours)
    // glEnable(GL_COLOR_MATERIAL);
    // glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);

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
