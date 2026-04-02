/*
 * ============================================================
 *  3D Solar System — OpenGL + GLUT
 *  Converted from JavaScript/Three.js original
 *
 *  HOW TO COMPILE IN CODE::BLOCKS:
 *  ─────────────────────────────────────────────────────────
 *  1. Install freeglut:
 *       Windows : download freeglut from https://freeglut.sourceforge.net/
 *                 Copy freeglut.dll next to your .exe
 *       Linux   : sudo apt install freeglut3-dev
 *       macOS   : brew install freeglut
 *
 *  2. In Code::Blocks:
 *       Project → Build Options → Linker settings → Add libs:
 *         Windows : freeglut, opengl32, glu32
 *         Linux   : glut, GL, GLU, m
 *         macOS   : "-framework OpenGL" "-framework GLUT"
 *
 *  3. Build & Run (F9)
 *
 * CONTROLS:
 * Mouse drag  - orbit camera
 * Scroll wheel  -zoom in/out
 * 1  - Top view
 * 2  - Slide View
 * 3  - Titled View
 * 4  - Track Earth
 * 5  - Track Moon
 * 6  - Sun close-up
 * +/-  - Speed up /slow down
 * P  - Pause/Resume
 * ESC  - Quit
 * ============================================================
 */

#ifdef _WIN32
  #include <windows.h>
#endif

#include <GL/glut.h>
#include <GL/glu.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <string>

/* ── Constants ───────────────────────────────────────────── */
static const double PI  = 3.14159265358979323846;
static const int    WIN_W = 1200, WIN_H = 700;

/* ── Star field ──────────────────────────────────────────── */
struct Star { float x, y, z; };
static std::vector<Star> g_stars;
static std::vector<Star> g_milky;

/* ── Simulation time ─────────────────────────────────────── */
static double g_t        = 0.0;   // days elapsed
static double g_speed    = 1.0;
static bool   g_paused   = false;

/* ── Camera ──────────────────────────────────────────────── */
static double g_camTheta  = 0.3,  g_camPhi  = 0.48,  g_camDist  = 220.0;
static double g_tgtTheta  = 0.3,  g_tgtPhi  = 0.48,  g_tgtDist  = 220.0;

enum CamMode { CAM_FREE, CAM_TOP, CAM_SIDE, CAM_TILT, CAM_EARTH, CAM_MOON, CAM_SUN };
static CamMode g_camMode = CAM_FREE;

/* ── Mouse drag ──────────────────────────────────────────── */
static bool g_dragging = false;
static int  g_lastMX = 0, g_lastMY = 0;

/* ── Orbital positions (updated each frame) ──────────────── */
static double g_earthX = 0, g_earthY = 0, g_earthZ = 0;
static double g_moonX  = 0, g_moonY  = 0, g_moonZ  = 0;

/* ── Sunspot animation ───────────────────────────────────── */
struct Sunspot {
    float phi, theta;   // position on sun sphere
    float size;
    float baseOpacity;
    float phase;
};
static std::vector<Sunspot> g_spots;

/* ══════════════════════════════════════════════════════════
   HELPERS
   ══════════════════════════════════════════════════════════ */
static float frand() { return (float)rand() / RAND_MAX; }

static void buildStars()
{
    srand(42);
    g_stars.reserve(10000);
    for (int i = 0; i < 10000; i++) {
        float r = 900.f + frand() * 1500.f;
        float t = frand() * (float)(2*PI);
        float p = acosf(2.f * frand() - 1.f);
        g_stars.push_back({ r*sinf(p)*cosf(t), r*sinf(p)*sinf(t), r*cosf(p) });
    }
    g_milky.reserve(4000);
    for (int i = 0; i < 4000; i++) {
        float t = frand() * (float)(2*PI);
        float r = 950.f + frand() * 500.f;
        g_milky.push_back({ r*cosf(t), (frand()-0.5f)*70.f, r*sinf(t) });
    }
}

