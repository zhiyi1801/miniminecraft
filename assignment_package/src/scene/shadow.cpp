#include "shadow.h"
#include "mathutils.h"

ShadowMap::ShadowMap(OpenGLContext *_context):mp_context(_context)
{}

void ShadowMap::initialize()
{
    // create framebuffer and depth texture
    mp_context->glGenFramebuffers(1, &(this->m_depthMapFBO));
    mp_context->glGenTextures(1, &m_depthMap);
    mp_context->glBindTexture(GL_TEXTURE_2D, m_depthMap);
    mp_context->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                             SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // attach depth framebuffer to depth texture
    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    mp_context->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0);
    glDrawBuffer(GL_NONE);
    mp_context->glReadBuffer(GL_NONE);
    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint ShadowMap::getDepthMapTex()
{
    return this->m_depthMap;
}

GLuint ShadowMap::getDepthMapFBO()
{
    return this->m_depthMapFBO;
}

void ShadowMap::bindDepthMapFBO()
{
    mp_context->glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    mp_context->glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::unbindDepthMapFBO()
{
    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::bindDepthMapTex(unsigned int texSlot)
{
    mp_context->glActiveTexture(GL_TEXTURE0 + texSlot);
    mp_context->glBindTexture(GL_TEXTURE_2D, m_depthMap);
}

void ShadowMap::setShader(ShaderProgram *shader)
{
    this->m_shdowMapShader = shader;
}

ShaderProgram& ShadowMap::getShadowMapShader()
{
    return *(this->m_shdowMapShader);
}

int ShadowMap::getResolution()
{
    return this->SHADOW_WIDTH;
}

//void ShadowMap::setShadowMapWidthHeight(int _width, int _height)
//{
//    this->m_width = _width;
//    this->m_height = _height;
//}


// **********CSM**********

CascadedShadowMap::CascadedShadowMap(OpenGLContext *_context):mp_context(_context),
    m_lightMats(m_shadowCascadeLevels.size() + 1),
    m_lightViewMats(m_shadowCascadeLevels.size() + 1),
    m_lightOrthoMats(m_shadowCascadeLevels.size() + 1),
    m_frustrumPlanesInWorld(m_shadowCascadeLevels.size() + 1)
{
    for(size_t i = 0; i < m_frustrumPlanesInWorld.size(); ++i)
    {
        m_frustrumPlanesInWorld[i].reserve(6);
    }
}

void CascadedShadowMap::initialize()
{
    mp_context->glGenFramebuffers(1, &m_depthMapFBO);
    mp_context->glGenTextures(1, &m_depthMap);
    mp_context->glBindTexture(GL_TEXTURE_2D_ARRAY, m_depthMap);
    mp_context->glTexImage3D(
        GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, SHADOW_WIDTH, SHADOW_HEIGHT, int(m_shadowCascadeLevels.size()) + 1,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);


    mp_context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    mp_context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    mp_context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    mp_context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    mp_context->glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    mp_context->glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthMap, 0);
    glDrawBuffer(GL_NONE);
    mp_context->glReadBuffer(GL_NONE);

    int status = mp_context->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
        throw 0;
    }

    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mp_context->glGenBuffers(1, &m_matricesUBO);
    mp_context->glBindBuffer(GL_UNIFORM_BUFFER, m_matricesUBO);
    mp_context->glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 16, nullptr, GL_STATIC_DRAW);
    mp_context->glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_matricesUBO);
    mp_context->glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void CascadedShadowMap::setLightDir(const glm::vec3 &lightDir)
{
    this->m_lightDir = glm::normalize(lightDir);
}

GLuint CascadedShadowMap::getDepthMapTex()
{
    return this->m_depthMap;
}

GLuint CascadedShadowMap::getDepthMapFBO()
{
    return this->m_depthMapFBO;
}

void CascadedShadowMap::setShader(ShaderProgram *shader)
{
    this->m_shdowMapShader = shader;
}

void CascadedShadowMap::bindDepthMapFBO()
{
    mp_context->glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    mp_context->glClear(GL_DEPTH_BUFFER_BIT);
}

