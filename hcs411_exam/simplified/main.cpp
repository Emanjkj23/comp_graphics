/* This file contains 2D graphics practice code from gemini chat on practicing to study for exam. This file is non related to all the other files.*/
#include <GL/glut.h>


void drawTable() {
    // Table top
    glPushMatrix();
        glColor3f(0.5, 0.35, 0.05); // Brown color 
        glScalef(1.0, 0.1, 1.0); // Make it flat
        glutSolidCube(1.0);
    glPopMatrix();

    // code for generating legs of the table | simplified by using 4 cubes instead of cylinders
    for (int i=0; i<4; i++) {
        glPushMatrix();
            glColor3f(0.5, 0.35, 0.05); // Brown color
            float x = (i < 2) ? -0.4 : 0.4; // Left or right
            float z = (i % 2 == 0) ? -0.4 : 0.4; // Front or back
            glTranslatef(x, -0.25, z);
            glScalef(0.1, 0.5, 0.1);
            glutSolidCube(1.0);
        glPopMatrix();
    }
}

void drawTeapot() {
    glPushMatrix();
        glColor3f(1.0, 1.0, 0.0); // Yellow color
        glTranslatef(1.0, 0.0, 0.0);
        // glScalef(1.0, 3.0, 0.0);
        glutSolidTeapot(1.0);
        // glutSolidTorus(0.2, 0.5, 20, 100);
        // glutSolidSphere(1.0, 100, 20);
    glPopMatrix();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // WDID
    glLoadIdentity();

    // Camera/Viewing (COncept 3)
    gluLookAt(2.0, 0.0, 5.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0); // WDID

    // drawTeapot();

    drawTable();

    glutSwapBuffers();  // WDID
}

void reshape(int w, int h) { //WDID
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("HCS411 Exam - Simplified");

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glutMainLoop();
    return 0;
}