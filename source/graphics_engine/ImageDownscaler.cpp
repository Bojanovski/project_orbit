#include "ImageDownscaler.h"
#include "ImageShaderPrograms.h"
#include "FullScreenQuad.h"


namespace graphics_engine
{
    class ImageDownscaleProgram : public ImageShaderPrograms
    {
    public:
        enum class DownscaleType { Horizontal, Vertical };

    public:
        ImageDownscaleProgram(DownscaleType type, int factor)
            : ImageShaderPrograms()
            , mFactor(factor)
            , mType(type)
        {
            std::string consts = "\n"
                "const int downscaleFactor = " + std::to_string(mFactor) + ";\n"
                "const float weight = 1.0 / float(downscaleFactor);\n"
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

            switch (mType)
            {

            case DownscaleType::Horizontal:
                mFragmentShaderSourceStr = ""
                    "#version 330 core\n"
                    "in vec2 texCoord;\n"
                    "layout (location = 0) out vec4 color;\n"

                    + consts + ""
                    + uniforms +

                    "void main()\n"
                    "{\n"
                    "   ivec2 inputTexCoord = ivec2(gl_FragCoord.xy - vec2(0.5, 0.5));\n"
                    "   inputTexCoord.x *= downscaleFactor;\n"
                    "   vec4 result = vec4(0.0);\n"
                    "   for (int i = 0; i < downscaleFactor; ++i)\n"
                    "   {\n"
                    "       ivec2 offset = ivec2(i, 0);\n"
                    "       result += texelFetch(inputTexture, inputTexCoord + offset, 0) * weight;\n"
                    "   }\n"
                    "   color = result;\n"
                    "}\n\0";
                break;

            case DownscaleType::Vertical:
                mFragmentShaderSourceStr = ""
                    "#version 330 core\n"
                    "in vec2 texCoord;\n"
                    "layout (location = 0) out vec4 color;\n"

                    + consts + ""
                    + uniforms +

                    "void main()\n"
                    "{\n"
                    "   ivec2 inputTexCoord = ivec2(gl_FragCoord.xy - vec2(0.5, 0.5));\n"
                    "   inputTexCoord.y *= downscaleFactor;\n"
                    "   vec4 result = vec4(0.0);\n"
                    "   for (int i = 0; i < downscaleFactor; ++i)\n"
                    "   {\n"
                    "       ivec2 offset = ivec2(0, i);\n"
                    "       result += texelFetch(inputTexture, inputTexCoord + offset, 0) * weight;\n"
                    "   }\n"
                    "   color = result;\n"
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

        void setImageSource(const RasterizerImage* src, int index = 0)
        {
            mTextureUnitCounter = 0;
            setUniformValue_Texture(mTexture, src, index);
        }

        int getFactor() const { return mFactor; }

    private:
        int mFactor;
        DownscaleType mType;

        int mTexture;
    };
}

using namespace graphics_engine;

ImageDownscaler::ImageDownscaler(uint factor)
	: mFactor(factor)
{

}

ImageDownscaler::~ImageDownscaler()
{
}

void ImageDownscaler::create(uint width, uint height)
{
	uint downscaledWidth = width / mFactor;
	uint downscaledHeight = height / mFactor;


	mHDownscaleSP = std::make_unique<ImageDownscaleProgram>(ImageDownscaleProgram::DownscaleType::Horizontal, mFactor);
	mHDownscaleSP->create();
	mVDownscaleSP = std::make_unique<ImageDownscaleProgram>(ImageDownscaleProgram::DownscaleType::Vertical, mFactor);
	mVDownscaleSP->create();

	mHDownscaleBuffer = std::make_unique<RenderTarget>();
	mHDownscaleBuffer->create(downscaledWidth, height, { RenderTarget::TextureDescriptor{ GL_RGBA16F, GL_RGBA, GL_FLOAT } });
	mVDownscaleBuffer = std::make_unique<RenderTarget>();
	mVDownscaleBuffer->create(downscaledWidth, downscaledHeight, { RenderTarget::TextureDescriptor{ GL_RGBA16F, GL_RGBA, GL_FLOAT } });
	mFullScreenQuad = std::make_unique<FullScreenQuad>();
	mFullScreenQuad->create();
}

void ImageDownscaler::resize(uint width, uint height)
{
	uint downscaledWidth = width / mFactor;
	uint downscaledHeight = height / mFactor;
	mHDownscaleBuffer->resize(downscaledWidth, height);
	mVDownscaleBuffer->resize(downscaledWidth, downscaledHeight);
}

void ImageDownscaler::downscale(RasterizerImage* img, int index)
{
	// horizontal downscale
	mHDownscaleBuffer->beginUse();
	mHDownscaleSP->bind();
	mHDownscaleSP->setImageSource(img, index);
	mFullScreenQuad->draw();
	mHDownscaleSP->unbind();
	mHDownscaleBuffer->endUse();

	// vertical downscale
	mVDownscaleBuffer->beginUse();
	mVDownscaleSP->bind();
	mVDownscaleSP->setImageSource(mHDownscaleBuffer.get(), 0);
	mFullScreenQuad->draw();
	mVDownscaleSP->unbind();
	mVDownscaleBuffer->endUse();
}

const RasterizerImage* ImageDownscaler::getResult() const 
{
	return mVDownscaleBuffer.get(); 
}
