// CPP program to render a single, large blue particle using Shaders
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

// --- 1. SHADER SOURCE CODE ---
// The Vertex Shader: Places the point at the center of the screen
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
)";

// The Fragment Shader: Colors the point solid blue (like the start of your image)
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f); // RGBA (Blue)
    }
)";

// Global variables for our GPU objects
GLuint VAO, VBO, shaderProgram;

// --- 2. SHADER COMPILATION UTILITY ---
void compileShaders() {
    // Compile Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Compile Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Link Shaders into a Program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Clean up individual shaders as they are now linked into the program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// --- 3. DATA SETUP ---
void setupParticle() {
    // Just one point exactly in the middle of the screen
    float particlePos[] = { 0.0f, 0.0f, 0.0f };

    // Generate our GPU objects
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind the VAO first, then bind and set vertex buffers, and then configure vertex attributes.
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particlePos), particlePos, GL_STATIC_DRAW);

    // Tell OpenGL how to interpret the data (3 floats per position)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind to prevent accidental modification
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
}

// --- 4. THE RENDER LOOP ---
void display() {
    // Clear the screen to a dark background (like your image)
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Activate our shader program
    glUseProgram(shaderProgram);

    // Make the point large enough to see
    glPointSize(20.0f);

    // Draw the particle
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, 1);

    // Swap buffers (display what we just drew)
    glutSwapBuffers();
}

int main(int argc, char** argv) {
    // Initialize FreeGLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Modern Particle River Foundation");

    // Initialize GLEW (Crucial for Modern OpenGL!)
    glewExperimental = GL_TRUE; 
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Setup our data and shaders
    compileShaders();
    setupParticle();

    // Set the display callback and start the loop
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}