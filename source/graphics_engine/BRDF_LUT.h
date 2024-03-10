#pragma once
#include "RasterizerImage.h"


namespace graphics_engine
{
    class BRDF_LUT : public RasterizerImage
    {
    public:
        BRDF_LUT();
        virtual ~BRDF_LUT();

        bool create(uint size);
        bool destroy();

        GLuint getWidth() const override { return mWidth; }
        GLuint getHeight() const override { return mHeight; }
        GLsizei getCount() const override { return 1; }
        GLuint getGLID(uchar) const override { return mTexture_glID; }

    private:
        GLuint mTexture_glID;
        GLuint mDepthRenderbuffer_glID;
        GLuint mFBO_glID;
        GLuint mWidth;
        GLuint mHeight;
    };
}
