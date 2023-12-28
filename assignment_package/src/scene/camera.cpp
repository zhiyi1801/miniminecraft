#include "camera.h"
#include "glm_includes.h"
#include "mathutils.h"

Camera::Camera(glm::vec3 pos)
    : Camera(400, 400, pos)
{}

Camera::Camera(unsigned int w, unsigned int h, glm::vec3 pos)
    : Entity(pos), m_fovy(45), m_width(w), m_height(h),
    m_near_clip(NEAR_PLANE), m_far_clip(FAR_PLANE), m_aspect(w / static_cast<float>(h)), m_projPlanesInWorld(6), m_orthoPlanesInWorld(6)
{}

Camera::Camera(const Camera &c)
    : Entity(c),
      m_fovy(c.m_fovy),
      m_width(c.m_width),
      m_height(c.m_height),
      m_near_clip(c.m_near_clip),
      m_far_clip(c.m_far_clip),
      m_aspect(c.m_aspect)
{}


void Camera::setWidthHeight(unsigned int w, unsigned int h) {
    m_width = w;
    m_height = h;
    m_aspect = w / static_cast<float>(h);
}

glm::ivec2 Camera::getWidthHeight()const
{
    return glm::ivec2(m_width, m_height);
}

void Camera::tick(float dT, InputBundle &input) {
    // Do nothing
}

float Camera::getFov()const
{
    return this->m_fovy;
}

float Camera::getAspect()const
{
    return this->m_aspect;
}

glm::mat4 Camera::getViewProj() const {
    return glm::perspective(glm::radians(m_fovy), m_aspect, m_near_clip, m_far_clip) * glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 Camera::getViewOrtho() const
{
    return glm::ortho(-1.f * ORTHO_WIDTH, 1.f * ORTHO_WIDTH, -1.f * ORTHO_HEIGHT, 1.f * ORTHO_HEIGHT, m_near_clip, m_far_clip) * glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 Camera::getView() const
{
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 Camera::getProj() const
{
    return glm::perspective(glm::radians(m_fovy), m_aspect, m_near_clip, m_far_clip);
}

glm::vec3 Camera::getForward() const
{
    return this->m_forward;
}

glm::vec3 Camera::getRight() const
{
    return this->m_right;
}

glm::vec3 Camera::getUp() const
{
    return this->m_up;
}

float Camera::getNearPlane() const
{
    return this->m_near_clip;
}

float Camera::getFarPlane() const
{
    return this->m_far_clip;
}

void Camera::updateProjPlanes()
{
    std::vector<glm::vec4> HomoPoints(6);
    std::vector<glm::vec4> PointsInView(6);
    glm::mat4 ProjInvMat = perspectiveInverse(this->getProj());
    glm::mat4 ViewInvMat = viewMatInverse(this->getView());
    HomoPoints[0] = glm::vec4(1, 0, 1, 1);//right farplane
    HomoPoints[1] = glm::vec4(-1, 0, 1, 1);//left
    HomoPoints[2] = glm::vec4(0, 1, 1, 1);//top
    HomoPoints[3] = glm::vec4(0, -1, 1, 1);//low
    HomoPoints[4] = glm::vec4(0, 0, -1, 1);//near
    HomoPoints[5] = glm::vec4(0, 0, 1, 1);//far

    // (x/z, y/z, 1, 1/z)
    for(int i = 0; i < 6; ++i)
    {
        PointsInView[i] = ProjInvMat * HomoPoints[i];
    }

    float RightSlope = PointsInView[0].x;
    float LeftSlope = PointsInView[1].x;
    float TopSlope = PointsInView[2].y;
    float BottomSlope = PointsInView[3].y;
    float NearPlane = 1 / PointsInView[4].w;
    float FarPlane = 1 / PointsInView[5].w;

    std::vector<glm::vec4> PlanesView(6);
    PlanesView[0] = glm::vec4(0, 0, -1, -NearPlane); //near plane
    PlanesView[1] = glm::vec4(0, 0, 1, FarPlane); //far plane
    PlanesView[2] = glm::vec4(-1, 0, -RightSlope, 0); //right plane
    PlanesView[3] = glm::vec4(1, 0, LeftSlope, 0); //left plane
    PlanesView[4] = glm::vec4(0, -1, -TopSlope, 0); //top plane
    PlanesView[5] = glm::vec4(0, 1, BottomSlope, 0); //bottom plane

    for(int i = 0; i < 6; ++i)
    {
        glm::vec3 normalInView = glm::vec3(PlanesView[i]);
        float d = PlanesView[i].w;
        // don't have to normalize, we need the length of this vector
//        normalInView = glm::normalize(normalInView);
        glm::vec3 norInWorld = glm::mat3(ViewInvMat) * glm::vec3(normalInView);
        norInWorld = glm::normalize(norInWorld);

        glm::vec4 pointInView = glm::vec4(-normalInView * d / glm::length2(normalInView), 1.f);
        glm::vec4 pointInWorld = ViewInvMat * pointInView;
        float dInWorld = -glm::dot(glm::vec3(pointInWorld), norInWorld);
        this->m_projPlanesInWorld[i] = glm::vec4(norInWorld, dInWorld);
    }
}

void Camera::updateOrthoPlanes()
{

}
