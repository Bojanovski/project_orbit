#pragma once
#include "GBuffer.h"
#include "ImageShaderPrograms.h"
#include <cmath>


namespace graphics_engine
{
    class FinalizePBRShaderProgram : public ImageShaderPrograms
    {
    public:
        FinalizePBRShaderProgram()
            : ImageShaderPrograms()
        {
            std::string uniforms = "\n"
                "uniform sampler2D pbrTexture;\n"
                "uniform sampler2D blurredEmmisiveTexture;\n"
                "\n";

            mVertexShaderSourceStr = ""
                "#version 330 core\n"
                "layout (location = 0) in vec3 position;\n"
                "out vec2 texCoord;\n"

                + uniforms +

                "void main()\n"
                "{\n"
                "   texCoord = position.xy * 0.5 + 0.5;\n"
                "   gl_Position = vec4(position, 1.0);\n"
                "}\0";

            mFragmentShaderSourceStr = ""
                "#version 330 core\n"
                "in vec2 texCoord;\n"
                "layout (location = 0) out vec4 color;\n"

                + uniforms +

                "void main()\n"
                "{\n"
                "   color = texture(pbrTexture, texCoord);\n"

                // Apply emissive blur
                "   vec3 blurredEmissive = texture(blurredEmmisiveTexture, texCoord).rgb;\n"
                "   color.rgb += blurredEmissive;\n"

                //"   color.rgb /= color.a;\n"

                //"   float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));\n"
                //"   brightness = clamp(brightness, 0.0, 0.8);\n"
                //"   vec3 emissive = mix(blurredEmissive, vec3(0.0), brightness);\n"
                //"   color.rgb += emissive;\n"

                // HDR to LDR
                "   color.rgb = color.rgb / (color.rgb + vec3(1.0));\n"
                "   color.rgb = pow(color.rgb, vec3(1.0/2.2));\n"
                //"   color.a = 0.5;\n"

                "}\n\0";
        }

        virtual bool create() override
        {
            bool isOk = ShaderProgramBase::create();
            mPBRTexture = getUniformID("pbrTexture");
            mEmissiveBlurredTexture = getUniformID("blurredEmmisiveTexture");
            return isOk;
        }

        void setPBR_BlurredEmissive(const RasterizerImage* pbrSrc, const RasterizerImage* blurredEmmisiveSrc)
        {
            mTextureUnitCounter = 0;
            setUniformValue_Texture(mPBRTexture, pbrSrc, 0);

            setUniformValue_Texture(mEmissiveBlurredTexture, blurredEmmisiveSrc, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

    private:
        int mPBRTexture;
        int mEmissiveBlurredTexture;
    };
}
