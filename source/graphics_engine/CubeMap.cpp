#include "CubeMap.h"
#include "ShaderProgramBase.h"
#include "UserImage.h"
#include "FullScreenCube.h"
#include "Camera.h"
#include <QOpenGLContext>
#include <QDebug>


using namespace graphics_engine;

class EquirectangularToCubemapShaderProgram : public ShaderProgramBase
{
public:
    EquirectangularToCubemapShaderProgram()
        : ShaderProgramBase()
    {
        std::string uniforms = "\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"

            "uniform sampler2D equirectangularMap;\n"
            "\n";

        mVertexShaderSourceStr = ""
            "#version 330 core\n"
            "layout (location = 0) in vec3 position;\n"
            "out vec3 vertexPosition;\n"

            + uniforms +

            "void main()\n"
            "{\n"
            "   vertexPosition = position;\n"
            "   vec4 posVec4 = vec4(position, 1.0f);\n"
            "   gl_Position = projection * view * posVec4;\n"
            "}\0";

        mFragmentShaderSourceStr = ""
            "#version 330 core\n"
            "in vec3 vertexPosition;\n"
            "layout(location = 0) out vec4 color;\n"

            + uniforms +

            "const vec2 invAtan = vec2(0.1591, 0.3183);\n"
            "vec2 SampleSphericalMap(vec3 v)\n"
            "{\n"
            "    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));\n"
            "    uv *= invAtan;\n"
            "    uv += 0.5;\n"
            "    uv.y = 1.0f - uv.y;\n" // account for the vertical flip
            "    return uv;\n"
            "}\n\n"

            "void main()\n"
            "{\n"
            "    vec2 uv = SampleSphericalMap(normalize(vertexPosition));\n"
            "    vec3 color_rgb = texture(equirectangularMap, uv).rgb;\n"
            "    color = vec4(color_rgb, 1.0);\n"
            "}\n\0";
    }

    virtual bool create() override
    {
        bool isOk = ShaderProgramBase::create();
        mViewMatrix = getUniformID("view");
        mProjectionMatrix = getUniformID("projection");
        mEquirectangularTexture = getUniformID("equirectangularMap");
        return isOk;
    }

    void setView(const mat4x4& view)
    {
        setUniformValue_Matrix4x4(mViewMatrix, view);
    }

    void setProjection(const mat4x4& projection)
    {
        setUniformValue_Matrix4x4(mProjectionMatrix, projection);
    }

    void setTextureSource(const RasterizerImage* src)
    {
        mTextureUnitCounter = 0;
        setUniformValue_Texture(mEquirectangularTexture, src, 0);
    }

private:
    int mViewMatrix;
    int mProjectionMatrix;
    int mEquirectangularTexture;
};

class IrradianceMapConvolutionShaderProgram : public ShaderProgramBase
{
public:
    IrradianceMapConvolutionShaderProgram()
        : ShaderProgramBase()
    {
        std::string uniforms = "\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"

            "uniform samplerCube environmentMap;\n"
            "\n";

        mVertexShaderSourceStr = ""
            "#version 330 core\n"
            "layout (location = 0) in vec3 position;\n"
            "out vec3 vertexPosition;\n"

            + uniforms +

            "void main()\n"
            "{\n"
            "   vertexPosition = position;\n"
            "   vec4 posVec4 = vec4(position, 1.0f);\n"
            "   gl_Position = projection * view * posVec4;\n"
            "}\0";

        mFragmentShaderSourceStr = ""
            "#version 330 core\n"
            "in vec3 vertexPosition;\n"
            "layout(location = 0) out vec4 color;\n"

            + uniforms +

            "const float PI = 3.14159265359;\n"
            "void main()\n"
            "{\n"
                // the sample direction equals the hemisphere's orientation
            "   vec3 normal = normalize(vertexPosition);\n"
                // compute the irradiance
            "   vec3 irradiance = vec3(0.0);\n"
            "   vec3 up = vec3(0.0, 1.0, 0.0);\n"
            "   vec3 right = normalize(cross(up, normal));\n"
            "   up = normalize(cross(normal, right));\n"
            "   float sampleDelta = 0.025;\n"
            "   float nrSamples = 0.0;\n"
            "   for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)\n"
            "   {\n"
            "       for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)\n"
            "       {\n"
                        // spherical to cartesian (in tangent space)
            "           vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));\n"
                        // tangent space to world
            "           vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;\n"
            "           irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);\n"
            "           nrSamples++;\n"
            "       }\n"
            "   }\n"
            "   irradiance = PI * irradiance * (1.0 / float(nrSamples));\n"
                // out color
            "   color = vec4(irradiance, 1.0);\n"
            "}\n\0";
    }

