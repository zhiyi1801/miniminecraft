#include "mathutils.h"

glm::mat4 perspectiveInverse(const glm::mat4 &perspectiveMat)
{
    glm::mat4 ret{};

    ret[0][0] = 1.f / perspectiveMat[0][0];
    ret[1][1] = 1.f / perspectiveMat[1][1];
    ret[2][2] = 0.f;
    ret[2][3] = 1.f / perspectiveMat[3][2];
    ret[3][2] = -1.f;
    ret[3][3] = perspectiveMat[2][2] / perspectiveMat[3][2];

    return ret;
}

// fovy is radian
glm::mat4 perspectiveInverse(float fovy, float aspect, float near_clip, float far_clip)
{
    glm::mat4 perspectiveMat = glm::perspective(fovy, aspect, near_clip, far_clip);

    glm::mat4 ret = perspectiveInverse(perspectiveMat);

    return ret;
}

glm::mat4 viewMatInverse(const glm::mat4 &viewMat)
{
    glm::mat3 rotateMat = glm::mat3(viewMat);
    glm::vec3 d = glm::vec3(viewMat[3]);

    glm::mat4 ret(glm::transpose(rotateMat));
    ret[3] = glm::vec4(-1.f * glm::transpose(rotateMat) * d, 1.f);

    return ret;
}

glm::mat4 viewMatInverse(const glm::vec3 &position, const glm::vec3 &center, const glm::vec3 &up)
{
    glm::mat4 viewMat = glm::lookAt(position, center, up);
    glm::mat4 ret = viewMatInverse(viewMat);
    return ret;
}

glm::mat4 orthoMatInverse(const glm::mat4 &orthoMat)
{
    glm::mat4 ret = orthoMat;

    ret[0][0] = 1/ret[0][0];
    ret[1][1] = 1/ret[1][1];
    ret[2][2] = 1/ret[2][2];

    ret[3][0] *= -ret[0][0];
    ret[3][1] *= -ret[1][1];
    ret[3][2] *= -ret[2][2];

    return ret;
}
