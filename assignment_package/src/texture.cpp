#include "texture.h"
#include <QImage>
#include <QOpenGLWidget>

Texture::Texture(OpenGLContext *context)
    : context(context), m_textureHandle(-1), m_textureImage(nullptr)
{}

Texture::~Texture()
{}

void Texture::create(const char *texturePath, QImage::Format format)
{
    context->printGLErrorLog();

    QImage img(texturePath);
    img.convertToFormat(format);
    const uchar *data = img.constBits();
    img = img.mirrored();
    m_textureImage = std::make_shared<QImage>(img);
    context->glGenTextures(1, &m_textureHandle);

    context->printGLErrorLog();
}

void Texture::load(unsigned int texSlot = 0, size_t internalFormat, size_t externalFormat, size_t type, size_t repeat, size_t filter)
{
    context->printGLErrorLog();

    context->glActiveTexture(GL_TEXTURE0 + texSlot);
    context->glBindTexture(GL_TEXTURE_2D, m_textureHandle);

    // These parameters need to be set for EVERY texture you create
    // They don't always have to be set to the values given here, but they do need
    // to be set
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);

    context->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                          m_textureImage->width(), m_textureImage->height(),
                          0, externalFormat, type, m_textureImage->bits());
    context->printGLErrorLog();
}


void Texture::bind(unsigned int texSlot = 0)
{
    context->glActiveTexture(GL_TEXTURE0 + texSlot);
    context->glBindTexture(GL_TEXTURE_2D, m_textureHandle);
}
