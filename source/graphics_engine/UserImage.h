#pragma once
#include "RasterizerImage.h"


class QOpenGLTexture;

namespace graphics_engine
{
    class UserImage : public RasterizerImage
    {
    public:
        typedef float (*RandomGeneratorFunction)(int, int);

    public:
        UserImage();
        virtual ~UserImage();

        bool createFromMemory(const uchar* data, size_t dataSize, const std::string& mimeType);
        bool createFromFile(const wchar_t* file);
        bool createFromFileHDR(const wchar_t* file);
        bool createRandom(uint width, uint height, RandomGeneratorFunction function);
        bool destroy();

        GLuint getWidth() const override { return mWidth; }
        GLuint getHeight() const override { return mHeight; }
        GLsizei getCount() const override { return 1; }
        GLuint getGLID(uchar index = 0) const override;

    private:
        GLuint mTexture_glID;
        GLuint mWidth;
        GLuint mHeight;
    };
}
