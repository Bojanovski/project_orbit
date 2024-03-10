#include "UserImage.h"
#include <QOpenGLTexture>
#include <string>
#include <random>

#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4267)
#include <OpenImageIO/imagebuf.h>
#pragma warning(pop)


using namespace graphics_engine;

std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
std::default_random_engine generator;

float rndTangentGenerator(int i, int c)
{
    (void)i;
    (void)c;
    return randomFloats(generator);
};

UserImage::UserImage()
    : mTexture_glID(0)
    , mWidth(0)
    , mHeight(0)
{

}

UserImage::~UserImage()
{
    destroy();
}

bool UserImage::createFromMemory(const uchar *data, size_t dataSize, const std::string &mimeType)
{
    initializeOpenGLFunctions();
    destroy();
    QImage image;
    image.loadFromData(data, (int)dataSize, mimeType.c_str());
    image = image.convertToFormat(QImage::Format_RGBA8888);
    if (!mLinearSpace)
    {
        // Perform sRGB to linear conversion on the image data
        auto w = image.width();
        auto h = image.height();
        for (int y = 0; y < h; ++y)
        {
            QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < w; ++x) 
            {
                line[x] = qRgba(
                    qRed(line[x]) * qRed(line[x]) / 255,
                    qGreen(line[x]) * qGreen(line[x]) / 255,
                    qBlue(line[x]) * qBlue(line[x]) / 255,
                    qAlpha(line[x]));
            }
        }
    }

    mWidth = image.width();
    mHeight = image.height();
    glGenTextures(1, &mTexture_glID);
    glBindTexture(GL_TEXTURE_2D, mTexture_glID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool UserImage::createFromFile(const wchar_t* file)
{
    initializeOpenGLFunctions();
    destroy();
    QImage image;
    image.load(QString::fromWCharArray(file));
    image = image.convertToFormat(QImage::Format_RGBA8888);

    mWidth = image.width();
    mHeight = image.height();
    glGenTextures(1, &mTexture_glID);
    glBindTexture(GL_TEXTURE_2D, mTexture_glID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool UserImage::createFromFileHDR(const wchar_t* file)
{
    initializeOpenGLFunctions();
    destroy();

    auto filePathStr = QString::fromWCharArray(file).toStdString();
    OIIO::ImageBuf imgBuf(filePathStr);
    auto& imageSpec = imgBuf.spec();
    mWidth = (GLuint)imageSpec.width;
    mHeight = (GLuint)imageSpec.height;
    int channels = imageSpec.nchannels;
    assert(channels == 4);

    std::vector<float> pixels;
    pixels.resize(mWidth * mHeight * channels);
    OIIO::ROI roi(0, mWidth, 0, mHeight);
    imgBuf.get_pixels(roi, OIIO::TypeDesc::FLOAT, pixels.data());

    glGenTextures(1, &mTexture_glID);
    glBindTexture(GL_TEXTURE_2D, mTexture_glID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, pixels.data());

    // Set texture parameters (adjust as needed)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

bool UserImage::createRandom(uint width, uint height, RandomGeneratorFunction function)
{
    initializeOpenGLFunctions();
    destroy();

    if (function == nullptr) function = rndTangentGenerator;
    uint totalCount = width * height;
    std::vector<float> ssaoNoise(totalCount * 3);
    for (uint i = 0; i < totalCount; i++)
    {
        ssaoNoise[i * 3 + 0] = function(i, 0);
        ssaoNoise[i * 3 + 1] = function(i, 1);
        ssaoNoise[i * 3 + 2] = function(i, 2);
    }

    glGenTextures(1, &mTexture_glID);
    glBindTexture(GL_TEXTURE_2D, mTexture_glID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    mWidth = width;
    mHeight = height;

    return true;
}

bool UserImage::destroy()
{
    if (mTexture_glID != 0)
    {
        glDeleteTextures(1, &mTexture_glID);
        mTexture_glID = 0;
        return true;
    }
    return false;
}

GLuint UserImage::getGLID(uchar) const
{
    return mTexture_glID;
}
