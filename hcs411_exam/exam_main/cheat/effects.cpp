/*=============================================================================
 *  EFFECTS.CPP — Lighting, Materials, Shadows, Shading & Blending
 *  -------------------------------------------------------------------------
 *  Legacy OpenGL  |  GLUT only  |  No GLFW / GLAD / GLM
 *
 *  QUICK INDEX (Ctrl+F the tag to jump):
 *    [LGT]  setupBasicLighting, toggleLightingMode, setLightPosition
 *    [MAT]  setMaterial, preset materials (gold, silver, rubber, etc.)
 *    [SHD]  setupPlanarShadow, toggleShadeModel (flat ↔ smooth)
 *    [BLD]  enableAlphaBlending, disableAlphaBlending, setAlpha
 *    [FOG]  enableFog, disableFog
 *    [CLR]  color helper macros
 *=============================================================================*/

#include <GL/glut.h>
#include <cstdio>

/* ═══════════════════════════════════════════════════════════════════════════
 *  LIGHTING                                       [LGT]
 *
 *  OpenGL fixed-pipeline lighting model:
 *    Final colour = Ambient + Diffuse + Specular + Emissive
 *
 *  We set up GL_LIGHT0 and let you toggle between two presets:
 *    Mode 0 — "AMBIENT-dominant"  (soft, no shiny highlights)
 *    Mode 1 — "SPECULAR-dominant" (strong highlight, shinier)
 * ═══════════════════════════════════════════════════════════════════════════*/

static int lightingMode = 0;  // 0 = ambient, 1 = specular