static void buildSunspots()
{
    srand(7);
    for (int i = 0; i < 8; i++) {
        Sunspot s;
        s.phi         = frand() * (float)PI;
        s.theta       = frand() * (float)(2*PI);
        s.size        = 0.7f + frand() * 1.1f;
        s.baseOpacity = 0.4f + frand() * 0.35f;
        s.phase       = frand() * (float)(2*PI);
        g_spots.push_back(s);
    }
}

/* Draw a sphere at current matrix position */
static GLUquadric* g_quad = nullptr;
static void drawSphere(double r, int slices, int stacks)
{
    gluSphere(g_quad, r, slices, stacks);
}

/* Draw a circle (orbit ring) in XZ plane */
static void drawOrbitRing(double radius, float cr, float cg, float cb, float alpha)
{
    glColor4f(cr, cg, cb, alpha);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i++) {
        double a = (i / 360.0) * 2.0 * PI;
        glVertex3d(cos(a)*radius, 0.0, sin(a)*radius);
    }
    glEnd();
}

/* Simple text rendering at window-space position */
static void drawText2D(int x, int y, const char* s, float r, float g, float b)
{
    glColor3f(r, g, b);
    glRasterPos2i(x, y);
    while (*s) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s++);
}

/* ══════════════════════════════════════════════════════════
   DRAW FUNCTIONS
   ══════════════════════════════════════════════════════════ */

static void drawStars()
{
    glPointSize(1.0f);
    glColor3f(1,1,1);
    glBegin(GL_POINTS);
    for (auto& s : g_stars) glVertex3f(s.x, s.y, s.z);
    glEnd();

    glColor4f(0.6f, 0.6f, 1.0f, 0.45f);
    glBegin(GL_POINTS);
    for (auto& s : g_milky) glVertex3f(s.x, s.y, s.z);
    glEnd();
}

static void drawSun()
{
    /* Core */
    double hue   = 0.12 + 0.01 * sin(g_t * 0.25);
    double light = 0.58 + 0.02 * sin(g_t * 0.6);
    /* Simple HSL→RGB (S=1) */
    double c  = (1.0 - fabs(2.0*light - 1.0));
    double x0 = c * (1.0 - fabs(fmod(hue*6.0, 2.0) - 1.0));
    double m  = light - c*0.5;
    double ri, gi, bi;
    int h6 = (int)(hue * 6);
    if      (h6==0){ ri=c;  gi=x0; bi=0;  }
    else if (h6==1){ ri=x0; gi=c;  bi=0;  }
    else if (h6==2){ ri=0;  gi=c;  bi=x0; }
    else if (h6==3){ ri=0;  gi=x0; bi=c;  }
    else if (h6==4){ ri=x0; gi=0;  bi=c;  }
    else            { ri=c;  gi=0;  bi=x0; }
    glColor3d(ri+m, gi+m, bi+m);

    glPushMatrix();
    glRotated(g_t * 0.04, 0, 1, 0);   // sun rotation
    drawSphere(16, 64, 64);

    /* Sunspots */
    for (auto& sp : g_spots) {
        float op = sp.baseOpacity + 0.15f * sinf((float)g_t * 1.8f + sp.phase);
        glColor4f(1.0f, 0.2f, 0.0f, op);
        double r = 16.3;
        double sx = r * sin(sp.phi) * cos(sp.theta);
        double sy = r * cos(sp.phi);
        double sz = r * sin(sp.phi) * sin(sp.theta);
        glPushMatrix();
        glTranslated(sx, sy, sz);
        drawSphere(sp.size, 6, 6);
        glPopMatrix();
    }
    glPopMatrix();

    /* Glow halos */
    struct Glow { double r; float cr,cg,cb; float op; };
    Glow glows[] = {
        { 17.8, 1.0f, 0.93f, 0.53f, 0.22f },
        { 19.5, 1.0f, 0.60f, 0.0f,  0.09f },
        { 22.0, 1.0f, 0.33f, 0.0f,  0.04f }
    };
    glDepthMask(GL_FALSE);
    for (auto& gl2 : glows) {
        glColor4f(gl2.cr, gl2.cg, gl2.cb, gl2.op);
        glPushMatrix();
        glutSolidSphere(gl2.r, 24, 24);
        glPopMatrix();
    }
    glDepthMask(GL_TRUE);
}

