#ifndef SHADOW_H
#define SHADOW_H

#include "glm_includes.h"
#include <openglcontext.h>
#include <shaderprogram.h>
#include "smartpointerhelp.h"
#include "drawable.h"
#include "camera.h"
#include "mathutils.h"
#include "sun.h"

class ShadowMap
{
private:
    const GLuint SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;

    // a framebuffer for shadow map
    GLuint m_depthMapFBO;

    // a 2D texture
    GLuint m_depthMap;

    // opengl context
    OpenGLContext *mp_context;

    ShaderProgram *m_shdowMapShader;

public:
    ShadowMap(OpenGLContext *_context);
    void initialize();
    GLuint getDepthMapTex();
    GLuint getDepthMapFBO();
    void bindDepthMapFBO();
    void unbindDepthMapFBO();
    void bindDepthMapTex(unsigned int texSlot);
    void setShader(ShaderProgram *shader);
    ShaderProgram& getShadowMapShader();
    int getResolution();
};

class CascadedShadowMap
{
private:
    const GLuint SHADOW_WIDTH = 8192, SHADOW_HEIGHT = 8192;

    const Camera *m_cam;

    const Sun *m_sun;

    // a framebuffer for shadow map
    GLuint m_depthMapFBO;

    // a 2D texture
    GLuint m_depthMap;

    GLuint m_matricesUBO;

    // opengl context
    OpenGLContext *mp_context;

    ShaderProgram *m_shdowMapShader;

    float m_cameraFarPlane = FAR_PLANE;

    glm::vec3 m_lightDir;

public:
    const std::vector<float> m_shadowCascadeLevels{ m_cameraFarPlane / 10.0f, m_cameraFarPlane / 4.0f, m_cameraFarPlane/2.f/*, m_cameraFarPlane / 2.0f */};
    std::vector<glm::mat4> m_lightMats;
    std::vector<glm::mat4> m_lightViewMats;
    std::vector<glm::mat4> m_lightOrthoMats;
    std::vector<float> m_shdowWidth{};
    std::vector<float> m_shdowZLen{};

    //for frustrum clip
    std::vector<std::vector<glm::vec4>> m_frustrumPlanesInWorld;

    CascadedShadowMap(OpenGLContext *_context);
    void initialize();
    GLuint getDepthMapTex();
    GLuint getDepthMapFBO();
    void bindDepthMapFBO();
    void unbindDepthMapFBO();
    void bindDepthMapTex(unsigned int texSlot);
    void setShader(ShaderProgram *shader);
    ShaderProgram& getShadowMapShader();
    int getResolution();

    void setCamra(const Camera *cam);
    void setLightDir(const glm::vec3 &lightDir);
    void setSun(const Sun *sun);
    std::vector<glm::vec4> getFrustumCornersWorldSpace();
    std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
    glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane, int layer);
    void getLightSpaceMatrices();

    void updateLightSpaceMats();
    void updateFrustrumPlanes();
    void tick();
};

#endif // SHADOW_H
