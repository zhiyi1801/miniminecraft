#ifndef PROCGEN_H
#define PROCGEN_H

#include "glm_includes.h"

// ----------------ProcGen----------------
class ProcGen
{
public:
    enum NoiseType
    {
        VALUE, PERLIN, BPERLIN
    };
    static int getHeight(int x, int z);
    static float fbm2D(float x, float z, float turbu = 1.f, NoiseType noiseType = PERLIN);
    static float fbm2D(glm::vec2 p, float turbu = 1.f, NoiseType noiseType = PERLIN);
    static inline float fade5(float t);
    static inline float fade3(float t);

private:
    static constexpr int permutation[512] = {
        12, 68, 130, 195, 7, 32, 143, 90, 177, 84, 251, 109, 49, 147, 221, 203,
        62, 54, 136, 44, 110, 239, 155, 120, 72, 100, 37, 87, 194, 150, 180, 11,
        116, 169, 42, 29, 253, 118, 189, 201, 15, 217, 105, 225, 182, 66, 202, 23,
        13, 111, 38, 206, 96, 71, 79, 149, 246, 99, 215, 26, 210, 123, 174, 50,
        60, 24, 114, 165, 208, 213, 16, 231, 83, 148, 104, 57, 192, 35, 122, 70,
        33, 186, 1, 19, 175, 85, 132, 36, 58, 2, 41, 126, 167, 52, 161, 127,
        168, 172, 222, 163, 214, 188, 74, 0, 61, 124, 97, 245, 216, 80, 139, 129,
        27, 171, 8, 93, 113, 207, 125, 78, 187, 234, 28, 134, 135, 76, 244, 67,
        102, 48, 20, 73, 152, 106, 140, 94, 43, 198, 5, 82, 138, 242, 103, 31,
        88, 3, 117, 64, 211, 250, 173, 237, 197, 144, 191, 56, 230, 176, 75, 39,
        81, 30, 9, 226, 34, 45, 164, 220, 10, 128, 89, 179, 212, 6, 200, 243,
        47, 159, 235, 232, 227, 141, 178, 95, 53, 156, 249, 101, 223, 190, 65, 69,
        162, 133, 209, 228, 255, 233, 63, 55, 107, 199, 91, 240, 98, 131, 154, 185,
        185, 247, 77, 219, 157, 4, 142, 238, 18, 146, 236, 119, 224, 46, 14, 151,
        160, 193, 204, 40, 121, 115, 183, 229, 112, 196, 137, 17, 59, 166, 145, 254,
        153, 21, 86, 92, 170, 205, 22, 158, 108, 184, 218, 241, 248, 252, 25, 51,

        12, 68, 130, 195, 7, 32, 143, 90, 177, 84, 251, 109, 49, 147, 221, 203,
        62, 54, 136, 44, 110, 239, 155, 120, 72, 100, 37, 87, 194, 150, 180, 11,
        116, 169, 42, 29, 253, 118, 189, 201, 15, 217, 105, 225, 182, 66, 202, 23,
        13, 111, 38, 206, 96, 71, 79, 149, 246, 99, 215, 26, 210, 123, 174, 50,
        60, 24, 114, 165, 208, 213, 16, 231, 83, 148, 104, 57, 192, 35, 122, 70,
        33, 186, 1, 19, 175, 85, 132, 36, 58, 2, 41, 126, 167, 52, 161, 127,
        168, 172, 222, 163, 214, 188, 74, 0, 61, 124, 97, 245, 216, 80, 139, 129,
        27, 171, 8, 93, 113, 207, 125, 78, 187, 234, 28, 134, 135, 76, 244, 67,
        102, 48, 20, 73, 152, 106, 140, 94, 43, 198, 5, 82, 138, 242, 103, 31,
        88, 3, 117, 64, 211, 250, 173, 237, 197, 144, 191, 56, 230, 176, 75, 39,
        81, 30, 9, 226, 34, 45, 164, 220, 10, 128, 89, 179, 212, 6, 200, 243,
        47, 159, 235, 232, 227, 141, 178, 95, 53, 156, 249, 101, 223, 190, 65, 69,
        162, 133, 209, 228, 255, 233, 63, 55, 107, 199, 91, 240, 98, 131, 154, 185,
        185, 247, 77, 219, 157, 4, 142, 238, 18, 146, 236, 119, 224, 46, 14, 151,
        160, 193, 204, 40, 121, 115, 183, 229, 112, 196, 137, 17, 59, 166, 145, 254,
        153, 21, 86, 92, 170, 205, 22, 158, 108, 184, 218, 241, 248, 252, 25, 51
    };

    static int getMountain(int x, int z);
    static int getMountain(glm::vec2 p);

    static int getGrassland(int x, int z);
    static int getGrassland(glm::vec2 p);

    static float valueNoise2D(float x, float z);
    static float valueNoise2D(glm::vec2 p);

    static float perlinNoise2D(glm::vec2 p);
    static float perlinNoise2D(float x, float z);

    static float random2D(glm::vec2 p);
    static float random2D(float x, float z);

    static glm::vec2 random2D2D(glm::vec2 p);

    static float worleyNoise2D(glm::vec2 p);
    static float worleyNoise2D(float x, float z);

    static float grad(int hash, float x, float z);

    static float surflet(glm::vec2 p, glm::vec2 gridPoint);

    static float bperlinNoise2D(glm::vec2 uv);
    static float bperlinNoise2D(float x, float z);
};
#endif // PROCGEN_H
