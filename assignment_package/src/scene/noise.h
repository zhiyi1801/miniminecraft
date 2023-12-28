#ifndef NOISE_H
#define NOISE_H
#include <glm/glm.hpp>

#define GRASS_MIN 135
#define GRASS_MAX 180
#define MOUNTAIN_MIN 120
#define MOUNTAIN_MAX 240
#define HEIGHT_MAX 255
#define SEA_LEVEL 138
#define STONE_LEVEL 115
#define TERRAIN_DRAW_RADIUS 7
#define TERRAIN_CREATE_RADIUS 2

namespace Noise
{
    double perlinNoise2D(glm::vec2 xz);
    double fbmPerlin2D(glm::vec2 xz, int octaves = 8);

    double noise(glm::vec2 xz);
    double fbmNoise2D(glm::vec2 xz, int octaves = 8);

    inline float fade5(float t);
    inline float fade3(float t);

    inline glm::vec2 fade5(glm::vec2 p);
    inline glm::vec2 fade3(glm::vec2 p);

    glm::vec2 random2D2D(glm::vec2 p);

    double surflet(glm::vec2 P, glm::vec2 gridPoint);
};

#endif // NOISE_H