/*─── setupBasicLighting()  –  call once in init()  ─────────────────────*/
void setupBasicLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);   // let glColor affect material ambient+diffuse

    // Default light position  (positional light, w=1)
    GLfloat lightPos[]  = { 5.0f, 10.0f, 7.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // Start in ambient mode
    GLfloat amb[]  = { 0.6f, 0.6f, 0.6f, 1.0f };
    GLfloat diff[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat spec[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

    // Global ambient light (scene-wide)
    GLfloat globalAmb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

    printf("[effects] Lighting mode: AMBIENT\n");
}

/*─── toggleLightingMode()  –  press 'L' to flip between modes  ─────────*/
void toggleLightingMode() {
    lightingMode = 1 - lightingMode;   // flip 0 ↔ 1

    if (lightingMode == 0) {
        // ── AMBIENT-dominant ──
        GLfloat amb[]  = { 0.6f, 0.6f, 0.6f, 1.0f };
        GLfloat diff[] = { 0.5f, 0.5f, 0.5f, 1.0f };
        GLfloat spec[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

        // Reduce material shininess
        GLfloat matSpec[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
        glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);

        printf("[effects] Lighting mode: AMBIENT (soft, no highlights)\n");
    } else {
        // ── SPECULAR-dominant ──
        GLfloat amb[]  = { 0.1f, 0.1f, 0.1f, 1.0f };
        GLfloat diff[] = { 0.7f, 0.7f, 0.7f, 1.0f };
        GLfloat spec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

        // Increase material shininess
        GLfloat matSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
        glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);

        printf("[effects] Lighting mode: SPECULAR (shiny highlights)\n");
    }
}

/*─── setLightPosition(x, y, z)  –  call in display() to move light  ───*/
void setLightPosition(float x, float y, float z) {
    GLfloat pos[] = { x, y, z, 1.0f };  // w=1 → positional light
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
}

/*─── setDirectionalLight(dx, dy, dz)  –  infinite/directional light  ──*/
void setDirectionalLight(float dx, float dy, float dz) {
    GLfloat dir[] = { dx, dy, dz, 0.0f };  // w=0 → directional
    glLightfv(GL_LIGHT0, GL_POSITION, dir);
}

/*─── enableSecondLight()  –  GL_LIGHT1 as a fill/coloured light  ──────*/
void enableSecondLight(float x, float y, float z,
                       float r, float g, float b) {
    glEnable(GL_LIGHT1);
    GLfloat pos[]  = { x, y, z, 1.0f };
    GLfloat col[]  = { r, g, b, 1.0f };
    GLfloat amb[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, pos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  col);
    glLightfv(GL_LIGHT1, GL_SPECULAR, col);
    glLightfv(GL_LIGHT1, GL_AMBIENT,  amb);
}

/*─── disableSecondLight()  ─────────────────────────────────────────────*/
void disableSecondLight() {
    glDisable(GL_LIGHT1);
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  MATERIALS                                      [MAT]
 *
 *  Use these before drawing an object to change how it responds to light.
 *  Parameters: ambient, diffuse, specular, shininess.
 * ═══════════════════════════════════════════════════════════════════════════*/

/*─── setMaterial()  –  generic material setter  ────────────────────────*/
void setMaterial(float ar, float ag, float ab,    // ambient
                 float dr, float dg, float db,    // diffuse
                 float sr, float sg, float sb,    // specular
                 float shininess) {
    GLfloat amb[]  = { ar, ag, ab, 1.0f };
    GLfloat diff[] = { dr, dg, db, 1.0f };
    GLfloat spec[] = { sr, sg, sb, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   diff);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  spec);
    glMaterialf (GL_FRONT, GL_SHININESS, shininess);
}

/*─── Preset materials (call before drawing)  ──────────────────────────*/
void setMaterialGold() {
    setMaterial(0.24725f, 0.1995f, 0.0745f,
                0.75164f, 0.60648f, 0.22648f,
                0.628281f, 0.555802f, 0.366065f,
                51.2f);
}

void setMaterialSilver() {
    setMaterial(0.19225f, 0.19225f, 0.19225f,
                0.50754f, 0.50754f, 0.50754f,
                0.508273f, 0.508273f, 0.508273f,
                51.2f);
}

void setMaterialRubber(float r, float g, float b) {
    setMaterial(0.05f * r, 0.05f * g, 0.05f * b,
                0.5f * r,  0.5f * g,  0.5f * b,
                0.7f, 0.7f, 0.7f,
                10.0f);
}

void setMaterialChrome() {
    setMaterial(0.25f, 0.25f, 0.25f,
                0.4f, 0.4f, 0.4f,
                0.774597f, 0.774597f, 0.774597f,
                76.8f);
}

void setMaterialEmerald() {
    setMaterial(0.0215f, 0.1745f, 0.0215f,
                0.07568f, 0.61424f, 0.07568f,
                0.633f, 0.727811f, 0.633f,
                76.8f);
}

void setMaterialDefault() {
    setMaterial(0.2f, 0.2f, 0.2f,
                0.8f, 0.8f, 0.8f,
                0.0f, 0.0f, 0.0f,
                0.0f);
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  SHADING & SHADOWS                              [SHD]
 * ═══════════════════════════════════════════════════════════════════════════*/

static int shadeSmooth = 1;  // 1 = smooth (Gouraud), 0 = flat

/*─── toggleShadeModel()  –  switch between GL_SMOOTH and GL_FLAT  ──────*/
void toggleShadeModel() {
    shadeSmooth = 1 - shadeSmooth;
    if (shadeSmooth) {
        glShadeModel(GL_SMOOTH);
        printf("[effects] Shade model: SMOOTH (Gouraud)\n");
    } else {
        glShadeModel(GL_FLAT);
        printf("[effects] Shade model: FLAT\n");
    }
}

/*─── setupPlanarShadow()  –  simple ground-plane shadow using projection
 *
 *  USAGE:
 *    1) Draw your lit scene normally.
 *    2) Call beginPlanarShadow(lightX, lightY, lightZ).
 *    3) Re-draw the objects you want to cast shadows.
 *    4) Call endPlanarShadow().
 *
 *  The shadow is projected onto the y=0 plane (the ground).
 * ──────────────────────────────────────────────────────────────────────────*/

void beginPlanarShadow(float lx, float ly, float lz) {
    // Shadow projection matrix (project onto y = 0 plane)
    GLfloat shadowMat[16];
    // ground plane: y = 0 → normal = (0,1,0), d = 0
    float dot = ly;  // dot(lightPos, planeNormal) where plane = (0,1,0,0)

    shadowMat[ 0] = dot - lx * 0;  shadowMat[ 4] = 0 - lx * 1;
    shadowMat[ 8] = 0 - lx * 0;    shadowMat[12] = 0 - lx * 0;

    shadowMat[ 1] = 0 - ly * 0;    shadowMat[ 5] = dot - ly * 1;
    shadowMat[ 9] = 0 - ly * 0;    shadowMat[13] = 0 - ly * 0;

    shadowMat[ 2] = 0 - lz * 0;    shadowMat[ 6] = 0 - lz * 1;
    shadowMat[10] = dot - lz * 0;   shadowMat[14] = 0 - lz * 0;

    shadowMat[ 3] = 0 - 1 * 0;     shadowMat[ 7] = 0 - 1 * 1;
    shadowMat[11] = 0 - 1 * 0;     shadowMat[15] = dot - 1 * 0;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);  // so shadow draws on top of floor
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.1f, 0.1f, 0.1f, 0.5f);  // dark, semi-transparent

    glPushMatrix();
        glMultMatrixf(shadowMat);
        // Lift slightly to avoid z-fighting
        glTranslatef(0.0f, 0.01f, 0.0f);
}

void endPlanarShadow() {
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

/*─── setupStencilShadow()  –  stencil-based shadow (prevents double-blend)
 *  Same idea but uses stencil buffer for cleaner results.
 *  Requires GLUT_STENCIL in glutInitDisplayMode.                          */
void beginStencilShadow(float lx, float ly, float lz) {
    // Enable stencil to mark shadow pixels
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    beginPlanarShadow(lx, ly, lz);
}

void endStencilShadow() {
    endPlanarShadow();
    glDisable(GL_STENCIL_TEST);
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  ALPHA BLENDING & TRANSPARENCY                  [BLD]
 * ═══════════════════════════════════════════════════════════════════════════*/

/*─── enableAlphaBlending()  –  standard transparency  ──────────────────*/
void enableAlphaBlending() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);  // don't write depth for transparent objects
    printf("[effects] Alpha blending: ON\n");
}

/*─── disableAlphaBlending()  ───────────────────────────────────────────*/
void disableAlphaBlending() {
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    printf("[effects] Alpha blending: OFF\n");
}

/*─── setTransparency(alpha)  –  set current colour alpha (0=invisible) ─*/
void setTransparency(float alpha) {
    // Works with GL_COLOR_MATERIAL enabled
    GLfloat current[4];
    glGetFloatv(GL_CURRENT_COLOR, current);
    glColor4f(current[0], current[1], current[2], alpha);
}

/*─── drawTransparentObject example  ───────────────────────────────────-
 *  USAGE PATTERN:
 *    1) Draw ALL opaque objects first.
 *    2) Sort transparent objects back-to-front (furthest first).
 *    3) enableAlphaBlending();
 *    4) for each transparent object:
 *         glColor4f(r, g, b, alpha);
 *         drawCube(1.0f);
 *    5) disableAlphaBlending();                                           */


/* ═══════════════════════════════════════════════════════════════════════════
 *  FOG                                            [FOG]
 * ═══════════════════════════════════════════════════════════════════════════*/

void enableFog(float r, float g, float b, float density) {
    glEnable(GL_FOG);
    GLfloat fogColor[] = { r, g, b, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_EXP2);         // GL_LINEAR, GL_EXP, GL_EXP2
    glFogf(GL_FOG_DENSITY, density);
    glHint(GL_FOG_HINT, GL_NICEST);
    printf("[effects] Fog: ON (density=%.2f)\n", density);
}

void enableLinearFog(float r, float g, float b, float start, float end) {
    glEnable(GL_FOG);
    GLfloat fogColor[] = { r, g, b, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, start);
    glFogf(GL_FOG_END, end);
    glHint(GL_FOG_HINT, GL_NICEST);
}

void disableFog() {
    glDisable(GL_FOG);
    printf("[effects] Fog: OFF\n");
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  COLOUR HELPERS                                 [CLR]
 *
 *  These are convenience wrappers. With GL_COLOR_MATERIAL enabled,
 *  glColor3f sets both GL_AMBIENT and GL_DIFFUSE material properties.
 * ═══════════════════════════════════════════════════════════════════════════*/

void setColorRed()     { glColor3f(0.9f, 0.2f, 0.2f); }
void setColorGreen()   { glColor3f(0.2f, 0.9f, 0.2f); }
void setColorBlue()    { glColor3f(0.2f, 0.2f, 0.9f); }
void setColorYellow()  { glColor3f(0.9f, 0.9f, 0.2f); }
void setColorWhite()   { glColor3f(1.0f, 1.0f, 1.0f); }
void setColorGrey()    { glColor3f(0.5f, 0.5f, 0.5f); }
void setColorOrange()  { glColor3f(1.0f, 0.6f, 0.1f); }
void setColorPurple()  { glColor3f(0.6f, 0.1f, 0.9f); }
void setColorCyan()    { glColor3f(0.1f, 0.9f, 0.9f); }
void setColorBrown()   { glColor3f(0.55f, 0.27f, 0.07f); }
void setColorPink()    { glColor3f(1.0f, 0.4f, 0.7f); }
