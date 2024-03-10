#pragma once
#include <RenderTarget.h>


namespace graphics_engine
{
    class GBuffer : public RenderTarget
    {
    public:
        enum class BufferType
        {
            Albedo              = 0,
            CombinedValues_0    = 1,
            Normal              = 2,
            Position            = 3,
            Emissive            = 4,
        };

    public:
        GBuffer()
        {

        }

        void create(GLuint width, GLuint height)
        {
            std::vector<RenderTarget::TextureDescriptor> rttDesc =
            {
                RenderTarget::TextureDescriptor{ GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE },     // albedo
                RenderTarget::TextureDescriptor{ GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE },     // combinedValues_0
                RenderTarget::TextureDescriptor{ GL_RGB16F, GL_RGB, GL_FLOAT },             // normal
                RenderTarget::TextureDescriptor{ GL_RGB16F, GL_RGB, GL_FLOAT },             // position
                RenderTarget::TextureDescriptor{ GL_RGB16F, GL_RGB, GL_FLOAT },             // emissive
            };
            RenderTarget::create(width, height, rttDesc);
        }

        void beginUse()
        {
            RenderTarget::beginUse();
            GLfloat  albedo[]               = {0.0f, 0.0f, 0.0f, 0.0f};
            GLfloat  combinedValues_0[]     = {0.0f, 0.0f, 0.0f, 0.0f};
            GLfloat  normal[]               = {0.0f, 0.0f, 0.0f, 1.0f};
            GLfloat  position[]             = {0.0f, 0.0f, 0.0f, 1.0f};
            GLfloat  emissive[]             = {0.0f, 0.0f, 0.0f, 1.0f};
            glClearBufferfv(GL_COLOR, 0, albedo);
            glClearBufferfv(GL_COLOR, 1, combinedValues_0);
            glClearBufferfv(GL_COLOR, 2, normal);
            glClearBufferfv(GL_COLOR, 3, position);
            glClearBufferfv(GL_COLOR, 4, emissive);

            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
    };
}
