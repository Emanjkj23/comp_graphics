/*=============================================================================
 *  GEOMETRY.CPP — Shape Library  (2D & 3D, all with normals for lighting)
 *  -------------------------------------------------------------------------
 *  Legacy OpenGL  |  GLUT only  |  No GLFW / GLAD / GLM
 *
 *  QUICK INDEX (Ctrl+F the tag to jump):
 *    [2D]  drawTriangle2D, drawQuad2D, drawCircle2D, drawStar2D
 *    [3D]  drawTriangle, drawCube, drawCylinder, drawSphere, drawCone,
 *          drawTorus, drawPyramid, drawDisk, drawTeapot, drawPrism
 *   [CMP]  drawTable, drawChair           (composite / furniture)
 *   [UTL]  drawGrid, drawCoordinateAxes   (debug helpers)
 *=============================================================================*/

#include <GL/glut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 *  2D SHAPES — drawn on the XY plane at z = 0     [2D]
 * ═══════════════════════════════════════════════════════════════════════════*/

/*  drawTriangle2D()  –  equilateral, centred at origin, side = 1 unit
 *  Normal faces +Z so lighting still works in 2D view.                    */
void drawTriangle2D() {
    glBegin(GL_TRIANGLES);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex2f( 0.0f,  0.577f);   // top
        glVertex2f(-0.5f, -0.289f);   // bottom-left
        glVertex2f( 0.5f, -0.289f);   // bottom-right
    glEnd();
}

/*  drawQuad2D(w, h)  –  rectangle centred at origin                       */
void drawQuad2D(float w, float h) {
    float hw = w * 0.5f, hh = h * 0.5f;
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex2f(-hw, -hh);
        glVertex2f( hw, -hh);
        glVertex2f( hw,  hh);
        glVertex2f(-hw,  hh);
    glEnd();
}

/*  drawCircle2D(radius, segments)  –  filled circle                       */
void drawCircle2D(float radius, int segments) {
    glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex2f(0.0f, 0.0f);  // centre
        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            glVertex2f(cosf(angle) * radius, sinf(angle) * radius);
        }
    glEnd();
}

