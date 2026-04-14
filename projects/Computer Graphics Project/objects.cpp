// objects.cpp
// every 3D object in the scene has its own draw function here
// approach from weeks 3-4: build complex shapes from simple primitives
// (cubes, cylinders, spheres, torus) with glPushMatrix/glPopMatrix per sub-part

#include "objects.h"
#include "lighting.h"

// ---------------------------------------------------------
// CAR
// built from scaled cubes for body/cabin, torus+sphere for wheels,
// small spheres for lights, cylinders for exhaust
// ---------------------------------------------------------
void drawCarGeom(float cr, float cg, float cb, float wheelRot)
{
    // lower chassis block
    safemat(cr, cg, cb, .7f, .7f, .7f, 55.f);
    glPushMatrix(); glScalef(2.2f, .65f, 4.6f); glutSolidCube(1); glPopMatrix();

    // cabin on top - slightly darker shade of the car colour
    safemat(cr*.85f, cg*.85f, cb*.85f, .6f, .6f, .6f, 45.f);
    glPushMatrix(); glTranslatef(0, .62f, -.2f); glScalef(1.85f, .72f, 2.5f); glutSolidCube(1); glPopMatrix();

    // windshield glass - semi-transparent so needed to disable depth write
    safemat(.25f, .45f, .65f, .95f, .95f, 1.f, 120.f, .55f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glPushMatrix(); glTranslatef(0, .65f, -1.52f); glRotatef(18, 1,0,0); glScalef(1.75f, .58f, .08f); glutSolidCube(1); glPopMatrix();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);

    // headlights - bright near-white
    safemat(1.f, 1.f, .85f, 1.f, 1.f, .8f, 120.f);
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix(); glTranslatef(s*.78f, .18f, -2.35f); glScalef(.38f, .22f, .10f); glutSolidSphere(1, 8, 6); glPopMatrix();
    }

    // tail lights (red)
    safemat(.9f, .05f, .05f, .8f, .3f, .3f, 60.f);
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix(); glTranslatef(s*.78f, .18f, 2.35f); glutSolidSphere(.18f, 8, 6); glPopMatrix();
    }

    // front and rear bumpers
    safemat(.55f, .55f, .58f, .4f, .4f, .4f, 40.f);
    glPushMatrix(); glTranslatef(0, -.09f, -2.38f); glScalef(2.1f, .32f, .18f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(0, -.09f,  2.38f); glScalef(2.1f, .32f, .18f); glutSolidCube(1); glPopMatrix();

    // four wheels - tyre is a torus, hub cap is a small sphere
    // wheel y offset is -0.02 because car body is translated up by 0.38 in drawCar
    float wp[4][2] = { {-1.08f,-1.5f}, {1.08f,-1.5f}, {-1.08f,1.5f}, {1.08f,1.5f} };
    for (int w = 0; w < 4; w++) {
        glPushMatrix();
        glTranslatef(wp[w][0], -.02f, wp[w][1]);
        glRotatef(wheelRot, 1, 0, 0);   // spin based on distance travelled
        safemat(.12f, .12f, .12f, .08f, .08f, .08f, 5.f);
        glRotatef(90, 0, 1, 0); glutSolidTorus(.20f, .36f, 10, 16);
        safemat(.78f, .78f, .82f, .9f, .9f, .9f, 90.f);
        glScalef(.22f, .22f, .09f); glutSolidSphere(1, 10, 8);
        glPopMatrix();
    }

    // exhaust pipe
    safemat(.32f, .32f, .32f, .3f, .3f, .3f, 15.f);
    glPushMatrix(); glTranslatef(-.7f, -.27f, 2.36f); glRotatef(90, 1,0,0);
    glutSolidCylinder(.06f, .38f, 8, 2); glPopMatrix();
}

