/* This file contains 2D graphics practice code */
#include <GL/glut.h>

void drawTable() {
        glPushMatrix();
        glColor3f(0.5, 0.35, 0.05); // Brown color
        glBegin(GL_QUADS);
            glVertex3f(-1.0, -0.5, 0.0);
            glVertex3f(1.0, -0.5, 0.0);
            glVertex3f(1.0, 0.5, 0.0);
            glVertex3f(-1.0, 0.5, 0.0);
        glEnd();
    glPopMatrix();
}