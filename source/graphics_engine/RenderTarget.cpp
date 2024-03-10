#include "RenderTarget.h"
#include <QOpenGLContext>
#include <QDebug>
#include <QOpenGLFramebufferObject>


using namespace graphics_engine;

RenderTarget::RenderTarget()
    : mTexture_glIDs(0)
    , mDepthRenderbuffer_glID(0)
    , mFBO_glID(0)
{

}

RenderTarget::~RenderTarget()
{
    destroy();
}

bool RenderTarget::create(GLuint width, GLuint height, const std::vector<TextureDescriptor>& descArray)
{
    initializeOpenGLFunctions();
    destroy();

    mTextureDescriptors = descArray;
    uint colorAttachementCount = (uint)mTextureDescriptors.size();
    mTexture_glIDs.resize(colorAttachementCount, 0);
    for (uint i = 0; i < colorAttachementCount; ++i)
    {
        if (mTexture_glIDs[i] != 0) return false;
    }

    mWidth = (GLuint)width;
    mHeight = (GLuint)height;

    // Create an FBO
    glGenFramebuffers(1, &mFBO_glID);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);

    // Create textures for rendering
    for (uint i = 0; i < colorAttachementCount; ++i)
    {
        glGenTextures(1, &mTexture_glIDs[i]);
        glBindTexture(GL_TEXTURE_2D, mTexture_glIDs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0,
                     mTextureDescriptors[i].internalFormat,
                     mWidth, mHeight, 0,
                     mTextureDescriptors[i].format, mTextureDescriptors[i].type,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Create a renderbuffer for depth
    glGenRenderbuffers(1, &mDepthRenderbuffer_glID);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbuffer_glID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight);

    // Attach to the FBO
    for (uint i = 0; i < colorAttachementCount; ++i)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, mTexture_glIDs[i], 0);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbuffer_glID);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        qWarning() << "Framebuffer is not complete";
        return false;
    }

    // Unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

bool RenderTarget::destroy()
{
    int count = 0;
    if (mFBO_glID != 0)
    {
        glDeleteFramebuffers(1, &mFBO_glID);
        mFBO_glID = 0;
        count += 1;
    }
    uint colorAttachementCount = (uint)mTextureDescriptors.size();
    for (uint i = 0; i < colorAttachementCount; ++i)
    {
        if (mTexture_glIDs[i] != 0)
        {
            glDeleteTextures(1, &mTexture_glIDs[i]);
            mTexture_glIDs[i] = 0;
            count += 1;
        }
    }
    if (mDepthRenderbuffer_glID != 0)
    {
        glDeleteRenderbuffers(1, &mDepthRenderbuffer_glID);
        mDepthRenderbuffer_glID = 0;
        count += 1;
    }
    return (count == (2 + colorAttachementCount));
}

bool RenderTarget::resize(GLuint width, GLuint height)
{
    uint colorAttachementCount = (uint)mTextureDescriptors.size();
    for (uint i = 0; i < colorAttachementCount; ++i)
    {
        if (mTexture_glIDs[i] == 0) return false;
    }

    if (mDepthRenderbuffer_glID == 0)
    {
        return false; // not created
    }

    mWidth = (GLuint)width;
    mHeight = (GLuint)height;

    for (uint i = 0; i < colorAttachementCount; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, mTexture_glIDs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0,
                     mTextureDescriptors[i].internalFormat,
                     mWidth, mHeight, 0,
                     mTextureDescriptors[i].format, mTextureDescriptors[i].type,
                     nullptr);
    }

    glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbuffer_glID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        qWarning() << "Error resizing texture: " << error;
        return false;
    }

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        qWarning() << "Framebuffer is not complete after texture resize";
        return false;
    }

	return true;
}

void RenderTarget::beginUse()
{
    // Bind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);

    // Specify which color attachments we want to render to
    uint colorAttachementCount = (uint)mTextureDescriptors.size();
    std::vector<GLenum> drawBuffers(colorAttachementCount);
    for (uint i = 0; i < colorAttachementCount; ++i)
    {
        drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    glDrawBuffers(colorAttachementCount, drawBuffers.data());

    // Viewport and color
    GLfloat  color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 0, color);
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RenderTarget::endUse()
{
    //GLenum defaultDrawBuffers[] = {GL_BACK};
    //glDrawBuffers(1, defaultDrawBuffers);

    // Unbind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

QImage RenderTarget::getImage(uchar index)
{
    // Bind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);
    QImage::Format imageFormat = QImage::Format_Invalid;
    switch (mTextureDescriptors[index].format)
    {
    case GL_RGBA:
        imageFormat = QImage::Format_RGBA8888;
        break;
    case GL_RGB:
        imageFormat = QImage::Format_RGB888;
        break;
    default:
        throw;
        break;
    }

    QImage image(mWidth, mHeight, imageFormat);
    glReadPixels(0, 0, mWidth, mHeight, mTextureDescriptors[index].format, mTextureDescriptors[index].type, image.bits());
    // Flip the image vertically (if needed)
    image = image.mirrored(false, true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return image;
}