void drawCar(int idx)
{
    Car& c = cars[idx];
    glPushMatrix();
    glTranslatef(c.x, .38f, c.z);
    glRotatef(-c.heading, 0, 1, 0);

    // selection ring under the active car - red when blocked by a collision
    if (idx == selectedCar && !inShadowPass) {
        glDisable(GL_LIGHTING);
        if (collisionBlocked) glColor3f(1.f, .15f, .15f);
        else                  glColor3f(1.f, 1.f,  .0f);
        glPushMatrix(); glTranslatef(0, -.36f, 0); glRotatef(90, 1,0,0);
        glutWireTorus(.05f, 1.3f, 8, 20); glPopMatrix();
        if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
    }

    drawCarGeom(c.r, c.g, c.b, c.wheelRot);
    glPopMatrix();
}

// ---------------------------------------------------------
// TREE
// hierarchical: root flare -> trunk (2 sections) -> branch stubs -> 5 foliage layers
// foliage goes dark shadow-green at base to bright lime at crown
// ---------------------------------------------------------
void drawTree(float x, float z, float h)
{
    glPushMatrix(); glTranslatef(x, 0, z);

    // root flare - 4 ellipsoids radiating outward from the base
    mat(.32f, .20f, .08f, .06f, .04f, .02f, 5.f);
    for (int r = 0; r < 4; r++) {
        glPushMatrix();
        glRotatef(r * 90.f, 0, 1, 0);
        glTranslatef(.38f, .05f, 0);
        glScalef(.55f, .12f, .22f); glutSolidSphere(1, 8, 4);
        glPopMatrix();
    }

    // trunk with bark texture - split into lower (wider) and upper (narrower) section
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID[4]);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    mat(.42f, .27f, .11f, .12f, .08f, .04f, 10.f);
    glPushMatrix(); glRotatef(-90, 1,0,0); glutSolidCylinder(.30f, h*.45f, 14, 6); glPopMatrix();
    glPushMatrix(); glTranslatef(0, h*.45f, 0); glRotatef(-90, 1,0,0);
    glutSolidCylinder(.22f, h*.28f, 12, 4); glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    // three branch stubs sprouting sideways near the canopy
    mat(.38f, .24f, .10f, .08f, .06f, .03f, 6.f);
    for (int b = 0; b < 3; b++) {
        glPushMatrix();
        glRotatef(b*120.f + 15.f, 0, 1, 0);
        glTranslatef(0, h*.58f, 0);
        glRotatef(55.f, 0, 0, 1);
        glutSolidCylinder(.09f, h*.20f, 8, 2);
        glPopMatrix();
    }

    // 5 foliage clusters - colours get progressively lighter going upward
    mat(.13f, .46f, .09f, .04f, .12f, .02f, 8.f);
    glPushMatrix(); glTranslatef( .15f, h*.58f,  .10f); glScalef(1.35f,.85f,1.35f); glutSolidSphere(h*.29f,16,11); glPopMatrix();
    mat(.18f, .56f, .11f, .06f, .16f, .03f, 10.f);
    glPushMatrix(); glTranslatef(-.18f, h*.70f, -.08f); glScalef(1.15f,.95f,1.10f); glutSolidSphere(h*.24f,14,10); glPopMatrix();
    mat(.22f, .64f, .14f, .08f, .20f, .04f, 12.f);
    glPushMatrix(); glTranslatef( .08f, h*.80f,  .15f); glScalef(1.05f,1.00f,1.05f); glutSolidSphere(h*.20f,13,9); glPopMatrix();
    mat(.30f, .70f, .16f, .10f, .26f, .06f, 16.f);
    glPushMatrix(); glTranslatef(-.06f, h*.90f,  .05f); glScalef(.95f,1.00f,.90f);  glutSolidSphere(h*.16f,12,8); glPopMatrix();
    mat(.38f, .78f, .20f, .14f, .32f, .08f, 22.f);
    glPushMatrix(); glTranslatef( .02f, h*1.02f, .02f); glutSolidSphere(h*.10f,10,7); glPopMatrix();

    glPopMatrix();
}

