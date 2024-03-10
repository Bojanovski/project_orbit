#pragma once
#include "ShaderProgramBase.h"


#define MAT_ALBEDO_TEX                 0x00000001
#define MAT_NORMAL_TEX                 0x00000002
#define MAT_METALLIC_ROUGHNESS_TEX     0x00000004
#define MAT_EMISSIVE_TEX               0x00000008
#define MAT_TRANSMISSION_TEX           0x00000010
#define MAT_OCCLUSION_TEX              0x00000020

namespace graphics_engine
{
    struct MeshShaderParameters
    {
        uint materialFlags;
        uint morphTargetCount;
        uint coverageKernelDim;

        bool operator<(const MeshShaderParameters& other) const
        {
            return
                (materialFlags < other.materialFlags) ||
                ((materialFlags == other.materialFlags) && (morphTargetCount < other.morphTargetCount)) ||
                ((materialFlags == other.materialFlags) && (morphTargetCount == other.morphTargetCount) && (coverageKernelDim < other.coverageKernelDim));
        }
    };

    class MeshShaderProgramBase : public ShaderProgramBase
    {
    public:
        MeshShaderProgramBase(const MeshShaderParameters& params)
            : ShaderProgramBase()
            , mParams(params)
        { }

        virtual ~MeshShaderProgramBase() = 0 {}

        virtual std::string getVertexShader() const = 0;

        virtual std::string getMorphTargets(int startLocation) const
        {
            std::string ret = "";
            for (size_t i = 0; i < mParams.morphTargetCount; i++)
            {
                ret += "layout (location = " + std::to_string(startLocation + i * 3 + 0) + ") in vec3 mt_posDiff_"  + std::to_string(i) + ";\n";
                ret += "layout (location = " + std::to_string(startLocation + i * 3 + 1) + ") in vec3 mt_norDiff_"  + std::to_string(i) + ";\n";
                ret += "layout (location = " + std::to_string(startLocation + i * 3 + 2) + ") in vec3 mt_tanDiff_"  + std::to_string(i) + ";\n";
            }
            return ret;
        }

        virtual std::string getMorphedPNT() const
        {
            std::string ret = ""
                "   vec3 morphed_P = position;\n"
                "   vec3 morphed_N = normal;\n"
                "   vec3 morphed_T = tangent.xyz;\n";
            for (size_t i = 0; i < mParams.morphTargetCount; i++)
            {
                ret += "   morphed_P += morphTargetWeights[" + std::to_string(i) + "] * mt_posDiff_"   + std::to_string(i) + ";\n";
                ret += "   morphed_N += morphTargetWeights[" + std::to_string(i) + "] * mt_norDiff_"   + std::to_string(i) + ";\n";
                ret += "   morphed_T += morphTargetWeights[" + std::to_string(i) + "] * mt_tanDiff_"   + std::to_string(i) + ";\n";
            }
            return ret;
        }

