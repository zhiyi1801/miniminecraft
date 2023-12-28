#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <random> // necessary for generation of random floats (for sample kernel and noise texture)
#include <QElapsedTimer>


std::array<int, 72000> chunkMeshIdx{};

MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
    m_worldAxes(this),
    m_progLambert(this), m_progFlat(this), m_progInstanced(this),
    m_progShadowMap(this),m_progDebugDepthQuad(this),m_progDebugDepthViewer(this),m_progBlinnShadow(this),m_progLambertShadow(this),
    m_progCSMDepthGen(this), m_progDebugCSMQuad(this), m_progCSMLambert(this),
    m_progGbuffer(this), m_progGbufferWater(this), m_progGbufferDebug(this), m_progDeferredRender(this),
    m_progSSAO(this), m_progSSAOblur(this),
    m_terrainTexture(nullptr), m_terrain(this),
    m_noiseTexture128(nullptr),
    m_player(glm::vec3(-250.f, 149.f, 152.f), m_terrain),
    m_sunShadow(this), m_sunCSM(this),
    m_shader_time(0),
    m_sun(glm::vec3(0.f, 0.f, -1000.f), &m_player)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.f, 0.f, 0.f, 1);

    printGLErrorLog();

    initializeShaderTexture();

    initializeShadowMap();

    initializeSSAObuffer();

    // initialize chunkMeshIdx
    for(unsigned int i = 0; i < chunkMeshIdx.size(); i += 6)
    {
        int start = (i/6) * 4;
        chunkMeshIdx[i] = start;
        chunkMeshIdx[i + 1] = start + 1;
        chunkMeshIdx[i + 2] = start + 2;

        chunkMeshIdx[i + 3] = start;
        chunkMeshIdx[i + 4] = start + 2;
        chunkMeshIdx[i + 5] = start + 3;
    }

    //m_terrain.createSceneByPosition(glm::ivec3(m_player.getPos()));
    m_terrain.multiThreadUpdateTerrainGenerateRadius(glm::ivec3(m_player.getPos()));

    prePos = glm::ivec3(m_player.getPos());
    prePos.x = glm::floor(prePos.x / 64.0f) * 64;
    prePos.z = glm::floor(prePos.z / 64.0f) * 64;

    return;
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);
    m_progLambert.setModelMatrix(glm::mat4());

    initializeGbuffer();
    initializeSSAObuffer();
    m_player.updateProjPlanes();

    printGLErrorLog();
}