    virtual bool create() override
    {
        bool isOk = ShaderProgramBase::create();
        mViewMatrix = getUniformID("view");
        mProjectionMatrix = getUniformID("projection");
        mEnvironmentMapTexture = getUniformID("environmentMap");
        return isOk;
    }

    void setView(const mat4x4& view)
    {
        setUniformValue_Matrix4x4(mViewMatrix, view);
    }

    void setProjection(const mat4x4& projection)
    {
        setUniformValue_Matrix4x4(mProjectionMatrix, projection);
    }

    void setTextureSource(const EnvironmentMap* src)
    {
        mTextureUnitCounter = 0;
        setUniformValue_Texture(mEnvironmentMapTexture, src, 0);
    }

private:
    int mViewMatrix;
    int mProjectionMatrix;
    int mEnvironmentMapTexture;
};

class PrefilterMapConvolutionShaderProgram : public ShaderProgramBase
{
public:
    PrefilterMapConvolutionShaderProgram()
        : ShaderProgramBase()
    {
        std::string uniforms = "\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "uniform float roughness;\n"

            "uniform samplerCube environmentMap;\n"
            "\n";

        mVertexShaderSourceStr = ""
            "#version 330 core\n"
            "layout (location = 0) in vec3 position;\n"
            "out vec3 vertexPosition;\n"

            + uniforms +

            "void main()\n"
            "{\n"
            "   vertexPosition = position;\n"
            "   vec4 posVec4 = vec4(position, 1.0f);\n"
            "   gl_Position = projection * view * posVec4;\n"
            "}\0";

        mFragmentShaderSourceStr = ""
            "#version 330 core\n"
            "in vec3 vertexPosition;\n"
            "layout(location = 0) out vec4 color;\n"

            + uniforms +

            "const float PI = 3.14159265359;\n"

            "float RadicalInverse_VdC(uint bits)\n"
            "{\n"
            "    bits = (bits << 16u) | (bits >> 16u);\n"
            "    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
            "    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
            "    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
            "    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
            "    return float(bits) * 2.3283064365386963e-10; // / 0x100000000\n"
            "}\n"

            "vec2 Hammersley(uint i, uint N)\n"
            "{\n"
            "    return vec2(float(i) / float(N), RadicalInverse_VdC(i));\n"
            "}\n"

            "vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)\n"
            "{\n"
            "    float a = roughness * roughness;\n"
            "    float phi = 2.0 * PI * Xi.x;\n"
            "    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));\n"
            "    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);\n"
                 // from spherical coordinates to cartesian coordinates
            "    vec3 H;\n"
            "    H.x = cos(phi) * sinTheta;\n"
            "    H.y = sin(phi) * sinTheta;\n"
            "    H.z = cosTheta;\n"
                 // from tangent-space vector to world-space sample vector
            "    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
            "    vec3 tangent = normalize(cross(up, N));\n"
            "    vec3 bitangent = cross(N, tangent);\n"
            "    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
            "    return normalize(sampleVec);\n"
            "}\n"

            "void main()\n"
            "{\n"
            "    vec3 N = normalize(vertexPosition);\n"
            "    vec3 R = N;\n"
            "    vec3 V = R;\n"
            "    const uint SAMPLE_COUNT = 1024u;\n"
            "    float totalWeight = 0.0;\n"
            "    vec3 prefilteredColor = vec3(0.0);\n"
            "    for (uint i = 0u; i < SAMPLE_COUNT; ++i)\n"
            "    {\n"
            "        vec2 Xi = Hammersley(i, SAMPLE_COUNT);\n"
            "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
            "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
            "        float NdotL = max(dot(N, L), 0.0);\n"
            "        if (NdotL > 0.0)\n"
            "        {\n"
            "            prefilteredColor += texture(environmentMap, L).rgb * NdotL;\n"
            "            totalWeight += NdotL;\n"
            "        }\n"
            "    }\n"
            "    prefilteredColor = prefilteredColor / totalWeight;\n"
            "    color = vec4(prefilteredColor, 1.0);\n"
            "}\n\0";
    }

