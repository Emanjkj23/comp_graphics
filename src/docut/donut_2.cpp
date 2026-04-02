#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- 1. SHADERS (Now with Breeze Physics) ---
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;

    out vec3 vertexColor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float uTime; // Time variable for the breeze

    void main() {
        // --- THE BREEZE SIMULATION ---
        // We calculate a tiny offset based on time and the particle's position.
        // This makes different parts of the torus wave at different rates.
        vec3 offset;
        offset.x = sin(uTime * 2.0 + aPos.y * 10.0) * 0.05;
        offset.y = cos(uTime * 1.5 + aPos.z * 10.0) * 0.05;
        offset.z = sin(uTime * 1.8 + aPos.x * 10.0) * 0.05;

        // Add the breeze offset to the original mathematical position
        vec3 finalPos = aPos + offset;

        gl_Position = projection * view * model * vec4(finalPos, 1.0);
        vertexColor = aColor;
        
        // Vary the size slightly based on the breeze for a twinkling effect
        gl_PointSize = 1.5 + (sin(uTime * 5.0 + aPos.x * 20.0) * 0.5); 
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

// --- GLOBALS ---
GLuint VAO, VBO, shaderProgram;
int numTorusVertices = 0;
auto startTime = std::chrono::high_resolution_clock::now();
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- CAMERA GLOBALS ---
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  4.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

bool firstMouse = true;
float yaw   = -90.0f; // Pointing along -Z initially
float pitch =  0.0f;
float lastX =  1000.0f / 2.0;
float lastY =  600.0f / 2.0;

// --- INPUT HANDLING ---
// Keyboard for Movement (WASD)
void keyboard(unsigned char key, int x, int y) {
    float cameraSpeed = 5.0f * deltaTime; // Adjust speed here
    if (key == 'w') cameraPos += cameraSpeed * cameraFront;
    if (key == 's') cameraPos -= cameraSpeed * cameraFront;
    if (key == 'a') cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (key == 'd') cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (key == 27) exit(0); // ESC to quit
}

// Mouse for Looking around
void passiveMouseMotion(int xpos, int ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    // Prevent screen flip
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// --- SETUP FUNCTIONS ---
void setupTorus() {
    std::vector<float> vertices;
    int mainSegments = 200; // Increased density for better swarm effect
    int tubeSegments = 70;  
    float R = 1.0f; 
    float r = 0.4f;  

    for (int i = 0; i < mainSegments; i++) {
        float u = (float)i / mainSegments * 2.0f * glm::pi<float>();
        for (int j = 0; j < tubeSegments; j++) {
            float v = (float)j / tubeSegments * 2.0f * glm::pi<float>();

            float x = (R + r * cos(v)) * cos(u);
            float y = (R + r * cos(v)) * sin(u);
            float z = r * sin(v);

            float rCol = (cos(u) + 1.0f) * 0.5f;
            float gCol = (sin(v) + 1.0f) * 0.5f;
            float bCol = 1.0f - rCol;

            vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
            vertices.push_back(rCol); vertices.push_back(gCol); vertices.push_back(bCol);
        }
    }

    numTorusVertices = vertices.size() / 6;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

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

// --- THE RENDER LOOP ---
void display() {
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    // Calculate Delta Time (for smooth movement regardless of framerate)
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();
    deltaTime = time - lastFrame;
    lastFrame = time;

    // Send Time to Shader for the Breeze
    glUniform1f(glGetUniformLocation(shaderProgram, "uTime"), time);

    // 1. MODEL MATRIX (TRS)
    glm::mat4 model = glm::mat4(1.0f); 
    model = glm::translate(model, glm::vec3(0.0f, sin(time) * 0.2f, 0.0f)); // Bob up and down
    model = glm::rotate(model, time * glm::radians(20.0f), glm::vec3(1.0f, 0.5f, 0.0f)); // Tumble
    float scaleAmt = 1.0f + sin(time * 2.0f) * 0.1f;
    model = glm::scale(model, glm::vec3(scaleAmt)); // Breathe

    // 2. VIEW MATRIX (Camera)
    // The camera looks from cameraPos, towards cameraPos + cameraFront
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // 3. PROJECTION MATRIX (Perspective)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1000.0f / 600.0f, 0.1f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(VAO);
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
    glutCreateWindow("Torus Swarm - Final Assignment");

    glewExperimental = GL_TRUE; 
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE); 
    
    // Enable blending for that glowing look
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    compileShaders();
    setupTorus();

    // Hook up our Input Functions
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(passiveMouseMotion); // Tracks mouse without needing to click
    glutTimerFunc(16, timer, 0); 

    // Hide the cursor since we are using it for the camera
    glutSetCursor(GLUT_CURSOR_NONE);

    glutMainLoop();
    return 0;
}