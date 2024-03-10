#pragma once
#include "GraphicsEngineTypes.h"
#include <QOpenGLExtraFunctions>
#include <QImage>


namespace graphics_engine
{
    struct RasterizerImage : public Image, protected QOpenGLExtraFunctions
    {
    public:
        RasterizerImage() {}
        virtual ~RasterizerImage() {}

        virtual GLuint getWidth() const = 0;
        virtual GLuint getHeight() const = 0;
        virtual GLsizei getCount() const = 0;
        virtual GLuint getGLID(uchar index = 0) const = 0;
        virtual GLenum getGLType() const { return GL_TEXTURE_2D; }
    };
}
