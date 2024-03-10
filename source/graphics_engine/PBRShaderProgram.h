#pragma once
#include "GBuffer.h"
#include "ImageShaderPrograms.h"
#include "BRDF_LUT.h"
#include <cmath>


namespace graphics_engine
{
    class PBRShaderProgram : public ImageShaderPrograms
    {
    private:
        struct PointLight
        {
            vec3 position;
            vec3 radiantFlux;
        };

    public:
        PBRShaderProgram()
            : ImageShaderPrograms()
            , mPointLightCount(16)
        {
            mPointLights.resize(mPointLightCount);

            std::string consts = "\n"
                "const float PI = " + std::to_string(M_PI) + ";\n"
                "const int pointLightCount = " + std::to_string(mPointLightCount) + ";\n"
                "\n";

            std::string uniforms = "\n"
                "uniform mat4 inverseView;\n"
                
                "uniform sampler2D positionTexture;\n"
                "uniform sampler2D normalTexture;\n"
                "uniform sampler2D albedoTexture;\n"
                "uniform sampler2D combinedValuesTexture_0;\n"
                "uniform sampler2D emissiveTexture;\n"
                "uniform sampler2D emissiveBlurredTexture;\n"
                "uniform sampler2D ssaoTexture;\n"
                
                "uniform samplerCube irradianceTexture;\n"
                "uniform samplerCube prefilterTexture;\n"
                "uniform sampler2D brdfLUT_Texture;\n"
                
                "uniform int pointLightInUse;\n"
                "uniform vec3 plPosition[pointLightCount];\n"
                "uniform vec3 plRadiantFlux[pointLightCount];\n"
                "\n";

            mVertexShaderSourceStr = ""
                "#version 330 core\n"
                "layout (location = 0) in vec3 position;\n"
                "out vec2 texCoord;\n"

                + consts + ""
                + uniforms + ""
                //+ NormalDistributionFunction() + ""

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
                + uniforms + ""
                + Functions() + ""

                "void main()\n"
                "{\n"
                "   ivec2 textelCoord = ivec2(gl_FragCoord.xy - vec2(0.5, 0.5));\n"
                "   vec4 albedoVec4 = texelFetch(albedoTexture, textelCoord, 0);\n"
                "   vec3 albedo = albedoVec4.rgb;\n"
                "   float coverage = albedoVec4.a;\n"
                "   if (coverage == 0.0) discard;\n"
                "   vec4 combinedValues_0 = texelFetch(combinedValuesTexture_0, textelCoord, 0);\n"
                "   float transmission = combinedValues_0.r;\n"
                "   float metallic = combinedValues_0.b;\n"
                "   float roughness = combinedValues_0.g;\n"
                "   vec3 P = texelFetch(positionTexture, textelCoord, 0).rgb;\n"
                "   vec3 N = texelFetch(normalTexture, textelCoord, 0).rgb;\n"
                "   float ao = texelFetch(ssaoTexture, textelCoord, 0).r;\n"
                "   vec3 emissive = texelFetch(emissiveTexture, textelCoord, 0).rgb;\n"
                "   vec3 camPos = vec3(0.0);\n"
                "   vec3 V = normalize(camPos - P);\n"
                "   vec3 R = reflect(-V, N);\n"

                // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
                // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
                "   vec3 F0 = vec3(0.04);\n"
                "   F0 = mix(F0, albedo, metallic);\n"

                // reflectance equation
                "   vec3 Lo = vec3(0.0);\n"
                "   int iterations = min(pointLightCount, pointLightInUse);\n"
                "   for(int i = 0; i < iterations; ++i)\n"
                "   {\n"
                        // calculate per-light radiance
                "       vec3 L = normalize(plPosition[i] - P);\n"
                "       vec3 H = normalize(V + L);\n"
                "       float distance    = length(plPosition[i] - P);\n"
                "       float attenuation = 1.0 / (distance * distance);\n"
                "       vec3 radiance     = plRadiantFlux[i] * attenuation;\n"

                "       vec3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);\n"

                "       float NDF = DistributionGGX(N, H, roughness);\n"
                "       float G   = GeometrySmith(N, V, L, roughness);\n"

                "       vec3 numerator    = NDF * G * F;\n"
                "       float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0)  + 0.0001;\n"
                "       vec3 specular     = numerator / denominator;\n"

                "       vec3 kS = F;\n"
                "       vec3 kD = vec3(1.0) - kS;\n"
                "       kD *= 1.0 - metallic;\n"

                "       vec3 diffuse = kD * albedo / PI;\n"

                "       vec3 diffuse_transmission = vec3(0.0);\n"
                "       diffuse = mix(diffuse, diffuse_transmission, transmission);\n"

                "       vec3 specular_transmission = specular / coverage;\n"
                "       specular = mix(specular, specular_transmission, transmission);\n"

                "       float NdotL = max(dot(N, L), 0.0);\n"
                "       Lo += (diffuse + specular ) * radiance * NdotL;\n"

                "   }\n"

                // IBL
                "   vec3 N_worldSpace = mat3(inverseView) * N;\n"
                "   vec3 R_worldSpace = mat3(inverseView) * R;\n"
                "   vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);\n"
                "   vec3 kS = F;\n"
                "   vec3 kD = 1.0 - kS;\n"
                "   kD *= 1.0 - metallic;\n"
                "   vec3 irradiance = texture(irradianceTexture, N_worldSpace).rgb;\n"
                "   vec3 diffuse = kD * irradiance * albedo;\n"

                "   const float MAX_REFLECTION_LOD = 4.0;\n"
                "   vec3 prefilteredColor = textureLod(prefilterTexture, R_worldSpace, roughness * MAX_REFLECTION_LOD).rgb;\n"
                "   vec2 envBRDF = texture(brdfLUT_Texture, vec2(max(dot(N, V), 0.0), roughness)).rg;\n"
                "   vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);\n"

                "   vec3 diffuse_transmission = vec3(0.0);\n"
                "   diffuse = mix(diffuse, diffuse_transmission, transmission);\n"

                "   vec3 specular_transmission = specular / coverage;\n"
                "   specular = mix(specular, specular_transmission, transmission);\n"

                "   vec3 ambient = (diffuse + specular ) * ao;\n"

                // Final color
                "   color.rgb = ambient + Lo + emissive;\n"
                "   color.a = coverage;\n"

                "}\n\0";
        }

