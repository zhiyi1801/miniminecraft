#include "procgen.h"

// ---------ProcGen---------

int ProcGen::getHeight(int x, int z)
{
    float grass = getGrassland(x, z);
    float mtn = getMountain(x, z);

    double perlin = fbm2D((x + 32.7f)/2048.f, (z + 432.5f)/2048.f);
    perlin = glm::smoothstep(0.25, 0.75, perlin);

    return glm::mix(grass, mtn, perlin);
    //return mtn;
}

int ProcGen::getGrassland(int x, int z)
{
    float fx = x + 443322;
    float fz = z + 774499;

    float ret = 50 + (100 - 50) * worleyNoise2D(fx,fz);
    return ret;
}

int ProcGen::getGrassland(glm::vec2 p)
{
    return getGrassland(p.x, p.y);
}

int ProcGen::getMountain(int x, int z)
{
    float fx = (x + 332215) / 256.0f;
    float fz = (z + 443312) / 256.0f;
    float turbu = 1;

    float ret = 30 + (150 - 30) * fbm2D(fx, fz);
    return ret;
}

int ProcGen::getMountain(glm::vec2 p)
{
    return getMountain(p.x, p.y);
}

inline float ProcGen::fade5(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline float ProcGen::fade3(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

// return [0,1]
float ProcGen::fbm2D(float x, float z, float turbu, NoiseType noiseType)
{
    float ret = 0;
    int octaves = 8;
    float freq = 2 * turbu;
    float amp = 0.5;

    // 2^8 = 256, default max_height to be 255
    for(int i = 0; i < octaves; ++i)
    {
        switch(noiseType)
        {
        case VALUE:
            ret += valueNoise2D(x * freq, z * freq) * amp;
            break;
        case PERLIN:
            ret += (perlinNoise2D(x * freq, z * freq)+1) / 2 * amp;
            break;
        case BPERLIN:
            ret += glm::abs(bperlinNoise2D(x * freq, z * freq)) * amp;
            break;
        default:
            ret += valueNoise2D(x * freq, z * freq) * amp;
            break;
        }
        freq *= 2;
        amp *= 0.5;
    }

    ret += 1/(1 << octaves);
    return ret;
}

float ProcGen::fbm2D(glm::vec2 p, float turbu, NoiseType noiseType)
{
    return fbm2D(p.x, p.y, turbu, noiseType);
}

float ProcGen::valueNoise2D(glm::vec2 p)
{
    glm::vec2 corner = glm::vec2(glm::floor(p.x), glm::floor(p.y));
    glm::vec2 fracVec = glm::vec2(glm::fract(p.x), glm::fract(p.y));

    //r2--r3
    // | p |
    //r0--r1
    float r0 = random2D(corner);
    float r1 = random2D(corner + glm::vec2(1.f, .0f));
    float r2 = random2D(corner + glm::vec2(.0f, 1.f));
    float r3 = random2D(corner + glm::vec2(1.f, 1.f));

    fracVec.x = fade5(fracVec.x);
    fracVec.y = fade5(fracVec.y);

    return glm::mix(glm::mix(r0, r1, fracVec.x), glm::mix(r2, r3, fracVec.x), fracVec.y);
}

float ProcGen::valueNoise2D(float x, float z)
{
    return valueNoise2D(glm::vec2(x, z));
}

float ProcGen::perlinNoise2D(glm::vec2 p)
{
    glm::ivec2 pi = glm::floor(p);
    glm::vec2 pf = glm::fract(p);

    pi = pi & 255;

    float v[4];
//    int a = permutation[permutation[pi.x & 255] + (pi.y & 255)],
//        b = permutation[permutation[(pi.x + 1) & 255] + (pi.y & 255)],
//        c = permutation[permutation[(pi.x) & 255] + ((pi.y + 1) & 255)],
//        d = permutation[permutation[(pi.x + 1) & 255] + ((pi.y + 1) & 255)];

    v[0] = grad(permutation[permutation[pi.x] + (pi.y)], pf.x, pf.y);
    v[1] = grad(permutation[permutation[(pi.x + 1)] + (pi.y)], pf.x - 1, pf.y);
    v[2] = grad(permutation[permutation[(pi.x)] + (pi.y + 1)], pf.x, pf.y - 1);
    v[3] = grad(permutation[permutation[(pi.x + 1)] + (pi.y + 1)], pf.x - 1, pf.y - 1);

    pf.x = fade5(pf.x);
    pf.y = fade5(pf.y);
    return glm::mix(glm::mix(v[0], v[1], pf.x), glm::mix(v[2], v[3], pf.x), pf.y);
}

float ProcGen::perlinNoise2D(float x, float z)
{
    return perlinNoise2D(glm::vec2(x, z));
}

float ProcGen::random2D(glm::vec2 p)
{
    return glm::fract(glm::sin(glm::dot(p + glm::vec2(127.1, 532.22), glm::vec2(127.1, 311.7))) * 43758.5453123);
}


float ProcGen::random2D(float x, float z)
{
    return random2D(glm::vec2(x, z));
}

glm::vec2 ProcGen::random2D2D(glm::vec2 p)
{
    return glm::vec2(abs((sin(glm::dot(p, glm::vec2(79.19, 213.1)))) ),
                     abs((sin(glm::dot(p, glm::vec2(40.01, 119.7)))) ));
}

float ProcGen::grad(int hash, float x, float z)
{
    switch(hash & 3)
    {
    case 0:
        return x + z;
        break;
    case 1:
        return -x + z;
        break;
    case 2:
        return x - z;
        break;
    case 3:
        return -x - z;
        break;
    default:
        return 0;
        break;
    }
}

float ProcGen::worleyNoise2D(glm::vec2 p) {
    p /= 128;
    p = p + fbm2D(p / 16.f);
    glm::vec2 integer = glm::floor(p);
    glm::vec2 fac = glm::fract(p);
    glm::vec2 closest;
    float min1 = 2.f;
    float min2 = 2.f;
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            glm::vec2 other = glm::vec2(i, j);
            glm::vec2 randomOther = random2D2D(other + integer);
            float  distance = glm::length(other + randomOther - fac);
            if(distance < min1) {
                min2 = min1;
                min1 = distance;
                closest = randomOther;
            } else if (distance < min2) {
                min2 = distance;
            }
        }
    }
    float h = min2 - min1;
    //    h = glm::max(0.f, h - 0.1f) / 0.9f;

    return glm::smoothstep(0.f, 1.f, h);
}

float ProcGen::worleyNoise2D(float x, float z)
{
    return worleyNoise2D(glm::vec2(x, z));
}

float ProcGen::surflet(glm::vec2 p, glm::vec2 gridPoint)
{
    float distX = abs(p.x - gridPoint.x);
    float distY = abs(p.y - gridPoint.y);
    float tX = 1 - fade5(distX);
    float tY = 1 - fade5(distY);

    glm::vec2 gradient = 2.f * random2D2D(gridPoint) - glm::vec2(1.f);

    glm::vec2 diff = p - gridPoint;

    float height = glm::dot(diff, gradient);

    return height * tX * tY;
}

float ProcGen::bperlinNoise2D(glm::vec2 uv)
{
    float surfletSum = .0f;

    for(int dx = 0; dx <= 1; ++dx)
    {
        for(int dy = 0; dy <= 1; ++dy)
        {
            surfletSum += surflet(uv, glm::floor(uv) + glm::vec2(dx, dy));
        }
    }

    return surfletSum;
}

float ProcGen::bperlinNoise2D(float x, float z)
{
    return bperlinNoise2D(glm::vec2(x, z));
}
