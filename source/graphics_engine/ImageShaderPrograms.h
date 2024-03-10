#pragma once
#include "ShaderProgramBase.h"
#include "GBuffer.h"
#include "UserImage.h"


namespace graphics_engine
{
    class ImageShaderPrograms : public ShaderProgramBase
    {
    public:
        ImageShaderPrograms() : ShaderProgramBase() {}

        void bind() override
        {
            ShaderProgramBase::bind();
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glDisable(GL_CULL_FACE);
        }
    };

    class PassthroughShaderProgram : public ImageShaderPrograms
    {
    public:
        PassthroughShaderProgram()
            : ImageShaderPrograms()
        {
            std::string uniforms = "\n"
                "uniform sampler2D renderTexture;\n"
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
                "   color = texture(renderTexture, texCoord);\n"
                "   color.a = 1.0;\n"
                "}\n\0";
        }

        virtual bool create() override
        {
            bool isOk = ShaderProgramBase::create();
            mTexture = getUniformID("renderTexture");
            return isOk;
        }

        void setTextureSource(const RasterizerImage* src, bool linearSampling)
        {
            mTextureUnitCounter = 0;
            setUniformValue_Texture(mTexture, src, 0);
            if (linearSampling)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
        }

    private:
        int mTexture;
    };

    class SSAOShaderProgram : public ImageShaderPrograms
    {
    public:
        SSAOShaderProgram()
            : ImageShaderPrograms()
            , mKernelCount(64)
            , mKernelRadius(0.5f)
        {
            std::string consts = "\n"
                "const int kernelCount = " + std::to_string(mKernelCount) + ";\n"
                "const float kernelRadius = " + std::to_string(mKernelRadius) + ";\n"
                "const float bias = " + std::to_string(0.025f) + ";\n"
                "\n";

            std::string uniforms = "\n"
                "uniform mat4 P;\n"
                "uniform mat4 V;\n"
                "uniform sampler2D positionTexture;\n"
                "uniform sampler2D normalTexture;\n"
                "uniform sampler2D albedoTexture;\n"
                "uniform vec2 rndTexScale;\n"
                "uniform sampler2D randomTexture;\n"
                "uniform vec3 samples[kernelCount];\n"
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

            mFragmentShaderSourceStr = ""
                "#version 330 core\n"
                "in vec2 texCoord;\n"
                "layout (location = 0) out vec4 color;\n"

                + consts + ""
                + uniforms +

                "void main()\n"
                "{\n"
                "   vec3 randomVec  = texture(randomTexture, texCoord * rndTexScale).rgb;\n"
                "   vec3 normal     = texture(normalTexture, texCoord).rgb;\n"
                "   vec3 tangent    = normalize(randomVec - normal * dot(randomVec, normal));\n"
                "   vec3 bitangent  = cross(normal, tangent);\n"
                "   mat3 TBN        = mat3(tangent, bitangent, normal);\n"
                "   vec3 position   = texture(positionTexture, texCoord).rgb;\n"

                "   float occlusion = 0.0;\n"
                "   for(int i = 0; i < kernelCount; ++i)\n"
                "   {\n"
                "       vec3 samplePos = TBN * samples[i];\n"
                "       samplePos   = position + samplePos * kernelRadius;\n"
                "       vec4 offset = vec4(samplePos, 1.0);\n"
                "       offset      = P * offset;\n"
                "       offset.xyz /= offset.w;\n"
                "       offset.xyz  = offset.xyz * 0.5 + 0.5;\n"

                "       float sampleDepth = texture(positionTexture, offset.xy).z;\n"
                "       float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(position.z - sampleDepth));\n"
                "       occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;\n"
                "   }\n"
                "   occlusion = 1.0 - (occlusion / kernelCount);\n"
                "   color = vec4(occlusion, occlusion, occlusion, 1.0);\n"
                "}\n\0";
        }

