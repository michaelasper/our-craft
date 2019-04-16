#include "perlin.h"

Perlin::Perlin() {
    // p = new int[512];
    for (int x = 0; x < 512; x++) {
        p[x] = permutation[x % 256];
    }
}

double Perlin::fade(double t) {
    // Fade function defined by Ken Perlin
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double Perlin::grad(int hash, double x, double y, double z) {
    const std::int32_t h = hash & 15;
    const double u = h < 8 ? x : y;
    const double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double Perlin::lerp(double t, double a, double b) { return a + t * (b - a); }

// Computes the dot product of the distance and gradient vectors.
float dotGridGradient(glm::vec2 grad, float x, float y) {
    float dx = x - (float)grad.x;
    float dy = y - (float)grad.y;

    // Compute the dot-product
    return (dx * grad.x + dy * grad.y);
}

// Based of the Ken Perlin's implementation of improved perlin noise.
double Perlin::noise(double x, double y, std::vector<glm::vec2> grads) {
    // Determine grid cell coordinates
    int x0 = (int)x;
    int x1 = x0 + 1;
    int y0 = (int)y;
    int y1 = y0 + 1;

    // Determine interpolation weights
    // Could also use higher order polynomial/s-curve here
    float sx = x - (float)x0;
    float sy = y - (float)y0;

    // Interpolate between grid point gradients
    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(grads[0], x, y);
    n1 = dotGridGradient(grads[1], x, y);
    ix0 = lerp(sx, n0, n1);

    n0 = dotGridGradient(grads[2], x, y);
    n1 = dotGridGradient(grads[3], x, y);
    ix1 = lerp(sx, n0, n1);

    value = lerp(sy, ix0, ix1);
    return value;
}