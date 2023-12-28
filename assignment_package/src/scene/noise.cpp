#include "noise.h"
#include <iostream>

glm::vec2 random2( glm::vec2 p ) {
    return glm::normalize(2.f * glm::fract(glm::sin(glm::vec2(glm::dot(p,glm::vec2(127.1,311.7)),glm::dot(p,glm::vec2(269.5,183.3))))*43758.5453f) - 1.f);
}

double Noise::perlinNoise2D(glm::vec2 xz)
{
    // Tile the space
    glm::vec2 xzXLYL = glm::floor(xz);
    glm::vec2 xzXHYL = xzXLYL + glm::vec2(1,0);
    glm::vec2 xzXHYH = xzXLYL + glm::vec2(1,1);
    glm::vec2 xzXLYH = xzXLYL + glm::vec2(0,1);

    return surflet(xz, xzXLYL) + surflet(xz, xzXHYL) + surflet(xz, xzXHYH) + surflet(xz, xzXLYH);
}

double Noise::surflet(glm::vec2 P, glm::vec2 gridPoint)
{
    // Compute falloff function by converting linear distance to a polynomial (quintic smootherstep function)
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = 1 - 6 * pow(distX, 5.0) + 15 * pow(distX, 4.0) - 10 * pow(distX, 3.0);
    float tY = 1 - 6 * pow(distY, 5.0) + 15 * pow(distY, 4.0) - 10 * pow(distY, 3.0);

    // Get the random vector for the grid point, why use random2?
    //**************************************************************************
    glm::vec2 gradient = random2(gridPoint);
    // Get the vector from the grid point to P
    glm::vec2 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = glm::dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;
}

double Noise::fbmPerlin2D(glm::vec2 xz, int octaves)
{
    float a = 0.5;
    float f = 5.0;
    float n = 0.;
    int it = octaves;
    for(int i = 0; i < it; i++)
    {
        n += perlinNoise2D(xz*f)*a;
        a *= .5;
        f *= 2.;
    }
    return n;
}

double Noise::fbmNoise2D(glm::vec2 xz, int octaves)
{
    float a = 0.5;
    float f = 5.0;
    float n = 0.;
    int it = octaves;
    for(int i = 0; i < octaves; i++)
    {
        n += noise(xz*f)*a;
        a *= .5;
        f *= 2.;
    }
    return n;
}

glm::vec2 Noise::random2D2D(glm::vec2 p)
{
    return glm::vec2(abs((sin(glm::dot(p, glm::vec2(79.19, 213.1)))) ),
                     abs((sin(glm::dot(p, glm::vec2(40.01, 119.7)))) ));
}

double Noise::noise(glm::vec2 xz)
{
    const float k = 257.;
    glm::vec4 l  = glm::vec4(glm::floor(xz),glm::fract(xz));
    float u = l.x + l.y * k;
    glm::vec4 v  = glm::vec4(u, u+1.,u+k, u+k+1.);
    v = glm::fract(glm::fract(1.23456789f * v) * v / 0.987654321f);

    l = glm::vec4(glm::vec2(l.x, l.y), fade3(glm::vec2(l.z, l.w)));
    l.x = glm::mix(v.x, v.y, l.z);
    l.y = glm::mix(v.z, v.w, l.z);
    return glm::mix(l.x, l.y, l.w);
}

inline float Noise::fade5(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline float Noise::fade3(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

inline glm::vec2 Noise::fade3(glm::vec2 p)
{
    return p * p * (3.0f - 2.0f * p);
}

inline glm::vec2 Noise::fade5(glm::vec2 p)
{
    return p * p * p * (p * (p * 6.0f - 15.0f) + 10.0f);
}