// ---------------------------------------------------------
// OFFICE BUILDING
// concrete body drawn face-by-face so texture coordinates work correctly
// (glutSolidCube doesn't emit tex coords which is why we had to do it manually)
// glass windows use alpha blending - had to turn off depth write to avoid
// transparent faces blocking objects behind them
// ---------------------------------------------------------
void drawOffice()
{
    glPushMatrix(); glTranslatef(0, 0, -16.f);

    // concrete body - each face drawn as a textured quad with correct normals
    mat(.72f, .72f, .70f, .3f, .3f, .28f, 18.f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID[1]);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glPushMatrix(); glTranslatef(0, 5.5f, 0);
    glBegin(GL_QUADS); glNormal3f(0,0,1);
    glTexCoord2f(0,0); glVertex3f(-9,-5.5f, 3);
    glTexCoord2f(6,0); glVertex3f( 9,-5.5f, 3);
    glTexCoord2f(6,4); glVertex3f( 9, 5.5f, 3);
    glTexCoord2f(0,4); glVertex3f(-9, 5.5f, 3); glEnd();
    glBegin(GL_QUADS); glNormal3f(0,0,-1);
    glTexCoord2f(0,0); glVertex3f( 9,-5.5f,-3);
    glTexCoord2f(6,0); glVertex3f(-9,-5.5f,-3);
    glTexCoord2f(6,4); glVertex3f(-9, 5.5f,-3);
    glTexCoord2f(0,4); glVertex3f( 9, 5.5f,-3); glEnd();
    glBegin(GL_QUADS); glNormal3f(-1,0,0);
    glTexCoord2f(0,0); glVertex3f(-9,-5.5f,-3);
    glTexCoord2f(2,0); glVertex3f(-9,-5.5f, 3);
    glTexCoord2f(2,4); glVertex3f(-9, 5.5f, 3);
    glTexCoord2f(0,4); glVertex3f(-9, 5.5f,-3); glEnd();
    glBegin(GL_QUADS); glNormal3f(1,0,0);
    glTexCoord2f(0,0); glVertex3f( 9,-5.5f, 3);
    glTexCoord2f(2,0); glVertex3f( 9,-5.5f,-3);
    glTexCoord2f(2,4); glVertex3f( 9, 5.5f,-3);
    glTexCoord2f(0,4); glVertex3f( 9, 5.5f, 3); glEnd();
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-9, 5.5f,-3);
    glTexCoord2f(6,0); glVertex3f( 9, 5.5f,-3);
    glTexCoord2f(6,2); glVertex3f( 9, 5.5f, 3);
    glTexCoord2f(0,2); glVertex3f(-9, 5.5f, 3); glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    // roof parapet
    mat(.58f, .58f, .56f, .25f, .25f, .22f, 12.f);
    glPushMatrix(); glTranslatef(0, 11.3f, 0); glScalef(18.2f, .6f, 6.2f); glutSolidCube(1); glPopMatrix();

    // glass window panels - blended
    mat(.25f, .45f, .65f, .9f, .9f, 1.f, 120.f, .7f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    for (int floor = 0; floor < 3; floor++) {
        for (int col = -3; col <= 3; col++) {
            glPushMatrix(); glTranslatef(col*2.5f, 2.f+floor*3.f, 3.05f);
            glScalef(1.9f, 2.1f, .06f); glutSolidCube(1); glPopMatrix();
        }
    }
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);

    // entrance doors (dark glass)
    mat(.15f, .25f, .35f, .5f, .5f, .7f, 80.f, .8f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    glPushMatrix(); glTranslatef(0, .95f, 3.1f); glScalef(2.f, 1.9f, .06f); glutSolidCube(1); glPopMatrix();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);

    // entrance pillars
    mat(.85f, .83f, .80f, .4f, .4f, .38f, 35.f);
    for (int cp = -1; cp <= 1; cp += 2) {
        glPushMatrix(); glTranslatef(cp*1.2f, .95f, 3.0f);
        glRotatef(-90, 1,0,0); glutSolidCylinder(.12f, 1.9f, 10, 3); glPopMatrix();
    }

    // entrance canopy
    mat(.52f, .52f, .50f, .3f, .3f, .28f, 15.f);
    glPushMatrix(); glTranslatef(0, 2.1f, 3.8f); glScalef(3.f, .12f, 1.5f); glutSolidCube(1); glPopMatrix();

    // window glow at night - drawn without lighting for full brightness
    if (nightMode) {
        glDisable(GL_LIGHTING);
        for (int fl = 0; fl < 3; fl++) for (int cl = -3; cl <= 3; cl++) {
            glColor3f(1.f, .95f, .65f);
            glPushMatrix(); glTranslatef(cl*2.5f, 2.f+fl*3.f, 3.08f);
            glScalef(1.7f, 1.9f, .04f); glutSolidCube(1); glPopMatrix();
        }
        if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
    }
    glPopMatrix();
}

