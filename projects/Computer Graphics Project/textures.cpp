// textures.cpp
// procedural texture generation - all built from random noise, no image files needed
// week 10: texture mapping, mipmapping, anisotropic filtering
//
// gluBuild2DMipmaps generates the full mip chain automatically which stops
// the ground looking blurry/glittery when viewed at a shallow angle

#include "textures.h"

// fills an RGB buffer with noise around a base colour
// 'var' controls how much variation there is, 'seed' keeps it consistent between runs
static void makeNoise(unsigned char* p, int sz,
                      unsigned char r, unsigned char g, unsigned char b,
                      int var, unsigned seed)
{
    srand(seed);
    for (int i = 0; i < sz * sz; i++) {
        int v = rand() % var - var / 2;
        p[i*3+0] = (unsigned char)CLAMP(r + v, 0, 255);
        p[i*3+1] = (unsigned char)CLAMP(g + v, 0, 255);
        p[i*3+2] = (unsigned char)CLAMP(b + v, 0, 255);
    }
}

void initTextures()
{
    const int SZ = 128;  // must be power of two for mipmapping to work
    static unsigned char buf[SZ * SZ * 3];

    glGenTextures(4, texID);

    // helper lambda - uploads buf to texID[idx] with mipmaps + anisotropic filtering
    auto upload = [&](int idx) {
        glBindTexture(GL_TEXTURE_2D, texID[idx]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // anisotropic filtering sharpens ground textures when viewed at an angle
        float maxAniso = 1.f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        maxAniso > 8.f ? 8.f : maxAniso);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, SZ, SZ, GL_RGB, GL_UNSIGNED_BYTE, buf);
    };

    // texture 0: asphalt - medium grey with slight blue tint
    makeNoise(buf, SZ, 110, 110, 116, 20, 0xA5);
    upload(0);

    // texture 1: concrete - lighter grey for the building walls
    makeNoise(buf, SZ, 185, 183, 178, 22, 0xC3);
    upload(1);

    // texture 2: grass - base green then add lighter and darker blade variation on top
    makeNoise(buf, SZ, 95, 162, 48, 30, 0x7F);
    srand(0x2E);
    for (int i = 0; i < SZ*SZ/5; i++) {
        int px = rand() % (SZ * SZ);
        buf[px*3+0] = (unsigned char)CLAMP(buf[px*3+0] + 20, 0, 255);
        buf[px*3+1] = (unsigned char)CLAMP(buf[px*3+1] + 28, 0, 255);
        buf[px*3+2] = (unsigned char)CLAMP(buf[px*3+2] +  8, 0, 255);
    }
    srand(0xB1);
    for (int i = 0; i < SZ*SZ/8; i++) {
        int px = rand() % (SZ * SZ);
        buf[px*3+0] = (unsigned char)CLAMP(buf[px*3+0] - 30, 0, 255);
        buf[px*3+1] = (unsigned char)CLAMP(buf[px*3+1] - 40, 0, 255);
        buf[px*3+2] = (unsigned char)CLAMP(buf[px*3+2] - 12, 0, 255);
    }
    upload(2);

    // texture 3: road marking paint - off-white with a slight yellow cast
    makeNoise(buf, SZ, 238, 235, 200, 10, 0x11);
    upload(3);

    // texture 4: bark / tyre marks - dark brown with vertical streak pattern
    // reused for tree trunks and as a road smudge decal
    glGenTextures(1, &texID[4]);  // needs its own glGenTextures since we only did 4 above
    makeNoise(buf, SZ, 58, 36, 16, 18, 0x3D);
    srand(0x5C);
    for (int col = 0; col < SZ; col += 3 + rand() % 4) {
        unsigned char dark = (unsigned char)(28 + rand() % 18);
        int streak_w = 1 + rand() % 2;
        for (int row = 0; row < SZ; row++) {
            for (int sw = 0; sw < streak_w && col+sw < SZ; sw++) {
                int px = row * SZ + col + sw;
                buf[px*3+0] = (unsigned char)CLAMP(buf[px*3+0] - (int)dark,        0, 255);
                buf[px*3+1] = (unsigned char)CLAMP(buf[px*3+1] - (int)(dark*.8f),  0, 255);
                buf[px*3+2] = (unsigned char)CLAMP(buf[px*3+2] - (int)(dark*.4f),  0, 255);
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, texID[4]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, SZ, SZ, GL_RGB, GL_UNSIGNED_BYTE, buf);

    glBindTexture(GL_TEXTURE_2D, 0);  // unbind when done
}
