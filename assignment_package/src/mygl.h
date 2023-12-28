#ifndef MYGL_H
#define MYGL_H

#include "openglcontext.h"
#include "shaderprogram.h"
#include "texture.h"
#include "scene/worldaxes.h"
#include "scene/camera.h"
#include "scene/terrain.h"
#include "scene/player.h"
#include "scene/shadow.h"
#include "scene/sun.h"

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <smartpointerhelp.h>
#include <QDateTime>

class MyGL : public OpenGLContext
{
    Q_OBJECT
private:
    WorldAxes m_worldAxes; // A wireframe representation of the world axes. It is hard-coded to sit centered at (32, 128, 32).
    ShaderProgram m_progLambert;// A shader program that uses lambertian reflection
    ShaderProgram m_progFlat;// A shader program that uses "flat" reflection (no shadowing at all)
    ShaderProgram m_progInstanced;// A shader program that is designed to be compatible with instanced rendering
    ShaderProgram m_progShadowMap;
    ShaderProgram m_progDebugDepthQuad;
    ShaderProgram m_progDebugDepthViewer;
    ShaderProgram m_progBlinnShadow;
    ShaderProgram m_progLambertShadow;

    //CSM
    ShaderProgram m_progCSMDepthGen;
    ShaderProgram m_progDebugCSMQuad;
    ShaderProgram m_progCSMLambert;

    //Defferred render
    ShaderProgram m_progGbuffer;
    ShaderProgram m_progGbufferWater;
    ShaderProgram m_progGbufferDebug;
    ShaderProgram m_progDeferredRender;

    //SSAO
    ShaderProgram m_progSSAO;
    ShaderProgram m_progSSAOblur;

    sPtr<Texture> m_terrainTexture;
    sPtr<Texture> m_noiseTexture128;
    sPtr<Texture> m_waterNormalTexture128;

    GLuint vao; // A handle for our vertex array object. This will store the VBOs created in our geometry classes.
                // Don't worry too much about this. Just know it is necessary in order to render geometry.

    // for defferred render
    GLuint gBuffer;
    GLuint gPosition, gNormal, gColorSpec, rboDepth, gDepthAll, gDepthWithoutWater;

    // for SSAO
    GLuint ssaoFBO, ssaoBlurFBO;
    GLuint ssaoColorTexture, ssaoNoiseTexture, ssaoBlurTexture;

    // for shadow map debug
    GLuint quadVAO = 0;
    GLuint quadVBO;

    Terrain m_terrain; // All of the Chunks that currently comprise the world.
    Player m_player; // The entity controlled by the user. Contains a camera to display what it sees as well.
    InputBundle m_inputs; // A collection of variables to be updated in keyPressEvent, mouseMoveEvent, mousePressEvent, etc.

    QTimer m_timer; // Timer linked to w. Fires approximately 60 times per second.

    // record the previous position of player
    glm::ivec3 prePos;

    // shadow map of sun
    ShadowMap m_sunShadow;

    // cascaded shadow map of sun
    CascadedShadowMap m_sunCSM;

    void moveMouseToCenter(); // Forces the mouse position to the screen's center. You should call this
                              // from within a mouse move event after reading the mouse movement so that
                              // your mouse stays within the screen bounds and is always read.

    void sendPlayerDataToGUI() const;

    // current time
    QDateTime m_time;

    // time for shader
    int m_shader_time;

    Sun m_sun;
    int m_sunTime;
    // for calculate fps
    float m_dt;

    bool m_showDepth = false;
    bool m_showLayer = false;
    bool m_useSSAO = true;


public:
    explicit MyGL(QWidget *parent = nullptr);
    ~MyGL();

    // Called once when MyGL is initialized.
    // Once this is called, all OpenGL function
    // invocations are valid (before this, they
    // will cause segfaults)
    void initializeGL() override;
    void initializeGbuffer();
    void initializeShaderTexture();
    void initializeShadowMap();
    void initializeSSAObuffer();
    // Called whenever MyGL is resized.
    void resizeGL(int w, int h) override;
    // Called whenever MyGL::update() is called.
    // In the base code, update() is called from tick().
    void paintGL() override;

    // Called from paintGL().
    // Calls Terrain::draw().
    void renderTerrainLambert();
    void renderTerrainBlinnShadow();
    void renderTerrainLambertShadow();
    void renderShadowMap();
    void renderShadowDebugQuad();
    void renderShadowDebugViewer();

    void renderCSMShadowMap();
    void renderCSMDebugQuad();
    void renderCSMLambert();

    void renderGbufferDebug();
    void renderDeferred();

    void gBufferPass();
    void gBufferWaterPass();
    void ssaoPass();
    void compositePass();
    void terrainPass();

    void renderQuad();

    void bindTexture(GLint textureID, int slot);

protected:
    // Automatically invoked when the user
    // presses a key on the keyboard
    void keyPressEvent(QKeyEvent *e);
    // Automatically invoked when the user
    // moves the mouse
    void mouseMoveEvent(QMouseEvent *e);
    // Automatically invoked when the user
    // presses a mouse button
    void mousePressEvent(QMouseEvent *e);

public slots:
    void slot_testMakeLava();

private slots:
    void tick(); // Slot that gets called ~60 times per second by m_timer firing.

signals:
    void sig_sendPlayerPos(QString) const;
    void sig_sendPlayerVel(QString) const;
    void sig_sendPlayerAcc(QString) const;
    void sig_sendPlayerLook(QString) const;
    void sig_sendPlayerChunk(QString) const;
    void sig_sendPlayerTerrainZone(QString) const;
    void sig_sentPlayerFps(QString) const;
};


#endif // MYGL_H
