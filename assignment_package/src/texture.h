#pragma once

#include <openglcontext.h>
#include <glm_includes.h>
#include <memory>

class Texture
{
public:
    Texture(OpenGLContext* context);
    ~Texture();

    void create(const char *texturePath, QImage::Format format = QImage::Format_ARGB32);
    void load(unsigned int texSlot,
              size_t internalFormat = GL_RGBA,
              size_t externalFormat = GL_BGRA,
              size_t type = GL_UNSIGNED_INT_8_8_8_8_REV,
              size_t repeat = GL_CLAMP_TO_EDGE,
              size_t filter = GL_NEAREST
              );
    void bind(unsigned int texSlot);

private:
    OpenGLContext* context;
    GLuint m_textureHandle;
    std::shared_ptr<QImage> m_textureImage;
};
