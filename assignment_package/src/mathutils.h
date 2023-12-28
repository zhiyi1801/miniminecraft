#ifndef MATHUTILS_H
#define MATHUTILS_H

#include "glm_includes.h"

glm::mat4 perspectiveInverse(const glm::mat4 &perspectiveMat);

// fovy is radian
glm::mat4 perspectiveInverse(float fovy, float aspect, float near_clip, float far_clip);

glm::mat4 viewMatInverse(const glm::mat4 &viewMat);

glm::mat4 viewMatInverse(const glm::vec3 &position, const glm::vec3 &center, const glm::vec3 &up);

glm::mat4 orthoMatInverse(const glm::mat4 &orthoMat);

inline glm::vec3 getSphereCenter(float x1, float y1, float z1,
                              float x2, float y2, float z2,
                              float x3, float y3, float z3,
                                 float x4, float y4, float z4);

inline glm::vec3 getSphereCenter(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, const glm::vec3 &p4);

inline glm::vec3 getSphereCenter(const glm::vec4 &p1, const glm::vec4 &p2, const glm::vec4 &p3, const glm::vec4 &p4);

inline glm::vec3 getSphereCenter(float x1, float y1, float z1,
                                 float x2, float y2, float z2,
                                 float x3, float y3, float z3,
                                 float x4, float y4, float z4)
{
    float a11, a12, a13, a21, a22, a23, a31, a32, a33, b1, b2, b3, d, d1, d2, d3;
    a11 = 2 * (x2 - x1); a12 = 2 * (y2 - y1); a13 = 2 * (z2 - z1);
    a21 = 2 * (x3 - x2); a22 = 2 * (y3 - y2); a23 = 2 * (z3 - z2);
    a31 = 2 * (x4 - x3); a32 = 2 * (y4 - y3); a33 = 2 * (z4 - z3);
    b1 = x2 * x2 - x1 * x1 + y2 * y2 - y1 * y1 + z2 * z2 - z1 * z1;
    b2 = x3 * x3 - x2 * x2 + y3 * y3 - y2 * y2 + z3 * z3 - z2 * z2;
    b3 = x4 * x4 - x3 * x3 + y4 * y4 - y3 * y3 + z4 * z4 - z3 * z3;
    // Cramer's rule
    d = a11 * a22*a33 + a12 * a23*a31 + a13 * a21*a32 - a11 * a23*a32 - a12 * a21*a33 - a13 * a22*a31;
    d1 = b1 * a22*a33 + a12 * a23*b3 + a13 * b2*a32 - b1 * a23*a32 - a12 * b2*a33 - a13 * a22*b3;
    d2 = a11 * b2*a33 + b1 * a23*a31 + a13 * a21*b3 - a11 * a23*b3 - b1 * a21*a33 - a13 * b2*a31;
    d3 = a11 * a22*b3 + a12 * b2*a31 + b1 * a21*a32 - a11 * b2*a32 - a12 * a21*b3 - b1 * a22*a31;
    float x = d1 / d;
    float y = d2 / d;
    float z = d3 / d;
    return glm::vec3(x, y, z);
}


inline glm::vec3 getSphereCenter(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, const glm::vec3 &p4)
{
    glm::vec3 ret = getSphereCenter(p1.x, p1.y, p1.z,
                                    p2.x, p2.y, p2.z,
                                    p3.x, p3.y, p3.z,
                                    p4.x, p4.y, p4.z);
    return ret;
}

inline glm::vec3 getSphereCenter(const glm::vec4 &p1, const glm::vec4 &p2, const glm::vec4 &p3, const glm::vec4 &p4)
{
    glm::vec3 ret = getSphereCenter(p1.x, p1.y, p1.z,
                                    p2.x, p2.y, p2.z,
                                    p3.x, p3.y, p3.z,
                                    p4.x, p4.y, p4.z);
    return ret;
}

#endif // MATHUTILS_H