    virtual bool create() override
    {
        bool isOk = ShaderProgramBase::create();
        mViewMatrix = getUniformID("view");
        mProjectionMatrix = getUniformID("projection");
        mRoughness = getUniformID("roughness");
        mEnvironmentMapTexture = getUniformID("environmentMap");
        return isOk;
    }

    void setView(const mat4x4& view)
    {
        setUniformValue_Matrix4x4(mViewMatrix, view);
    }

    void setProjection(const mat4x4& projection)
    {
        setUniformValue_Matrix4x4(mProjectionMatrix, projection);
    }

    void setRoughness(float roughness)
    {
        setUniformValue_Float(mRoughness, roughness);
    }

    void setTextureSource(const EnvironmentMap* src)
    {
        mTextureUnitCounter = 0;
        setUniformValue_Texture(mEnvironmentMapTexture, src, 0);
    }

private:
    int mViewMatrix;
    int mProjectionMatrix;
    int mRoughness;
    int mEnvironmentMapTexture;
};


//
// Globals
//
vec3 gCameraPosition = vec3(0.0f, 0.0f, 0.0f);
vec3 gCameraDirectionVectors[] =
{
    vec3(1.0f,  0.0f,  0.0f),
    vec3(-1.0f, 0.0f,  0.0f),
    vec3(0.0f,  1.0f,  0.0f),
    vec3(0.0f, -1.0f,  0.0f),
    vec3(0.0f,  0.0f,  1.0f),
    vec3(0.0f,  0.0f, -1.0f),
};
vec3 gCameraUpVectors[] =
{
    vec3(0.0f, -1.0f,  0.0f),
    vec3(0.0f, -1.0f,  0.0f),
    vec3(0.0f,  0.0f,  1.0f),
    vec3(0.0f,  0.0f, -1.0f),
    vec3(0.0f, -1.0f,  0.0f),
    vec3(0.0f, -1.0f,  0.0f),
};
std::unique_ptr<EquirectangularToCubemapShaderProgram> gCubeConversionSP;
std::unique_ptr<IrradianceMapConvolutionShaderProgram> gIrradianceConvolutionSP;
std::unique_ptr<PrefilterMapConvolutionShaderProgram> gPrefilterMapConvolutionSP;
std::unique_ptr<FullScreenCube> gCubeMesh;
uint gCubeTextureUserCounter = 0;

CubeMap::CubeMap()
    : mTexture_glID(0)
    , mDepthRenderbuffer_glID(0)
    , mFBO_glID(0)
    , mSize(0)
{
    if (gCubeTextureUserCounter == 0)
    {
        gCubeConversionSP = std::make_unique<EquirectangularToCubemapShaderProgram>();
        gIrradianceConvolutionSP = std::make_unique<IrradianceMapConvolutionShaderProgram>();
        gPrefilterMapConvolutionSP = std::make_unique<PrefilterMapConvolutionShaderProgram>();
        gCubeMesh = std::make_unique<FullScreenCube>();
    }
    gCubeTextureUserCounter++;
}

CubeMap::~CubeMap()
{
    destroy();

    gCubeTextureUserCounter--;
    if (gCubeTextureUserCounter == 0)
    {
        gCubeConversionSP.reset();
        gIrradianceConvolutionSP.reset();
        gPrefilterMapConvolutionSP.reset();
        gCubeMesh.reset();
    }
}

bool CubeMap::destroy()
{
    int count = 0;
    if (mFBO_glID != 0)
    {
        glDeleteFramebuffers(1, &mFBO_glID);
        mFBO_glID = 0;
        count += 1;
    }
    if (mTexture_glID != 0)
    {
        glDeleteTextures(1, &mTexture_glID);
        mTexture_glID = 0;
        count += 1;
    }
    if (mDepthRenderbuffer_glID != 0)
    {
        glDeleteRenderbuffers(1, &mDepthRenderbuffer_glID);
        mDepthRenderbuffer_glID = 0;
        count += 1;
    }
    return (count == 3);
}

EnvironmentMap::EnvironmentMap()
    : CubeMap()
{

}

