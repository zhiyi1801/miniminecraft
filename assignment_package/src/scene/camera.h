#pragma once
#include "glm_includes.h"
#include "scene/entity.h"
#include "smartpointerhelp.h"

//A perspective projection camera
//Receives its eye position and reference point from the scene XML file
class Camera : public Entity {
private:
    float m_fovy;
    unsigned int m_width, m_height;  // Screen dimensions
    float m_near_clip;  // Near clip plane distance
    float m_far_clip;  // Far clip plane distance
    float m_aspect;    // Aspect ratio

public:
    // for frustrum clip in cpu
    std::vector<glm::vec4> m_projPlanesInWorld;
    std::vector<glm::vec4> m_orthoPlanesInWorld;

    Camera(glm::vec3 pos);
    Camera(unsigned int w, unsigned int h, glm::vec3 pos);
    Camera(const Camera &c);
    void setWidthHeight(unsigned int w, unsigned int h);
    glm::ivec2 getWidthHeight()const;

    void tick(float dT, InputBundle &input) override;

    float getFov()const;
    float getAspect()const;
    glm::mat4 getViewProj() const;
    glm::mat4 getViewOrtho() const;
    glm::mat4 getView() const;
    glm::mat4 getProj() const;
    glm::vec3 getForward() const;
    glm::vec3 getRight()const;
    glm::vec3 getUp()const;
    float getNearPlane()const;
    float getFarPlane()const;

    void updateProjPlanes();
    void updateOrthoPlanes();
};
