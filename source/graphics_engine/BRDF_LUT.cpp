#include "BRDF_LUT.h"
#include "ShaderProgramBase.h"
#include "FullScreenQuad.h"


using namespace graphics_engine;

class IntegrateBRDFShaderProgram : public ShaderProgramBase
{
public:
    IntegrateBRDFShaderProgram()
        : ShaderProgramBase()
    {
        std::string uniforms = "\n"
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
            "layout (location = 0) out vec2 color;\n"

            + uniforms +

            "const float PI = 3.14159265359;\n"

            // http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
            // efficient VanDerCorpus calculation.
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
                 // from spherical coordinates to cartesian coordinates - halfway vector
            "    vec3 H;\n"
            "    H.x = cos(phi) * sinTheta;\n"
            "    H.y = sin(phi) * sinTheta;\n"
            "    H.z = cosTheta;\n"
                 // from tangent-space H vector to world-space sample vector
            "    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
            "    vec3 tangent = normalize(cross(up, N));\n"
            "    vec3 bitangent = cross(N, tangent);\n"
            "    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
            "    return normalize(sampleVec);\n"
            "}\n"

            "float GeometrySchlickGGX(float NdotV, float roughness)\n"
            "{\n"
            "    float a = roughness;\n"
            "    float k = (a * a) / 2.0;\n"
            "    float nom = NdotV;\n"
            "    float denom = NdotV * (1.0 - k) + k;\n"
            "    return nom / denom;\n"
            "}\n"

            "float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)\n"
            "{\n"
            "    float NdotV = max(dot(N, V), 0.0);\n"
            "    float NdotL = max(dot(N, L), 0.0);\n"
            "    float ggx2 = GeometrySchlickGGX(NdotV, roughness);\n"
            "    float ggx1 = GeometrySchlickGGX(NdotL, roughness);\n"
            "    return ggx1 * ggx2;\n"
            "}\n"

            "vec2 IntegrateBRDF(float NdotV, float roughness)\n"
            "{\n"
            "    vec3 V;\n"
            "    V.x = sqrt(1.0 - NdotV * NdotV);\n"
            "    V.y = 0.0;\n"
            "    V.z = NdotV;\n"
            "    float A = 0.0;\n"
            "    float B = 0.0;\n"
            "    vec3 N = vec3(0.0, 0.0, 1.0);\n"
            "    const uint SAMPLE_COUNT = 1024u;\n"
            "    for (uint i = 0u; i < SAMPLE_COUNT; ++i)\n"
            "    {\n"
            "        vec2 Xi = Hammersley(i, SAMPLE_COUNT);\n"
            "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
            "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
            "        float NdotL = max(L.z, 0.0);\n"
            "        float NdotH = max(H.z, 0.0);\n"
            "        float VdotH = max(dot(V, H), 0.0);\n"
            "        if (NdotL > 0.0)\n"
            "        {\n"
            "            float G = GeometrySmith(N, V, L, roughness);\n"
            "            float G_Vis = (G * VdotH) / (NdotH * NdotV);\n"
            "            float Fc = pow(1.0 - VdotH, 5.0);\n"
            "            A += (1.0 - Fc) * G_Vis;\n"
            "            B += Fc * G_Vis;\n"
            "        }\n"
            "    }\n"
            "    A /= float(SAMPLE_COUNT);\n"
            "    B /= float(SAMPLE_COUNT);\n"
            "    return vec2(A, B);\n"
            "}\n"

            "void main()\n"
            "{\n"
            "    vec2 integratedBRDF = IntegrateBRDF(texCoord.x, texCoord.y);\n"
            "    color = integratedBRDF;\n"
            "}\n\0";
    }

    virtual bool create() override
    {
        bool isOk = ShaderProgramBase::create();
        return isOk;
    }

private:
};

//
// Globals
//
std::unique_ptr<IntegrateBRDFShaderProgram> gIntegrateBRDFSP;
std::unique_ptr<FullScreenQuad> gQuadMesh;
uint gBRDF_LUT_UserCounter = 0;

BRDF_LUT::BRDF_LUT()
    : mTexture_glID(0)
    , mDepthRenderbuffer_glID(0)
    , mFBO_glID(0)
    , mWidth(0)
    , mHeight(0)
{
    if (gBRDF_LUT_UserCounter == 0)
    {
        gIntegrateBRDFSP = std::make_unique<IntegrateBRDFShaderProgram>();
        gQuadMesh = std::make_unique<FullScreenQuad>();
    }
    gBRDF_LUT_UserCounter++;
}

BRDF_LUT::~BRDF_LUT()
{
    destroy();

    gBRDF_LUT_UserCounter--;
    if (gBRDF_LUT_UserCounter == 0)
    {
        gIntegrateBRDFSP.reset();
        gQuadMesh.reset();
    }
}

bool BRDF_LUT::create(uint size)
{
    initializeOpenGLFunctions();
    destroy();

    if (!gIntegrateBRDFSP->isCreated())
    {
        gIntegrateBRDFSP->create();
    }
    if (!gQuadMesh->isCreated())
    {
        gQuadMesh->create();
    }

    mWidth = (GLuint)(size);
    mHeight = (GLuint)(size);


    // Create an FBO
    glGenFramebuffers(1, &mFBO_glID);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_glID);

    // Create textures for rendering
    glGenTextures(1, &mTexture_glID);
    glBindTexture(GL_TEXTURE_2D, mTexture_glID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, mWidth, mHeight, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture_glID, 0);

    // Create a renderbuffer for depth
    glGenRenderbuffers(1, &mDepthRenderbuffer_glID);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbuffer_glID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mWidth, mHeight);


    // Draw the texture
    gIntegrateBRDFSP->bind();
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gQuadMesh->draw();
    gIntegrateBRDFSP->unbind();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

bool BRDF_LUT::destroy()
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