        virtual std::string getConsts() const
        {
            int coverageKernelTileSize = mParams.coverageKernelDim * mParams.coverageKernelDim;
            int coverageKernelDepth = coverageKernelTileSize + 1;
            int coverageKernelSize = coverageKernelDepth * coverageKernelTileSize;
            std::vector<bool> coverageKernel;
            coverageKernel.reserve(coverageKernelSize);
            if (mParams.coverageKernelDim == 2)
            {
                std::vector<bool> coverageKernelTile(coverageKernelTileSize, false);

                // 0
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 1
                coverageKernelTile[0 + 0 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 2
                coverageKernelTile[1 + 1 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 3
                coverageKernelTile[0 + 1 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 4
                coverageKernelTile[1 + 0 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());
            }
            else if (mParams.coverageKernelDim == 4)
            {
                std::vector<bool> coverageKernelTile(coverageKernelTileSize, false);

                // 0
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 1
                coverageKernelTile[1 + 1 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 2
                coverageKernelTile[2 + 2 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 3
                coverageKernelTile[0 + 3 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 4
                coverageKernelTile[3 + 0 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 5
                coverageKernelTile[0 + 1 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 6
                coverageKernelTile[3 + 2 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 7
                coverageKernelTile[2 + 3 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 8
                coverageKernelTile[1 + 0 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 9
                coverageKernelTile[1 + 2 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 10
                coverageKernelTile[2 + 1 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 11
                coverageKernelTile[1 + 3 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 12
                coverageKernelTile[3 + 1 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 13
                coverageKernelTile[3 + 3 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 14
                coverageKernelTile[0 + 0 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 15
                coverageKernelTile[0 + 2 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());

                // 16
                coverageKernelTile[2 + 0 * mParams.coverageKernelDim] = true;
                coverageKernel.insert(coverageKernel.end(), coverageKernelTile.begin(), coverageKernelTile.end());
            }
            else
            {
                throw;
            }

            std::string coverageKernelStr = "";
            for (size_t i = 0; i < coverageKernel.size(); ++i)
            {
                coverageKernelStr += std::to_string(coverageKernel[i]);
                if ((i + 1) < coverageKernel.size())
                    coverageKernelStr += ", ";
            }
            std::string consts = "\n"
                "const int coverageKernelDim = " + std::to_string(mParams.coverageKernelDim) + ";\n"
                "const int coverageKernelDepth = " + std::to_string(coverageKernelDepth) + ";\n"
                "const int coverageKernelTileSize = " + std::to_string(coverageKernelTileSize) + ";\n"
                "const bool coverageKernel[" + std::to_string(coverageKernel.size()) + "] = bool[] ( " + coverageKernelStr + " );\n";
            return consts;
        }

        virtual std::string getUniforms() const
        {
            std::string morphTargetWeights;
            if (mParams.morphTargetCount > 0)
                morphTargetWeights = "uniform float morphTargetWeights[" + std::to_string(mParams.morphTargetCount) + "];\n";
            else
                morphTargetWeights = "\n";

            // create the uniforms substring of the shader
            std::string uniforms = "\n"
                "uniform mat4 MVP;\n"
                "uniform mat4 MV;\n"
                "uniform mat4 VP;\n"
                "uniform mat4 P;\n"
                "uniform mat4 V;\n"
                "uniform mat4 M;\n"
                "uniform mat4 M_InverseTranspose;\n"
                "uniform mat4 MV_InverseTranspose;\n"

                + morphTargetWeights +

                "uniform vec4 albedoColor;\n"
                "uniform sampler2D albedoTexture;\n"

                "uniform float metallicFactor;\n"
                "uniform float roughnessFactor;\n"
                "uniform sampler2D metallicRoughnessTexture;\n"

                "uniform float normalScale;\n"
                "uniform sampler2D normalTexture;\n"

                "uniform vec4 emissiveFactor;\n"
                "uniform sampler2D emissiveTexture;\n"
                "uniform float emissiveStrength;\n"

                "uniform float transmissionFactor;\n"
                "uniform sampler2D transmissionTexture;\n"
                "\n";
            return uniforms;
        }

        virtual std::string getFragmentShader() const
        {
            std::string albedoStr = "   albedo = albedoColor;\n";
            if (mParams.materialFlags & MAT_ALBEDO_TEX)
                albedoStr += "   albedo = albedo * texture(albedoTexture, vertexUV);\n";

            std::string normalStr;
            if (mParams.materialFlags & MAT_NORMAL_TEX)
                normalStr = "   vec3 normalSample = 2.0 * texture(normalTexture, vertexUV).rgb - 1.0;\n"
                "   normalSample = normalSample * vec3(normalScale, normalScale, 1.0f);\n"
                "   normalSample = normalize(normalSample);\n";
            else
                normalStr = "   vec3 normalSample = vec3(0.0f, 0.0f, 1.0f);\n";

            std::string metallicRoughnessStr = "   vec3 metallicRoughness = vec3(0.0f, roughnessFactor, metallicFactor);\n";
            if (mParams.materialFlags & MAT_METALLIC_ROUGHNESS_TEX)
                metallicRoughnessStr += "   metallicRoughness *= texture(metallicRoughnessTexture, vertexUV).rgb;\n";

            std::string emissiveStr = "   vec3 emissive_rgb = emissiveFactor.rgb;\n";
            if (mParams.materialFlags & MAT_EMISSIVE_TEX)
                emissiveStr += "   emissive_rgb *= texture(emissiveTexture, vertexUV).rgb;\n";
            emissiveStr += "   emissive = emissive_rgb * emissiveStrength;\n";

            std::string transmissionStr = "   float transmission = transmissionFactor;\n";
            if (mParams.materialFlags & MAT_TRANSMISSION_TEX)
                transmissionStr += "   transmission *= texture(transmissionTexture, vertexUV).r;\n";
            transmissionStr += "   transmission = mix(transmission, 0.0f, metallicRoughness.b);\n";

            std::string fragmentShader = ""
                "#version 330 core\n"
                "in vec3 vertexPosition;\n"
                "in vec3 vertexNormal;\n"
                "in vec3 vertexTangent;\n"
                "in vec3 vertexBitangent;\n"
                "in vec2 vertexUV;\n"
                "layout(location = 0) out vec4 albedo;\n"
                "layout(location = 1) out vec4 combinedValues_0;\n"
                "layout(location = 2) out vec3 normal;\n"
                "layout(location = 3) out vec3 position;\n"
                "layout(location = 4) out vec3 emissive;\n"

                + getConsts() + ""
                + getUniforms() +

                "void coverageDiscard(int coverageLevel)"
                "{\n"
                "   vec2 sc = gl_FragCoord.xy - vec2(0.5, 0.5) ;\n"
                "   ivec2 coord;\n"
                "   coord.x = int(sc.x) % coverageKernelDim;\n"
                "   coord.y = int(sc.y) % coverageKernelDim;\n"
                "   if (!coverageKernel[coord.x + coord.y * coverageKernelDim + coverageLevel * coverageKernelTileSize]) discard;\n"
                "}\n"

                "void main()\n"
                "{\n"
                + albedoStr + metallicRoughnessStr + transmissionStr +
                "   int maxCoverageLevel = coverageKernelDepth - 1;\n"
                "   float coverageValueSize = 1.0 / float(maxCoverageLevel);\n"
                "   float absorption = 1.0 - transmission;\n"
                "   float alpha = albedo.a;\n"
                "   float coverage = alpha * absorption;\n"
                "   int coverageLevel = int(coverage / coverageValueSize);\n"
                // make sure that only sufficiently small alpha can produce coverageLevel that is 0
                "   if (alpha >= coverageValueSize) coverageLevel = max(1, coverageLevel);\n"
                "   coverageDiscard(coverageLevel);\n"
                // calculate and store the actual coverage to the alpha channel
                "   albedo.a = float(coverageLevel) * coverageValueSize;\n"

                "   vec3 t = normalize(vertexTangent);\n"
                "   vec3 b = normalize(vertexBitangent);\n"
                "   vec3 n = normalize(vertexNormal);\n"
                "   mat3 tbnMatrix = mat3(t, b, n);\n"

                + normalStr +
                "   normal = normalize(tbnMatrix * normalSample);\n"

                + emissiveStr +
                "   combinedValues_0 = vec4(transmission, metallicRoughness.g, metallicRoughness.b, 0.0);\n"

                "   position = vertexPosition;\n"
                "}\n\0";
            return fragmentShader;
        }

        bool create() override
        {
            mVertexShaderSourceStr = getVertexShader();
            mFragmentShaderSourceStr = getFragmentShader();
            bool isOk = ShaderProgramBase::create();

            mMorphTargetWeights = getUniformID("morphTargetWeights");
            mDefaultMorphTargetWeights.resize(mParams.morphTargetCount, 0.0f);

            mViewMatrix = getUniformID("V");
            mProjectionMatrix = getUniformID("P");
            mViewProjectionMatrix = getUniformID("VP");

            mAlbedoColor = getUniformID("albedoColor");
            mAlbedoTexture = getUniformID("albedoTexture");

            mMetallicFactor = getUniformID("metallicFactor");
            mRoughnessFactor = getUniformID("roughnessFactor");
            mMetallicRoughnessTexture = getUniformID("metallicRoughnessTexture");

            mNormalScale = getUniformID("normalScale");
            mNormalTexture = getUniformID("normalTexture");

            mEmissiveFactor = getUniformID("emissiveFactor");
            mEmissiveTexture = getUniformID("emissiveTexture");
            mEmissiveStrength = getUniformID("emissiveStrength");

            mTransmissionFactor = getUniformID("transmissionFactor");
            mTransmissionTexture = getUniformID("transmissionTexture");

            return isOk;
        }

        void setMorphTargetWeights(const std::vector<float>& weights)
        {
            if (weights.size() == mParams.morphTargetCount)
                setUniformValue_FloatArray(mMorphTargetWeights, weights);
            else
                setUniformValue_FloatArray(mMorphTargetWeights, mDefaultMorphTargetWeights);
        }

        void setCameraTransforms(const mat4x4& view, const mat4x4& projection)
        {
            mView = view;
            mProjection = projection;
            mViewProjection = mProjection * mView;
            setUniformValue_Matrix4x4(mViewMatrix, mView);
            setUniformValue_Matrix4x4(mProjectionMatrix, mProjection);
            setUniformValue_Matrix4x4(mViewProjectionMatrix, mViewProjection);
        }

        void setSeed(uint)
        {
            //std::mt19937 rng(seed);
            //std::uniform_real_distribution<float> dist(0.0f, 100.0f);
            //float x = dist(rng);
            //float y = dist(rng);
        }

        void setMaterial(const Material& mat)
        {
            mTextureUnitCounter = 0;
            setUniformValue_Color(mAlbedoColor, mat.albedoColor);
            setUniformValue_Texture(mAlbedoTexture, (RasterizerImage*)mat.albedoTexture.image, 0);

            setUniformValue_Float(mMetallicFactor, mat.metallicFactor);
            setUniformValue_Float(mRoughnessFactor, mat.roughnessFactor);
            setUniformValue_Texture(mMetallicRoughnessTexture, (RasterizerImage*)mat.metallicRoughnessTexture.image, 0);

            setUniformValue_Float(mNormalScale, mat.normalScale);
            setUniformValue_Texture(mNormalTexture, (RasterizerImage*)mat.normalTexture.image, 0);

            setUniformValue_Color(mEmissiveFactor, mat.emissiveFactor);
            setUniformValue_Texture(mEmissiveTexture, (RasterizerImage*)mat.emissiveTexture.image, 0);
            setUniformValue_Float(mEmissiveStrength, mat.emissiveStrength);

            setUniformValue_Float(mTransmissionFactor, mat.transmissionFactor);
            setUniformValue_Texture(mTransmissionTexture, (RasterizerImage*)mat.transmissionTexture.image, 0);
        }

    protected:
        const MeshShaderParameters mParams;

        mat4x4 mView, mProjection, mViewProjection;

        int mMorphTargetWeights;
        std::vector<float> mDefaultMorphTargetWeights;

        int mViewMatrix = -1;
        int mProjectionMatrix = -1;
        int mViewProjectionMatrix = -1;

        int mAlbedoColor = -1;
        int mAlbedoTexture = -1;

        int mMetallicFactor = -1;
        int mRoughnessFactor = -1;
        int mMetallicRoughnessTexture = -1;

        int mNormalScale = -1;
        int mNormalTexture = -1;

        int mEmissiveFactor = -1;
        int mEmissiveTexture = -1;
        int mEmissiveStrength = -1;

        int mTransmissionFactor = -1;
        int mTransmissionTexture = -1;
    };

    class StaticMeshShaderProgram : public MeshShaderProgramBase
    {
    public:
        StaticMeshShaderProgram(const MeshShaderParameters& params)
            : MeshShaderProgramBase(params)
        { }

        virtual std::string getVertexShader() const override
        {
            std::string vertexShader = ""
                "#version 330 core\n"
                "layout (location = 0) in vec3 position;\n"
                "layout (location = 1) in vec2 uv;\n"
                "layout (location = 2) in vec3 normal;\n"
                "layout (location = 3) in vec4 tangent;\n"

                + getMorphTargets(4) +

                "out vec3 vertexPosition;\n"
                "out vec3 vertexNormal;\n"
                "out vec3 vertexTangent;\n"
                "out vec3 vertexBitangent;\n"
                "out vec2 vertexUV;\n"

                + getConsts() + ""
                + getUniforms() +

                "void main()\n"
                "{\n"

                + getMorphedPNT() +

                "   vec4 posVec4 = vec4(morphed_P, 1.0f);\n"
                "   vertexPosition = (MV * posVec4).xyz;\n"
                "   gl_Position = MVP * posVec4;\n"
                "   vertexUV = uv;\n"

                "   vertexNormal = (MV_InverseTranspose * vec4(morphed_N, 1.0)).xyz;\n"
                "   vertexTangent = (MV_InverseTranspose * vec4(morphed_T, 1.0)).xyz;\n"
                "   float fSign = tangent.w;\n"
                "   vertexBitangent = fSign * cross(vertexNormal, vertexTangent);\n"

                "}\0";
            return vertexShader;
        }

        bool create() override
        {
            bool isOk = MeshShaderProgramBase::create();

            // Get a handle for the uniforms
            mModelViewProjectionMatrix = getUniformID("MVP");
            mModelViewMatrix = getUniformID("MV");
            mModelMatrix = getUniformID("M");
            mModelInverseTransposeMatrix = getUniformID("M_InverseTranspose");
            mModelViewInverseTransposeMatrix = getUniformID("MV_InverseTranspose");

            return isOk;
        }

        void setMeshTransform(const transform& model)
        {
            mat4x4 mv = mView * model.matrix();
            setUniformValue_Matrix4x4(mModelViewMatrix, mv);
            mat4x4 mvp = mViewProjection * model.matrix();
            setUniformValue_Matrix4x4(mModelViewProjectionMatrix, mvp);
            setUniformValue_Transform(mModelMatrix, model);
            auto modelInverse = model.inverse().matrix();
            auto modelInverseTranspose = modelInverse.transpose();
            setUniformValue_Matrix4x4(mModelInverseTransposeMatrix, modelInverseTranspose);
            auto modelViewInverseTranspose = mv.inverse().transpose();
            setUniformValue_Matrix4x4(mModelViewInverseTransposeMatrix, modelViewInverseTranspose);
        }

    private:
        int mModelViewProjectionMatrix = -1;
        int mModelViewMatrix = -1;
        int mModelMatrix = -1;
        int mModelInverseTransposeMatrix = -1;
        int mModelViewInverseTransposeMatrix = -1;
    };

    class SkinnedMeshShaderProgram : public MeshShaderProgramBase
    {
    public:
        SkinnedMeshShaderProgram(const MeshShaderParameters& params)
            : MeshShaderProgramBase(params)
            , mMaxBones(50)
        { }

        virtual std::string getVertexShader() const override
        {
            std::string vertexShader = ""
                "#version 330 core\n"
                "layout (location = 0) in vec3 position;\n"
                "layout (location = 1) in vec2 uv;\n"
                "layout (location = 2) in vec3 normal;\n"
                "layout (location = 3) in vec4 tangent;\n"
                "layout (location = 4) in uvec4 boneID;\n"
                "layout (location = 5) in vec4 weight;\n"

                + getMorphTargets(6) +

                "out vec3 vertexPosition;\n"
                "out vec3 vertexNormal;\n"
                "out vec3 vertexTangent;\n"
                "out vec3 vertexBitangent;\n"
                "out vec2 vertexUV;\n"

                + getConsts() + ""
                + getUniforms() +
                "uniform mat4 boneTransforms[" + std::to_string(mMaxBones) + "];\n"
                "uniform int numBones;\n"

                "void main()\n"
                "{\n"
                // Calculate skinned vertex data
                "   vec4 posVec4 = vec4(position, 1.0f);\n"
                "   float fSign = tangent.w;\n"
                "   vec3 bitangent = fSign * cross(normal, tangent.xyz);\n"
                "   vec4 skinnedPosition = vec4(0.0);\n"
                "   vec3 skinnedNormal = vec3(0.0);\n"
                "   vec3 skinnedTangent = vec3(0.0);\n"
                "   vec3 skinnedBitangent = vec3(0.0);\n"
                "   for (int i = 0; i < 4; ++i) {\n"
                "      mat4 boneTransform = boneTransforms[boneID[i]];\n"
                "      float w = weight[i];\n"
                "      skinnedPosition  += boneTransform * posVec4 * w;\n"
                "      skinnedNormal    += mat3(boneTransform) * normal * w;\n"
                "      skinnedTangent   += mat3(boneTransform) * tangent.xyz * w;\n"
                "      skinnedBitangent += mat3(boneTransform) * bitangent * w;\n"
                "   }\n"
                // Move to view space and normalize
                "   vertexPosition  = (V * skinnedPosition).xyz;\n"
                "   vertexNormal    = normalize(mat3(V) * skinnedNormal);\n"
                "   vertexTangent   = normalize(mat3(V) * skinnedTangent);\n"
                "   vertexBitangent = normalize(mat3(V) * skinnedBitangent);\n"
                "   vertexUV = uv;\n"

                "   gl_Position = P * vec4(vertexPosition, 1.0f);\n"
                "}\0";
            return vertexShader;
        }

        bool create() override
        {
            bool isOk = MeshShaderProgramBase::create();

            mModelViewProjectionMatrix = getUniformID("MVP");
            mModelViewMatrix = getUniformID("MV");
            mModelMatrix = getUniformID("M");
            mModelInverseTransposeMatrix = getUniformID("M_InverseTranspose");
            mModelViewInverseTransposeMatrix = getUniformID("MV_InverseTranspose");

            mBoneTransforms = getUniformID("boneTransforms");
            mBoneCount = getUniformID("numBones");

            return isOk;
        }

        void setBones(const std::vector<mat4x4>& boneTransforms)
        {
            assert(boneTransforms.size() <= mMaxBones);

            setUniformValue_Int(mBoneCount, (int)boneTransforms.size());
            setUniformValue_Matrix4x4Array(mBoneTransforms, boneTransforms);


            //mat4x4 mv = mView * model.matrix();
            //setUniformValue_Matrix4x4(mModelViewMatrix, mv);
            //mat4x4 mvp = mViewProjection * model.matrix();
            //setUniformValue_Matrix4x4(mModelViewProjectionMatrix, mvp);
            //setUniformValue_Transform(mModelMatrix, model);
            //auto modelInverse = model.inverse().matrix();
            //auto modelInverseTranspose = modelInverse.transpose();
            //setUniformValue_Matrix4x4(mModelInverseTransposeMatrix, modelInverseTranspose);
            //auto modelViewInverseTranspose = mv.inverse().transpose();
            //setUniformValue_Matrix4x4(mModelViewInverseTransposeMatrix, modelViewInverseTranspose);
        }

    private:
        const uint mMaxBones;

        int mBoneTransforms = -1;
        int mBoneCount = -1;


        int mModelViewProjectionMatrix = -1;
        int mModelViewMatrix = -1;
        int mModelMatrix = -1;
        int mModelInverseTransposeMatrix = -1;
        int mModelViewInverseTransposeMatrix = -1;
    };
}