void MyGL::initializeShaderTexture()
{
    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    m_progShadowMap.create(":/glsl/shadow/shadow_sample.vert.glsl", ":/glsl/shadow/shadow_sample.frag.glsl");
    m_progDebugDepthQuad.create(":/glsl/shadow/shadow_debug_quad.vert.glsl", ":/glsl/shadow/shadow_debug_quad.frag.glsl");
    m_progBlinnShadow.create(":/glsl/shadow/shadow_render.vert.glsl", ":/glsl/shadow/shadow_render.frag.glsl");
    m_progLambertShadow.create(":/glsl/shadow/lambert_shadow.vert.glsl", ":/glsl/shadow/lambert_shadow.frag.glsl");
    m_progDebugDepthViewer.create(":/glsl/shadow/shadow_debug_viewer.vert.glsl", ":/glsl/shadow/shadow_debug_viewer.frag.glsl");

    //CSM
    m_progCSMDepthGen.create(":/glsl/shadow/CSM/CSM_depth_gen.vert.glsl", ":/glsl/shadow/CSM/CSM_depth_gen.frag.glsl", ":/glsl/shadow/CSM/CSM_depth_gen.geom.glsl");
    m_progDebugCSMQuad.create(":/glsl/shadow/CSM/CSM_debug_quad.vert.glsl", ":/glsl/shadow/CSM/CSM_debug_quad.frag.glsl");
    m_progCSMLambert.create(":/glsl/shadow/CSM/CSM_Lambertian.vert.glsl", ":/glsl/shadow/CSM/CSM_Lambertian.frag.glsl");

    //Deferred render
    m_progGbuffer.create(":/glsl/deffer/gbuffer.vert.glsl", ":/glsl/deffer/gbuffer.frag.glsl");
    m_progGbufferWater.create(":/glsl/deffer/gbuffer_water.vert.glsl", ":/glsl/deffer/gbuffer_water.frag.glsl");
    m_progDeferredRender.create(":/glsl/deffer/composite.vert.glsl", ":/glsl/deffer/composite.frag.glsl");
    m_progGbufferDebug.create(":/glsl/deffer/gbuffer_debug.vert.glsl", ":/glsl/deffer/gbuffer_debug.frag.glsl");

    //SSAO
    m_progSSAO.create(":/glsl/SSAO/ssao.vert.glsl", ":/glsl/SSAO/ssao.frag.glsl");
    m_progSSAOblur.create(":/glsl/SSAO/ssao_blur.vert.glsl", ":/glsl/SSAO/ssao_blur.frag.glsl");

    m_terrainTexture = mkS<Texture>(this);
    m_terrainTexture->create(":/textures/minecraft_textures_all.png");
    m_terrainTexture->load(0);

    m_noiseTexture128 = mkS<Texture>(this);
    m_noiseTexture128->create(":/textures/noise_texture_128.png", QImage::Format_Grayscale8);
    m_noiseTexture128->load(0, GL_RED, GL_BGRA, GL_UNSIGNED_BYTE, GL_REPEAT);

    m_waterNormalTexture128 = mkS<Texture>(this);
    m_waterNormalTexture128->create(":/textures/water_normal.png", QImage::Format_RGB888);
    m_waterNormalTexture128->load(0, GL_RGB, GL_BGRA, GL_UNSIGNED_BYTE, GL_REPEAT, GL_LINEAR);

    // Set a color with which to draw geometry.
    // This will ultimately not be used when you change
    // your program to render Chunks with vertex colors
    // and UV coordinates
    m_progFlat.setInt("u_Sampler2D", 0);

    m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));
    m_progLambert.setInt("u_Sampler2D", 0);

    m_progDebugDepthQuad.setInt("shadowMap", 1);
    m_progDebugDepthViewer.setInt("shadowMap", 1);

    m_progBlinnShadow.setInt("shadowMap", 1);
    m_progBlinnShadow.setInt("diffuseTexture", 0);

    m_progLambertShadow.setInt("shadowMap", 1);
    m_progLambertShadow.setInt("diffuseTexture", 0);
    m_progLambertShadow.setFloat("u_FarPlane", static_cast<float>(FAR_PLANE));
    m_progLambertShadow.setFloat("u_CamViewSize", ORTHO_HEIGHT * 2);

    m_progCSMDepthGen.setMat4("u_Model", glm::mat4());

    m_progDebugCSMQuad.setInt("shadowMap", 1);

    m_progCSMLambert.setInt("diffuseTexture", 0);
    m_progCSMLambert.setInt("shadowMap", 1);
    m_progCSMLambert.setFloat("u_FarPlane", static_cast<float>(FAR_PLANE));

    m_progGbuffer.setInt("diffuseTexture", 0);
    m_progGbuffer.setInt("specularTexture", 1);

    m_progGbufferWater.setInt("diffuseTexture", 0);
    m_progGbufferWater.setInt("specularTexture", 1);
    m_progGbufferWater.setInt("noiseTexture128", 2);
    m_progGbufferWater.setInt("depthNoWater", 3);
    m_progGbufferWater.setInt("waterNormalTexture", 4);

    m_progGbufferDebug.setInt("gPosition", 0);
    m_progGbufferDebug.setInt("gNormal", 1);
    m_progGbufferDebug.setInt("gAlbedoSpec", 2);

    m_progDeferredRender.setInt("gPosition", 0);
    m_progDeferredRender.setInt("gNormal", 1);
    m_progDeferredRender.setInt("gAlbedoSpec", 2);
    m_progDeferredRender.setInt("shadowMap", 3);
    m_progDeferredRender.setInt("gSSAO", 4);
    m_progDeferredRender.setInt("depthAll", 5);
    m_progDeferredRender.setInt("depthNoWater", 6);
    m_progDeferredRender.setInt("noiseTexture128", 7);

    m_progSSAO.setInt("gPosition", 0);
    m_progSSAO.setInt("gNormal", 1);
    m_progSSAO.setInt("texNoise", 2);

    m_progSSAOblur.setInt("ssaoInput", 0);
    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);
}

void MyGL::initializeShadowMap()
{
    m_sunShadow.initialize();
    m_sunShadow.setShader(&m_progShadowMap);

    m_sunCSM.initialize();
    m_sunCSM.setCamra(&m_player.mcr_camera);
    m_sunCSM.setSun(&this->m_sun);
    m_sunCSM.setShader(&m_progCSMDepthGen);
    m_sunCSM.setLightDir(-1.f * m_sun.m_cam.getForward());
    m_sunCSM.updateLightSpaceMats();
    m_sunCSM.updateFrustrumPlanes();
}