bool EnvironmentMap::createFromFileEquirectangularHDR(uint edge, const wchar_t* file)
{
    initializeOpenGLFunctions();
    destroy();

    if (!gCubeConversionSP->isCreated())
    {
        gCubeConversionSP->create();
    }
    if (!gCubeMesh->isCreated())
    {
        gCubeMesh->create();
    }

    UserImage equirectangularImg;
    equirectangularImg.createFromFileHDR(file);

    mSize = (GLuint)(edge);
    glGenFramebuffers(1, &mFBO_glID);
    glGenRenderbuffers(1, &mDepthRenderbuffer_glID);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbuffer_glID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mSize, mSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbuffer_glID);

    glGenTextures(1, &mTexture_glID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTexture_glID);
    for (unsigned int i = 0; i < 6; ++i)
    {
        // note that we store each face with 16 bit floating point values
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
            mSize, mSize, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Convert the texture from equirectangular to cube map
    Camera cam;
    cam.setPerspective(90.0f, 1.0f);
    gCubeConversionSP->bind();
    gCubeConversionSP->setProjection(cam.getProjection());
    gCubeConversionSP->setTextureSource(&equirectangularImg);
    glViewport(0, 0, mSize, mSize);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);
    for (unsigned int i = 0; i < 6; ++i)
    {
        cam.setLookAt(gCameraPosition, gCameraDirectionVectors[i], gCameraUpVectors[i]);
        gCubeConversionSP->setView(cam.getView().matrix());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mTexture_glID, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gCubeMesh->draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gCubeConversionSP->unbind();

    return true;
}

IrradianceMap::IrradianceMap()
    : CubeMap()
{

}

bool IrradianceMap::createFromEnvironmentMap(uint edge, const EnvironmentMap* envMap)
{
    initializeOpenGLFunctions();
    destroy();

    if (!gIrradianceConvolutionSP->isCreated())
    {
        gIrradianceConvolutionSP->create();
    }
    if (!gCubeMesh->isCreated())
    {
        gCubeMesh->create();
    }

    mSize = (GLuint)(edge);
    glGenFramebuffers(1, &mFBO_glID);
    glGenRenderbuffers(1, &mDepthRenderbuffer_glID);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbuffer_glID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mSize, mSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbuffer_glID);

    glGenTextures(1, &mTexture_glID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTexture_glID);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, mSize, mSize, 0,
            GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Compute the irradiance map
    Camera cam;
    cam.setPerspective(90.0f, 1.0f);
    gIrradianceConvolutionSP->bind();
    gIrradianceConvolutionSP->setProjection(cam.getProjection());
    gIrradianceConvolutionSP->setTextureSource(envMap);
    glViewport(0, 0, mSize, mSize);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);
    for (unsigned int i = 0; i < 6; ++i)
    {
        cam.setLookAt(gCameraPosition, gCameraDirectionVectors[i], gCameraUpVectors[i]);
        gIrradianceConvolutionSP->setView(cam.getView().matrix());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mTexture_glID, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gCubeMesh->draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gIrradianceConvolutionSP->unbind();

    return true;
}

PrefilterMap::PrefilterMap()
    : CubeMap()
{

}

bool PrefilterMap::createFromEnvironmentMap(uint edge, const EnvironmentMap* envMap)
{
    initializeOpenGLFunctions();
    destroy();

    if (!gPrefilterMapConvolutionSP->isCreated())
    {
        gPrefilterMapConvolutionSP->create();
    }
    if (!gCubeMesh->isCreated())
    {
        gCubeMesh->create();
    }




    mSize = (GLuint)(edge);
    glGenFramebuffers(1, &mFBO_glID);
    glGenRenderbuffers(1, &mDepthRenderbuffer_glID);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbuffer_glID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mSize, mSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbuffer_glID);

    glGenTextures(1, &mTexture_glID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTexture_glID);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);





    // Compute the prefilter map
    Camera cam;
    cam.setPerspective(90.0f, 1.0f);
    gPrefilterMapConvolutionSP->bind();
    gPrefilterMapConvolutionSP->setProjection(cam.getProjection());
    gPrefilterMapConvolutionSP->setTextureSource(envMap);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = 128 * std::pow(0.5, mip);
        unsigned int mipHeight = 128 * std::pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbuffer_glID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);
        float roughness = (float)mip / (float)(maxMipLevels - 1);
        gPrefilterMapConvolutionSP->setRoughness(roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            cam.setLookAt(gCameraPosition, gCameraDirectionVectors[i], gCameraUpVectors[i]);
            gPrefilterMapConvolutionSP->setView(cam.getView().matrix());
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mTexture_glID, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            gCubeMesh->draw();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gPrefilterMapConvolutionSP->unbind();

    return true;
}
