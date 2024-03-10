#pragma once
#include "GraphicsEngineTypes.h"


namespace graphics_engine
{
	struct RasterizerImage;
	struct RenderTarget;
	class ImageDownscaleProgram;
	class FullScreenQuad;

	class ImageDownscaler
	{
	public:
		ImageDownscaler(uint factor);
		~ImageDownscaler();

		void create(uint width, uint height);
		void resize(uint width, uint height);
		void downscale(RasterizerImage* texture, int index = 0);
		const RasterizerImage* getResult() const;
		uint getFactor() const { return mFactor; }

	private:
		const uint mFactor;
		std::unique_ptr<ImageDownscaleProgram> mHDownscaleSP;
		std::unique_ptr<ImageDownscaleProgram> mVDownscaleSP;
		std::unique_ptr<RenderTarget> mHDownscaleBuffer;
		std::unique_ptr<RenderTarget> mVDownscaleBuffer;
		std::unique_ptr<FullScreenQuad> mFullScreenQuad;
	};
}