/* Land blob at (lat,lon) on a sphere of radius r */
static void drawLandBlob(double lat, double lon, double blobR, double sphereR,
                          float cr, float cg, float cb)
{
    double phi = (90.0 - lat) * PI / 180.0;
    double th  = lon * PI / 180.0;
    double x   = sphereR * sin(phi) * cos(th);
    double y   = sphereR * cos(phi);
    double z   = sphereR * sin(phi) * sin(th);
    glColor3f(cr, cg, cb);
    glPushMatrix();
    glTranslated(x, y, z);
    drawSphere(blobR, 8, 8);
    glPopMatrix();
}

static void drawEarth()
{
    /* Blue ocean base */
    glColor3f(0.13f, 0.40f, 0.73f);
    drawSphere(5.0, 64, 64);

    /* Continents */
    glPushMatrix();
    drawLandBlob( 50,-100, 1.20, 5.05, 0.13f, 0.53f, 0.20f); // N. America
    drawLandBlob( 15, -65, 0.90, 5.05, 0.13f, 0.67f, 0.27f); // S. America
    drawLandBlob( 52,  15, 1.10, 5.05, 0.20f, 0.60f, 0.27f); // Europe
    drawLandBlob(  5,  22, 1.40, 5.05, 0.20f, 0.73f, 0.33f); // Africa
    drawLandBlob( 48,  90, 1.90, 5.05, 0.13f, 0.67f, 0.27f); // Asia
    drawLandBlob(-25, 134, 0.75, 5.05, 0.27f, 0.80f, 0.40f); // Australia
    glPopMatrix();

    /* Clouds */
    glDepthMask(GL_FALSE);
    glColor4f(1,1,1, 0.17f);
    drawSphere(5.22, 48, 48);
    glDepthMask(GL_TRUE);

    /* Atmosphere glow */
    glDepthMask(GL_FALSE);
    glColor4f(0.27f, 0.53f, 1.0f, 0.07f);
    drawSphere(5.55, 32, 32);
    glColor4f(0.27f, 0.53f, 1.0f, 0.035f);
    drawSphere(5.9,  32, 32);
    glDepthMask(GL_TRUE);
}

static void drawMoon()
{
    glColor3f(0.80f, 0.80f, 0.73f);
    drawSphere(1.4, 32, 32);

    /* Craters */
    struct Cr { double lat, lon, sz; };
    Cr craters[] = {{ 30, 20, 0.18 },{ -20, 60, 0.14 },{ 50,-40, 0.16 },{ -5,-10, 0.12 }};
    for (auto& c : craters) {
        double phi = (90.0-c.lat)*PI/180.0, th = c.lon*PI/180.0, r = 1.42;
        glColor3f(0.67f,0.67f,0.67f);
        glPushMatrix();
        glTranslated(r*sin(phi)*cos(th), r*cos(phi), r*sin(phi)*sin(th));
        drawSphere(c.sz, 6, 6);
        glPopMatrix();
    }
}

/* ══════════════════════════════════════════════════════════
   HUD  (2-D overlay)
   ══════════════════════════════════════════════════════════ */
