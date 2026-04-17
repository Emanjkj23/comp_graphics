#include <GL/glut.h>

void drawFigure() {
    glBegin(GL_QUADS);
        glVertex2f(-2.0, 0.0);
        glVertex2f(0.0, -2.0);
        glVertex2f(-2.0, -4.0);
        glVertex2f(-4.0, -2.0);
    glEnd();
}

void displayQ1() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Basic Camera
    gluLookAt(0.0, 0.0, 10.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0);

    // Draw Shape
    glPushMatrix();
        glColor3f(0.0, 0.0, 1.0); // Original shape is blue
        drawFigure();
    glPopMatrix();

    // i) RScale shape - factor(2)
    glPushMatrix();
        glColor3f(1.0, 0.0, 0.0); // Chnage to red after scaling
        glScalef(2.0, 2.0, 2.0);
        drawFigure();
    glPopMatrix();

    // ii) roatet 45 deg shape
    glPushMatrix();
        glColor3f(0.0, 1.0, 0.0); // change to green after rotation
        glRotatef(45.0, 0.0, 0.0, 1.0);
        glScalef(2.0, 2.0, 2.0); // maitinaing of prev scale
        drawFigure();
    glPopMatrix();

    // ii) Transalte shape
    glPushMatrix();
        glTranslatef(-5.0, -0.75, -0.4);
        glRotatef(45.0, 0.0, 0.0, 1.0);
        glScalef(2.0, 2.0, 2.0); // maitinaing of prev scale
        // glColor3f(1.0, 1.0, 1.0); // thinking white to show that its final state
        drawFigure();
    glPopMatrix();

    glutSwapBuffers();
}

void reshapeQ1(int w, int h) { //WDID
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); 
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("HCS411 Exam - Question 1");

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(displayQ1);
    glutReshapeFunc(reshapeQ1);
    glutTimerFunc(16, timer, 0); 

    glutMainLoop();
    return 0;
}