        virtual bool create() override
        {
            bool isOk = ShaderProgramBase::create();

            mInverseViewMatrix = getUniformID("inverseView");
            mPointLightsInUse = getUniformID("pointLightInUse");
            mPositionPointLights = getUniformID("plPosition");
            mRadiantFluxPointLights = getUniformID("plRadiantFlux");

            mPositionTexture = getUniformID("positionTexture");
            mNormalTexture = getUniformID("normalTexture");
            mAlbedoTexture = getUniformID("albedoTexture");
            mCombinedValuesTexture_0 = getUniformID("combinedValuesTexture_0");
            mEmissiveTexture = getUniformID("emissiveTexture");
            mSSAOTexture = getUniformID("ssaoTexture");
            mIrradianceTexture = getUniformID("irradianceTexture");
            mPrefilterTexture = getUniformID("prefilterTexture");
            mBRDF_LUT_Texture = getUniformID("brdfLUT_Texture");
            return isOk;
        }

        virtual void bind() override
        {
            ImageShaderPrograms::bind();
            mPointLightsInUseCount = 0;
        }

        void addPointLight(const vec3 &worldSpacePosition, const vec3 &luminousFlux)
        {
            assert(mPointLightsInUseCount < mPointLightCount);
            // Luminous efficacy (lumenToWatt) of an ideal monochromatic source: 555 nm:
            // https://en.wikipedia.org/wiki/Luminous_efficacy
            const float lumenToWatt = 1.0f / 683.002f;
            vec3 radiantFlux = luminousFlux * lumenToWatt; // lument to watt
            mPointLights[mPointLightsInUseCount].position = worldSpacePosition;
            mPointLights[mPointLightsInUseCount].radiantFlux = radiantFlux;
            mPointLightsInUseCount++;
        }

