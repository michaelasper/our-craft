#include "terrain.h"
// #include "perlin.h"
#include <algorithm>
#include "noise.h"

Chunk::Chunk(const glm::ivec2& pos, int size, std::mt19937& gen,
             Terrain* terrain) {
    this->gen = gen;
    this->tex_seed = gen();
    this->pos = pos;
    this->size = size;
    this->terrain = terrain;

    this->n1 =
        CombinedNoise(OctaveNoise(8, this->gen), OctaveNoise(8, this->gen));
    this->n2 =
        CombinedNoise(OctaveNoise(8, this->gen), OctaveNoise(8, this->gen));

    this->n3 = OctaveNoise(6, this->gen);
}

std::vector<float> Chunk::heightMap() {
    std::vector<float> heightMap;
    heightMap.resize(size * size);

    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            int index = x + z * size;
            double heightMin = n1.compute(x * 1.3f + (pos.x * size),
                                          z * 1.3f + (pos.y * size)) /
                                   6 -
                               4;
            double height = heightMin;

            if (n3.compute(x + (pos.x * size), z + (pos.y * size)) <= 0) {
                double heightMax = n2.compute(x * 1.3f + (pos.x * size),
                                              z * 1.3f + (pos.y * size)) /
                                       5 +
                                   6;
                height = std::max(heightMin, heightMax);
            }

            height *= 0.5;
            if (height < 0) height *= 0.8f;

            heightMap[index] = height;
        }
    }

    return heightMap;
}

glm::ivec2 Terrain::toChunkCoords(glm::vec3 coords) const {
    return glm::ivec2((int)coords.x / this->size, (int)coords.z / this->size);
}

std::vector<glm::vec3> Terrain::genChunkSurface(glm::ivec2 chunkCoords) {
    Chunk& chunk = this->getChunk(chunkCoords);
    std::vector<float> heightMap = chunk.heightMap();
    std::vector<glm::vec3> surfaceMap;
    surfaceMap.resize(this->size * this->size);

    for (int i = 0; i < this->size; i++) {
        for (int j = 0; j < this->size; j++) {
            int index = i + this->size * j;
            glm::vec3 coords(chunk.pos.x + i, round(heightMap[index]),
                             chunk.pos.y + j);
            surfaceMap[index] = coords;
        }
    }

    return surfaceMap;
}

void fill(std::vector<glm::vec3>& surfaceMap, int size) {
    for (int i = (int)surfaceMap.size() - 1; i >= 0; i--) {
        std::vector<int> neighbors = {i + 1, i - 1, i + size, i - size};

        for (int j : neighbors) {
            if (j < size * size && j > 0) {
                float gapSize =
                    floor(surfaceMap[i].y - surfaceMap[j].y - 0.001);
                if (gapSize <= 0.0) continue;
                for (int k = 1; k <= gapSize; k++) {
                    surfaceMap.push_back(glm::vec3(surfaceMap[i].x,
                                                   surfaceMap[i].y - (float)k,
                                                   surfaceMap[i].z));
                }
            }
        }
    }
}

std::vector<glm::vec3> Terrain::getSurfaceForRender(glm::vec3 pos) {
    glm::ivec2 center = this->toChunkCoords(pos);
    int distance = 5;
    std::vector<glm::vec3> surfaceMap;
    int mapSize = distance * size;
    surfaceMap.resize(mapSize * mapSize);
    ;

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            glm::ivec2 c(center + glm::ivec2(i - 2, j - 2));  // Chunk's indices
            std::vector<glm::vec3> cOffsets = this->genChunkSurface(c);
            for (auto& offset : cOffsets) {
                offset += glm::vec3(c.x, 0.0, c.y) * (float)(this->size - 1);
            }

            // Map into offsets at the right locations
            for (int cj = 0; cj < this->size; cj++) {
                for (int ci = 0; ci < this->size; ci++) {
                    int ind = ci + i * this->size + cj * mapSize +
                              j * mapSize * this->size;
                    surfaceMap[ind] = cOffsets[ci + this->size * cj];
                }
            }
        }
    }

    fill(surfaceMap, mapSize);

    while (surfaceMap.size() < 12000) {
        surfaceMap.emplace_back(0.0f, -1000.0f, 0.0f);
    }

    return surfaceMap;
}

Chunk& Terrain::getChunk(glm::ivec2 chunkCoords) {
    auto chunk = this->chunkMap.find(chunkCoords);
    if (chunk == this->chunkMap.end()) {
        Chunk c = Chunk(chunkCoords, this->size, this->gen, this);
        auto status = this->chunkMap.insert({chunkCoords, c});
        return status.first->second;
    } else {
        return chunk->second;
    }
}