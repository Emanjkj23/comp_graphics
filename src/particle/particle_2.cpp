#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <string>

// --- 1. THE SHADERS (Simplified for Debugging) ---
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec4 aData; 
    uniform float uTime;
    out float vProgress;

    void main() {
        float t = fract(aData.w + uTime * 0.05); 
        vProgress = t;

        float pathX = (t - 0.5) * 3.0;
        float pathY = sin(t * 10.0) * 0.4; 
        float thickness = sin(t * 3.14159); 

        vec3 finalPos = vec3(pathX, pathY, 0.0) + (aData.xyz * 0.15 * thickness);
        finalPos.y += sin(uTime * 3.0 + aData.x * 20.0) * 0.02;

        gl_Position = vec4(finalPos, 1.0);
        gl_PointSize = 2.0 + (aData.z * 2.0); 
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in float vProgress;
    out vec4 FragColor;

    void main() {
        // I have temporarily removed the 'discard' circular math.
        // Some Windows drivers fail silently on gl_PointCoord.
        // Let's ensure we can at least see the squares first!

        vec3 startColor = vec3(0.0, 0.3, 1.0); // Blue
        vec3 endColor = vec3(1.0, 0.4, 0.0);   // Orange
        vec3 finalColor = mix(startColor, endColor, vProgress);

        // Fixed alpha to guarantee visibility
        FragColor = vec4(finalColor, 0.8); 
    }
)";

// --- GLOBALS ---
GLuint VAO, VBO, shaderProgram;
GLint timeLocation;
const int NUM_PARTICLES = 100000;
auto startTime = std::chrono::high_resolution_clock::now();

// --- 2. ERROR CHECKING UTILITY ---
void checkCompileErrors(GLuint shader, std::string type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

// --- 3. SETUP ---
void compileShaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkCompileErrors(vertexShader, "VERTEX"); // <--- NEW: Checks for syntax errors

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT"); // <--- NEW

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM"); // <--- NEW

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    timeLocation = glGetUniformLocation(shaderProgram, "uTime");
}

void setupParticles() {
    std::vector<float> particleData(NUM_PARTICLES * 4);
    std::mt19937 rng(42); 
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distT(0.0f, 1.0f);

    for (int i = 0; i < NUM_PARTICLES * 4; i += 4) {
        particleData[i]     = dist(rng); 
        particleData[i + 1] = dist(rng); 
        particleData[i + 2] = dist(rng); 
        particleData[i + 3] = distT(rng); 
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, particleData.size() * sizeof(float), particleData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

// --- 4. THE RENDER LOOP & TIMER ---
void display() {
    glClearColor(0.02f, 0.02f, 0.04f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);

    auto currentTime = std::chrono::high_resolution_clock::now();
    float timeValue = std::chrono::duration<float>(currentTime - startTime).count();

    glUseProgram(shaderProgram);
    glUniform1f(timeLocation, timeValue);

    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

    // Swap buffers (display what we just drew)
    glutSwapBuffers();
    glutPostRedisplay();  // Tell FreeGLUT to render the next frame immediately
}

int main(int argc, char** argv) {
    // Initialize FreeGLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(1000, 600);
    glutCreateWindow("100,000 Particle River");

    // Initialize GLEW (Crucial for Modern OpenGL!)
    glewExperimental = GL_TRUE; 
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // --- ENABLE ADDITIVE BLENDING FOR GLOW ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    // This setting allows the shader to edit the size of gl_point
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Setup our data and shaders
    compileShaders();
    setupParticles();

    // Set the display callback and start the loop
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}