static void drawHUD()
{
    /* Switch to 2-D orthographic */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    /* Semi-transparent background panel */
    glColor4f(0,0,0,0.45f);
    glBegin(GL_QUADS);
    glVertex2i(8,  WIN_H-8);
    glVertex2i(230,WIN_H-8);
    glVertex2i(230,WIN_H-120);
    glVertex2i(8,  WIN_H-120);
    glEnd();

    /* Text */
    char buf[128];
    drawText2D(16, WIN_H-28,  "SOLAR SYSTEM",       0.47f,0.47f,0.47f);

    int earthDay = (int)g_t % 365;
    int moonDay  = (int)fmod(g_t, 27.3);
    sprintf(buf, "Earth orbit: %3d / 365 days", earthDay);
    drawText2D(16, WIN_H-50,  buf, 0.31f, 0.64f, 0.88f);

    sprintf(buf, "Moon orbit:  %2d / 27 days",  moonDay);
    drawText2D(16, WIN_H-66,  buf, 0.78f,0.78f,0.78f);

    drawText2D(16, WIN_H-90,  "Drag:Orbit  Scroll:Zoom",    0.33f,0.33f,0.33f);

    /* Controls legend */
    drawText2D(16, WIN_H-108, "1-6:Camera  +/-:Speed  P:Pause  ESC:Quit", 0.28f,0.28f,0.28f);

    /* Speed / pause indicator */
    if (g_paused) {
        drawText2D(WIN_W/2-20, 20, "PAUSED", 1,0.8f,0.2f);
    } else {
        sprintf(buf, "Speed: %.1fx", g_speed);
        drawText2D(WIN_W/2-30, 20, buf, 0.6f,0.9f,0.6f);
    }

    /* Camera mode label */
    const char* modeLabel = "";
    switch(g_camMode){
        case CAM_TOP:   modeLabel="[1] Top view";    break;
        case CAM_SIDE:  modeLabel="[2] Side view";   break;
        case CAM_TILT:  modeLabel="[3] Tilted view"; break;
        case CAM_EARTH: modeLabel="[4] Track Earth"; break;
        case CAM_MOON:  modeLabel="[5] Track Moon";  break;
        case CAM_SUN:   modeLabel="[6] Sun close-up";break;
        default:        modeLabel="[free camera]";   break;
    }
    drawText2D(WIN_W-180, WIN_H-28, modeLabel, 0.6f,0.75f,1.0f);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

/* ══════════════════════════════════════════════════════════
   MAIN DISPLAY
   ══════════════════════════════════════════════════════════ */
static void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    /* ── Camera position ── */
    double lookX=0, lookY=0, lookZ=0;

    if (g_camMode == CAM_EARTH) {
        lookX = g_earthX; lookY = g_earthY; lookZ = g_earthZ;
        gluLookAt(
            g_earthX + sin(g_camTheta)*28, g_earthY+10, g_earthZ + cos(g_camTheta)*28,
            lookX, lookY, lookZ,
            0,1,0);
    } else if (g_camMode == CAM_MOON) {
        lookX = g_moonX; lookY = g_moonY; lookZ = g_moonZ;
        gluLookAt(
            g_moonX + sin(g_camTheta)*14, g_moonY+5, g_moonZ + cos(g_camTheta)*14,
            lookX, lookY, lookZ,
            0,1,0);
    } else if (g_camMode == CAM_SUN) {
        gluLookAt(
            sin(g_camTheta)*g_camDist*0.22, g_camDist*0.12, cos(g_camTheta)*g_camDist*0.22,
            0,0,0,  0,1,0);
    } else {
        double cx = g_camDist * sin(g_camPhi) * cos(g_camTheta);
        double cy = g_camDist * cos(g_camPhi);
        double cz = g_camDist * sin(g_camPhi) * sin(g_camTheta);
        gluLookAt(cx,cy,cz, 0,0,0, 0,1,0);
    }

    /* ── Stars ── */
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawStars();

    /* ── Sun ── */
    drawSun();

    /* ── Earth orbit ring ── */
    drawOrbitRing(80, 0.27f,0.47f,0.67f, 0.28f);

    /* ── Earth ── */
    double earthAngle = g_t * (2*PI / 365.0);
    g_earthX = 80.0 * cos(earthAngle);
    g_earthY = 0.0;
    g_earthZ = 80.0 * sin(earthAngle);

    glPushMatrix();
    glTranslated(g_earthX, g_earthY, g_earthZ);

    /* Moon orbit ring (relative to earth) */
    drawOrbitRing(14, 0.47f,0.47f,0.47f, 0.30f);

    /* Earth axial tilt + day rotation */
    glRotated(23.5, 0, 0, 1);
    glRotated(g_t * 360.0 / 1.0, 0, 1, 0);  // self-rotation (1 day per unit)
    drawEarth();
    glPopMatrix();

    /* ── Moon ── */
    double moonAngle = g_t * (2*PI / 27.3);
    g_moonX = g_earthX + 14.0 * cos(moonAngle);
    g_moonY = g_earthY;
    g_moonZ = g_earthZ + 14.0 * sin(moonAngle);

    glPushMatrix();
    glTranslated(g_moonX, g_moonY, g_moonZ);
    glRotated(g_t * 360.0 / 27.3, 0, 1, 0);
    drawMoon();
    glPopMatrix();

    /* ── HUD ── */
    drawHUD();

    glutSwapBuffers();
}

