// lighting.cpp
// phong lighting and shadow matrix setup
// week 5: shadow math   week 7: light sources and material properties

#include "lighting.h"

// shadow matrix - flattens any point onto y=0 based on where the light is
// derivation from week 5 notes: P' = P - (Py / Ly) * L
// each column of the matrix handles one component of that formula
void shadowMat(float lx, float ly, float lz, float m[16])
{
    if (ly < 0.01f) ly = 0.01f;  // clamp so we don't divide by near-zero when sun is on horizon

    m[0]  =  ly;  m[1]  = 0;   m[2]  = 0;   m[3]  = 0;
    m[4]  = -lx;  m[5]  = 0;   m[6]  = -lz; m[7]  = 0;
    m[8]  =  0;   m[9]  = 0;   m[10] = ly;  m[11] = 0;
    m[12] =  0;   m[13] = 0;   m[14] = 0;   m[15] = ly;
}

// sets opengl material for phong shading
// ambient is kept at roughly 28% of diffuse - looked good after some testing
void mat(float r, float g, float b,
         float sr, float sg, float sb,
         float sh, float a)
{
    GLfloat amb[] = { r*.28f, g*.28f, b*.28f, a };
    GLfloat dif[] = { r, g, b, a };
    GLfloat spc[] = { sr, sg, sb, a };
    GLfloat sh2[] = { sh };

    // strip specular out when in simplified modes
    if (lightMode == LM_NOSPEC || lightMode == LM_AMBONLY) {
        spc[0] = spc[1] = spc[2] = 0;
        sh2[0] = 0;
    }

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  spc);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, sh2);
}

// only calls mat() when not doing the shadow pass
// saves redundant GL state changes since shadows are flat colour anyway
void safemat(float r, float g, float b,
             float sr, float sg, float sb,
             float sh, float a)
{
    if (!inShadowPass) mat(r, g, b, sr, sg, sb, sh, a);
}

// sets up all scene lights
// GL_LIGHT0 = sun (directional, w=0 so no position attenuation)
// GL_LIGHT1 = lamp post at entrance (point, w=1, with attenuation)
// GL_LIGHT2/3/4 = office window glow lights (visible at night)
void setupLights()
{
    glEnable(GL_NORMALIZE);  // keeps normals correct after scaling objects

    if (lightMode == LM_OFF) {
        glDisable(GL_LIGHTING);
        return;
    }
    glEnable(GL_LIGHTING);

    // convert sun angle + height into a direction vector
    float sx = cosf(D2R(sunAngle)) * cosf(D2R(sunHeight * 90.f));
    float sy = sinf(D2R(sunHeight * 90.f));
    float sz = sinf(D2R(sunAngle)) * cosf(D2R(sunHeight * 90.f));
    LX = sx; LY = sy; LZ = sz;

    GLfloat sunPos[] = { sx, sy, sz, 0.f };  // w=0 makes this directional

    // colour the sun: warm orange near the horizon, whiter at noon
    float dayF = CLAMP(sy, 0.f, 1.f);
    float rs, gs, bs;
    if (!nightMode) {
        float srb = CLAMP(1.f - dayF * 2.5f, 0.f, 1.f);
        rs = 0.95f; gs = 0.85f - srb * .35f; bs = 0.80f - srb * .6f;
        float iv = 0.25f + dayF * .75f;
        rs *= iv; gs *= iv; bs *= iv;
    } else {
        rs = 0.08f; gs = 0.09f; bs = 0.22f;
    }

    GLfloat sD[] = { rs, gs, bs, 1 };
    GLfloat sS[] = { rs*.6f, gs*.6f, bs*.6f, 1 };
    GLfloat sA[] = {
        nightMode ? .05f : rs*.38f,
        nightMode ? .05f : gs*.38f,
        nightMode ? .10f : bs*.42f, 1
    };
    if (lightMode == LM_AMBONLY) { sD[0]=sD[1]=sD[2]=0; sS[0]=sS[1]=sS[2]=0; }

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  sD);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sS);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  sA);

    // car park lamp post - warm yellow point light above the entrance
    GLfloat lpPos[] = { 0.f, 6.f, 14.f, 1.f };  // w=1 = positional
    float lR = nightMode?1.f:.2f, lG = nightMode?.9f:.18f, lB = nightMode?.5f:.09f;
    GLfloat lD[]  = { lR, lG, lB, 1 };
    GLfloat lA[]  = { 0, 0, 0, 1 };
    GLfloat lSp[] = { lR*.4f, lG*.4f, lB*.2f, 1 };
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, lpPos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  lD);
    glLightfv(GL_LIGHT1, GL_AMBIENT,  lA);
    glLightfv(GL_LIGHT1, GL_SPECULAR, lSp);
    glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  .5f);
    glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    .08f);
    glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, .015f);

    // building window glow - three rows of windows, only really noticeable at night
    float wR = nightMode?.95f:.01f, wG = nightMode?.85f:.01f, wB = nightMode?.40f:.01f;
    GLfloat wA[]  = { 0, 0, 0, 1 };
    GLfloat wSp[] = { wR*.3f, wG*.3f, wB*.1f, 1 };
    GLenum  wLights[]  = { GL_LIGHT2, GL_LIGHT3, GL_LIGHT4 };
    float   wHeights[] = { 2.f, 5.f, 8.f };

    for (int fl = 0; fl < 3; fl++) {
        GLfloat wPos[] = { 0.f, wHeights[fl], -12.5f, 1.f };
        GLfloat wD[]   = { wR, wG, wB, 1 };
        glEnable(wLights[fl]);
        glLightfv(wLights[fl], GL_POSITION, wPos);
        glLightfv(wLights[fl], GL_DIFFUSE,  wD);
        glLightfv(wLights[fl], GL_AMBIENT,  wA);
        glLightfv(wLights[fl], GL_SPECULAR, wSp);
        glLightf (wLights[fl], GL_CONSTANT_ATTENUATION,  1.0f);
        glLightf (wLights[fl], GL_LINEAR_ATTENUATION,    0.10f);
        glLightf (wLights[fl], GL_QUADRATIC_ATTENUATION, 0.02f);
    }
}
