#ifndef TERR_H
#define TERR_H

#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "noise.h"

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#define v3 std::vector<glm::vec3>

class Chunk;

class Terrain {
    std::mt19937 gen;
    int size = 16;
    std::unordered_map<glm::ivec2, Chunk, std::hash<glm::ivec2>,
                       std::equal_to<glm::ivec2>>
        chunkMap;

   public:
    // Perlin p = Perlin();

    Terrain(std::mt19937& gen) : gen(gen) {}
    Chunk& getChunk(glm::ivec2);
    v3 getSurfaceForRender(glm::vec3 camCoords);
    glm::ivec2 toChunkCoords(glm::vec3 coords) const;
    v3 genChunkSurface(glm::ivec2 chunkCoords);
};

class Chunk {
   public:
    int size;
    uint32_t tex_seed;
    glm::ivec2 pos;
    std::vector<float> heightMap();
    Terrain* terrain;
    std::mt19937 gen;

    std::vector<glm::vec2> gradients;

    std::vector<float> perlinNoise(uint64_t seed) const;
    Chunk(const glm::ivec2& pos, int extent, std::mt19937& gen,
          Terrain* terrain);

   private:
    CombinedNoise n1, n2;
    OctaveNoise n3;
};

#endif