/* ══════════════════════════════════════════════════════════
   TIMER / IDLE
   ══════════════════════════════════════════════════════════ */
static void timer(int)
{
    if (!g_paused) {
        g_t += 0.016 * g_speed;
    }

    /* Smooth lerp camera */
    g_camTheta += (g_tgtTheta - g_camTheta) * 0.06;
    g_camPhi   += (g_tgtPhi   - g_camPhi)   * 0.06;
    g_camDist  += (g_tgtDist  - g_camDist)  * 0.06;

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // ~60 fps
}

/* ══════════════════════════════════════════════════════════
   INPUT
   ══════════════════════════════════════════════════════════ */
static void setMode(CamMode m)
{
    g_camMode = m;
    switch(m){
        case CAM_TOP:   g_tgtPhi=0.04;              g_tgtDist=260; break;
        case CAM_SIDE:  g_tgtPhi=PI/2-0.02;         g_tgtDist=260; break;
        case CAM_TILT:  g_tgtPhi=0.45;              g_tgtDist=230; break;
        case CAM_SUN:   g_tgtDist=50; g_tgtPhi=0.4; break;
        case CAM_EARTH: g_tgtDist=70;                break;
        case CAM_MOON:  g_tgtDist=25;                break;
        default: break;
    }
}

static void keyboard(unsigned char key, int, int)
{
    switch(key){
        case '1': setMode(CAM_TOP);   break;
        case '2': setMode(CAM_SIDE);  break;
        case '3': setMode(CAM_TILT);  break;
        case '4': setMode(CAM_EARTH); break;
        case '5': setMode(CAM_MOON);  break;
        case '6': setMode(CAM_SUN);   break;
        case '+': case '=': g_speed = (g_speed*1.6 < 10.0) ? g_speed*1.6 : 10.0; break;
        case '-': case '_': g_speed = (g_speed*0.6 > 0.2)  ? g_speed*0.6 : 0.2;  break;
        case 'p': case 'P': g_paused = !g_paused; break;
        case 27:  exit(0); break;
        default: break;
    }
}

static void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        g_dragging = (state == GLUT_DOWN);
        g_lastMX = x; g_lastMY = y;
        if (state == GLUT_DOWN) g_camMode = CAM_FREE;
    }
    /* Scroll wheel zoom */
    if (button == 3) { g_tgtDist = (g_tgtDist-8 < 30)  ? 30  : g_tgtDist-8; g_camMode=CAM_FREE; }
    if (button == 4) { g_tgtDist = (g_tgtDist+8 > 500) ? 500 : g_tgtDist+8; g_camMode=CAM_FREE; }
}

static void mouseMotion(int x, int y)
{
    if (!g_dragging) return;
    g_tgtTheta -= (x - g_lastMX) * 0.007;
    double newPhi = g_tgtPhi + (y - g_lastMY) * 0.006;
    g_tgtPhi = (newPhi < 0.03) ? 0.03 : (newPhi > 1.5) ? 1.5 : newPhi;
    g_lastMX = x; g_lastMY = y;
}

static void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w/h, 0.1, 6000.0);
    glMatrixMode(GL_MODELVIEW);
}

/* ══════════════════════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════════════════════ */
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("3D Solar System — OpenGL");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.01f, 1.0f);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    g_quad = gluNewQuadric();
    gluQuadricNormals(g_quad, GLU_SMOOTH);

    buildStars();
    buildSunspots();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();

    gluDeleteQuadric(g_quad);
    return 0;
}