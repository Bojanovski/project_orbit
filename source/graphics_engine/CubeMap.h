#pragma once
#include "RasterizerImage.h"


namespace graphics_engine
{
    struct CubeMap : public RasterizerImage
    {
    public:
        CubeMap();
        virtual ~CubeMap();

        bool destroy();

        GLuint getWidth() const override { return mSize; }
        GLuint getHeight() const override { return mSize; }
        GLsizei getCount() const override { return 1; }
        GLuint getGLID(uchar) const override { return mTexture_glID; }
        GLenum getGLType() const override { return GL_TEXTURE_CUBE_MAP; }

    protected:
        GLuint mTexture_glID;
        GLuint mDepthRenderbuffer_glID;
        GLuint mFBO_glID;
        GLuint mSize;
    };

    struct EnvironmentMap : public CubeMap
    {
    public:
        EnvironmentMap();
        bool createFromFileEquirectangularHDR(uint edge, const wchar_t* file);
    };

    struct IrradianceMap : public CubeMap
    {
    public:
        IrradianceMap();
        bool createFromEnvironmentMap(uint edge, const EnvironmentMap*);
    };

    struct PrefilterMap : public CubeMap
    {
    public:
        PrefilterMap();
        bool createFromEnvironmentMap(uint edge, const EnvironmentMap*);
    };
}
