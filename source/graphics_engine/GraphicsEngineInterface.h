#pragma once
#include "GraphicsEngineTypes.h"
#include "Camera.h"
#include "RenderJob.h"
#include <vector>
#include <string>


namespace graphics_engine
{
    struct MouseState
    {
        float x, y;
        int wheel;
        bool lmb, rmb;
        bool inside;
    };

    struct LoadParameters
    {
        std::wstring filepath;
        LoadedFile* preloadedFile = nullptr;
        bool allAnimations = false;
        bool allVariants = false;
        std::set<std::string> variantFilter;
    };

    typedef void (*LoadModelCallback)(Model*);

    class GraphicsEngineInterface
    {
    public:
        virtual ~GraphicsEngineInterface() = default;

        virtual const Camera& getCamera() = 0;
        virtual void setCameraLookAt(const vec3& pos, const vec3& dir, const vec3& up) = 0;
        virtual RayCastHit rayCast(NodeContainer*, const ray3&) = 0;

        virtual Scene* createScene(const std::string&) = 0;
        virtual Node* importScene(NodeContainer*, Scene*) = 0;
        virtual Node* importNode(NodeContainer*, Node*) = 0;
        virtual Node* createNode(
            const std::string&,
            NodeContainer*,
            const vec3&,
            const vec3&,
            const quat&, 
            Mesh* = nullptr,
            Skin* = nullptr) = 0;
        virtual void updateNodeLocalTransform(Node* target, const vec3& position, const vec3& scale, const quat& rotation) = 0;
        virtual bool loadModel(const LoadParameters&, Model*) = 0;
        virtual void loadModel_Async(const LoadParameters&, LoadModelCallback) = 0;
        virtual void setActiveScene(Scene*) = 0;
        virtual Scene* getActiveScene() = 0;

        virtual Skin* createSkin(const std::vector<mat4x4>& inverseBindMatrices) = 0;
        virtual Mesh* createMesh(
            const std::string& name,
            const std::vector<Material*>& materials,
            const std::vector<Mesh::Buffer>& positionBuffers,
            const std::vector<Mesh::Buffer>& uvBuffers,
            const std::vector<Mesh::Buffer>& normalBuffers,
            const std::vector<Mesh::Buffer>& tangentBuffers,
            const std::vector<Mesh::Buffer>& jointBuffers,
            const std::vector<Mesh::Buffer>& weightBuffers,
            const std::vector<Mesh::Buffer>& indexBuffers,
            const std::vector<Mesh::MorphTargets>& morphTargets,
            const std::vector<bounds3>& submeshBounds) = 0;
        virtual Mesh* createMeshVariant(const std::vector<Material*>& materials, Mesh* src) = 0;
        virtual Image* createImage(
            const std::string& name,
            const uchar* data,
            size_t dataSize,
            uint width,
            uint height,
            const std::string& mimeType, 
            bool isLinearSpace) = 0;
        virtual Material* createMaterial(
            const std::string& name,
            const color& albedoColor,
            const Texture& albedoTexture,
            float metallicFactor,
            float roughnessFactor,
            const Texture& metallicRoughnessTexture,
            float normalScale,
            const Texture& normalTexture,
            const color& emissiveFactor,
            const Texture& emissiveTexture,
            bool uses_KHR_materials_emissive_strength,
            float emissiveStrength,
            bool uses_KHR_materials_transmission,
            float transmissionFactor,
            const Texture& transmissionTexture,
            bool Uses_KHR_texture_transform,
            const TextureTransform& albedoTransform,
            const TextureTransform& metallicRoughnessTransform,
            const TextureTransform& normalTransform,
            const TextureTransform& occlusionTransform,
            const TextureTransform& emissiveTransform,
            const TextureTransform& transmissionTransform) = 0;

        virtual void retargetAnimation(AnimationTarget*, Node*, const AnimationTarget&) { }
        virtual void setAnimationTime(const AnimationTarget&, float, float) {}

        virtual void renderOffline(const RenderJob&) = 0;


        //
        // Input
        //
        virtual const MouseState &getMouse() = 0;
    };

    class RasterizerEngineInterface : public GraphicsEngineInterface
    {
    public:
        virtual void enableContext() = 0;
        virtual void disableContext() = 0;

    };

    class PathtracerEngineInterface : public GraphicsEngineInterface
    {
    public:
        //virtual void renderOffline(const RenderJob&) = 0;
    };

    struct ContextGuard
    {
        ContextGuard(RasterizerEngineInterface* rei) : i(rei) { i->enableContext(); }
        ~ContextGuard() { i->disableContext(); }
    private:
        RasterizerEngineInterface* i;
    };
}
