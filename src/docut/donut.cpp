#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- 1. SHADERS ---
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;

    out vec3 vertexColor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        vertexColor = aColor;
        gl_PointSize = 2.0; // Size of our particles
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
int numTorusVertices = 0;
auto startTime = std::chrono::high_resolution_clock::now();

// --- 2. TORUS MATHEMATICS ---
void setupTorus() {
    std::vector<float> vertices;
    
    // Resolution of our point cloud
    int mainSegments = 150; // Points around the big ring
    int tubeSegments = 50;  // Points around the tube thickness
    
    float R = 1.0f;  // Major radius (distance from center to tube)
    float r = 0.4f;  // Minor radius (thickness of the tube)

    for (int i = 0; i < mainSegments; i++) {
        // Angle u goes from 0 to 2*PI
        float u = (float)i / mainSegments * 2.0f * glm::pi<float>();
        
        for (int j = 0; j < tubeSegments; j++) {
            // Angle v goes from 0 to 2*PI
            float v = (float)j / tubeSegments * 2.0f * glm::pi<float>();

            // The Parametric Equations
            float x = (R + r * cos(v)) * cos(u);
            float y = (R + r * cos(v)) * sin(u);
            float z = r * sin(v);

            // Give it a cool color gradient based on its position
            float rCol = (cos(u) + 1.0f) * 0.5f; // Red shifts around the main ring
            float gCol = (sin(v) + 1.0f) * 0.5f; // Green shifts around the tube
            float bCol = 1.0f - rCol;            // Blue does the inverse of red

            // Push Position (X, Y, Z)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            // Push Color (R, G, B)
            vertices.push_back(rCol);
            vertices.push_back(gCol);
            vertices.push_back(bCol);
        }
    }

    numTorusVertices = vertices.size() / 6; // 6 floats per vertex (3 pos + 3 color)

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Position Attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color Attribute
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

// --- 4. THE RENDER LOOP (T-R-S Demonstration) ---
void display() {
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    // ==========================================
    // ASSIGNMENT REQUIREMENT: T-R-S ANIMATION
    // Order of operations in code is reverse of math: 
    // We code Translate -> Rotate -> Scale, which means the GPU applies Scale -> Rotate -> Translate.
    // ==========================================
    
    glm::mat4 model = glm::mat4(1.0f); 

    // 1. TRANSLATE: Move it up and down in a sine wave
    float moveY = sin(time * 2.0f) * 0.5f;
    model = glm::translate(model, glm::vec3(0.0f, moveY, 0.0f));

    // 2. ROTATE: Tumble it on two axes continuously
    model = glm::rotate(model, time * glm::radians(45.0f), glm::vec3(1.0f, 0.3f, 0.0f));

    // 3. SCALE: Make it "breathe" (grow and shrink slightly)
    float scaleAmt = 1.0f + sin(time * 3.0f) * 0.2f;
    model = glm::scale(model, glm::vec3(scaleAmt, scaleAmt, scaleAmt));

    // ==========================================
    // VIEW & PROJECTION
    // ==========================================
    
    // View Matrix (Camera pulled back 4 units on the Z axis)
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -4.0f));

    // Projection Matrix (Standard 45 degree FOV perspective)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1000.0f / 600.0f, 0.1f, 100.0f);

    // Send to GPU
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // --- DRAW THE PARTICLES ---
    glBindVertexArray(VAO);
    
    // Notice we use GL_POINTS here to render our mathematical cloud!
    glDrawArrays(GL_POINTS, 0, numTorusVertices);

    glutSwapBuffers();
}

void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); 
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1000, 600);
    glutCreateWindow("Torus Particle Simulation - TRS Animation");

    glewExperimental = GL_TRUE; 
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE); // Allows shader to set gl_PointSize

    compileShaders();
    setupTorus();

    glutDisplayFunc(display);
    glutTimerFunc(16, timer, 0); 

    glutMainLoop();
    return 0;
}