        void setTransforms(const mat4x4& projection, const mat4x4& view)
        {
            (void)projection;
            addPointLight(vec3(0.0f, 0.9f, 0.0f), 5000.0f * vec3(1.0f, 0.0f, 0.0f));

            std::vector<vec3> plPos(mPointLightCount);
            std::vector<vec3> plCol(mPointLightCount);
            for(int i = 0; i < mPointLightCount; ++i)
            {
                if (i < mPointLightsInUseCount)
                {
                    vec4 pos = view * mPointLights[i].position.homogeneous();
                    plPos[i] = pos.hnormalized();
                    plCol[i] = mPointLights[i].radiantFlux;
                }
                else
                {
                    break;
                }
            }

            setUniformValue_Matrix4x4(mInverseViewMatrix, view.inverse());
            setUniformValue_Int(mPointLightsInUse, mPointLightsInUseCount);
            setUniformValue_Vector3Array(mPositionPointLights, plPos);
            setUniformValue_Vector3Array(mRadiantFluxPointLights, plCol);
        }

        void setGBuffer(const GBuffer* gbuf)
        {
            mTextureUnitCounter = 0;
            setUniformValue_Texture(mPositionTexture, gbuf, (int)GBuffer::BufferType::Position);
            setUniformValue_Texture(mNormalTexture, gbuf, (int)GBuffer::BufferType::Normal);
            setUniformValue_Texture(mAlbedoTexture, gbuf, (int)GBuffer::BufferType::Albedo);
            setUniformValue_Texture(mCombinedValuesTexture_0, gbuf, (int)GBuffer::BufferType::CombinedValues_0);
            setUniformValue_Texture(mEmissiveTexture, gbuf, (int)GBuffer::BufferType::Emissive);
        }

        void setSSAO(const RasterizerImage* src)
        {
            setUniformValue_Texture(mSSAOTexture, src, 0);
        }

        void setIBL(const IrradianceMap* irradiance, const PrefilterMap* prefilter, const BRDF_LUT* brdflut)
        {
            setUniformValue_Texture(mIrradianceTexture, irradiance, 0);
            setUniformValue_Texture(mPrefilterTexture, prefilter, 0);
            setUniformValue_Texture(mBRDF_LUT_Texture, brdflut, 0);
        }

    private:
        std::string Functions() const
        {
            auto retVal = ""
                "float DistributionGGX(vec3 N, vec3 H, float roughness)\n"
                "{\n"
                "    float a      = roughness*roughness;\n"
                "    float a2     = a*a;\n"
                "    float NdotH  = max(dot(N, H), 0.0);\n"
                "    float NdotH2 = NdotH*NdotH;\n"
                "    float num   = a2;\n"
                "    float denom = (NdotH2 * (a2 - 1.0) + 1.0);\n"
                "    denom = PI * denom * denom;\n"
                "    return num / denom;\n"
                "}\n"

                "float GeometrySchlickGGX(float NdotV, float roughness)\n"
                "{\n"
                "    float r = (roughness + 1.0);\n"
                "    float k = (r*r) / 8.0;\n"
                "    float num   = NdotV;\n"
                "    float denom = NdotV * (1.0 - k) + k;\n"
                "    return num / denom;\n"
                "}\n"

                "float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)\n"
                "{\n"
                "    float NdotV = max(dot(N, V), 0.0);\n"
                "    float NdotL = max(dot(N, L), 0.0);\n"
                "    float ggx2  = GeometrySchlickGGX(NdotV, roughness);\n"
                "    float ggx1  = GeometrySchlickGGX(NdotL, roughness);\n"
                "    return ggx1 * ggx2;\n"
                "}\n"

                "vec3 fresnelSchlick(float cosTheta, vec3 F0)\n"
                "{\n"
                "    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);\n"
                "}\n"

                "vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)\n"
                "{\n"
                "    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);\n"
                "}\n"
                "\0";
            return retVal;
        }

    private:
        const int mPointLightCount;
        std::vector<PointLight> mPointLights;
        int mPointLightsInUseCount = 0;

        int mInverseViewMatrix;
        int mPointLightsInUse;
        int mPositionPointLights;
        int mRadiantFluxPointLights;
        int mPositionTexture;
        int mNormalTexture;
        int mAlbedoTexture;
        int mCombinedValuesTexture_0;
        int mEmissiveTexture;
        int mSSAOTexture;
        int mIrradianceTexture;
        int mPrefilterTexture;
        int mBRDF_LUT_Texture;
    };
}
