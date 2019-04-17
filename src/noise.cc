#include "noise.h"
#include <functional>

Noise::Noise(JavaRandom& gen) {
    // fisher-yates
    for (size_t i = 0; i < 256; i++) {
        p[i] = i;
    }

    for (size_t i = 0; i < 256; i++) {
        char j = gen.Next(1, 256);
        char temp = p[i];
        p[i] = p[j];
        p[j] = temp;
    }
    for (size_t i = 0; i < 256; i++) {
        p[i + 256] = p[i];
    }
}

double Noise::fade(double t) {
    // Fade function defined by Ken Perlin
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double Noise::compute(double x, double y) {
    int xFloor = x >= 0 ? (int)x : (int)x - 1;
    int yFloor = y >= 0 ? (int)y : (int)y - 1;
    int X = xFloor & 0xFF, Y = yFloor & 0xFF;
    x -= xFloor;
    y -= yFloor;

    double u = fade(x);  // Fade(x)
    double v = fade(y);  // Fade(y)

    int A = p[X] + Y, B = p[X + 1] + Y;

    // god fucking mode
    const int xFlags = 0x46552222, yFlags = 0x2222550A;

    int hash = (p[p[A]] & 0xF) << 1;
    double g22 = (((xFlags >> hash) & 3) - 1) * x +
                 (((yFlags >> hash) & 3) - 1) * y;  // Grad(p[p[A], x, y)
    hash = (p[p[B]] & 0xF) << 1;
    double g12 = (((xFlags >> hash) & 3) - 1) * (x - 1) +
                 (((yFlags >> hash) & 3) - 1) * y;  // Grad(p[p[B], x - 1, y)
    double c1 = g22 + u * (g12 - g22);

    hash = (p[p[A + 1]] & 0xF) << 1;
    double g21 =
        (((xFlags >> hash) & 3) - 1) * x +
        (((yFlags >> hash) & 3) - 1) * (y - 1);  // Grad(p[p[A + 1], x, y - 1)
    hash = (p[p[B + 1]] & 0xF) << 1;
    double g11 = (((xFlags >> hash) & 3) - 1) * (x - 1) +
                 (((yFlags >> hash) & 3) - 1) *
                     (y - 1);  // Grad(p[p[B + 1], x - 1, y - 1)
    double c2 = g21 + u * (g11 - g21);

    return c1 + v * (c2 - c1);
}

OctaveNoise::OctaveNoise(int octaves, JavaRandom& gen) {
    for (int i = 0; i < octaves; i++) {
        noises.push_back(Noise(gen));
    }
}

double OctaveNoise::compute(double x, double y) {
    double amplitude = 1, frequency = 1;
    double sum = 0;
    for (int i = 0; i < noises.size(); i++) {
        sum += noises[i].compute(x * frequency, y * frequency) * amplitude;
        amplitude *= 2.0;
        frequency *= 0.5;
    }
    return sum;
}