        virtual bool create() override
        {
            bool isOk = ShaderProgramBase::create();

            isOk |= mRandomTextureObject.createRandom(4, 4, rndTangentGenerator);

            mProjection = getUniformID("P");
            mView = getUniformID("V");
            mPositionTexture = getUniformID("positionTexture");
            mNormalTexture = getUniformID("normalTexture");
            mAlbedoTexture = getUniformID("albedoTexture");
            mRndTexScale = getUniformID("rndTexScale");
            mRandomTexture = getUniformID("randomTexture");
            mSamples = getUniformID("samples");

            // Create the sample kernel
            mKernel.resize(mKernelCount);
            auto lerp = [](float a, float b, float f) -> float { return a + f * (b - a); };
            for (uint i = 0; i < mKernelCount; ++i)
            {
                vec3 sample(
                    sRandomFloats(sGenerator) * 2.0 - 1.0,
                    sRandomFloats(sGenerator) * 2.0 - 1.0,
                    sRandomFloats(sGenerator)
                );
                sample.normalize();
                sample *= sRandomFloats(sGenerator);
                // scale samples s.t. they're more aligned to center of kernel
                float scale = (float)i / mKernelCount;
                scale = lerp(0.1f, 1.0f, scale * scale);
                sample *= scale;
                mKernel[i] = sample;
            }

            return isOk;
        }

        void bind() override
        {
            ImageShaderPrograms::bind();
            setUniformValue_Vector3Array(mSamples, mKernel);
        }

        void setTransforms(const mat4x4& projection, const mat4x4& view)
        {
            setUniformValue_Matrix4x4(mProjection, projection);
            setUniformValue_Matrix4x4(mView, view);
        }

        void setGBuffer(const GBuffer* gbuf)
        {
            mTextureUnitCounter = 0;
            setUniformValue_Texture(mPositionTexture, gbuf, (int)GBuffer::BufferType::Position);
            setUniformValue_Texture(mNormalTexture, gbuf, (int)GBuffer::BufferType::Normal);
            setUniformValue_Texture(mAlbedoTexture, gbuf, (int)GBuffer::BufferType::Albedo);

            auto rndScale = vec2(gbuf->getWidth()/mRandomTextureObject.getWidth(), gbuf->getHeight()/mRandomTextureObject.getHeight());
            setUniformValue_Vector2(mRndTexScale, rndScale);
            setUniformValue_Texture(mRandomTexture, &mRandomTextureObject, 0);
        }

    private:
        static float rndTangentGenerator(int i, int c)
        {
            (void)i;
            if (c == 0 || c == 1)   return sRandomFloats(sGenerator) * 2.0f - 1.0f;
            else                    return 0.0f;
        };

    private:
        const uint mKernelCount;
        const float mKernelRadius;
        std::vector<vec3> mKernel;
        UserImage mRandomTextureObject;

        int mPositionTexture;
        int mNormalTexture;
        int mAlbedoTexture;
        int mRndTexScale;
        int mRandomTexture;
        int mSamples;
        int mProjection;
        int mView;
    };

    class TextureOperationProgram : public ImageShaderPrograms
    {
    public:
        struct Operation
        {
            enum class Type { Add, Sub, Min, Max };

            Type type;
            vec4 operand;
        };

    public:
        TextureOperationProgram(std::vector<Operation> operations)
            : ImageShaderPrograms()
            , mOperations(operations)
        {
            std::string operationsStr = "\n";
            for (size_t i = 0; i < mOperations.size(); i++)
            {
                auto& op = mOperations[i];
                std::string operandStr = "vec4(" +
                    std::to_string(op.operand.x()) + "," +
                    std::to_string(op.operand.y()) + "," +
                    std::to_string(op.operand.z()) + "," +
                    std::to_string(op.operand.w()) + "" +
                    ")";
                switch (op.type)
                {
                case Operation::Type::Add:
                    operationsStr += "    result += " + operandStr + ";\n";
                    break;

                case Operation::Type::Sub:
                    operationsStr += "    result -= " + operandStr + ";\n";
                    break;

                case Operation::Type::Min:
                    operationsStr += "    result = min(result, " + operandStr + ");\n";
                    break;

                case Operation::Type::Max:
                    operationsStr += "    result = max(result, " + operandStr + ");\n";
                    break;

                default:
                    break;
                }
            }

            std::string consts = "\n"
                "\n";

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

            mFragmentShaderSourceStr = ""
                "#version 330 core\n"
                "in vec2 texCoord;\n"
                "layout (location = 0) out vec4 color;\n"

                + consts + ""
                + uniforms +

                "void main()\n"
                "{\n"
                "   ivec2 inputTexCoord = ivec2(gl_FragCoord.xy - vec2(0.5, 0.5));\n"
                "   vec4 result = texelFetch(inputTexture, inputTexCoord, 0);\n"
                + operationsStr +
                "   color = result;\n"
                "}\n\0";
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
        std::vector<Operation> mOperations;

        int mTexture;
    };
}
