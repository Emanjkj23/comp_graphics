#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <cmath>

#define WIDTH 900
#define HEIGHT 900
#define PARTICLES 400000

struct Particle { float x, y, vx, vy; };

// ---------- shader helpers ----------
GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

GLuint program(std::initializer_list<GLuint> shaders) {
    GLuint p = glCreateProgram();
    for (auto s : shaders) glAttachShader(p, s);
    glLinkProgram(p);
    return p;
}

// ================= COMPUTE SHADER =================
const char* computeSrc = R"(#version 430
layout(local_size_x = 256) in;

struct Particle { vec2 pos; vec2 vel; };
layout(std430, binding = 0) buffer P { Particle p[]; };

uniform float time;
uniform float dt;

// ----------- CURL NOISE -----------
vec2 curlNoise(vec2 p) {
    float eps = 0.001;

    float n1 = sin(p.y + time);
    float n2 = cos(p.x - time);

    float dx = (sin(p.y + eps) - sin(p.y - eps)) / (2.0*eps);
    float dy = (cos(p.x + eps) - cos(p.x - eps)) / (2.0*eps);

    return vec2(dy, -dx);
}

// ----------- BIOT–SAVART VORTEX RING -----------
vec2 vortexRing(vec2 pos) {
    float R = 0.35 + 0.1*sin(time*0.3); // ring radius
    float r = length(pos);

    float dist = r - R;

    float strength = 1.0 / (dist*dist + 0.02); // Biot-Savart-like

    vec2 tangent = normalize(vec2(-pos.y, pos.x));

    // add 6-fold symmetry (flower petals)
    float angle = atan(pos.y, pos.x);
    float lobes = sin(angle * 6.0 + time);

    return tangent * strength * (1.0 + 0.6 * lobes);
}

void main() {
    uint id = gl_GlobalInvocationID.x;

    vec2 pos = p[id].pos;

    vec2 v1 = vortexRing(pos);
    vec2 v2 = curlNoise(pos * 3.0);

    vec2 velocity = v1 + 0.3 * v2;

    p[id].vel = mix(p[id].vel, velocity, 0.08);
    p[id].pos += p[id].vel * dt;

    p[id].vel *= 0.995;

    if (length(p[id].pos) > 1.5)
        p[id].pos *= 0.3;
}
)";

// ================= RENDER =================
const char* vs = R"(#version 430
layout(location=0) out vec2 uv;

struct Particle { vec2 pos; vec2 vel; };
layout(std430, binding = 0) buffer P { Particle p[]; };

uniform float time;

void main() {
    vec2 pos = p[gl_VertexID].pos;

    // camera rotation
    float a = time * 0.1;
    mat2 rot = mat2(cos(a), -sin(a), sin(a), cos(a));
    pos = rot * pos;

    // zoom breathing
    float zoom = 1.2 + 0.2*sin(time*0.3);
    pos *= zoom;

    uv = pos;
    gl_Position = vec4(pos, 0, 1);
    gl_PointSize = 2.0;
}
)";

const char* fs = R"(#version 430
in vec2 uv;
out vec4 FragColor;

void main() {
    float d = length(gl_PointCoord - vec2(0.5));
    float alpha = smoothstep(0.5, 0.0, d);

    // ===== COLOR CONTROL (EDIT HERE) =====
    vec3 BLUE   = vec3(0.05, 0.15, 0.9);
    vec3 ORANGE = vec3(1.0, 0.65, 0.15);

    float t = clamp(length(uv)*1.2, 0.0, 1.0);
    vec3 color = mix(BLUE, ORANGE, t);

    FragColor = vec4(color, alpha);
}
)";

// ================= TRAIL SHADER =================
const char* screenVS = R"(#version 430
out vec2 uv;
void main() {
    vec2 pos = vec2((gl_VertexID<<1)&2, gl_VertexID&2);
    uv = pos;
    gl_Position = vec4(pos*2.0-1.0,0,1);
}
)";

const char* trailFS = R"(#version 430
in vec2 uv;
out vec4 FragColor;
uniform sampler2D prevFrame;

void main() {
    vec3 col = texture(prevFrame, uv).rgb;

    // fade trails slowly
    col *= 0.97;

    FragColor = vec4(col, 1.0);
}
)";

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* win = glfwCreateWindow(WIDTH, HEIGHT, "Vortex Ultimate", 0, 0);
    glfwMakeContextCurrent(win);

    gladLoadGL();

    // particles
    std::vector<Particle> data(PARTICLES);
    for (int i=0;i<PARTICLES;i++) {
        float a = rand()/float(RAND_MAX)*6.28f;
        float r = 0.3f + rand()/float(RAND_MAX)*0.2f;
        data[i].x = cos(a)*r;
        data[i].y = sin(a)*r;
    }

    GLuint ssbo;
    glGenBuffers(1,&ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle)*PARTICLES, data.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,ssbo);

    GLuint compProg = program({compile(GL_COMPUTE_SHADER, computeSrc)});
    GLuint renderProg = program({
        compile(GL_VERTEX_SHADER, vs),
        compile(GL_FRAGMENT_SHADER, fs)
    });

    // ----- framebuffer ping-pong -----
    GLuint fbo[2], tex[2];
    glGenFramebuffers(2,fbo);
    glGenTextures(2,tex);

    for(int i=0;i<2;i++){
        glBindTexture(GL_TEXTURE_2D, tex[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,WIDTH,HEIGHT,0,GL_RGB,GL_UNSIGNED_BYTE,NULL);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER,fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex[i],0);
    }

    GLuint trailProg = program({
        compile(GL_VERTEX_SHADER, screenVS),
        compile(GL_FRAGMENT_SHADER, trailFS)
    });

    int ping = 0;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    while(!glfwWindowShouldClose(win)){
        float t = glfwGetTime();

        // compute
        glUseProgram(compProg);
        glUniform1f(glGetUniformLocation(compProg,"time"),t);
        glUniform1f(glGetUniformLocation(compProg,"dt"),0.01f);
        glDispatchCompute(PARTICLES/256,1,1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // trail pass
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[ping]);
        glUseProgram(trailProg);
        glBindTexture(GL_TEXTURE_2D, tex[1-ping]);
        glDrawArrays(GL_TRIANGLES,0,3);

        // draw particles
        glUseProgram(renderProg);
        glUniform1f(glGetUniformLocation(renderProg,"time"),t);
        glDrawArrays(GL_POINTS,0,PARTICLES);

        // present
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glBindTexture(GL_TEXTURE_2D, tex[ping]);
        glDrawArrays(GL_TRIANGLES,0,3);

        ping = 1 - ping;

        glfwSwapBuffers(win);
        glfwPollEvents();
    }
}