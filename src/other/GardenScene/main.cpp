// =============================================================================
// GARDEN SCENE v3 - Complete Lighting, Shadow & Depth Demonstration
//
// IMPROVEMENTS IN THIS VERSION:
//   [1] LIGHT GREEN GRASS  - natural meadow green, subtle tile variation
//   [2] REALISTIC PLANAR SHADOWS - every object casts a proper projected
//       shadow ON THE GROUND, shaped like the object, positioned by light
//   [3] PAUSE BUTTON (P key) - freezes entire animation
//   [4] ON-SCREEN CLICKABLE BUTTONS - HUD buttons for L / Z / N / P / S
//       Click buttons with LEFT mouse button
//   [5] SHADOW FIXES - shadows only ON ground, correct direction from sun/moon
//   [6] BALL shadow uses planar matrix too (correct elongation at low sun)
//   [7] FLOWER shadows included
//   [8] Sun angle slider shown on HUD
//
// CONTROLS:
//   WASD / Arrow Keys  - Move ball
//   SPACE              - Bounce ball
//   P                  - PAUSE / RESUME
//   N                  - Toggle Day / Night
//   L                  - Cycle lighting modes
//   Z                  - Toggle depth test
//   S                  - Pause/Resume sun orbit
//   R                  - Reset ball
//   Mouse Left Drag    - Orbit camera
//   Mouse Left Click   - Click HUD buttons
//   Scroll Wheel       - Zoom
//   ESC                - Quit
// =============================================================================

#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <glm/glm.hpp>

#define PI    3.14159265358979f
#define D2R(d) ((d)*PI/180.0f)
#define R2D(r) ((r)*180.0f/PI)
#define CLAMP(v,a,b) ((v)<(a)?(a):(v)>(b)?(b):(v))

int WIN_W=1200, WIN_H=740;

// =============================================================================
// SCENE STATE
// =============================================================================
enum LightMode { LM_FULL=0, LM_NOSPEC=1, LM_AMBONLY=2, LM_OFF=3, LM_COUNT=4 };
enum DepthMode { DM_ON=0, DM_OFF=1, DM_COUNT=2 };

LightMode lightMode  = LM_FULL;
DepthMode depthMode  = DM_ON;
bool      nightMode  = false;
bool      paused     = false;      // NEW: pause flag
bool      sunOrbiting= true;
float     sunAngle   = 45.0f;
float     sunHeight  = 0.8f;
float     sceneTime  = 0.0f;

// Light direction (unit vector toward the sky, updated every frame)
float LX=0, LY=1, LZ=0;   // current effective light direction

// Ball
struct Vec3 { float x,y,z; };
Vec3  ballPos   = {0.f,0.4f,0.f};
Vec3  ballVel   = {0.f,0.f,0.f};
const float BALL_R    = 0.4f;
const float GROUND_Y  = 0.4f;
const float GRAVITY   = -0.018f;
const float BOUNCEDAMP= 0.72f;
float ballRX=0, ballRZ=0;

// door
float doorAngle = 0.0f;
bool doorOpening = false;
float eggCookProgress = 0.0f;  // 0 to 1 for cooking animation
bool isCooking = false;

// Camera -- Orbit based
float camDist  = 22.f;
float camH     = 30.f;
float camV     = 28.f;

// Camera -- First person
Vec3 camPos = {0.0f, 1.8f, 5.0f};   // eye position
Vec3 camFront = {0.0f, 0.0f, -1.0f}; // direction camera is facing

float camX = 0.0f, camY = 1.8f, camZ = 8.0f;
float yaw = -90.0f; // looking forward
float pitch = 0.0f;

// direction vector
float dirX, dirY, dirZ;

// Input
bool keys[256] = {};
bool sk[8]     = {};

// Mouse
int   lastMX=0, lastMY=0;
bool  mDrag=false;
int   clickX=0, clickY=0;
bool  clicked=false;

// =============================================================================
// PLANAR SHADOW MATRIX
// Projects any geometry onto the y=0 ground plane given light direction (lx,ly,lz)
// lx,ly,lz = direction FROM scene TOWARD light  (ly must be > 0)
//
// Derivation: a point P at height Py, with directional light L,
// casts a shadow at ground (y=0) via:
//   t = Py/ly  (parametric ray: shadow_point = P - t*L)
//   sx = Px - (Py/ly)*lx
//   sz = Pz - (Py/ly)*lz
// Written as 4x4 column-major matrix (multiply through by ly):
// =============================================================================
void shadowMatrix(float lx,float ly,float lz, float m[16])
{
    if (ly < 0.01f) ly = 0.01f;
    // Column 0
    m[0]=ly;  m[1]=0;  m[2]=0;  m[3]=0;
    // Column 1  (maps y → flattened)
    m[4]=-lx; m[5]=0;  m[6]=-lz;m[7]=0;
    // Column 2
    m[8]=0;   m[9]=0;  m[10]=ly;m[11]=0;
    // Column 3
    m[12]=0;  m[13]=0; m[14]=0; m[15]=ly;
}

// =============================================================================
// MATERIAL HELPER
// =============================================================================
void setMat(float r,float g,float b,float sr,float sg,float sb,float sh,float a=1.f)
{
    GLfloat amb[] ={r*.30f,g*.30f,b*.30f,a};
    GLfloat dif[] ={r,g,b,a};
    GLfloat spec[]={sr,sg,sb,a};
    GLfloat shine[]={sh};
    if (lightMode==LM_NOSPEC||lightMode==LM_AMBONLY){spec[0]=spec[1]=spec[2]=0;shine[0]=0;}
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,  amb);
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,  dif);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR, spec);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,shine);
}

