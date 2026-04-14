#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <iostream>

using namespace std;

void display() {
    std::printf("Hello, OpenGL!\n");
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(1080, 720);
    glutCreateWindow("Hellow world! What's up!!!!!!!");
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}