/*  drawStar2D(outerR, innerR, points)  –  5-pointed star by default       */
void drawStar2D(float outerR, float innerR, int points) {
    glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex2f(0.0f, 0.0f);
        for (int i = 0; i <= points * 2; i++) {
            float angle = M_PI / 2.0f + i * M_PI / points;
            float r = (i % 2 == 0) ? outerR : innerR;
            glVertex2f(cosf(angle) * r, sinf(angle) * r);
        }
    glEnd();
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  3D SHAPES — centred at origin unless noted     [3D]
 * ═══════════════════════════════════════════════════════════════════════════*/

/*─── drawTriangle()  –  flat triangle in XY plane (with +Z normal)  ─────*/
void drawTriangle() {
    glBegin(GL_TRIANGLES);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f( 0.0f,  1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glVertex3f( 1.0f, -1.0f, 0.0f);
    glEnd();
}

/*─── drawCube(size)  –  axis-aligned, centred at origin  ────────────────
 *  Each face has its own outward-facing normal.                           */
void drawCube(float size) {
    float s = size * 0.5f;

    glBegin(GL_QUADS);
        // Front  (+Z)
        glNormal3f(0, 0, 1);
        glVertex3f(-s, -s,  s);  glVertex3f( s, -s,  s);
        glVertex3f( s,  s,  s);  glVertex3f(-s,  s,  s);
        // Back   (-Z)
        glNormal3f(0, 0, -1);
        glVertex3f( s, -s, -s);  glVertex3f(-s, -s, -s);
        glVertex3f(-s,  s, -s);  glVertex3f( s,  s, -s);
        // Left   (-X)
        glNormal3f(-1, 0, 0);
        glVertex3f(-s, -s, -s);  glVertex3f(-s, -s,  s);
        glVertex3f(-s,  s,  s);  glVertex3f(-s,  s, -s);
        // Right  (+X)
        glNormal3f(1, 0, 0);
        glVertex3f( s, -s,  s);  glVertex3f( s, -s, -s);
        glVertex3f( s,  s, -s);  glVertex3f( s,  s,  s);
        // Top    (+Y)
        glNormal3f(0, 1, 0);
        glVertex3f(-s,  s,  s);  glVertex3f( s,  s,  s);
        glVertex3f( s,  s, -s);  glVertex3f(-s,  s, -s);
        // Bottom (-Y)
        glNormal3f(0, -1, 0);
        glVertex3f(-s, -s, -s);  glVertex3f( s, -s, -s);
        glVertex3f( s, -s,  s);  glVertex3f(-s, -s,  s);
    glEnd();
}

/*─── drawSphere(radius, slices, stacks)  ────────────────────────────────
 *  UV-sphere with per-vertex normals.                                     */
void drawSphere(float radius, int slices, int stacks) {
    for (int i = 0; i < stacks; i++) {
        float lat0 = M_PI * (-0.5f + (float)i / stacks);
        float lat1 = M_PI * (-0.5f + (float)(i + 1) / stacks);
        float y0 = sinf(lat0), yr0 = cosf(lat0);
        float y1 = sinf(lat1), yr1 = cosf(lat1);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float lng = 2.0f * M_PI * (float)j / slices;
            float x = cosf(lng), z = sinf(lng);

            glNormal3f(x * yr0, y0, z * yr0);
            glVertex3f(radius * x * yr0, radius * y0, radius * z * yr0);

            glNormal3f(x * yr1, y1, z * yr1);
            glVertex3f(radius * x * yr1, radius * y1, radius * z * yr1);
        }
        glEnd();
    }
}

/*─── drawCylinder(radius, height, slices)  ──────────────────────────────
 *  Base at y=0, top at y=height. Includes top + bottom caps.              */
void drawCylinder(float radius, float height, int slices) {
    // ── Side surface ──
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * M_PI * i / slices;
        float x = cosf(angle), z = sinf(angle);
        glNormal3f(x, 0.0f, z);
        glVertex3f(radius * x, 0.0f,    radius * z);
        glVertex3f(radius * x, height,  radius * z);
    }
    glEnd();

    // ── Top cap ──
    glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, height, 0.0f);
        for (int i = 0; i <= slices; i++) {
            float angle = 2.0f * M_PI * i / slices;
            glVertex3f(radius * cosf(angle), height, radius * sinf(angle));
        }
    glEnd();

    // ── Bottom cap ──
    glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        for (int i = slices; i >= 0; i--) {
            float angle = 2.0f * M_PI * i / slices;
            glVertex3f(radius * cosf(angle), 0.0f, radius * sinf(angle));
        }
    glEnd();
}

/*─── drawCone(radius, height, slices)  ──────────────────────────────────
 *  Base at y=0, apex at y=height.                                         */
void drawCone(float radius, float height, int slices) {
    float slopeLen = sqrtf(radius * radius + height * height);
    float ny = radius / slopeLen;   // normal's y-component
    float nr = height / slopeLen;   // normal's radial component

    // ── Side ──
    glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 1.0f, 0.0f);  // apex normal (approx)
        glVertex3f(0.0f, height, 0.0f); // apex
        for (int i = 0; i <= slices; i++) {
            float angle = 2.0f * M_PI * i / slices;
            float x = cosf(angle), z = sinf(angle);
            glNormal3f(x * nr, ny, z * nr);
            glVertex3f(radius * x, 0.0f, radius * z);
        }
    glEnd();

    // ── Base cap ──
    glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        for (int i = slices; i >= 0; i--) {
            float angle = 2.0f * M_PI * i / slices;
            glVertex3f(radius * cosf(angle), 0.0f, radius * sinf(angle));
        }
    glEnd();
}