// ---------------------------------------------------------
// WASTE BIN, BENCH, LAMP POST, FLOWER BED
// ---------------------------------------------------------
void drawBin(float x, float z)
{
    glPushMatrix(); glTranslatef(x, 0, z);
    mat(.12f, .38f, .12f, .1f, .2f, .1f, 15.f);
    glPushMatrix(); glRotatef(-90,1,0,0); glutSolidCylinder(.22f, .85f, 12, 4); glPopMatrix();
    mat(.10f, .30f, .10f, .1f, .15f, .1f, 10.f);
    glPushMatrix(); glTranslatef(0, .9f, 0); glScalef(1.f, .12f, 1.f); glutSolidCylinder(.24f, 0.f, 12, 1); glPopMatrix();
    glPopMatrix();
}

void drawBench(float x, float z, float ry)
{
    glPushMatrix(); glTranslatef(x, 0, z); glRotatef(ry, 0, 1, 0);
    // seat slats
    mat(.55f, .35f, .15f, .15f, .10f, .08f, 10.f);
    for (int s = 0; s < 3; s++) {
        glPushMatrix(); glTranslatef(0, .48f, s*.14f-.14f); glScalef(1.6f, .06f, .09f); glutSolidCube(1); glPopMatrix();
    }
    // back rest
    for (int s = 0; s < 2; s++) {
        glPushMatrix(); glTranslatef(0, .75f+s*.15f, .22f); glScalef(1.6f, .06f, .06f); glutSolidCube(1); glPopMatrix();
    }
    // metal legs
    mat(.45f, .45f, .48f, .5f, .5f, .5f, 45.f);
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix(); glTranslatef(s*.65f, .24f, 0); glScalef(.06f, .5f, .06f); glutSolidCube(1); glPopMatrix();
        glPushMatrix(); glTranslatef(s*.65f, .6f, .22f); glScalef(.06f, .7f, .06f); glutSolidCube(1); glPopMatrix();
    }
    glPopMatrix();
}