void MyGL::initializeGbuffer()
{

    this->glDeleteFramebuffers(1, &gBuffer);
    this->glGenFramebuffers(1, &gBuffer);
    this->glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    float ratio = this->devicePixelRatio();
    glm::ivec2 screenWidthHeight = glm::ivec2(static_cast<int>(this->width() * ratio), static_cast<int>(this->height() * ratio));
    // - position buffer
    this->glDeleteTextures(1, &gPosition);
    this->glGenTextures(1, &gPosition);
    this->glBindTexture(GL_TEXTURE_2D, gPosition);
    this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidthHeight.x, screenWidthHeight.y, 0, GL_RGB, GL_FLOAT, NULL);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    this->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // - normal buffer
    this->glDeleteTextures(1, &gNormal);
    this->glGenTextures(1, &gNormal);
    this->glBindTexture(GL_TEXTURE_2D, gNormal);
    this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidthHeight.x, screenWidthHeight.y, 0, GL_RGBA, GL_FLOAT, NULL);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    this->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // - albedo and specular buffer
    this->glDeleteTextures(1, &gColorSpec);
    this->glGenTextures(1, &gColorSpec);
    this->glBindTexture(GL_TEXTURE_2D, gColorSpec);
    this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidthHeight.x, screenWidthHeight.y, 0, GL_RGBA, GL_FLOAT, NULL);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    this->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec, 0);

    this->glDeleteTextures(1, &gDepthAll);
    this->glGenTextures(1, &gDepthAll);
    this->glBindTexture(GL_TEXTURE_2D, gDepthAll);
    this->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, screenWidthHeight.x, screenWidthHeight.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    this->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepthAll, 0);

    this->glDeleteTextures(1, &gDepthWithoutWater);
    this->glGenTextures(1, &gDepthWithoutWater);
    this->glBindTexture(GL_TEXTURE_2D, gDepthWithoutWater);
    this->glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, screenWidthHeight.x, screenWidthHeight.y, 0, GL_RED, GL_FLOAT, NULL);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    this->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gDepthWithoutWater, 0);

    // - 告诉OpenGL我们将要使用(帧缓冲的)哪种颜色附件来进行渲染
    GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    this->glDrawBuffers(4, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MyGL::initializeSSAObuffer()
{
    float ratio = this->devicePixelRatio();
    glm::ivec2 screenWidthHeight = glm::ivec2(static_cast<int>(this->width() * ratio), static_cast<int>(this->height() * ratio));

    this->glDeleteFramebuffers(1, &ssaoFBO);
    this->glGenFramebuffers(1, &ssaoFBO);
    this->glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    this->glDeleteTextures(1, &ssaoColorTexture);
    this->glGenTextures(1, &ssaoColorTexture);
    this->glBindTexture(GL_TEXTURE_2D, ssaoColorTexture);
    this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidthHeight.x, screenWidthHeight.y, 0, GL_RGB, GL_FLOAT, NULL);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorTexture, 0);

    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoNoise;
    for (GLuint i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
    this->glDeleteTextures(1, &ssaoNoiseTexture);
    this->glGenTextures(1, &ssaoNoiseTexture);
    this->glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
    this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //ssao blur
    this->glDeleteFramebuffers(1, &ssaoBlurFBO);
    this->glGenFramebuffers(1, &ssaoBlurFBO);
    this->glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    this->glDeleteTextures(1, &ssaoBlurTexture);
    this->glGenTextures(1, &ssaoBlurTexture);
    this->glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidthHeight.x, screenWidthHeight.y, 0, GL_RGB, GL_FLOAT, NULL);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurTexture, 0);

    this->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    // Update the player's position and velocity based on its acceleration

    // get the dt
    QDateTime currentTime = QDateTime::currentDateTime();
    float dt = m_time.msecsTo(currentTime)/300.f;
    m_dt = dt * 300 / 1000;
    m_time = currentTime;

    m_inputs = InputBundle();
    m_player.tick(dt,m_inputs);

    glm::ivec3 curPos = glm::ivec3(glm::floor(m_player.getPos()));
    //m_terrain.updateVBObyPosition(curPos, prePos);
    m_terrain.multiThreadUpdateTerrainGenerateRadius(curPos);
    m_terrain.multiThreadUpdateTerrainDrawRadius(curPos, 2);

    m_sunTime = m_sun.tick();

    m_sunCSM.tick();

    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data

    prePos = curPos;

    return;
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
    emit sig_sentPlayerFps(QString::fromStdString(std::to_string(static_cast<int>(glm::floor(1.f/m_dt))) + "/s"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gBufferPass();
    ssaoPass();
    gBufferWaterPass();
    //renderGbufferDebug();
    terrainPass();

    m_progLambert.setTime(m_shader_time);
    m_shader_time++;

}

void MyGL::gBufferPass()
{
    this->glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // - 告诉OpenGL我们将要使用(帧缓冲的)哪种颜色附件来进行渲染
    GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    this->glDrawBuffers(4, attachments);

    float ratio = this->devicePixelRatio();
    glm::ivec2 widthHeight = glm::ivec2(static_cast<int>(this->width() * ratio), static_cast<int>(this->height() * ratio));
    this->glViewport(0, 0, widthHeight.x, widthHeight.y);

    m_progGbuffer.setModelMatrix(glm::mat4());
    m_progGbuffer.setMat4("u_ViewProj", m_player.mcr_camera.getViewProj());
    m_progGbuffer.setMat4("u_View", m_player.mcr_camera.getView());
    m_progGbuffer.setMat4("u_Proj", m_player.mcr_camera.getProj());
    m_progGbuffer.setFloat("u_Near", m_player.mcr_camera.getNearPlane());
    m_progGbuffer.setFloat("u_Far", m_player.mcr_camera.getFarPlane());

    m_terrainTexture->bind(0);
    m_terrainTexture->bind(1);

    glm::ivec3 pos = glm::ivec3(glm::floor(m_player.getPos()));
    int minX = glm::floor(pos.x / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxX = glm::floor(pos.x / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    int minZ = glm::floor(pos.z / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxZ = glm::floor(pos.z / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);

    const Camera &cam = m_player.mcr_camera;
    std::vector<std::vector<glm::vec4>> tempList;
    tempList.push_back(cam.m_projPlanesInWorld);
    m_terrain.draw(minX, maxX, minZ, maxZ, &m_progGbuffer, tempList);

    this->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MyGL::gBufferWaterPass()
{
    this->glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    float ratio = this->devicePixelRatio();
    glm::ivec2 widthHeight = glm::ivec2(static_cast<int>(this->width() * ratio), static_cast<int>(this->height() * ratio));
    this->glViewport(0, 0, widthHeight.x, widthHeight.y);

    m_progGbufferWater.setModelMatrix(glm::mat4());
    m_progGbufferWater.setMat4("u_ViewProj", m_player.mcr_camera.getViewProj());
    m_progGbufferWater.setMat4("u_Proj", m_player.mcr_camera.getProj());
    m_progGbufferWater.setFloat("u_Near", m_player.mcr_camera.getNearPlane());
    m_progGbufferWater.setFloat("u_Far", m_player.mcr_camera.getFarPlane());
    m_progGbufferWater.setFloat("u_Time", (static_cast<float>(this->m_sunTime) / m_sun.getCycleTime()) * 24.f);
    m_progGbufferWater.setMat4("u_Proj", m_player.mcr_camera.getProj());
    m_progGbufferWater.setMat4("u_ProjInv", perspectiveInverse(m_player.mcr_camera.getProj()));
    m_progGbufferWater.setMat4("u_ViewInv", viewMatInverse(m_player.mcr_camera.getView()));
    m_progGbufferWater.setMat4("u_View", m_player.mcr_camera.getView());

    m_terrainTexture->bind(0);
    m_terrainTexture->bind(1);
    m_noiseTexture128->bind(2);
    bindTexture(gDepthWithoutWater, 3);
    m_waterNormalTexture128->bind(4);

    glm::ivec3 pos = glm::ivec3(glm::floor(m_player.getPos()));
    int minX = glm::floor(pos.x / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxX = glm::floor(pos.x / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    int minZ = glm::floor(pos.z / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxZ = glm::floor(pos.z / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);

    const Camera &cam = m_player.mcr_camera;
    std::vector<std::vector<glm::vec4>> tempList;
    tempList.push_back(cam.m_projPlanesInWorld);

    // - 告诉OpenGL我们将要使用(帧缓冲的)哪种颜色附件来进行渲染

    m_terrain.drawTrans(minX, maxX, minZ, maxZ, &m_progGbufferWater, tempList);

    this->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MyGL::terrainPass()
{
    //renderShadowMap();
    //renderTerrainLambert();
    //renderShadowDebugQuad();
    //renderTerrainBlinnShadow();
    //renderTerrainLambertShadow();
    //renderShadowDebugViewer();

    //CSM
    renderCSMShadowMap();
    //renderCSMLambert();
    //renderCSMDebugQuad();

    //Deferred render
    renderDeferred();
    //renderGbufferDebug();

    //    glDisable(GL_DEPTH_TEST);
    //    m_progFlat.setModelMatrix(glm::mat4());
    //    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    //    m_progFlat.draw(m_worldAxes);
    //    glEnable(GL_DEPTH_TEST);
}

void MyGL::ssaoPass()
{
    this->glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    float ratio = this->devicePixelRatio();
    glm::ivec2 widthHeight = glm::ivec2(static_cast<int>(this->width() * ratio), static_cast<int>(this->height() * ratio));
    this->glViewport(0, 0, widthHeight.x, widthHeight.y);

    // random vector
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (GLuint i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        GLfloat scale = GLfloat(i) / 64.0;

        // Scale samples s.t. they're more aligned to center of kernel
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // Send kernel + rotation
    for (GLuint i = 0; i < 64; ++i)
    {
        m_progSSAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    }
    m_progSSAO.setMat4("u_View", m_player.mcr_camera.getView());
    m_progSSAO.setMat4("u_Projection", m_player.mcr_camera.getProj());
    m_progSSAO.setMat4("u_Model", glm::mat4());
    m_progSSAO.setFloat("u_ScreenWidth", widthHeight.x);
    m_progSSAO.setFloat("u_ScreenHeight", widthHeight.y);

    //set texture
    this->bindTexture(gPosition, 0);
    this->bindTexture(gNormal, 1);
    this->bindTexture(ssaoNoiseTexture, 2);

    m_progSSAO.useMe();
    this->renderQuad();

    this->glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    this->glViewport(0, 0, widthHeight.x, widthHeight.y);

    this->bindTexture(ssaoColorTexture, 0);
    m_progSSAOblur.useMe();
    this->renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)
void MyGL::renderTerrainLambert() {
    m_terrainTexture->bind(0);
    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setModelMatrix(glm::mat4());
    m_progFlat.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    glm::ivec3 pos = glm::ivec3(glm::floor(m_player.getPos()));
    int minX = glm::floor(pos.x / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxX = glm::floor(pos.x / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    int minZ = glm::floor(pos.z / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxZ = glm::floor(pos.z / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    m_terrain.draw(minX, maxX, minZ, maxZ, &m_progLambert);
    m_terrain.drawTrans(minX, maxX, minZ, maxZ, &m_progLambert);
}

void MyGL::renderTerrainLambertShadow()
{
    m_progLambertShadow.setModelMatrix(glm::mat4());
    m_progLambertShadow.setMat4("u_LightSpaceMatrix", m_sun.m_cam.getViewOrtho());
    m_progLambertShadow.setMat4("u_ViewProj", m_player.mcr_camera.getViewProj());
    m_progLambertShadow.setVec3("u_LightVec", glm::vec3(0.0f) - m_sun.m_cam.getForward());
    m_progLambert.setMat4("u_ViewProj", m_player.mcr_camera.getViewProj());
    m_progLambertShadow.setBool("u_showDepth", m_showDepth);
    m_progLambert.setModelMatrix(glm::mat4());

    float sunRad = -1.f * m_sun.m_cam.getForward()[1];
    sunRad = glm::asin(sunRad);
    m_progLambertShadow.setFloat("u_SunRad", sunRad);

    m_terrainTexture->bind(0);
    m_sunShadow.bindDepthMapTex(1);

    glm::ivec3 pos = glm::ivec3(glm::floor(m_player.getPos()));
    int minX = glm::floor(pos.x / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxX = glm::floor(pos.x / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    int minZ = glm::floor(pos.z / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxZ = glm::floor(pos.z / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    m_terrain.draw(minX, maxX, minZ, maxZ, &m_progLambertShadow);
    //m_terrain.drawTrans(minX, maxX, minZ, maxZ, &m_progLambert);
}

void MyGL::renderTerrainBlinnShadow()
{
    m_progBlinnShadow.setModelMatrix(glm::mat4());
    m_progBlinnShadow.setMat4("u_LightSpaceMatrix", m_player.mcr_camera.getViewOrtho());
    m_progBlinnShadow.setMat4("u_ViewProj", m_player.mcr_camera.getViewProj());
    m_progBlinnShadow.setVec3("u_LightPos", m_player.mcr_camera.mcr_position);
    m_progBlinnShadow.setVec3("u_ViewPos", m_player.mcr_camera.mcr_position);

    m_terrainTexture->bind(0);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, m_sunShadow.getDepthMapTex());

    glm::ivec3 pos = glm::ivec3(glm::floor(m_player.getPos()));
    int minX = glm::floor(pos.x / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxX = glm::floor(pos.x / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    int minZ = glm::floor(pos.z / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxZ = glm::floor(pos.z / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    m_terrain.draw(minX, maxX, minZ, maxZ, &m_progBlinnShadow);
    m_terrain.drawTrans(minX, maxX, minZ, maxZ, &m_progBlinnShadow);
}

void MyGL::renderShadowMap()
{
    m_sunShadow.bindDepthMapFBO();
    m_sunShadow.getShadowMapShader().setMat4("u_Model", glm::mat4());
    m_sunShadow.getShadowMapShader().setMat4("u_LightSpaceMatrix", m_sun.m_cam.getViewOrtho());

    glm::ivec3 pos = glm::ivec3(glm::floor(m_player.getPos()));
    int minX = glm::floor(pos.x / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxX = glm::floor(pos.x / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    int minZ = glm::floor(pos.z / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxZ = glm::floor(pos.z / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    m_terrain.draw(minX, maxX, minZ, maxZ, &m_progShadowMap);

    m_sunShadow.unbindDepthMapFBO();

    float ratio = this->devicePixelRatio();
    glm::ivec2 widthHeight = glm::ivec2(static_cast<int>(this->width() * ratio), static_cast<int>(this->height() * ratio));
    this->glViewport(0, 0, widthHeight.x, widthHeight.y);
}

void MyGL::renderShadowDebugQuad()
{
    // renderQuad() renders a 1x1 XY quad in NDC
    // -----------------------------------------
    m_progDebugDepthQuad.useMe();
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, m_sunShadow.getDepthMapTex());

    renderQuad();
}

void MyGL::renderCSMLambert()
{
    m_progCSMLambert.setMat4("u_Model", glm::mat4());
    m_progCSMLambert.setMat4("u_ViewProj", m_player.mcr_camera.getViewProj());
    m_progCSMLambert.setMat4("u_View", m_player.mcr_camera.getView());
    m_progCSMLambert.setVec3("u_LightVec", glm::vec3(0.0f) - m_sun.m_cam.getForward());
    m_progCSMLambert.setInt("cascadeCount", m_sunCSM.m_shadowCascadeLevels.size());
    m_progCSMLambert.setBool("u_showDepth", m_showDepth);
    m_progCSMLambert.setBool("u_showLayer", m_showLayer);

    for(size_t i = 0; i < m_sunCSM.m_shadowCascadeLevels.size() + 1; ++i)
    {
        if(i < m_sunCSM.m_shadowCascadeLevels.size())
        {
            m_progCSMLambert.setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", m_sunCSM.m_shadowCascadeLevels[i]);
        }
        m_progCSMLambert.setFloat("cascadePortWidth[" + std::to_string(i) + "]", m_sunCSM.m_shdowWidth[i]);
        m_progCSMLambert.setFloat("cascadeZLen[" + std::to_string(i) + "]", m_sunCSM.m_shdowZLen[i]);
    }

    m_progLambert.setMat4("u_ViewProj", m_player.mcr_camera.getViewProj());

    m_terrainTexture->bind(0);
    m_sunCSM.bindDepthMapTex(1);

    glm::ivec3 pos = glm::ivec3(glm::floor(m_player.getPos()));
    int minX = glm::floor(pos.x / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxX = glm::floor(pos.x / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    int minZ = glm::floor(pos.z / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxZ = glm::floor(pos.z / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    m_terrain.draw(minX, maxX, minZ, maxZ, &m_progCSMLambert);
    //m_terrain.drawTrans(minX, maxX, minZ, maxZ, &m_progLambert);
}

void MyGL::renderCSMShadowMap()
{
    m_sunCSM.bindDepthMapFBO();
    m_sunCSM.getShadowMapShader().setMat4("u_Model", glm::mat4());

    glCullFace(GL_FRONT);  // peter panning

    glm::ivec3 pos = glm::ivec3(glm::floor(m_player.getPos()));
    int minX = glm::floor(pos.x / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxX = glm::floor(pos.x / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    int minZ = glm::floor(pos.z / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxZ = glm::floor(pos.z / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    m_terrain.draw(minX, maxX, minZ, maxZ, &m_progCSMDepthGen, m_sunCSM.m_frustrumPlanesInWorld);

    glCullFace(GL_BACK);
    m_sunCSM.unbindDepthMapFBO();

    float ratio = this->devicePixelRatio();
    glm::ivec2 widthHeight = glm::ivec2(static_cast<int>(this->width() * ratio), static_cast<int>(this->height() * ratio));
    this->glViewport(0, 0, widthHeight.x, widthHeight.y);
}

void MyGL::renderShadowDebugViewer() {
    m_progDebugDepthViewer.setModelMatrix(glm::mat4());
    m_progDebugDepthViewer.setMat4("u_LightSpaceMatrix", m_sun.m_cam.getViewOrtho());
    m_progDebugDepthViewer.setMat4("u_ViewProj", m_player.mcr_camera.getViewProj());
    m_progDebugDepthViewer.setVec3("u_LightVec", glm::vec3(0.0f) - m_sun.m_cam.getForward());

    m_sunShadow.bindDepthMapTex(1);

    glm::ivec3 pos = glm::ivec3(glm::floor(m_player.getPos()));
    int minX = glm::floor(pos.x / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxX = glm::floor(pos.x / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    int minZ = glm::floor(pos.z / 64.0f) * 64 - 64 * TERRAIN_DRAW_RADIUS, maxZ = glm::floor(pos.z / 64.0f) * 64 + 64 * (TERRAIN_DRAW_RADIUS + 1);
    m_terrain.draw(minX, maxX, minZ, maxZ, &m_progDebugDepthViewer);
    m_terrain.drawTrans(minX, maxX, minZ, maxZ, &m_progDebugDepthViewer);
}


void MyGL::renderCSMDebugQuad()
{
    m_progDebugCSMQuad.useMe();
    int debugLayer = 1;
    m_progDebugCSMQuad.setInt("layer", debugLayer);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_sunCSM.getDepthMapTex());

    renderQuad();
}

void MyGL::renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(vao);
}

void MyGL::renderGbufferDebug()
{
    this->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_progGbufferDebug.useMe();
    m_progGbufferDebug.setInt("u_Layer", 2);

    float ratio = this->devicePixelRatio();
    glm::ivec2 widthHeight = glm::ivec2(static_cast<int>(this->width() * ratio), static_cast<int>(this->height() * ratio));
    this->glViewport(0, 0, widthHeight.x, widthHeight.y);

    bindTexture(this->gPosition, 0);
    bindTexture(this->gNormal, 1);
    bindTexture(this->gColorSpec, 2);

    renderQuad();
}

void MyGL::renderDeferred()
{
    m_progDeferredRender.useMe();
    m_progDeferredRender.setMat4("u_ViewProj", m_player.mcr_camera.getViewProj());
    m_progDeferredRender.setVec3("u_LightVec", glm::vec3(0.0f) - m_sun.m_cam.getForward());
    m_progDeferredRender.setInt("cascadeCount", m_sunCSM.m_shadowCascadeLevels.size());
    m_progDeferredRender.setBool("u_showDepth", m_showDepth);
    m_progDeferredRender.setBool("u_showLayer", m_showLayer);
    m_progDeferredRender.setBool("u_useSSAO", m_useSSAO);
    m_progDeferredRender.setInt("u_CycleTime", m_sun.getCycleTime());
    m_progDeferredRender.setInt("u_CurrentTime", this->m_sunTime);
    m_progDeferredRender.setFloat("u_FarPlane", m_player.mcr_camera.getFarPlane());
    m_progDeferredRender.setFloat("u_NearPlane", m_player.mcr_camera.getNearPlane());
    m_progDeferredRender.setFloat("u_Time", (static_cast<float>(this->m_sunTime) / m_sun.getCycleTime()) * 24.f);
    glm::vec3 sunDirInView = glm::vec3(m_player.mcr_camera.getView() * glm::vec4(m_sun.m_cam.getPos(), 1.f));
    sunDirInView = glm::normalize(sunDirInView);
    m_progDeferredRender.setVec3("u_SunDirView", sunDirInView);
//    bool isNight = this->m_sunTime/500000.f > 0.5;
//    m_progDeferredRender.setBool("u_IsNight", isNight);
    m_progDeferredRender.setMat4("u_Proj", m_player.mcr_camera.getProj());
    m_progDeferredRender.setMat4("u_ProjInv", perspectiveInverse(m_player.mcr_camera.getProj()));
    m_progDeferredRender.setMat4("u_ViewInv", viewMatInverse(m_player.mcr_camera.getView()));
    m_progDeferredRender.setMat4("u_View", m_player.mcr_camera.getView());
    m_progDeferredRender.setBool("u_EyeInWater", m_terrain.getBlockAt(m_player.getPos()));
    for(size_t i = 0; i < m_sunCSM.m_shadowCascadeLevels.size() + 1; ++i)
    {
        if(i < m_sunCSM.m_shadowCascadeLevels.size())
        {
            m_progDeferredRender.setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", m_sunCSM.m_shadowCascadeLevels[i]);
        }
        m_progDeferredRender.setFloat("cascadePortWidth[" + std::to_string(i) + "]", m_sunCSM.m_shdowWidth[i]);
        m_progDeferredRender.setFloat("cascadeZLen[" + std::to_string(i) + "]", m_sunCSM.m_shdowZLen[i]);
    }

    float ratio = this->devicePixelRatio();
    glm::ivec2 widthHeight = glm::ivec2(static_cast<int>(this->width() * ratio), static_cast<int>(this->height() * ratio));
    this->glViewport(0, 0, widthHeight.x, widthHeight.y);

    bindTexture(this->gPosition, 0);
    bindTexture(this->gNormal, 1);
    bindTexture(this->gColorSpec, 2);
    m_sunCSM.bindDepthMapTex(3);
    bindTexture(this->ssaoBlurTexture, 4);
    bindTexture(this->gDepthAll, 5);
    bindTexture(this->gDepthWithoutWater, 6);
    m_noiseTexture128->bind(7);

    renderQuad();
}

void MyGL::keyPressEvent(QKeyEvent *e) {

    float amount = 1.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount *= 3.0f;
    }

    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        m_player.rotateOnUpGlobal(-amount);
    } else if (e->key() == Qt::Key_Left) {
        m_player.rotateOnUpGlobal(amount);
    } else if (e->key() == Qt::Key_Up) {
        m_player.rotateOnRightLocal(-amount);
    } else if (e->key() == Qt::Key_Down) {
        m_player.rotateOnRightLocal(amount);
    } else if (e->key() == Qt::Key::Key_1) {
        this->m_showDepth = !this->m_showDepth;
    } else if (e->key() == Qt::Key::Key_2) {
        this->m_showLayer = !this->m_showLayer;
    } else if (e->key() == Qt::Key::Key_3) {
        this->m_useSSAO = !this->m_useSSAO;
    }

    m_inputs.wPressed = e->key() == Qt::Key_W;
    m_inputs.aPressed = e->key() == Qt::Key_A;
    m_inputs.sPressed = e->key() == Qt::Key_S;
    m_inputs.dPressed = e->key() == Qt::Key_D;
    m_inputs.qPressed = e->key() == Qt::Key_Q;
    m_inputs.ePressed = e->key() == Qt::Key_E;
    m_inputs.fPressed = e->key() == Qt::Key_F;

    m_inputs.spacePressed = e->key() == Qt::Key_Space;

    // call the player's handleInput function

    m_player.tick(amount, m_inputs);

    m_inputs = InputBundle();

}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    float centerX = width() / 2.0f;
    float centerY = height() / 2.0f;

    float mouseX = e->position().x();
    float mouseY = e->position().y();

    float deltaX = (centerX - mouseX)/width();
    float deltaY = (centerY - mouseY)/height();

    m_inputs.mouseX = mouseX;
    m_inputs.mouseY = mouseY;

    m_player.rotateOnUpGlobal(deltaX*360*0.05);
    m_player.rotateOnRightLocal(deltaY*360*0.05);

    moveMouseToCenter();
}

void MyGL::mousePressEvent(QMouseEvent *e) {

    // check if the left mouse is clicked

    //    m_inputs.mouseX = e->position().x();
    //    m_inputs.mouseY = e->position().y();

    //    m_inputs.leftClickPressed = e->button() == Qt::LeftButton;
    //    m_inputs.rightClickPressed = e->button() == Qt::RightButton;

    //    // a scaler indicates the units we look foward to place/destroy a block
    //    int view = 3;

    //    if (m_inputs.leftClickPressed){
    //        // shoot a ray from the cemera position toward the forward direction, go for three units
    //        // find the first unit that was hit, and then change the block to empty
    //        // if no block was hit, then do nothing
    //        glm::vec3 ray = m_player.mcr_camera.mcr_position;
    //        for (int i = 0; i < view; i++){
    //            ray += m_player.getFoward();
    //            if (m_terrain.getBlockAt(ray.x, ray.y, ray.z) != EMPTY){
    //                m_terrain.setBlockAt(ray.x, ray.y, ray.z, EMPTY);
    //                m_terrain.getChunkAt(ray.x, ray.z)->setReadyToVBO(false);
    //                m_terrain.getChunkAt(ray.x, ray.z)->createVBOdata();
    //                break;
    //            }

    //        }
    //    }

    //    if (m_inputs.rightClickPressed){
    //        glm::vec3 ray = m_player.mcr_camera.mcr_position;
    //        ray += float(view)*m_player.getFoward();
    //        for (int i = 0; i < view; i++){
    //            if (m_terrain.getBlockAt(ray.x, ray.y, ray.z) == EMPTY){
    //                m_terrain.setBlockAt(ray.x, ray.y, ray.z, STONE);
    //                m_terrain.getChunkAt(ray.x, ray.z)->setReadyToVBO(false);
    //                m_terrain.getChunkAt(ray.x, ray.z)->createVBOdata();
    //                break;
    //            }
    //            ray -= m_player.getFoward();
    //        }
    //    }

    if (e->button() == Qt::LeftButton){
        m_player.removeBlock();
    }else if (e->button() == Qt::RightButton){
        m_player.addBlock();
    }

}

void MyGL::slot_testMakeLava()
{
    if (0) {
        std::cout << "make lava" << std::endl;
        std::cout << "playerPos:::" << glm::to_string(m_player.getPos()) << std::endl;
        glm::vec3 playerPos = m_player.getPos();
        BlockType testBlock = LAVA;
        m_terrain.setBlockAt(playerPos.x, playerPos.y, playerPos.z, testBlock);
    }
}

void MyGL::bindTexture(GLint textureID, int slot)
{
    this->glActiveTexture(GL_TEXTURE0 + slot);
    this->glBindTexture(GL_TEXTURE_2D, textureID);
}
