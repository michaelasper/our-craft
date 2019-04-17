#ifndef NOISE_H
#define NOISE_H

#include <random>
#include <vector>

class JavaRandom;

class Noise {
   public:
    Noise();
    Noise(JavaRandom& gen);
    double compute(double x, double y);
    double fade(double t);

   private:
    uint8_t p[512];
};

class OctaveNoise {
    std::vector<Noise> noises;

   public:
    OctaveNoise(){};
    OctaveNoise(int octaves, JavaRandom& gen);
    double compute(double x, double y);
};

class CombinedNoise {
    OctaveNoise noise1;
    OctaveNoise noise2;

   public:
    CombinedNoise() : noise1(), noise2() {}
    CombinedNoise(OctaveNoise noise1, OctaveNoise noise2)
        : noise1(noise1), noise2(noise2) {}

    double compute(double x, double y) {
        double offset = noise2.compute(x, y);
        return noise1.compute(x + offset, y);
    }
};

class JavaRandom {
    long seed;
    const long value = 0x5DEECE66DL;
    const long mask = (1L << 48) - 1;

   public:
    JavaRandom(int seed) { SetSeed(seed); }
    void SetSeed(int seed) { this->seed = (seed ^ value) & mask; }

    int Next(int min, int max) { return min + Next(max - min); }

    int Next(int n) {
        if ((n & -n) == n) {
            seed = (seed * value + 0xBL) & mask;
            long raw = (long)((unsigned long)seed >> (48 - 31));
            return (int)((n * raw) >> 31);
        }

        int bits, val;
        do {
            seed = (seed * value + 0xBL) & mask;
            bits = (int)((unsigned long)seed >> (48 - 31));
            val = bits % n;
        } while (bits - val + (n - 1) < 0);
        return val;
    }

    float NextFloat() {
        seed = (seed * value + 0xBL) & mask;
        int raw = (int)((unsigned long)seed >> (48 - 24));
        return raw / ((float)(1 << 24));
    }
};

#endif
