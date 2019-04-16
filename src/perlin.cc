#include "perlin.h"

Perlin::Perlin() {
    //p = new int[512];
    for (int x = 0; x < 512; x++) {
        p[x] = permutation[x % 256];
    }
}

double Perlin::fade(double t) {
    // Fade function defined by Ken Perlin
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double Perlin::grad(int hash, double x, double y, double z) {
    switch (hash & 0x0F) {
        case 0x0: return  x + y;
        case 0x1: return -x + y;
        case 0x2: return  x - y;
        case 0x3: return -x - y;
        case 0x4: return  x + z;
        case 0x5: return -x + z;
        case 0x6: return  x - z;
        case 0x7: return -x - z;
        case 0x8: return  y + z;
        case 0x9: return -y + z;
        case 0xA: return  y - z;
        case 0xB: return -y - z;
        case 0xC: return  y + x;
        case 0xD: return -y + z;
        case 0xE: return  y - x;
        case 0xF: return -y - z;
    }
}

double Perlin::lerp(double t, double a, double b) {
    return a + t * (b - a);
}

// Based of the Ken Perlin's implementation of improved perlin noise.
double Perlin::noise(double x, double y, double z) {
    // Find unit cube that contains point
    int X = (int)x & 255;
    int Y = (int)y & 255;
    int Z = (int)z & 255;

    // Find relative point of X, Y, Z
    x -= (int)x;
    y -= (int)y;
    z -= (int)z;

    // Compute the fade curves for X, Y, Z
    double u = fade(x);
    double v = fade(y);
    double w = fade(z);

    // Hash coordinates of the 8 cube corners
    int A = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;

    // Blended results
    return lerp(w, 
                lerp(v, 
                    lerp(u, grad(p[AA], x, y, z),     grad(p[BA], x - 1, y, z)),
                    lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
                    lerp(v, 
                        lerp(u, grad(p[AA + 1], x, y, z - 1),     grad(p[BA + 1], x - 1, y, z - 1)),
                        lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));
}