void CascadedShadowMap::unbindDepthMapFBO()
{
    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void CascadedShadowMap::bindDepthMapTex(unsigned int texSlot)
{
    mp_context->glActiveTexture(GL_TEXTURE0 + texSlot);
    mp_context->glBindTexture(GL_TEXTURE_2D_ARRAY, m_depthMap);
}

ShaderProgram& CascadedShadowMap::getShadowMapShader()
{
    return *(this->m_shdowMapShader);
}

void CascadedShadowMap::setCamra(const Camera *cam)
{
    this->m_cam = cam;
}

void CascadedShadowMap::setSun(const Sun *sun)
{
    this->m_sun = sun;
}

int CascadedShadowMap::getResolution()
{
    return this->SHADOW_WIDTH;
}

std::vector<glm::vec4> CascadedShadowMap::getFrustumCornersWorldSpace()
{
    const glm::mat4 NDC2World = viewMatInverse(m_cam->getView()) * perspectiveInverse(m_cam->getProj());

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt = NDC2World * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}

std::vector<glm::vec4> CascadedShadowMap::getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
    const glm::mat4 NDC2World = viewMatInverse(view) * perspectiveInverse(proj);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int z = 0; z < 2; ++z)
    {
        for (unsigned int x = 0; x < 2; ++x)
        {
            for (unsigned int y = 0; y < 2; ++y)
            {
                    const glm::vec4 pt = NDC2World * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}

#define USE_SPHERE false

#if USE_SPHERE

glm::mat4 CascadedShadowMap::getLightSpaceMatrix(const float nearPlane, const float farPlane)
{
    const glm::mat4 projMat = glm::perspective(
        glm::radians(m_cam->getFov()), m_cam->getAspect(), nearPlane,
        farPlane);

    const std::vector<glm::vec4> corners = getFrustumCornersWorldSpace(projMat, m_cam->getView());

    glm::vec3 center = getSphereCenter(corners[0], corners[4], corners[5], corners[6]);
    float radius = glm::length(center - glm::vec3(corners[0]));
    radius = glm::length(center - glm::vec3(corners[1]));
    radius = glm::length(center - glm::vec3(corners[2]));
    radius = glm::length(center - glm::vec3(corners[3]));
    radius = glm::length(center - glm::vec3(corners[4]));
    radius = glm::length(center - glm::vec3(corners[5]));
    radius = glm::length(center - glm::vec3(corners[6]));
    radius = glm::length(center - glm::vec3(corners[7]));

    float unitLen = 2.0f * radius / this->getResolution();
    center /= unitLen;
    center = glm::floor(center) * unitLen;

    const auto lightView = glm::lookAt(center + m_lightDir, center, m_sun->m_cam.getUp());

    float minX = -1.f * radius;
    float maxX = radius;

    float minY = -1.f * radius;
    float maxY = radius;

    float minZ = -1.f * radius;
    float maxZ = radius;

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

#else
glm::mat4 CascadedShadowMap::getLightSpaceMatrix(const float nearPlane, const float farPlane, int layer)
{
    const glm::mat4 projMat = glm::perspective(
        glm::radians(m_cam->getFov()), m_cam->getAspect(), nearPlane,
        farPlane);

    const std::vector<glm::vec4> corners = getFrustumCornersWorldSpace(projMat, m_cam->getView());

    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const auto& v : corners)
    {
        center += glm::vec3(v);
    }
    center /= corners.size();

    float farDist = glm::length(corners[4]- corners[7]);
    float crossDist = glm::length(corners[0]- corners[5]);
    float viewPortSize = glm::max(farDist, crossDist);
    float unitLen = viewPortSize / this->getResolution();

    glm::vec4 centerInLightSpace = glm::lookAt(glm::vec3(0.f)+m_lightDir, -1.f * m_lightDir, m_sun->m_cam.getUp()) * glm::vec4(center, 1.0f);
    centerInLightSpace /= unitLen;
    centerInLightSpace = glm::floor(centerInLightSpace) * unitLen;
    centerInLightSpace[3] = 1.f;
    center = glm::vec3(viewMatInverse(glm::lookAt(glm::vec3(0.f)+m_lightDir, -1.f * m_lightDir, m_sun->m_cam.getUp())) * centerInLightSpace);

    const auto lightView = glm::lookAt(center, center - m_lightDir, m_sun->m_cam.getUp());
//    qDebug() << glm::acos(glm::dot(m_lightDir, glm::vec3(0,0,1.f)));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : corners)
    {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    // Tune this parameter according to the scene
    constexpr float zMult = 10.0f;
    if (minZ < 0)
    {
        //minZ *= zMult;
        minZ = glm::abs(minZ * zMult - minZ) < 50 ? minZ - 50 : minZ * zMult;
    }
    else
    {
        //minZ /= zMult;
        minZ = glm::abs(minZ = minZ - minZ/zMult) < 50 ? minZ - 50 : minZ/zMult;
    }
    if (maxZ < 0)
    {
        //maxZ /= zMult;
        maxZ = glm::abs(maxZ - maxZ / zMult) < 50 ? maxZ + 50 : maxZ / zMult;
    }
    else
    {
        //maxZ *= zMult;
        maxZ = glm::abs(maxZ - maxZ * zMult) < 50 ? maxZ + 50 : maxZ * zMult;
    }

    minX = minX / unitLen;
    minX = glm::floor(minX) * unitLen;
    minY = minY / unitLen;
    minY = glm::floor(minY) * unitLen;
    minZ = minZ / unitLen;
    minZ = glm::floor(minZ) * unitLen;

    maxX = (minX + viewPortSize) / unitLen;
    maxX = glm::round(maxX) * unitLen;
    maxY = (minY + viewPortSize) / unitLen;
    maxY = glm::round(maxY) * unitLen;
    maxZ = maxZ / unitLen;
    maxZ = glm::floor(maxZ) * unitLen;

    this->m_shdowWidth.push_back(viewPortSize);
    this->m_shdowZLen.push_back(maxZ - minZ);
    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

    m_lightMats[layer] = lightProjection * lightView;
    m_lightViewMats[layer] = lightView;
    m_lightOrthoMats[layer] = lightProjection;

    return lightProjection * lightView;
}
#endif // USE_SPHERE

void CascadedShadowMap::getLightSpaceMatrices()
{
    for (size_t i = 0; i < m_shadowCascadeLevels.size() + 1; ++i)
    {
        if (i == 0)
        {
            getLightSpaceMatrix(NEAR_PLANE, m_shadowCascadeLevels[i], i);
        }
        else if (i < m_shadowCascadeLevels.size())
        {
            getLightSpaceMatrix(m_shadowCascadeLevels[i - 1], m_shadowCascadeLevels[i], i);
        }
        else
        {
            getLightSpaceMatrix(m_shadowCascadeLevels[i - 1], FAR_PLANE, i);
        }
    }
}

void CascadedShadowMap::updateLightSpaceMats()
{
    this->m_shdowWidth.clear();
    this->m_shdowZLen.clear();
    this->getLightSpaceMatrices();

    mp_context->glBindBuffer(GL_UNIFORM_BUFFER, m_matricesUBO);
    for (size_t i = 0; i < m_lightMats.size(); ++i)
    {
        mp_context->glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &m_lightMats[i]);
    }
    mp_context->glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void CascadedShadowMap::updateFrustrumPlanes()
{
    for(size_t ifrustrum = 0; ifrustrum < m_frustrumPlanesInWorld.size(); ++ifrustrum)
    {
        std::vector<glm::vec4> HomoPoints(2);
        std::vector<glm::vec4> PointsInView(2);
        glm::mat4 OrthoInvMat = orthoMatInverse(m_lightOrthoMats[ifrustrum]);
        glm::mat4 temp = glm::mat4(0);
        temp = OrthoInvMat * m_lightOrthoMats[ifrustrum];
        glm::mat4 ViewInvMat = viewMatInverse(m_lightViewMats[ifrustrum]);
        HomoPoints[0] = glm::vec4(1, 1, -1, 1);//pmax
        HomoPoints[1] = glm::vec4(-1, -1, 1, 1);//pmin


        // (x,y,z,1)
        for(int i = 0; i < PointsInView.size(); ++i)
        {
            PointsInView[i] = OrthoInvMat * HomoPoints[i];
        }

        float Right = PointsInView[0].x;
        float Left = PointsInView[1].x;
        float Top= PointsInView[0].y;
        float Bottom = PointsInView[1].y;
        float NearPlane = PointsInView[0].z;
        float FarPlane = PointsInView[1].z;

        std::vector<glm::vec4> PlanesView(6);
        PlanesView[0] = glm::vec4(0, 0, -1, -NearPlane); //near plane
        PlanesView[1] = glm::vec4(0, 0, 1, FarPlane); //far plane
        PlanesView[2] = glm::vec4(-1, 0, 0, Right); //right plane
        PlanesView[3] = glm::vec4(1, 0, 0, -Left); //left plane
        PlanesView[4] = glm::vec4(0, -1, 0, Top); //top plane
        PlanesView[5] = glm::vec4(0, 1, 0, -Bottom); //bottom plane

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
            this->m_frustrumPlanesInWorld[ifrustrum][i] = glm::vec4(norInWorld, dInWorld);
        }
    }
}

void CascadedShadowMap::tick()
{
    setLightDir(-1.f * this->m_sun->m_cam.getForward());
    updateLightSpaceMats();
    updateFrustrumPlanes();
}