void drawLampPost(float x, float z)
{
    glPushMatrix(); glTranslatef(x, 0, z);
    mat(.30f, .30f, .32f, .5f, .5f, .5f, 60.f);
    glPushMatrix(); glRotatef(-90,1,0,0); glutSolidCylinder(.07f, 5.5f, 10, 3); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 5.5f, 0); glScalef(.5f, .3f, .5f); glutSolidCube(1); glPopMatrix();
    // glow sphere - much brighter at night
    glDisable(GL_LIGHTING);
    float glow = nightMode ? 1.f : .25f;
    glColor3f(glow, glow*.88f, glow*.5f);
    glPushMatrix(); glTranslatef(0, 5.4f, 0); glutSolidSphere(.16f, 10, 8); glPopMatrix();
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawFlowerBed(float cx, float cz, float w, float d)
{
    // soil patch
    glPushMatrix(); glTranslatef(cx, .01f, cz);
    mat(.42f, .28f, .15f, .05f, .04f, .02f, 5.f);
    glScalef(w, .04f, d); glutSolidCube(1);
    glPopMatrix();

    // individual flowers - stem cylinder + coloured sphere head
    float fw = w*.5f, fd = d*.5f;
    float posX[] = { -.8f,  0,  .7f, -.4f,  .5f, -.6f,  .2f };
    float posZ[] = { -.5f, .3f, -.2f, .6f, -.7f,  .1f, -.4f };
    float fcR[]  = { .9f, .1f, .9f, 1.f, .2f, .8f, .1f };
    float fcG[]  = { .1f, .8f, .1f, .7f, .9f, .1f, .9f };
    float fcB[]  = { .1f, .1f, .8f, .0f, .1f, .9f, .1f };
    for (int i = 0; i < 7; i++) {
        float fx = cx + posX[i]*fw*.9f, fz2 = cz + posZ[i]*fd*.9f;
        mat(.15f, .52f, .10f, .05f, .12f, .03f, 5.f);
        glPushMatrix(); glTranslatef(fx, .35f, fz2); glRotatef(-90,1,0,0);
        glutSolidCylinder(.04f, .65f, 5, 2); glPopMatrix();
        mat(fcR[i], fcG[i], fcB[i], .4f, .3f, .3f, 30.f);
        glPushMatrix(); glTranslatef(fx, .72f, fz2); glutSolidSphere(.18f, 9, 7); glPopMatrix();
    }
}

