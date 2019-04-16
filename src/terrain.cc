#include "terrain.h"

Chunk::Chunk(const glm::ivec2& pos, int size, std::mt19937& gen,
             Terrain* terrian) {
    this->tex_seed = gen();
    this->pos = pos;
    this->size = size;
}

std::vector<float> Chunk::heightMap(float min, float max) const {
    float delta = max - min;
    std::vector<float> noise = this->perlinNoise(this->tex_seed);

    for (auto& elem : noise) {
        elem = delta * elem + min;
    }
    return noise;
}

std::vector<float> Chunk::perlinNoise(uint64_t seed) const {
    std::mt19937 gen(seed);
    glm::vec2 grads[9];
    std::vector<float> result;
    std::uniform_real_distribution<> dis(1.0, 8.0);
    for (int i = 0; i < 9; i++) {
        grads[i] = glm::vec2(cos(gen()), sin(gen()));
        result.push_back(dis(gen));
    }

    return result;
}

glm::ivec2 Terrain::toChunkCoords(glm::vec3 coords) const {
    return glm::ivec2((int)coords.x / this->size, (int)coords.z / this->size);
}

std::vector<glm::vec3> Terrain::genChunkSurface(glm::ivec2 chunkCoords,
                                                glm::vec2 heights) {
    const Chunk& chunk = this->getChunk(chunkCoords);
    std::vector<float> heightMap = chunk.heightMap(heights.x, heights.y);
    std::vector<glm::vec3> surfaceMap;
    surfaceMap.resize(this->size * this->size);

    for (int i = 0; i < this->size; i++) {
        for (int j = 0; j < this->size; j++) {
            int index = i + this->size * j;
            glm::vec3 coords(chunk.pos.x + i, heightMap[index],
                             chunk.pos.y + j);
            surfaceMap[index] = coords;
        }
    }

    return surfaceMap;
}

std::vector<glm::vec3> Terrain::getSurfaceForRender(glm::vec3 pos,
                                                    glm::vec2 heights) {
    glm::ivec2 center = this->toChunkCoords(pos);
    int distance = 5;
    std::vector<glm::ivec2> chunkCoords;
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            chunkCoords.push_back(center + glm::ivec2(i, j));
        }
    }

    std::vector<glm::vec3> surfaceMap;
    surfaceMap.reserve((distance * this->size) * (distance * this->size));

    for (const auto& c : chunkCoords) {
        std::vector<glm::vec3> cOffsets = this->genChunkSurface(c, heights);
        for (auto& offset : cOffsets) {
            offset += glm::vec3(c.x, 0.0, c.y) * (float)(this->size - 1);
        }
        surfaceMap.insert(surfaceMap.end(), cOffsets.begin(), cOffsets.end());
    }

    return surfaceMap;
}

const Chunk& Terrain::getChunk(glm::ivec2 chunkCoords) {
    auto chunk = this->chunkMap.find(chunkCoords);
    if (chunk == this->chunkMap.end()) {
        Chunk c = Chunk(chunkCoords, this->size, this->gen, this);
        auto status = this->chunkMap.insert({chunkCoords, c});
        return status.first->second;
    } else {
        return chunk->second;
    }
}