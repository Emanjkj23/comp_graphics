/* Include necessary OpenGL headers 
 * Note: glew.h must be included before freeglut.h to avoid compilation issues [cite: 87]
 * This code sets up a rotating colored cube using modern OpenGL with shaders and MVP matrices.
 * C++17 is used for string literals and chrono features.
 * References:
 * [87] https://stackoverflow.com/questions/3683301/glew-and-glut-headers-inclusion-order
 * [88] https://glm.g-truc.net/0.9.9/index.html
*/
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <chrono>

// Include GLM [cite: 88]
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- 1. SHADERS (Now with MVP Matrices) ---
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;

    out vec3 vertexColor;

    // The Matrix Uniforms
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        // The core equation of 3D Graphics: P * V * M * Position
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        vertexColor = aColor;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 vertexColor;
    out vec4 FragColor;

    void main() {
        FragColor = vec4(vertexColor, 1.0);
    }
)";

GLuint VAO, VBO, shaderProgram;
auto startTime = std::chrono::high_resolution_clock::now();

// --- 2. THE CUBE DATA ---
void setupCube() {
    // 36 Vertices to make a cube (6 faces * 2 triangles * 3 vertices)
    // Each row: X, Y, Z (Position), R, G, B (Color)
    float vertices[] = {
        // Back face (Red)
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,

        // Front face (Green)
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,

        // Left face (Blue)
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,

        // Right face (Yellow)
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

        // Bottom face (Cyan)
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,

        // Top face (Magenta)
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position Attribute (first 3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color Attribute (last 3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

// --- 3. SHADER COMPILATION ---
void compileShaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// --- 4. THE RENDER LOOP ---
void display() {
    // Clear both the color buffer AND the depth buffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    auto currentTime = std::chrono::high_resolution_clock::now();
    float timeValue = std::chrono::duration<float>(currentTime - startTime).count();

    // --- CONSTRUCT MATRICES ---

    // 1. Model Matrix (Rotate over time)
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix
    model = glm::rotate(model, timeValue * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));

    // 2. View Matrix (Move camera backwards to see the cube)
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

    // 3. Projection Matrix (Field of View, Aspect Ratio, Near plane, Far plane)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1000.0f / 600.0f, 0.1f, 100.0f);

    // --- SEND MATRICES TO GPU ---
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // --- DRAW ---
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glutSwapBuffers();
}

void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); 
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    // Request a depth buffer from FreeGLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1000, 600);
    glutCreateWindow("3D Transform Pipeline");

    glewExperimental = GL_TRUE; 
    glewInit();

    // Enable Depth Testing so front faces occlude back faces
    glEnable(GL_DEPTH_TEST);

    compileShaders();
    setupCube();

    glutDisplayFunc(display);
    glutTimerFunc(16, timer, 0); 

    glutMainLoop();
    return 0;
}