// ---------------------------------------------------------
// GROUND
// textured quads for tarmac, grass and pavement
// bay lines drawn without lighting so the white stays pure white
// ---------------------------------------------------------
void drawGround()
{
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // main tarmac
    mat(.52f, .52f, .54f, .15f, .15f, .15f, 10.f);
    glBindTexture(GL_TEXTURE_2D, texID[0]);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-20,0,-18);
    glTexCoord2f(8,0); glVertex3f( 20,0,-18);
    glTexCoord2f(8,9); glVertex3f( 20,0, 18);
    glTexCoord2f(0,9); glVertex3f(-20,0, 18); glEnd();

    // grass strips left and right
    mat(.50f, .78f, .30f, .06f, .14f, .04f, 6.f);
    glBindTexture(GL_TEXTURE_2D, texID[2]);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-20,.01f,-18);
    glTexCoord2f(2,0); glVertex3f(-15,.01f,-18);
    glTexCoord2f(2,9); glVertex3f(-15,.01f, 18);
    glTexCoord2f(0,9); glVertex3f(-20,.01f, 18); glEnd();
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(15,.01f,-18);
    glTexCoord2f(2,0); glVertex3f(20,.01f,-18);
    glTexCoord2f(2,9); glVertex3f(20,.01f, 18);
    glTexCoord2f(0,9); glVertex3f(15,.01f, 18); glEnd();

    // central drive aisle (slightly darker)
    mat(.44f, .44f, .46f, .12f, .12f, .12f, 10.f);
    glBindTexture(GL_TEXTURE_2D, texID[0]);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-4,.01f,-18);
    glTexCoord2f(2,0); glVertex3f( 4,.01f,-18);
    glTexCoord2f(2,8); glVertex3f( 4,.01f, 16);
    glTexCoord2f(0,8); glVertex3f(-4,.01f, 16); glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    // parking bay lines - white
    glColor3f(.95f, .95f, .95f);
    for (int b = 0; b < 3; b++) {
        float bz = -12.f + b*8.f;
        glBegin(GL_QUADS);
        glVertex3f(-14,.02f,bz); glVertex3f(-4,.02f,bz);
        glVertex3f(-4,.02f,bz+.1f); glVertex3f(-14,.02f,bz+.1f); glEnd();
        for (int l = 0; l <= 4; l++) {
            float bx = -14.f + l*2.5f;
            glBegin(GL_QUADS);
            glVertex3f(bx,.02f,bz); glVertex3f(bx+.08f,.02f,bz);
            glVertex3f(bx+.08f,.02f,bz+7.5f); glVertex3f(bx,.02f,bz+7.5f); glEnd();
        }
    }
    for (int b = 0; b < 3; b++) {
        float bz = -12.f + b*8.f;
        glBegin(GL_QUADS);
        glVertex3f(4,.02f,bz); glVertex3f(14,.02f,bz);
        glVertex3f(14,.02f,bz+.1f); glVertex3f(4,.02f,bz+.1f); glEnd();
        for (int l = 0; l <= 4; l++) {
            float bx = 4.f + l*2.5f;
            glBegin(GL_QUADS);
            glVertex3f(bx,.02f,bz); glVertex3f(bx+.08f,.02f,bz);
            glVertex3f(bx+.08f,.02f,bz+7.5f); glVertex3f(bx,.02f,bz+7.5f); glEnd();
        }
    }

    // centre yellow dashed line
    glColor3f(.9f, .85f, .0f);
    for (int d = -4; d < 5; d++) {
        float dz = (float)d * 3.5f;
        glBegin(GL_QUADS);
        glVertex3f(-.08f,.02f,dz); glVertex3f(.08f,.02f,dz);
        glVertex3f(.08f,.02f,dz+2.f); glVertex3f(-.08f,.02f,dz+2.f); glEnd();
    }

    // zebra crossing at entrance
    glColor3f(.95f, .95f, .95f);
    for (int zs = 0; zs < 6; zs++) {
        float zx = -3.5f + zs*.7f;
        glBegin(GL_QUADS);
        glVertex3f(zx,.02f,13.5f); glVertex3f(zx+.5f,.02f,13.5f);
        glVertex3f(zx+.5f,.02f,16.f); glVertex3f(zx,.02f,16.f); glEnd();
    }

    // concrete pavement strip
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID[1]);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
    mat(.80f, .77f, .72f, .22f, .22f, .20f, 12.f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-20,.02f,14);
    glTexCoord2f(8,0); glVertex3f( 20,.02f,14);
    glTexCoord2f(8,2); glVertex3f( 20,.02f,18);
    glTexCoord2f(0,2); glVertex3f(-20,.02f,18); glEnd();
    glDisable(GL_TEXTURE_2D);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}

// ---------------------------------------------------------
// SKY
// gradient quad box that shifts colour based on sun height and day/night
// 700 random star points drawn at night
// ---------------------------------------------------------
void drawSky()
{
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    float dayF = CLAMP(sinf(D2R(sunHeight*90.f)), 0.f, 1.f);
    float tR, tG, tB, hR, hG, hB;
    if (!nightMode) {
        float srb = CLAMP(1.f - dayF*2.5f, 0.f, 1.f);
        tR=.28f+dayF*.1f; tG=.48f+dayF*.1f; tB=.88f;
        hR=.72f+srb*.25f; hG=.72f-srb*.22f; hB=.82f-srb*.42f;
    } else { tR=.01f; tG=.01f; tB=.06f; hR=.03f; hG=.03f; hB=.10f; }
    glClearColor(hR, hG, hB, 1);
    float W=250, H=100;
    auto face = [&](float x0,float y0,float z0, float x1,float y1,float z1,
                    float x2,float y2,float z2, float x3,float y3,float z3) {
        glBegin(GL_QUADS);
        glColor3f(hR,hG,hB); glVertex3f(x0,y0,z0); glVertex3f(x1,y1,z1);
        glColor3f(tR,tG,tB); glVertex3f(x2,y2,z2); glVertex3f(x3,y3,z3);
        glEnd();
    };
    face(-W,0,-W,  W,0,-W,  W,H,-W, -W,H,-W);
    face(-W,0, W,  W,0, W,  W,H, W, -W,H, W);
    face(-W,0,-W, -W,0, W, -W,H, W, -W,H,-W);
    face( W,0,-W,  W,0, W,  W,H, W,  W,H,-W);
    glBegin(GL_QUADS); glColor3f(tR,tG,tB);
    glVertex3f(-W,H,-W); glVertex3f(W,H,-W); glVertex3f(W,H,W); glVertex3f(-W,H,W); glEnd();

    if (nightMode) {
        glPointSize(1.8f); srand(11111);
        glBegin(GL_POINTS);
        for (int i = 0; i < 700; i++) {
            float a1 = ((float)rand()/RAND_MAX)*2*PI;
            float a2 = ((float)rand()/RAND_MAX)*PI*.48f;
            float b  = .3f + ((float)rand()/RAND_MAX)*.7f;
            glColor3f(b, b, b*.9f);
            glVertex3f(100*cosf(a2)*cosf(a1), 100*sinf(a2)+5, 100*cosf(a2)*sinf(a1));
        }
        glEnd();
    }
    if (depthMode != DM_OFF) glEnable(GL_DEPTH_TEST);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}

