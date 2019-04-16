#include "terrain.h"
// #include "perlin.h"

Chunk::Chunk(const glm::ivec2& pos, int size, std::mt19937& gen,
             Terrain* terrian) {
    this->tex_seed = gen();
    this->pos = pos;
    this->size = size;

    for (int i = 0; i < 4; ++i) {
        float theta = gen();
        gradients.push_back(glm::vec2(sin(theta), cos(theta)));
    }
    // std::cout << pos.x << " " << pos.y << std::endl;

    if (pos == glm::ivec2(0, 0)) return;
    // std::cout << pos.x << " " << pos.y << std::endl;
    if (pos.x > 0) {
        const Chunk& C = terrian->getChunk(pos + glm::ivec2(-1, 0));
        gradients[0] = C.gradients[1];
        gradients[2] = C.gradients[3];
    } else if (pos.x < 0) {  // Replace right edge
        const Chunk& C = terrian->getChunk(pos + glm::ivec2(1, 0));
        gradients[1] = C.gradients[0];
        gradients[3] = C.gradients[2];
    }

    if (pos.y > 0) {
        const Chunk& C = terrian->getChunk(pos + glm::ivec2(0, -1));
        gradients[0] = C.gradients[2];
        gradients[1] = C.gradients[3];
    } else if (pos.y < 0) {
        const Chunk& C = terrian->getChunk(pos + glm::ivec2(0, 1));
        gradients[2] = C.gradients[0];
        gradients[3] = C.gradients[1];
    }
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
    // glm::vec2 grads[16];
    std::vector<float> result;
    std::uniform_real_distribution<> dis(1.0, 3.0);
    for (int i = 0; i < 16 * 16; i++) {
        // grads[i] = glm::vec2(cos(gen()), sin(gen()));
        result.push_back(sin(i * (3.14 / 16)) * 3.14);
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
            double height =
                p.noise(chunk.pos.x + i, chunk.pos.y + j, chunk.gradients) *
                    .5 +
                .5;
            // std::cout << height << std::endl;
            glm::vec3 coords(chunk.pos.x + i, height, chunk.pos.y + j);
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

std::vector<glm::vec3> Terrain::getSurfaceForRender(glm::vec3 pos,
                                                    glm::vec2 heights) {
    glm::ivec2 center = this->toChunkCoords(pos);
    int distance = 5;
    std::vector<glm::vec3> surfaceMap;
    surfaceMap.resize(5 * 5 * this->size * this->size);
    std::fill(surfaceMap.begin(), surfaceMap.end(),
              glm::vec3(0.0f, -1000.0f, 0.0f));

    std::vector<glm::ivec2> chunk;
    for (int i = -2; i < 3; i++) {
        for (int j = -2; j < 3; j++) {
            chunk.push_back(center + glm::ivec2(i, j));
        }
    }

    for (auto& c : chunk) {
        std::vector<glm::vec3> chunkSurfacePos =
            this->genChunkSurface(c, heights);
        for (auto& offset : chunkSurfacePos) {
            offset += glm::vec3(c.x, 0.0, c.y) * (float)(this->size - 1);
        }
        surfaceMap.insert(surfaceMap.end(), chunkSurfacePos.begin(),
                          chunkSurfacePos.end());
    }

    fill(surfaceMap, this->size);

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