// =============================================================================
// LIGHTING SETUP — called after gluLookAt each frame
// =============================================================================
void setupLights()
{
    glEnable(GL_NORMALIZE);
    if (lightMode==LM_OFF){ glDisable(GL_LIGHTING); return; }
    glEnable(GL_LIGHTING);

    // --- Sun / Moon direction ---
    float sx=cosf(D2R(sunAngle))*cosf(D2R(sunHeight*90.f));
    float sy=sinf(D2R(sunHeight*90.f));
    float sz=sinf(D2R(sunAngle))*cosf(D2R(sunHeight*90.f));
    LX=sx; LY=sy; LZ=sz;   // store for shadow use

    GLfloat sunPos[]={sx,sy,sz,0.f};   // w=0 directional

    float dayF=CLAMP(sy,0.f,1.f);
    float r_s,g_s,b_s;
    if (!nightMode) {
        // DAY LIGHTING: warm yellowish sun
        float srb=CLAMP(1.f-dayF*2.5f,0.f,1.f);
        r_s=0.95f; g_s=0.85f-srb*.35f; b_s=0.80f-srb*.60f;
        float iv=0.3f+dayF*.7f;
        r_s*=iv; g_s*=iv; b_s*=iv;
    } else { 
        // NIGHT LIGHTING: cool blue-grey (NOT yellow!)
        r_s=0.08f; g_s=0.10f; b_s=0.20f; 
    }

    GLfloat sD[]={r_s,g_s,b_s,1};
    GLfloat sS[]={r_s*.6f,g_s*.6f,b_s*.6f,1};
    GLfloat sA[]={nightMode?.03f:r_s*.20f, nightMode?.03f:g_s*.20f, nightMode?.06f:b_s*.25f, 1};
    if (lightMode==LM_AMBONLY){sD[0]=sD[1]=sD[2]=0;sS[0]=sS[1]=sS[2]=0;}
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0,GL_POSITION,sunPos);
    glLightfv(GL_LIGHT0,GL_DIFFUSE, sD);
    glLightfv(GL_LIGHT0,GL_SPECULAR,sS);
    glLightfv(GL_LIGHT0,GL_AMBIENT, sA);

    // --- Outdoor garden lamp (point light) ---
    GLfloat lPos[]={-4.f,3.5f,3.f,1.f};
    float lR=nightMode?1.f:.18f, lG=nightMode?.85f:.15f, lB=nightMode?.50f:.09f;
    GLfloat lD[]={lR,lG,lB,1};
    GLfloat lA[]={0,0,0,1};
    GLfloat lSp[]={lR*.5f,lG*.5f,lB*.3f,1};
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1,GL_POSITION,lPos);
    glLightfv(GL_LIGHT1,GL_DIFFUSE, lD);
    glLightfv(GL_LIGHT1,GL_AMBIENT, lA);
    glLightfv(GL_LIGHT1,GL_SPECULAR,lSp);
    glLightf (GL_LIGHT1,GL_CONSTANT_ATTENUATION, 0.4f);
    glLightf (GL_LIGHT1,GL_LINEAR_ATTENUATION,   0.08f);
    glLightf (GL_LIGHT1,GL_QUADRATIC_ATTENUATION,0.02f);

    // --- Indoor light (warm, activates when inside) ---
    // Check if camera is inside house: z < -5, |x| < 3
    bool insideHouse = (camZ < -5.0f) && (camX > -3.0f) && (camX < 3.0f);
    
    GLfloat indoorPos[] = {0.0f, 2.8f, -7.5f, 1.0f};
    GLfloat indoorDiffuse[] = {1.0f, 0.95f, 0.85f, 1.0f};  // warm white
    GLfloat indoorSpecular[] = {0.8f, 0.8f, 0.7f, 1.0f};
    
    if (insideHouse) {
        glEnable(GL_LIGHT2);
        glLightfv(GL_LIGHT2, GL_POSITION, indoorPos);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, indoorDiffuse);
        glLightfv(GL_LIGHT2, GL_SPECULAR, indoorSpecular);
        glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.3f);
        glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.05f);
        glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.01f);
    } else {
        glDisable(GL_LIGHT2);
    }
}

// =============================================================================
// DRAW OBJECTS (exact geometry, used for both real draw and shadow draw)
// Each function is called twice: once normally, once inside shadow matrix
// =============================================================================

// --- House geometry ---
void geomHouse()
{
    glPushMatrix();
    glTranslatef(0,0,-7.5f);
    // External walls (hollow cube for interior)
    glPushMatrix(); 
    glTranslatef(0,1.8f,0); 
    glScalef(6.5f,3.6f,5.f); 
    glutSolidCube(1); 
    glPopMatrix();
    // Interior floor
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-3, 0, -2);
    glVertex3f( 3, 0, -2);
    glVertex3f( 3, 0,  2);
    glVertex3f(-3, 0,  2);
    glEnd();
    glPopMatrix();
}

// --- Tree geometry ---
void geomTree()
{
    glPushMatrix();
    glTranslatef(-5.f,0,-3.f);
    // Trunk
    glPushMatrix(); glRotatef(-90,1,0,0); glutSolidCylinder(0.35f,3.2f,12,4); glPopMatrix();
    // Foliage mass (larger unified sphere for shadow purposes)
    glPushMatrix(); glTranslatef(0.f,4.5f,0.f); glScalef(1.6f,2.2f,1.6f);
    glutSolidSphere(1.5f,14,10); glPopMatrix();
    glPopMatrix();
}

// --- Lamp post geometry ---
void geomLamp()
{
    glPushMatrix(); glTranslatef(-4.f,0,3.f);
    glPushMatrix(); glRotatef(-90,1,0,0); glutSolidCylinder(.07f,3.5f,8,2); glPopMatrix();
    glPushMatrix(); glTranslatef(0,3.5f,0); glScalef(.5f,.35f,.5f); glutSolidCube(1); glPopMatrix();
    glPopMatrix();
}

// --- Fence posts geometry (shadow subset) ---
void geomFencePosts()
{
    struct Run {float x0,z0,x1,z1;};
    Run runs[]={{-12,12,12,12},{-12,-12,12,-12},{-12,-12,-12,12},{12,-12,12,12}};
    for (auto& r:runs){
        float dx=r.x1-r.x0,dz=r.z1-r.z0;
        float len=sqrtf(dx*dx+dz*dz);
        int n=(int)(len/2.f)+1;
        for (int p=0;p<n;p+=2){
            float t=(float)p/(n-1);
            glPushMatrix(); glTranslatef(r.x0+dx*t,.65f,r.z0+dz*t);
            glScalef(.12f,1.3f,.12f); glutSolidCube(1); glPopMatrix();
        }
    }
}

// --- Flowers geometry (small spheres) ---
void geomFlowers()
{
    float fxs[]={-5,-4.2f,-3.5f,-2.8f,-4.5f,-3,2,3,4,5,2.5f,4.5f};
    float fzs[]={1.5f,1.2f,0.8f,1.6f,-0.8f,-1,1.5f,0.8f,1.6f,1,-0.8f,-1};
    for (int i=0;i<12;i++){
        glPushMatrix(); glTranslatef(fxs[i],.78f,fzs[i]);
        glutSolidSphere(0.22f,6,4); glPopMatrix();
        glPushMatrix(); glTranslatef(fxs[i],.35f,fzs[i]);
        glRotatef(-90,1,0,0); glutSolidCylinder(.05f,.7f,4,1); glPopMatrix();
    }
}

// --- Ball geometry ---
void geomBall()
{
    glPushMatrix(); glTranslatef(ballPos.x,ballPos.y,ballPos.z);
    glutSolidSphere(BALL_R,24,18);
    glPopMatrix();
}

