#pragma once
#include "RasterizerImage.h"


namespace graphics_engine
{
    struct RenderTarget : public RasterizerImage
    {
    public:
        struct TextureDescriptor
        {
            GLenum internalFormat;
            GLenum format;
            GLenum type;
        };

    public:
        RenderTarget();
        ~RenderTarget();

        bool create(GLuint width, GLuint height, const std::vector<TextureDescriptor>&);
        bool destroy();
        bool resize(GLuint width, GLuint height);

        GLuint getWidth() const override { return mWidth; }
        GLuint getHeight() const override { return mHeight; }
        GLsizei getCount() const override { return (GLsizei)mTexture_glIDs.size(); }
        GLuint getGLID(uchar index) const override { return mTexture_glIDs[index]; }

        void beginUse();
        void endUse();

        QImage getImage(uchar);

    private:
        std::vector<GLuint> mTexture_glIDs;
        GLuint mDepthRenderbuffer_glID;
        GLuint mFBO_glID;
        GLuint mWidth;
        GLuint mHeight;
        std::vector<TextureDescriptor> mTextureDescriptors;
    };
}
