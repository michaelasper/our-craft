#ifndef NOISE_H
#define NOISE_H

#include <random>
#include <vector>
class Noise {
   public:
    Noise();
    Noise(std::mt19937& gen);
    double compute(double x, double y);
    double fade(double t);

   private:
    uint8_t p[512];
};

class OctaveNoise {
    std::vector<Noise> noises;

   public:
    OctaveNoise(){};
    OctaveNoise(int octaves, std::mt19937& gen);
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

#endif