/*─── drawPyramid(base, height)  –  square-base pyramid  ────────────────*/
void drawPyramid(float base, float height) {
    float b = base * 0.5f;
    float ny = base / sqrtf(base * base + 4.0f * height * height); // approx
    float nr = 2.0f * height / sqrtf(base * base + 4.0f * height * height);

    glBegin(GL_TRIANGLES);
        // Front
        glNormal3f(0.0f, nr, ny);
        glVertex3f( 0.0f, height, 0.0f);
        glVertex3f(-b, 0.0f,  b);
        glVertex3f( b, 0.0f,  b);
        // Right
        glNormal3f(nr, ny, 0.0f);
        glVertex3f( 0.0f, height, 0.0f);
        glVertex3f( b, 0.0f,  b);
        glVertex3f( b, 0.0f, -b);
        // Back
        glNormal3f(0.0f, nr, -ny);
        glVertex3f( 0.0f, height, 0.0f);
        glVertex3f( b, 0.0f, -b);
        glVertex3f(-b, 0.0f, -b);
        // Left
        glNormal3f(-nr, ny, 0.0f);
        glVertex3f( 0.0f, height, 0.0f);
        glVertex3f(-b, 0.0f, -b);
        glVertex3f(-b, 0.0f,  b);
    glEnd();

    // Base
    glBegin(GL_QUADS);
        glNormal3f(0, -1, 0);
        glVertex3f(-b, 0, -b); glVertex3f( b, 0, -b);
        glVertex3f( b, 0,  b); glVertex3f(-b, 0,  b);
    glEnd();
}

/*─── drawTorus(innerR, outerR, sides, rings)  ──────────────────────────*/
void drawTorus(float innerR, float outerR, int sides, int rings) {
    for (int i = 0; i < rings; i++) {
        float theta0 = 2.0f * M_PI * i / rings;
        float theta1 = 2.0f * M_PI * (i + 1) / rings;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= sides; j++) {
            float phi = 2.0f * M_PI * j / sides;
            float cosPhi = cosf(phi), sinPhi = sinf(phi);

            for (int k = 0; k < 2; k++) {
                float theta = (k == 0) ? theta0 : theta1;
                float cosT = cosf(theta), sinT = sinf(theta);
                float r = outerR + innerR * cosPhi;

                float x = r * cosT;
                float y = innerR * sinPhi;
                float z = r * sinT;
                // normal points outward from tube centre
                float nx = cosPhi * cosT;
                float ny = sinPhi;
                float nz = cosPhi * sinT;
                glNormal3f(nx, ny, nz);
                glVertex3f(x, y, z);
            }
        }
        glEnd();
    }
}

/*─── drawDisk(innerR, outerR, slices)  –  flat ring on XZ plane  ───────*/
void drawDisk(float innerR, float outerR, int slices) {
    glBegin(GL_QUAD_STRIP);
    glNormal3f(0.0f, 1.0f, 0.0f);
    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * M_PI * i / slices;
        float x = cosf(angle), z = sinf(angle);
        glVertex3f(innerR * x, 0.0f, innerR * z);
        glVertex3f(outerR * x, 0.0f, outerR * z);
    }
    glEnd();
}

/*─── drawPrism(base, height, depth)  –  triangular prism along Z  ──────*/
void drawPrism(float base, float height, float depth) {
    float b = base * 0.5f, d = depth * 0.5f;

    // Front triangle
    glBegin(GL_TRIANGLES);
        glNormal3f(0, 0, 1);
        glVertex3f(-b, 0,  d);
        glVertex3f( b, 0,  d);
        glVertex3f( 0, height, d);
    glEnd();

    // Back triangle
    glBegin(GL_TRIANGLES);
        glNormal3f(0, 0, -1);
        glVertex3f( b, 0, -d);
        glVertex3f(-b, 0, -d);
        glVertex3f( 0, height, -d);
    glEnd();

    // Bottom face
    glBegin(GL_QUADS);
        glNormal3f(0, -1, 0);
        glVertex3f(-b, 0, -d); glVertex3f( b, 0, -d);
        glVertex3f( b, 0,  d); glVertex3f(-b, 0,  d);
    glEnd();

    // Left slope
    float lnx = -height, lny = b;
    float lnLen = sqrtf(lnx*lnx + lny*lny);
    lnx /= lnLen; lny /= lnLen;
    glBegin(GL_QUADS);
        glNormal3f(lnx, lny, 0);
        glVertex3f(-b, 0, -d); glVertex3f(-b, 0,  d);
        glVertex3f( 0, height, d); glVertex3f( 0, height, -d);
    glEnd();

    // Right slope
    glBegin(GL_QUADS);
        glNormal3f(-lnx, lny, 0);
        glVertex3f( b, 0,  d); glVertex3f( b, 0, -d);
        glVertex3f( 0, height, -d); glVertex3f( 0, height, d);
    glEnd();
}

