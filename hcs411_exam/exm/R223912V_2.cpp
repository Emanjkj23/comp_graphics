#include <GL/glut.h>

void drawWireFrame() {
    glDisable(GL_LIGHTING); // I remived this because colors won't show properly if lighting is on with normals

    glBegin(GL_LINES);
        glColor3f(0.0, 0.0, 1.0);  // Line p1-p2 (Blue)
        glVertex3f(0.5, 0.5, 1.0);  // p1
        glVertex3f(1.0, 0.0, 1.0);  // p2


        glColor3f(1.0, 1.0, 0.0);  // Line p2-p3 (Yellow) || I used (reed + green) to get yelloww.
        glVertex3f(1.0, 0.0, 1.0);  // p2
        glVertex3f(0.0, 0.0, 1.0);  // p3


        glColor3f(1.0, 0.0, 0.0);  // Line p3-p4 (Red)
        glVertex3f(0.0, 0.0, 1.0);  // p3
        glVertex3f(1.0, 1.0, 2.0);  // p4
    glEnd();
}

void displayQ2() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Basic Camera
    gluLookAt(0.0, 0.0, 10.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0);

    // Draw Shape
    glPushMatrix();
        drawWireFrame();
    glPopMatrix();

    glutSwapBuffers();
}

void reshapeQ2(int w, int h) { //WDID
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
    glutCreateWindow("HCS411 Exam - Question 2");

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(displayQ2);
    glutReshapeFunc(reshapeQ2);
    glutTimerFunc(16, timer, 0); 

    glutMainLoop();
    return 0;
}