void drawSunDisc()
{
    float sx = cosf(D2R(sunAngle))*18.f, sy = sinf(D2R(sunHeight*90.f))*18.f, sz = sinf(D2R(sunAngle))*18.f;
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    if (!nightMode) {
        float srb = CLAMP(1.f-(sy/18.f)*2.5f, 0.f, 1.f);
        glColor3f(1.f, .95f-srb*.3f, .4f-srb*.3f);
        glPushMatrix(); glTranslatef(sx,sy,sz); glutSolidSphere(1.4f,16,12); glPopMatrix();
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.f,.9f,.5f,.15f);
        glPushMatrix(); glTranslatef(sx,sy,sz); glutSolidSphere(2.2f,12,9); glPopMatrix();
        glDisable(GL_BLEND);
    } else {
        glColor3f(.88f,.88f,.80f);
        glPushMatrix(); glTranslatef(sx*.65f,sy*.65f,sz*.65f); glutSolidSphere(1.1f,14,10); glPopMatrix();
    }
    if (depthMode != DM_OFF) glEnable(GL_DEPTH_TEST);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}

// ---------------------------------------------------------
// SHADOWS
// draws all shadow-casting geometry flat on the ground using the shadow matrix
// inShadowPass stops material calls running during this pass
// ---------------------------------------------------------
void drawShadowGeom()
{
    inShadowPass = true;
    for (int i = 0; i < 4; i++) drawCar(i);
    float txs[] = { -17,-17,-17, 17, 17, 17,-17, 17 };
    float tzs[] = { -10,  0, 10,-10,  0, 10, -4, -4 };
    for (int t = 0; t < 8; t++) drawTree(txs[t], tzs[t]);
    drawOffice();
    drawLampPost(0,14); drawLampPost(-12,14); drawLampPost(12,14);
    drawBin(-5,12); drawBin(5,12);
    drawBench(-6,10); drawBench(6,10,180);
    inShadowPass = false;
}

void drawAllShadows()
{
    if (LY < 0.06f) return;  // sun too low, shadows would stretch to infinity
    float alpha = CLAMP(LY*.65f, .08f, .60f);
    if (nightMode) alpha = .10f;

    float sm[16]; shadowMat(LX, LY, LZ, sm);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if (!nightMode) glColor4f(.04f,.05f,.03f,alpha);
    else            glColor4f(.04f,.04f,.14f,alpha);

    glPushMatrix();
    glTranslatef(0, .015f, 0);  // tiny lift to avoid z-fighting with ground
    glMultMatrixf(sm);
    drawShadowGeom();
    glPopMatrix();

    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    if (lightMode != LM_OFF) glEnable(GL_LIGHTING);
}