/*─── drawTeapot(size)  –  GLUT's classic Utah teapot  ──────────────────*/
void drawTeapot(float size) {
    glutSolidTeapot(size);          // already includes normals
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  COMPOSITE SHAPES — built from primitives       [CMP]
 * ═══════════════════════════════════════════════════════════════════════════*/

/*─── drawTable(legH, topW, topD, topThick)  ────────────────────────────*/
void drawTable(float legH, float topW, float topD, float topThick) {
    float legR = 0.1f;  // leg radius (thin cylinders)
    float hw = topW * 0.5f - legR;
    float hd = topD * 0.5f - legR;

    // ── Table top ──
    glPushMatrix();
        glTranslatef(0.0f, legH, 0.0f);
        glScalef(topW, topThick, topD);
        drawCube(1.0f);
    glPopMatrix();

    // ── 4 legs (cylinders) ──
    float positions[4][2] = { {-hw, -hd}, {hw, -hd}, {hw, hd}, {-hw, hd} };
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
            glTranslatef(positions[i][0], 0.0f, positions[i][1]);
            drawCylinder(legR, legH, 12);
        glPopMatrix();
    }
}

/*─── drawChair(seatH, seatW, seatD, backH)  ──────────────────────────*/
void drawChair(float seatH, float seatW, float seatD, float backH) {
    float legR = 0.08f;
    float hw = seatW * 0.5f - legR;
    float hd = seatD * 0.5f - legR;

    // ── Seat ──
    glPushMatrix();
        glTranslatef(0.0f, seatH, 0.0f);
        glScalef(seatW, 0.08f, seatD);
        drawCube(1.0f);
    glPopMatrix();

    // ── 4 legs ──
    float positions[4][2] = { {-hw, -hd}, {hw, -hd}, {hw, hd}, {-hw, hd} };
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
            glTranslatef(positions[i][0], 0.0f, positions[i][1]);
            drawCylinder(legR, seatH, 8);
        glPopMatrix();
    }

    // ── Backrest ──
    glPushMatrix();
        glTranslatef(0.0f, seatH + backH * 0.5f, -hd);
        glScalef(seatW, backH, 0.08f);
        drawCube(1.0f);
    glPopMatrix();
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  UTILITY / DEBUG HELPERS                         [UTL]
 * ═══════════════════════════════════════════════════════════════════════════*/

/*─── drawGrid(halfSize, step)  –  flat grid on the XZ plane at y=0  ────
 *  Draws from -halfSize to +halfSize with the given step.                 */
void drawGrid(int halfSize, float step) {
    glDisable(GL_LIGHTING);         // draw grid without lighting
    glColor3f(0.35f, 0.35f, 0.35f); // subtle grey
    glBegin(GL_LINES);
        for (float i = -halfSize; i <= halfSize; i += step) {
            // lines parallel to Z (varying X)
            glVertex3f(i, 0.0f, (float)-halfSize);
            glVertex3f(i, 0.0f, (float) halfSize);
            // lines parallel to X (varying Z)
            glVertex3f((float)-halfSize, 0.0f, i);
            glVertex3f((float) halfSize, 0.0f, i);
        }
    glEnd();
    glEnable(GL_LIGHTING);          // restore lighting
}

/*─── drawCoordinateAxes(length)  –  RGB = XYZ  ─────────────────────────
 *  Red = +X,  Green = +Y,  Blue = +Z                                     */
void drawCoordinateAxes(float length) {
    glDisable(GL_LIGHTING);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
        // X axis – Red
        glColor3f(1, 0, 0);  glVertex3f(0, 0, 0);  glVertex3f(length, 0, 0);
        // Y axis – Green
        glColor3f(0, 1, 0);  glVertex3f(0, 0, 0);  glVertex3f(0, length, 0);
        // Z axis – Blue
        glColor3f(0, 0, 1);  glVertex3f(0, 0, 0);  glVertex3f(0, 0, length);
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}
