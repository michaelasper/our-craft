#include "terrain.h"
// #include "perlin.h"
#include <algorithm>
#include "noise.h"

#include <glm/glm.hpp>

Chunk::Chunk(const glm::ivec2& pos, int size, std::mt19937& gen,
             Terrain* terrain, int seed) {
    this->gen = gen;
    this->tex_seed = gen();
    this->pos = pos;
    this->size = size;
    this->terrain = terrain;
    JavaRandom rnd(seed);

    this->n1 = CombinedNoise(OctaveNoise(8, rnd), OctaveNoise(8, rnd));
    this->n2 = CombinedNoise(OctaveNoise(14, rnd), OctaveNoise(12, rnd));

    this->n3 = OctaveNoise(6, rnd);
}

std::vector<float> Chunk::heightMap() {
    std::vector<float> heightMap;
    heightMap.resize(size * size);

    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            int index = x + z * size;
            double heightMin =
                n1.compute(x + (pos.x * size), z + (pos.y * size)) / 6 - 4;
            double height = heightMin;

            // if (n3.compute(x + (pos.x * size), z + (pos.y * size)) <= 0) {
            double heightMax =
                n2.compute(x + (pos.x * size), z + (pos.y * size)) / 5 + 6;
            height = std::max(heightMin, heightMax);
            // }

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

    for (int z = 0; z < 4; z++) {
        std::vector<float> neighborNoise;
        int start, Nstart;
        int stride, Nstride;

        switch (z) {
            case 0:
                start = 0;
                Nstart = this->size * this->size - this->size;

                stride = 1;
                Nstride = 1;
                neighborNoise =
                    this->getChunk(chunkCoords + glm::ivec2(0, -1)).heightMap();
                break;
            case 1:
                start = 0;
                Nstart = this->size - 1;
                stride = this->size;
                Nstride = this->size;
                neighborNoise =
                    this->getChunk(chunkCoords + glm::ivec2(-1, 0)).heightMap();
                break;
            case 2:
                start = this->size - 1;
                Nstart = 0;
                stride = this->size;
                Nstride = this->size;
                neighborNoise =
                    this->getChunk(chunkCoords + glm::ivec2(1, 0)).heightMap();
                break;
            case 3:
                start = this->size * this->size - this->size;
                stride = 1;
                Nstart = 0;
                Nstride = 1;
                neighborNoise =
                    this->getChunk(chunkCoords + glm::ivec2(0, 1)).heightMap();
                break;
        }

        for (int i = 0; i < this->size; i++) {
            heightMap[i * stride + start] =
                glm::mix(heightMap[i * stride + start],
                         neighborNoise[i * Nstride + Nstart], .4);
        }
    }

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
    int distance = 9;
    std::vector<glm::vec3> surfaceMap;
    int mapSize = distance * size;
    surfaceMap.resize(mapSize * mapSize);
    ;

    for (int i = 0; i < distance; i++) {
        for (int j = 0; j < distance; j++) {
            glm::ivec2 c(center +
                         glm::ivec2(i - distance / 2, j - distance / 2));
            std::vector<glm::vec3> cOffsets = this->genChunkSurface(c);
            for (auto& offset : cOffsets) {
                offset += glm::vec3(c.x, 0.0, c.y) * (float)(this->size - 1);
            }

            for (int cj = 0; cj < this->size; cj++) {
                for (int ci = 0; ci < this->size; ci++) {
                    int index = ci + i * this->size + cj * mapSize +
                                j * mapSize * this->size;
                    surfaceMap[index] = cOffsets[ci + this->size * cj];
                }
            }
        }
    }

    fill(surfaceMap, mapSize);

    while (surfaceMap.size() < 50000) {
        surfaceMap.emplace_back(0.0f, -1000.0f, 0.0f);
    }

    return surfaceMap;
}

Chunk& Terrain::getChunk(glm::ivec2 chunkCoords) {
    auto chunk = this->chunkMap.find(chunkCoords);
    if (chunk == this->chunkMap.end()) {
        Chunk c =
            Chunk(chunkCoords, this->size, this->gen, this, this->chunkSeed);
        auto status = this->chunkMap.insert({chunkCoords, c});
        return status.first->second;
    } else {
        return chunk->second;
    }
}