// =============================================================================
// DRAW ALL SHADOWS
// Shadow approach:
//   1. Save matrix
//   2. Translate up by tiny epsilon (0.015) to sit just above ground → no z-fight
//   3. Apply planar shadow matrix (squashes all geometry to y=0)
//   4. Draw each object in flat dark colour
//   5. Pop matrix
// The offset + shadow matrix means shadow appears exactly on the ground surface.
// =============================================================================
void drawAllShadows()
{
    // Need light above horizon
    if (LY < 0.04f) return;

    // Shadow colour and intensity based on light elevation
    float elev  = LY;   // = sin(elevation_angle)
    float alpha = CLAMP(elev * 0.70f, 0.10f, 0.62f);
    if (nightMode) alpha = 0.13f;

    float sm[16];
    shadowMatrix(LX, LY, LZ, sm);

    // Blended flat shadow — disable lighting so colour is pure flat
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);    // Don't write to z-buffer (shadow is a decal)

    // Shadow colour: dark olive in day, cool blue-grey at night
    if (!nightMode)
        glColor4f(0.06f, 0.08f, 0.04f, alpha);
    else
        glColor4f(0.06f, 0.06f, 0.18f, alpha);

    glPushMatrix();
        // Tiny Y lift so shadow doesn't z-fight with grass
        glTranslatef(0.f, 0.015f, 0.f);
        // Apply shadow projection matrix — squashes all geometry to ground plane
        glMultMatrixf(sm);

        // Draw shadow silhouettes of each object
        geomHouse();
        geomTree();
        geomLamp();
        geomFencePosts();
        geomFlowers();
        geomBall();

    glPopMatrix();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    if (lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// =============================================================================
// DRAW GROUND — light natural green grass + path + flower beds
// =============================================================================
void drawGround()
{
    // GRASS — light meadow green tiles (two shades for texture effect)
    int   tiles = 16;
    float ts    = 1.8f;
    float half  = tiles*ts*0.5f;

    for (int i=0;i<tiles;i++){
        for (int j=0;j<tiles;j++){
            float tx=-half+i*ts, tz=-half+j*ts;
            if ((i+j)%2==0)
                // Light fresh green
                setMat(0.38f,0.68f,0.22f, 0.05f,0.10f,0.03f,5.f);
            else
                // Slightly darker natural green
                setMat(0.32f,0.60f,0.18f, 0.04f,0.09f,0.03f,5.f);
            glBegin(GL_QUADS);
            glNormal3f(0,1,0);
            glVertex3f(tx,   0,tz);   glVertex3f(tx+ts,0,tz);
            glVertex3f(tx+ts,0,tz+ts);glVertex3f(tx,   0,tz+ts);
            glEnd();
        }
    }

    // STONE PATH
    setMat(0.62f,0.58f,0.52f, 0.2f,0.2f,0.18f,18.f);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-1.2f,0.01f,2.f);  glVertex3f(1.2f,0.01f,2.f);
    glVertex3f(1.2f, 0.01f,-6.5f);glVertex3f(-1.2f,0.01f,-6.5f);
    glEnd();
    // Path stones
    setMat(0.58f,0.55f,0.50f,0.3f,0.3f,0.28f,25.f);
    for (int i=0;i<8;i++){
        glPushMatrix();
        glTranslatef((i%2==0)?-.3f:.3f, 0.02f, 1.5f-i*1.1f);
        glScalef(.8f,.07f,.7f); glutSolidCube(1); glPopMatrix();
    }

    // FLOWER BEDS (soil)
    setMat(0.42f,0.28f,0.15f,0.05f,0.04f,0.02f,5.f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(-5.5f,0.02f,2.f); glVertex3f(-1.5f,0.02f,2.f);
    glVertex3f(-1.5f,0.02f,-1.5f); glVertex3f(-5.5f,0.02f,-1.5f); glEnd();
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(1.5f,0.02f,2.f); glVertex3f(5.5f,0.02f,2.f);
    glVertex3f(5.5f,0.02f,-1.5f); glVertex3f(1.5f,0.02f,-1.5f); glEnd();
}

// =============================================================================
// DRAW TREE with bright layered foliage
// =============================================================================
void drawTree()
{
    glPushMatrix(); glTranslatef(-5,0,-3);
    // Trunk
    setMat(0.38f,0.25f,0.12f,0.10f,0.08f,0.05f,8.f);
    glPushMatrix(); glRotatef(-90,1,0,0); glutSolidCylinder(0.35f,3.2f,14,6); glPopMatrix();
    glPushMatrix(); glTranslatef(0.1f,3,0); glRotatef(5,0,0,1); glRotatef(-90,1,0,0);
    glutSolidCylinder(0.22f,1.2f,10,4); glPopMatrix();
    // Branches
    struct Br{float tx,ty,tz,rx,ry,rz,len,rad;};
    Br brs[]={{.2f,2.8f,0,30,0,0,1.8f,.12f},{-.2f,2.5f,.1f,-25,45,0,1.6f,.11f},
              {0,3,-.2f,15,90,10,1.4f,.10f},{.1f,3.5f,.1f,-20,-60,5,1.5f,.10f},
              {-.1f,3.8f,0,10,180,0,1.2f,.09f}};
    setMat(0.35f,0.22f,0.10f,0.08f,0.06f,0.04f,6.f);
    for (auto& b:brs){
        glPushMatrix(); glTranslatef(b.tx,b.ty,b.tz);
        glRotatef(b.rx,1,0,0); glRotatef(b.ry,0,1,0); glRotatef(b.rz,0,0,1);
        glRotatef(-90,1,0,0); glutSolidCylinder(b.rad,b.len,8,3); glPopMatrix();
    }
    // Foliage — vivid natural greens, layered
    setMat(0.18f,0.58f,0.12f,0.06f,0.18f,0.04f,10.f);
    glPushMatrix(); glTranslatef(.2f,3.4f,.1f); glScalef(1.4f,1.f,1.4f);
    glutSolidSphere(1.5f,18,12); glPopMatrix();
    setMat(0.22f,0.65f,0.15f,0.07f,0.22f,0.05f,12.f);
    glPushMatrix(); glTranslatef(-.1f,4.5f,-.1f); glScalef(1.2f,1.1f,1.2f);
    glutSolidSphere(1.25f,16,10); glPopMatrix();
    setMat(0.26f,0.72f,0.18f,0.08f,0.25f,0.06f,15.f);
    glPushMatrix(); glTranslatef(.05f,5.5f,.05f); glScalef(.9f,1.2f,.9f);
    glutSolidSphere(1.f,12,9); glPopMatrix();
    setMat(0.30f,0.78f,0.22f,0.10f,0.28f,0.08f,18.f);
    glPushMatrix(); glTranslatef(.1f,6.3f,0); glutSolidSphere(.65f,10,7); glPopMatrix();
    glPopMatrix();
}

// =============================================================================
// DRAW HOUSE — full detailed version with interior
// =============================================================================
void drawHouse()
{
    float width = 6.5f;
    float height = 3.6f;
    float depth = 5.0f;

    float doorWidth = 1.5f;
    float doorHeight = 2.2f;
    
    glPushMatrix(); glTranslatef(0,0,-7.5f);
    
    // EXTERIOR WALLS (basic structure first)
    setMat(0.88f,0.82f,0.72f,0.25f,0.23f,0.20f,22.f);
    
    // Back wall
    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    glVertex3f(-3.25f, 0, -2.5f);
    glVertex3f(3.25f, 0, -2.5f);
    glVertex3f(3.25f, 3.6f, -2.5f);
    glVertex3f(-3.25f, 3.6f, -2.5f);
    glEnd();
    
    // Left wall
    glBegin(GL_QUADS);
    glNormal3f(-1, 0, 0);
    glVertex3f(-3.25f, 0, -2.5f);
    glVertex3f(-3.25f, 0, 2.5f);
    glVertex3f(-3.25f, 3.6f, 2.5f);
    glVertex3f(-3.25f, 3.6f, -2.5f);
    glEnd();
    
    // Right wall
    glBegin(GL_QUADS);
    glNormal3f(1, 0, 0);
    glVertex3f(3.25f, 0, -2.5f);
    glVertex3f(3.25f, 3.6f, -2.5f);
    glVertex3f(3.25f, 3.6f, 2.5f);
    glVertex3f(3.25f, 0, 2.5f);
    glEnd();

    // Interior floor - light wood color
    setMat(0.6f, 0.45f, 0.3f, 0.3f, 0.25f, 0.18f, 15.f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-3, 0.01f, -2);
    glVertex3f(3, 0.01f, -2);
    glVertex3f(3, 0.01f, 2);
    glVertex3f(-3, 0.01f, 2);
    glEnd();
    
    // Interior ceiling
    setMat(0.8f, 0.8f, 0.75f, 0.2f, 0.2f, 0.18f, 10.f);
    glBegin(GL_QUADS);
    glNormal3f(0, -1, 0);
    glVertex3f(-3, 3.5f, -2);
    glVertex3f(3, 3.5f, -2);
    glVertex3f(3, 3.5f, 2);
    glVertex3f(-3, 3.5f, 2);
    glEnd();

    // DOOR (with animation)
    setMat(0.38f,0.20f,0.10f,0.15f,0.10f,0.08f,12.f);
    glPushMatrix();
    glTranslatef(0, 0, 2.51f);  // front face
    glTranslatef(-0.75f, 1.1f, 0);  // hinge position
    glRotatef(doorAngle, 0, 1, 0);  // rotate around hinge
    glTranslatef(0.75f, -1.1f, 0);  // back to position
    glScalef(1.5f, 2.2f, 0.1f);
    glutSolidCube(1);
    glPopMatrix();

    // Door frame
    setMat(0.5f,0.35f,0.20f,0.2f,0.15f,0.10f,10.f);
    glBegin(GL_QUADS);
    // Left frame
    glNormal3f(0, 0, 1);
    glVertex3f(-1.5f, 0, 2.55f);
    glVertex3f(-1.35f, 0, 2.55f);
    glVertex3f(-1.35f, 2.4f, 2.55f);
    glVertex3f(-1.5f, 2.4f, 2.55f);
    glEnd();
    glBegin(GL_QUADS);
    // Right frame
    glNormal3f(0, 0, 1);
    glVertex3f(1.35f, 0, 2.55f);
    glVertex3f(1.5f, 0, 2.55f);
    glVertex3f(1.5f, 2.4f, 2.55f);
    glVertex3f(1.35f, 2.4f, 2.55f);
    glEnd();
    glBegin(GL_QUADS);
    // Top frame
    glNormal3f(0, 0, 1);
    glVertex3f(-1.5f, 2.25f, 2.55f);
    glVertex3f(1.5f, 2.25f, 2.55f);
    glVertex3f(1.5f, 2.4f, 2.55f);
    glVertex3f(-1.5f, 2.4f, 2.55f);
    glEnd();
    
    // Door handle
    setMat(0.80f,0.72f,0.10f,0.9f,0.85f,.2f,90.f);
    glPushMatrix();
    glTranslatef(0.6f, 1.1f, 2.61f);
    glutSolidSphere(0.08f, 8, 6);
    glPopMatrix();

    // ROOF
    setMat(0.62f,0.22f,0.15f,0.20f,0.10f,0.08f,15.f);
    glBegin(GL_TRIANGLES);
    glNormal3f(0,0,1);
    glVertex3f(-3.25f,3.6f,2.5f); glVertex3f(3.25f,3.6f,2.5f); glVertex3f(0,6.f,2.5f);
    glEnd();
    glBegin(GL_TRIANGLES);
    glNormal3f(0,0,-1);
    glVertex3f(-3.25f,3.6f,-2.5f); glVertex3f(0,6.f,-2.5f); glVertex3f(3.25f,3.6f,-2.5f);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(-0.88f,0.47f,0);
    glVertex3f(-3.25f,3.6f,-2.5f); glVertex3f(-3.25f,3.6f,2.5f);
    glVertex3f(0,6.f,2.5f); glVertex3f(0,6.f,-2.5f);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(0.88f,0.47f,0);
    glVertex3f(3.25f,3.6f,-2.5f); glVertex3f(0,6.f,-2.5f);
    glVertex3f(0,6.f,2.5f); glVertex3f(3.25f,3.6f,2.5f);
    glEnd();

    // CHIMNEY
    setMat(0.60f,0.35f,0.25f,0.15f,0.12f,0.10f,10.f);
    glPushMatrix(); glTranslatef(1.8f,4.8f,0); glScalef(.7f,2.5f,.7f); glutSolidCube(1); glPopMatrix();
    setMat(0.40f,0.25f,0.20f,0.2f,0.15f,0.12f,15.f);
    glPushMatrix(); glTranslatef(1.8f,6.15f,0); glScalef(.85f,.22f,.85f); glutSolidCube(1); glPopMatrix();

    // SMOKE
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int s=0;s<5;s++){
        float so=fmodf(sceneTime*.4f+s*.3f,1.5f), sa=0.4f-so*.27f;
        if(sa<0)sa=0;
        glColor4f(.7f,.7f,.7f,sa);
        glPushMatrix();
        glTranslatef(1.8f,6.4f+so*1.5f,0);
        glScalef(.3f+so*.3f,.3f+so*.2f,.3f+so*.3f);
        glutSolidSphere(1,8,6);
        glPopMatrix();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    if (lightMode!=LM_OFF) glEnable(GL_LIGHTING);

    // WINDOWS
    setMat(0.92f,0.92f,0.88f,0.4f,0.4f,0.38f,35.f);
    for (int wx=-1;wx<=1;wx+=2){
        glPushMatrix();
        glTranslatef((float)wx*1.9f,2.2f,2.52f);
        glScalef(1.1f,1.f,.10f);
        glutSolidCube(1);
        glPopMatrix();
        setMat(0.55f,0.75f,0.95f,0.9f,0.9f,1.f,110.f,.6f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glPushMatrix();
        glTranslatef((float)wx*1.9f,2.2f,2.56f);
        glScalef(.90f,.80f,.05f);
        glutSolidCube(1);
        glPopMatrix();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        setMat(0.92f,0.92f,0.88f,0.4f,0.4f,0.38f,35.f);
        glPushMatrix();
        glTranslatef((float)wx*1.9f,1.65f,2.62f);
        glScalef(1.2f,.10f,.25f);
        glutSolidCube(1);
        glPopMatrix();
    }

    // PORCH
    setMat(0.60f,0.58f,0.55f,0.2f,0.2f,0.18f,20.f);
    glPushMatrix();
    glTranslatef(0,.10f,3.f);
    glScalef(1.6f,.20f,.9f);
    glutSolidCube(1);
    glPopMatrix();
    setMat(0.62f,0.22f,0.15f,0.2f,.10f,.08f,15.f);
    glPushMatrix();
    glTranslatef(0,3.f,3.f);
    glScalef(2.5f,.15f,1.4f);
    glutSolidCube(1);
    glPopMatrix();
    setMat(0.92f,0.90f,0.85f,0.4f,0.4f,0.38f,40.f);
    for (int pp=-1;pp<=1;pp+=2){
        glPushMatrix();
        glTranslatef((float)pp*.85f,1.5f,3.f);
        glRotatef(-90,1,0,0);
        glutSolidCylinder(.10f,1.55f,8,3);
        glPopMatrix();
    }
    glPopMatrix();
}

// =============================================================================
// DRAW STOVE WITH COOKING ANIMATION
// =============================================================================
void drawStove() {
    // Only draw if inside house
    bool insideHouse = (camZ < -5.0f) && (camX > -3.0f) && (camX < 3.0f);
    if (!insideHouse) return;

    glPushMatrix();
    glTranslatef(0, 0, -7.5f);
    
    // Stove base - dark grey metal
    setMat(0.2f, 0.2f, 0.2f, 0.15f, 0.15f, 0.15f, 12.f);
    glPushMatrix();
    glTranslatef(0, 0.5f, 0);
    glScalef(1.5f, 1.0f, 1.0f);
    glutSolidCube(1);
    glPopMatrix();

    // Stove top surface
    setMat(0.15f, 0.15f, 0.15f, 0.2f, 0.2f, 0.2f, 15.f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-0.75f, 1.0f, -0.5f);
    glVertex3f(0.75f, 1.0f, -0.5f);
    glVertex3f(0.75f, 1.0f, 0.5f);
    glVertex3f(-0.75f, 1.0f, 0.5f);
    glEnd();

    // Burner 1 (left)
    setMat(0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 8.f);
    glPushMatrix();
    glTranslatef(-0.3f, 1.02f, 0);
    glutSolidSphere(0.15f, 10, 10);
    glPopMatrix();

    // Burner 2 (right)
    glPushMatrix();
    glTranslatef(0.3f, 1.02f, 0);
    glutSolidSphere(0.15f, 10, 10);
    glPopMatrix();

    // Pan on left burner
    setMat(0.3f, 0.3f, 0.35f, 0.4f, 0.4f, 0.45f, 20.f);
    glPushMatrix();
    glTranslatef(-0.3f, 1.25f, 0);
    glutSolidTorus(0.04f, 0.25f, 10, 20);
    glPopMatrix();

    // Pan bottom (flat disc for egg to sit on)
    glBegin(GL_POLYGON);
    glNormal3f(0, -1, 0);
    for (int i = 0; i < 20; i++) {
        float a = (6.28f * i) / 20.f;
        glVertex3f(-0.3f + 0.22f * cosf(a), 1.20f, 0 + 0.22f * sinf(a));
    }
    glEnd();

    // Egg with cooking animation
    if (isCooking) {
        eggCookProgress = fmodf(eggCookProgress + 0.02f, 1.0f);
        
        // Raw egg: yellowish white
        if (eggCookProgress < 0.5f) {
            float cookRatio = eggCookProgress * 2.0f;
            setMat(1.0f - cookRatio*0.3f, 1.0f - cookRatio*0.2f, 0.9f - cookRatio*0.7f,
                   0.8f, 0.8f, 0.7f, 25.f);
        } else {
            // Cooked egg: golden yellow
            float cookRatio = (eggCookProgress - 0.5f) * 2.0f;
            setMat(0.95f - cookRatio*0.1f, 0.85f - cookRatio*0.2f, 0.2f + cookRatio*0.1f,
                   0.7f, 0.6f, 0.3f, 20.f);
        }
        
        glPushMatrix();
        glTranslatef(-0.3f, 1.30f, 0);
        // Egg wobbles slightly while cooking
        glRotatef(sinf(eggCookProgress * 6.28f) * 3.0f, 0, 0, 1);
        glutSolidSphere(0.12f, 12, 10);
        glPopMatrix();
    } else {
        // Idle egg: pale yellow-white
        setMat(1.0f, 1.0f, 0.85f, 0.8f, 0.8f, 0.7f, 25.f);
        glPushMatrix();
        glTranslatef(-0.3f, 1.30f, 0);
        glutSolidSphere(0.12f, 12, 10);
        glPopMatrix();
    }

    glPopMatrix();  // pop house translation
}

// =============================================================================
// DRAW FENCE
// =============================================================================
void drawFence()
{
    setMat(0.55f,0.38f,0.20f,0.15f,0.12f,0.08f,12.f);
    struct Run{float x0,z0,x1,z1;};
    Run runs[]={{-12,12,12,12},{-12,-12,12,-12},{-12,-12,-12,12},{12,-12,12,12}};
    for (auto& r:runs){
        float dx=r.x1-r.x0,dz=r.z1-r.z0,len=sqrtf(dx*dx+dz*dz);
        int n=(int)(len/2.f)+1;
        float hd=R2D(atan2f(dx,dz));
        for (int p=0;p<n;p++){
            float t=(float)p/(n-1),px=r.x0+dx*t,pz=r.z0+dz*t;
            glPushMatrix(); glTranslatef(px,.65f,pz); glScalef(.12f,1.3f,.12f); glutSolidCube(1); glPopMatrix();
            glPushMatrix(); glTranslatef(px,1.35f,pz); glRotatef(-90,1,0,0); glutSolidCone(.10f,.22f,6,2); glPopMatrix();
        }
        for (int rl=0;rl<2;rl++){
            glPushMatrix(); glTranslatef((r.x0+r.x1)*.5f,.5f+rl*.55f,(r.z0+r.z1)*.5f);
            glRotatef(hd,0,1,0); glScalef(.08f,.10f,len); glutSolidCube(1); glPopMatrix();
        }
    }
}

// =============================================================================
// DRAW FLOWERS
// =============================================================================
void drawFlowers()
{
    float fxL[]={-5,-4.2f,-3.5f,-2.8f,-4.5f,-3};
    float fzL[]={1.5f,1.2f,0.8f,1.6f,-0.8f,-1};
    float frL[]={.9f,.9f,.2f,.9f,.2f,.8f};
    float fgL[]={.1f,.7f,.8f,.1f,.9f,.1f};
    float fbL[]={.1f,.1f,.1f,.8f,.1f,.9f};
    float fxR[]={2,3,4,5,2.5f,4.5f};
    float fzR[]={1.5f,.8f,1.6f,1,-.8f,-1};
    float frR[]={1,.9f,.1f,1,.9f,.5f};
    float fgR[]={.5f,.1f,.9f,.8f,.1f,0};
    float fbR[]={0,.8f,.1f,0,.9f,.8f};
    for (int i=0;i<6;i++){
        setMat(.10f,.55f,.08f,.04f,.10f,.04f,5.f);
        glPushMatrix(); glTranslatef(fxL[i],.35f,fzL[i]); glRotatef(-90,1,0,0);
        glutSolidCylinder(.05f,.7f,6,2); glPopMatrix();
        setMat(frL[i],fgL[i],fbL[i],.4f,.3f,.3f,30.f);
        glPushMatrix(); glTranslatef(fxL[i],.78f,fzL[i]); glutSolidSphere(.22f,10,8); glPopMatrix();
        setMat(.10f,.55f,.08f,.04f,.10f,.04f,5.f);
        glPushMatrix(); glTranslatef(fxR[i],.35f,fzR[i]); glRotatef(-90,1,0,0);
        glutSolidCylinder(.05f,.7f,6,2); glPopMatrix();
        setMat(frR[i],fgR[i],fbR[i],.4f,.3f,.3f,30.f);
        glPushMatrix(); glTranslatef(fxR[i],.78f,fzR[i]); glutSolidSphere(.22f,10,8); glPopMatrix();
    }
}

// =============================================================================
// DRAW LAMP POST
// =============================================================================
void drawLampPost()
{
    glPushMatrix(); glTranslatef(-4,0,3);
    setMat(.25f,.25f,.30f,.5f,.5f,.5f,60.f);
    glPushMatrix(); glRotatef(-90,1,0,0); glutSolidCylinder(.07f,3.5f,10,3); glPopMatrix();
    glPushMatrix(); glTranslatef(0,3.5f,0); glScalef(.5f,.35f,.5f); glutSolidCube(1); glPopMatrix();
    glDisable(GL_LIGHTING);
    float glow=nightMode?1.f:.3f;
    glColor3f(glow,glow*.85f,glow*.5f);
    glPushMatrix(); glTranslatef(0,3.4f,0); glutSolidSphere(.18f,10,8); glPopMatrix();
    if (lightMode!=LM_OFF) glEnable(GL_LIGHTING);
    glPopMatrix();
}

// =============================================================================
// DRAW BALL
// =============================================================================
void drawBall()
{
    glPushMatrix(); glTranslatef(ballPos.x,ballPos.y,ballPos.z);
    glRotatef(ballRX,1,0,0); glRotatef(ballRZ,0,0,1);
    setMat(.90f,.10f,.10f,.80f,.65f,.65f,72.f);
    glutSolidSphere(BALL_R,32,24);
    setMat(.95f,.92f,.88f,.70f,.70f,.70f,60.f);
    glPushMatrix(); glRotatef(90,1,0,0);
    glutSolidTorus(.07f,BALL_R*.8f,8,24); glPopMatrix();
    glPopMatrix();
}

// =============================================================================
// SKY
// =============================================================================
void drawSky()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    float dayF=CLAMP(sinf(D2R(sunHeight*90.f)),0.f,1.f);
    float tR,tG,tB,hR,hG,hB;
    if (!nightMode){
        float srb=CLAMP(1.f-dayF*2.5f,0.f,1.f);
        tR=.30f+dayF*.12f; tG=.50f+dayF*.12f; tB=.90f;
        hR=.75f+srb*.25f; hG=.75f-srb*.25f; hB=.85f-srb*.45f;
    } else { tR=.01f;tG=.01f;tB=.06f; hR=.04f;hG=.04f;hB=.12f; }
    glClearColor(hR,hG,hB,1);
    float W=200,H=120;
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
    glVertex3f(-W,H,-W);glVertex3f(W,H,-W);glVertex3f(W,H,W);glVertex3f(-W,H,W); glEnd();
    if (nightMode){
        glPointSize(2.f); srand(54321);
        glBegin(GL_POINTS);
        for(int i=0;i<600;i++){
            float a1=((float)rand()/RAND_MAX)*2*PI,a2=((float)rand()/RAND_MAX)*PI*.5f;
            float b=.4f+((float)rand()/RAND_MAX)*.6f;
            glColor3f(b,b,b*.92f);
            glVertex3f(90*cosf(a2)*cosf(a1),90*sinf(a2)+5.f,90*cosf(a2)*sinf(a1));
        }
        glEnd();
    }
    glEnable(GL_DEPTH_TEST);
    if (lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// =============================================================================
// SUN / MOON DISC
// =============================================================================
void drawSunDisc()
{
    float sx=cosf(D2R(sunAngle))*15,sy=sinf(D2R(sunHeight*90.f))*15,sz=sinf(D2R(sunAngle))*15;
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    if (!nightMode){
        float srb=CLAMP(1.f-(sy/15.f)*2.5f,0.f,1.f);
        glColor3f(1.f,.95f-srb*.3f,.4f-srb*.3f);
        glPushMatrix(); glTranslatef(sx,sy,sz); glutSolidSphere(1.2f,16,12); glPopMatrix();
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.f,.9f,.5f,.15f);
        glPushMatrix(); glTranslatef(sx,sy,sz); glutSolidSphere(1.8f,12,9); glPopMatrix();
        glDisable(GL_BLEND);
        glLineWidth(2); glColor3f(1,1,.3f);
        glBegin(GL_LINES); glVertex3f(sx,sy,sz); glVertex3f(sx*.3f,sy*.3f,sz*.3f); glEnd();
        glLineWidth(1);
    } else {
        glColor3f(.92f,.92f,.85f);
        glPushMatrix(); glTranslatef(sx*.7f,sy*.7f,sz*.7f); glutSolidSphere(.9f,14,10); glPopMatrix();
    }
    glEnable(GL_DEPTH_TEST);
    if (lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// =============================================================================
// ON-SCREEN BUTTONS — clickable HUD buttons
// Each button has a bounding box; clicking fires the action
// =============================================================================
struct Button {
    int x,y,w,h;        // screen coords (bottom-left origin)
    const char* label;
    float br,bg,bb;      // button colour
    void (*action)();    // callback
};

// Globals for action callbacks (can't use lambdas as fn pointers easily)
void actLight()  { lightMode=(LightMode)((lightMode+1)%LM_COUNT); }
void actDepth()  { depthMode=(DepthMode)((depthMode+1)%DM_COUNT); }
void actNight()  { nightMode=!nightMode; }
void actPause()  { paused=!paused; }
void actSun()    { sunOrbiting=!sunOrbiting; }
void actReset()  { ballPos={0,GROUND_Y,0}; ballVel={0,0,0}; ballRX=0; ballRZ=0; }
void actBounce() { if(ballPos.y<=GROUND_Y+.02f) ballVel.y=0.28f; }

Button buttons[] = {
    {8,  160, 160, 30, "[L] Lighting",   0.2f,0.7f,0.2f, actLight },
    {8,  126, 160, 30, "[Z] Depth Test", 0.7f,0.3f,0.2f, actDepth },
    {8,   92, 160, 30, "[N] Day/Night",  0.2f,0.4f,0.8f, actNight },
    {8,   58, 160, 30, "[P] Pause",      0.6f,0.2f,0.7f, actPause },
    {8,   24, 160, 30, "[S] Sun Orbit",  0.7f,0.6f,0.1f, actSun   },
    {176,  58, 130, 30, "[R] Reset Ball", 0.5f,0.5f,0.5f, actReset },
    {176,  24, 130, 30, "[SPC] Bounce",  0.8f,0.4f,0.1f, actBounce},
};
const int NUM_BUTTONS = 7;

void drawButtons()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    for (int i=0;i<NUM_BUTTONS;i++){
        Button& b=buttons[i];

        // Shadow
        glColor4f(0,0,0,.4f);
        glBegin(GL_QUADS);
        glVertex2f(b.x+3,b.y-3); glVertex2f(b.x+b.w+3,b.y-3);
        glVertex2f(b.x+b.w+3,b.y+b.h-3); glVertex2f(b.x+3,b.y+b.h-3); glEnd();

        // Button body
        bool isActive=false;
        if (i==0) isActive=(lightMode!=LM_FULL);
        if (i==1) isActive=(depthMode==DM_OFF);
        if (i==2) isActive=nightMode;
        if (i==3) isActive=paused;
        if (i==4) isActive=sunOrbiting;

        float alpha = isActive ? 0.92f : 0.72f;
        float brr = isActive ? b.br*1.3f : b.br;
        float bgg = isActive ? b.bg*1.3f : b.bg;
        float bbb = isActive ? b.bb*1.3f : b.bb;
        glColor4f(CLAMP(brr,0,1),CLAMP(bgg,0,1),CLAMP(bbb,0,1),alpha);
        glBegin(GL_QUADS);
        glVertex2f(b.x,b.y); glVertex2f(b.x+b.w,b.y);
        glVertex2f(b.x+b.w,b.y+b.h); glVertex2f(b.x,b.y+b.h); glEnd();

        // Border
        glColor4f(1,1,1,.7f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(b.x,b.y); glVertex2f(b.x+b.w,b.y);
        glVertex2f(b.x+b.w,b.y+b.h); glVertex2f(b.x,b.y+b.h); glEnd();
        glLineWidth(1);

        // Label
        glColor3f(1,1,1);
        glRasterPos2i(b.x+8, b.y+10);
        for(const char*c=b.label;*c;c++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*c);
    }
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    if (lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// =============================================================================
// HUD — info panel + status
// =============================================================================
void drawHUD()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Top info panel
    glColor4f(0,0,0,.60f);
    glBegin(GL_QUADS);
    glVertex2f(6,WIN_H-6); glVertex2f(700,WIN_H-6);
    glVertex2f(700,WIN_H-130); glVertex2f(6,WIN_H-130); glEnd();
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
    t9(14,WIN_H-24,"=== GARDEN SCENE — Lighting, Shadow & Depth Demo ===",0.3f,1.f,0.4f);

    const char* lmN[]={"FULL PHONG (Ambient+Diffuse+Specular)","NO SPECULAR","AMBIENT ONLY","LIGHTING OFF"};
    float lmR[]={.4f,1.f,1.f,.7f},lmG[]={1.f,.8f,.5f,.7f},lmB[]={.4f,.2f,.2f,.7f};
    snprintf(buf,256,"Lighting: %s",lmN[lightMode]);
    t8(14,WIN_H-46,buf,lmR[lightMode],lmG[lightMode],lmB[lightMode]);

    snprintf(buf,256,"Depth Test: %s | Mode: %s | Sun: %.0f deg | %s",
        depthMode==DM_ON?"ON (correct)":"OFF (broken!)",
        nightMode?"NIGHT":"DAY", sunAngle,
        paused?"*** PAUSED ***":"Running");
    t8(14,WIN_H-64,buf,
        paused?1.f:0.8f, paused?0.3f:0.8f, paused?0.3f:0.8f);

    t8(14,WIN_H-82,"Shadows: ALL objects cast planar shadow from sun/moon direction",0.9f,0.75f,0.3f);

    const char* lExp[]=
        {"Full Phong = shape+depth clear, specular highlight moves with sun",
         "No Specular = dull surfaces, shape still visible from diffuse",
         "Ambient Only = NO depth cues at all — everything flat, 3D lost!",
         "Lights Off = flat paint — no 3D illusion whatsoever"};
    snprintf(buf,256,"Effect: %s",lExp[lightMode]);
    t8(14,WIN_H-100,buf,1.f,1.f,.6f);

    t8(14,WIN_H-118,"WASD=Move Camera  E=Door  C=Cook Egg  P=Pause  N=Night  L=Light  Scroll=Zoom  ESC=Quit",
       .6f,.6f,.6f);

    // Sun elevation bar
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,.5f);
    glBegin(GL_QUADS);
    glVertex2f(WIN_W-260,WIN_H-130); glVertex2f(WIN_W-6,WIN_H-130);
    glVertex2f(WIN_W-6,WIN_H-6);     glVertex2f(WIN_W-260,WIN_H-6); glEnd();
    glDisable(GL_BLEND);

    t8(WIN_W-255,WIN_H-24,"SHADOW DIRECTION",0.4f,0.9f,1.f);
    snprintf(buf,256,"Light elevation: %.0f%%", CLAMP(LY*100,0,100));
    t8(WIN_W-255,WIN_H-42,buf,1.f,1.f,.5f);
    // Progress bar for elevation
    float barW=235.f, fill=CLAMP(LY,0.f,1.f)*barW;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(.3f,.3f,.3f,.8f);
    glBegin(GL_QUADS);
    glVertex2f(WIN_W-255,WIN_H-62); glVertex2f(WIN_W-20,WIN_H-62);
    glVertex2f(WIN_W-20,WIN_H-48); glVertex2f(WIN_W-255,WIN_H-48); glEnd();
    glColor4f(1.f,.8f,.2f,.9f);
    glBegin(GL_QUADS);
    glVertex2f(WIN_W-255,WIN_H-62); glVertex2f(WIN_W-255+fill,WIN_H-62);
    glVertex2f(WIN_W-255+fill,WIN_H-48); glVertex2f(WIN_W-255,WIN_H-48); glEnd();
    glDisable(GL_BLEND);

    t8(WIN_W-255,WIN_H-82,"Shadow alpha = sin(sun_elev)",0.8f,0.7f,0.4f);
    snprintf(buf,256,"Shadow intensity: %.0f%%", CLAMP(LY*.70f,.10f,.62f)*100.f/0.62f*100.f/100.f*100.f);
    t8(WIN_W-255,WIN_H-98,buf,.8f,.7f,.4f);
    t8(WIN_W-255,WIN_H-116,"Low sun = long faint shadow",0.6f,0.6f,0.5f);

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    if (lightMode!=LM_OFF) glEnable(GL_LIGHTING);
}

// =============================================================================
// RESHAPE
// =============================================================================
void reshape(int w,int h){
    WIN_W=w; WIN_H=(h==0)?1:h;
    glViewport(0,0,WIN_W,WIN_H);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(55.,(double)WIN_W/WIN_H,.3,400.);
    glMatrixMode(GL_MODELVIEW);
}

// =============================================================================
// DISPLAY
// Render order:
//   1. Sky (background, no depth write)
//   2. Sun/Moon disc
//   3. ALL SHADOWS on ground (blended, before opaque so they don't clip each other)
//   4. Ground (opaque — receives shadows)
//   5. World objects (opaque)
//   6. Ball
//   7. HUD (2D overlay)
//   8. Buttons (2D clickable)
// =============================================================================
void display()
{
    if (depthMode==DM_OFF) glDisable(GL_DEPTH_TEST);
    else                   glEnable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // First-person camera
    dirX = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
    dirY = sinf(glm::radians(pitch));
    dirZ = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));

    gluLookAt(
        camX, camY, camZ,
        camX + dirX,
        camY + dirY,
        camZ + dirZ,
        0.0f, 1.0f, 0.0f
    );

    setupLights();

    drawSky();
    drawSunDisc();

    // SHADOWS: drawn on ground BEFORE ground so they blend into the grass
    drawAllShadows();

    // OPAQUE WORLD
    drawGround();
    drawFence();
    drawTree();
    drawHouse();
    drawStove();
    drawLampPost();
    drawFlowers();
    drawBall();

    // 2D OVERLAYS
    drawHUD();
    drawButtons();

    glutSwapBuffers();
}

// =============================================================================
// UPDATE TIMER
// =============================================================================
void update(int v)
{
    if (!paused) {
        sceneTime += 0.016f;

        if (sunOrbiting && !nightMode) {
            sunAngle  = fmodf(sunAngle+.25f,360.f);
            sunHeight = sinf(D2R(sunAngle))*.8f+.2f;
            sunHeight = CLAMP(sunHeight,0.f,1.f);
        }

        // Door animation
        if (doorOpening && doorAngle < 90.0f) {
            doorAngle += 4.0f;
            if (doorAngle > 90.0f) doorAngle = 90.0f;
        } else if (!doorOpening && doorAngle > 0.0f) {
            doorAngle -= 4.0f;
            if (doorAngle < 0.0f) doorAngle = 0.0f;
        }

        // WASD camera movement (first-person)
        float moveSpeed = 0.15f;
        if (keys['w']||keys['W']) {
            camX += dirX * moveSpeed;
            camZ += dirZ * moveSpeed;
        }
        if (keys['s']||keys['S']) {
            camX -= dirX * moveSpeed;
            camZ -= dirZ * moveSpeed;
        }
        // Strafe left
        float strafeX = -dirZ;
        float strafeZ = dirX;
        if (keys['a']||keys['A']) {
            camX += strafeX * moveSpeed;
            camZ += strafeZ * moveSpeed;
        }
        // Strafe right
        if (keys['d']||keys['D']) {
            camX -= strafeX * moveSpeed;
            camZ -= strafeZ * moveSpeed;
        }

        // Collision with side walls
        if (camX > 3.0f) camX = 3.0f;
        if (camX < -3.0f) camX = -3.0f;
        // Collision with front wall
        if (camZ < -6.0f) camZ = -6.0f;
        // Collision with back boundary
        if (camZ > 11.0f) camZ = 11.0f;

        float ms=.08f;
        if (keys['w']||keys['W']||sk[0]){ballPos.z-=ms;ballRX-=ms*R2D(1.f/BALL_R);ballVel.z=-.015f;}
        if (keys['s']||keys['S']||sk[1]){ballPos.z+=ms;ballRX+=ms*R2D(1.f/BALL_R);ballVel.z= .015f;}
        if (keys['a']||keys['A']||sk[2]){ballPos.x-=ms;ballRZ+=ms*R2D(1.f/BALL_R);ballVel.x=-.015f;}
        if (keys['d']||keys['D']||sk[3]){ballPos.x+=ms;ballRZ-=ms*R2D(1.f/BALL_R);ballVel.x= .015f;}
        ballPos.x=CLAMP(ballPos.x,-11,11);
        ballPos.z=CLAMP(ballPos.z,-11,11);
        ballVel.y+=GRAVITY;
        ballPos.y+=ballVel.y;
        if (ballPos.y<=GROUND_Y){
            ballPos.y=GROUND_Y;
            ballVel.y=-ballVel.y*BOUNCEDAMP;
            if (fabsf(ballVel.y)<.015f) ballVel.y=0;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16,update,0);
}

// =============================================================================
// KEYBOARD
// =============================================================================
void keyDown(unsigned char k,int x,int y){
    keys[k]=true;
    switch(k){
        case 27: exit(0);
        case ' ': actBounce(); break;
        case 'p':case'P': actPause(); break;
        case 'n':case'N': actNight(); break;
        case 'l':case'L': actLight(); break;
        case 'z':case'Z': actDepth(); break;
        case 's':case'S': actSun();   break;
        case 'r':case'R': actReset(); break;
        case 'e':case'E': {
            // Toggle door
            if (doorAngle < 45.0f) doorOpening = true;
            else doorOpening = false;
            break;
        }
        case 'c':case'C': {
            // Toggle cooking (only works inside)
            bool insideHouse = (camZ < -5.0f) && (camX > -3.0f) && (camX < 3.0f);
            if (insideHouse) {
                isCooking = !isCooking;
                eggCookProgress = 0.0f;
            }
            break;
        }
    }
}
void keyUp(unsigned char k,int x,int y){ keys[k]=false; }
void specDown(int k,int x,int y){
    if(k==GLUT_KEY_UP)    sk[0]=true;
    if(k==GLUT_KEY_DOWN)  sk[1]=true;
    if(k==GLUT_KEY_LEFT)  sk[2]=true;
    if(k==GLUT_KEY_RIGHT) sk[3]=true;
}
void specUp(int k,int x,int y){
    if(k==GLUT_KEY_UP)    sk[0]=false;
    if(k==GLUT_KEY_DOWN)  sk[1]=false;
    if(k==GLUT_KEY_LEFT)  sk[2]=false;
    if(k==GLUT_KEY_RIGHT) sk[3]=false;
}

// =============================================================================
// MOUSE
// =============================================================================
void mouseBtn(int btn,int state,int x,int y)
{
    if (btn==GLUT_LEFT_BUTTON){
        if (state==GLUT_DOWN){
            mDrag=true; lastMX=x; lastMY=y;
            // Check button clicks (convert y: GLUT y=0 is top, our buttons are bottom-origin)
            int sy2=WIN_H-y;
            for (int i=0;i<NUM_BUTTONS;i++){
                Button& b=buttons[i];
                if (x>=b.x && x<=b.x+b.w && sy2>=b.y && sy2<=b.y+b.h){
                    b.action();
                    mDrag=false;  // don't orbit when clicking button
                    break;
                }
            }
        } else mDrag=false;
    }
    if (btn==3){camDist-=.8f; if(camDist<4)camDist=4;}
    if (btn==4){camDist+=.8f; if(camDist>60)camDist=60;}
}
void mouseMove(int x,int y){
    if (mDrag){
        // Mouse controls camera look direction
        float dx = (x - lastMX) * 0.5f;
        float dy = (y - lastMY) * 0.4f;
        
        yaw += dx;
        pitch -= dy;
        pitch = CLAMP(pitch, -89.0f, 89.0f);  // Prevent flipping
    }
    lastMX=x; lastMY=y;
}

// =============================================================================
// MAIN
// =============================================================================
int main(int argc,char** argv)
{
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(WIN_W,WIN_H);
    glutInitWindowPosition(60,40);
    glutCreateWindow("Garden Scene v3 - Shadows, Lighting & Depth (OpenGL)");

    glClearColor(.55f,.78f,.98f,1);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);

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
