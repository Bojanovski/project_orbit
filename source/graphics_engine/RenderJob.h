#pragma once
#include "GraphicsEngineTypes.h"
#include "Camera.h"
#include <QImage>


namespace graphics_engine
{

    struct RenderJob
    {
        typedef void (*CallBack)(const RenderJob&);
        enum class OutputType { File, QImage, None };

        Scene* scene;
        Camera camera;
        uint imageWidth;
        uint imageHeight;
        uint spp;

        // Output
        OutputType outputType;
        std::string outputFile;
        QImage outputImage;

        // Callback
        void* userData = nullptr;
        CallBack callback = nullptr;
    };
}
