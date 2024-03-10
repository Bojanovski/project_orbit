#pragma once
#include "ImageShaderPrograms.h"

namespace graphics_engine
{
    class BlurShaderProgram : public ImageShaderPrograms
    {
    public:
        enum class BlurType { Simple, GaussianHorizontal, GaussianVertical };

    public:
        BlurShaderProgram(int radius, BlurType type, float standardDeviation = 1.0f)
            : ImageShaderPrograms()
            , mBlurRadius(radius)
            , mType(type)
        {
            bool usingGaussian =
                    (mType == BlurType::GaussianHorizontal) ||
                    (mType == BlurType::GaussianVertical);
            if (usingGaussian)
            {
                mGaussianFilter.resize(mBlurRadius * 2 + 1);
                float sum = 0.0f;
                for (int i = -mBlurRadius; i <= mBlurRadius; ++i)
                {
                    float x = (float)i;
                    int index = i + mBlurRadius;
                    mGaussianFilter[index] =
                            1.0f /
                            (sqrtf(2.0f * M_PI * standardDeviation)) *
                            expf(-(x * x) / (2.0f * standardDeviation));
                    sum += mGaussianFilter[index];
                }
                for (size_t i = 0; i < mGaussianFilter.size(); ++i)
                {
                    mGaussianFilter[i] /= sum;
                }
            }

            std::string weightsStr = "";
            for (size_t i = 0; i < mGaussianFilter.size(); ++i)
            {
                weightsStr += std::to_string(mGaussianFilter[i]);
                if ((i + 1) < mGaussianFilter.size())
                    weightsStr += ", ";
            }

            std::string consts = "\nconst int blurRadius = " + std::to_string(mBlurRadius) + ";\n";
            if (usingGaussian) consts += "const float weights[" + std::to_string(mGaussianFilter.size()) + "] = float[] ( " + weightsStr + " );\n";
            consts += "\n";

            std::string uniforms = "\n"
                "uniform sampler2D inputTexture;\n"
                "\n";

            mVertexShaderSourceStr = ""
                "#version 330 core\n"
                "layout (location = 0) in vec3 position;\n"
                "out vec2 texCoord;\n"

                + consts + ""
                + uniforms +

                "void main()\n"
                "{\n"
                "   texCoord = position.xy * 0.5 + 0.5;\n"
                "   gl_Position = vec4(position, 1.0);\n"
                "}\0";

            switch (mType)
            {
                case BlurType::Simple:
                    mFragmentShaderSourceStr = ""
                        "#version 330 core\n"
                        "in vec2 texCoord;\n"
                        "layout (location = 0) out vec4 color;\n"

                        + consts + ""
                        + uniforms +

                        "void main()\n"
                        "{\n"
                        "   vec2 texelSize = 1.0 / vec2(textureSize(inputTexture, 0));\n"
                        "   float result = 0.0;\n"
                        "   for (int x = -blurRadius; x <= blurRadius; ++x)\n"
                        "   {\n"
                        "       for (int y = -blurRadius; y <= blurRadius; ++y)\n"
                        "       {\n"
                        "           vec2 offset = vec2(float(x), float(y)) * texelSize;\n"
                        "           result += texture(inputTexture, texCoord + offset).r;\n"
                        "       }\n"
                        "   }\n"
                        "   int rowPixelCount = (2 * blurRadius + 1);\n"
                        "   int columnPixelCount = (2 * blurRadius + 1);\n"
                        "   result = result / (rowPixelCount * columnPixelCount);\n"
                        "   color = vec4(result, result, result, 1.0);\n"
                        "}\n\0";
                break;

            case BlurType::GaussianHorizontal:
                mFragmentShaderSourceStr = ""
                    "#version 330 core\n"
                    "in vec2 texCoord;\n"
                    "layout (location = 0) out vec4 color;\n"

                    + consts + ""
                    + uniforms +

                    "void main()\n"
                    "{\n"
                    "   vec2 texelSize = 1.0 / vec2(textureSize(inputTexture, 0));\n"
                    "   vec4 result = vec4(0.0);\n"
                    "   for (int i = -blurRadius; i <= blurRadius; ++i)\n"
                    "   {\n"
                    "       vec2 offset = vec2(float(i), 0.0) * texelSize;\n"
                    "       int weightIndex = i + blurRadius;\n"
                    "       result += texture(inputTexture, texCoord + offset) * weights[weightIndex];\n"
                    "   }\n"
                    "   color = result;\n"
                    //"   color.a = 1.0f;\n"
                    "}\n\0";
            break;

            case BlurType::GaussianVertical:
                mFragmentShaderSourceStr = ""
                    "#version 330 core\n"
                    "in vec2 texCoord;\n"
                    "layout (location = 0) out vec4 color;\n"

                    + consts + ""
                    + uniforms +

                    "void main()\n"
                    "{\n"
                    "   vec2 texelSize = 1.0 / vec2(textureSize(inputTexture, 0));\n"
                    "   vec4 result = vec4(0.0);\n"
                    "   for (int i = -blurRadius; i <= blurRadius; ++i)\n"
                    "   {\n"
                    "       vec2 offset = vec2(0.0, float(i)) * texelSize;\n"
                    "       int weightIndex = i + blurRadius;\n"
                    "       result += texture(inputTexture, texCoord + offset) * weights[weightIndex];\n"
                    "   }\n"
                    "   color = result;\n"
                    //"   color.a = 1.0f;\n"
                    "}\n\0";
            break;
            }
        }

        virtual bool create() override
        {
            bool isOk = ShaderProgramBase::create();
            mTexture = getUniformID("inputTexture");
            return isOk;
        }

        void setTextureSource(const RasterizerImage* src, int index = 0)
        {
            mTextureUnitCounter = 0;
            setUniformValue_Texture(mTexture, src, index);
        }

    private:
        int mBlurRadius;
        BlurType mType;

        std::vector<float> mGaussianFilter;

        int mTexture;
    };
}
