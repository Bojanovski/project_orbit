#pragma once
#include "GraphicsEngineInterface.h"
#include "GraphicsEngineTypes.h"


namespace graphics_engine
{
    class PathtracerEngine : public PathtracerEngineInterface
    {
        friend class PathtracerEngineWidget;

    public:
        PathtracerEngine();
        ~PathtracerEngine();

        void initialize(void* windowHWND);
        void deinitialize();
        void resize(int w, int h);
        void update(float dt);
        void draw();

        // Interface methods
        virtual const Camera& getCamera();
        virtual void setCameraLookAt(const vec3&, const vec3&, const vec3&);
        virtual RayCastHit rayCast(NodeContainer*, const ray3&) { throw; }
        virtual Scene* createScene(const std::string&);
        virtual Node* importScene(NodeContainer*, Scene*) { throw; }
        virtual Node* importNode(NodeContainer*, Node*) { throw; }
        virtual Node* createNode(
            const std::string&,
            NodeContainer*,
            const vec3&,
            const vec3&,
            const quat&,
            Mesh*,
            Skin*);
        virtual void updateNodeLocalTransform(Node*, const vec3&, const vec3&, const quat&) { throw; }
        virtual bool loadModel(const LoadParameters&, Model*);
        virtual void loadModel_Async(const LoadParameters&, LoadModelCallback) {}
        virtual void setActiveScene(Scene*);
        virtual Scene* getActiveScene();

        virtual Skin* createSkin(const std::vector<mat4x4>&) { return nullptr; }
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
            const std::vector<bounds3>& submeshBounds);
        virtual Mesh* createMeshVariant(const std::vector<Material*>& materials, Mesh* src) { throw; }
        virtual Image* createImage(
            const std::string& name,
            const uchar* data,
            size_t dataSize,
            uint width,
            uint height,
            const std::string& mimeType,
            bool isLinearSpace);
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
            const TextureTransform& transmissionTransform);
        virtual const MouseState& getMouse() { throw; }

        virtual void renderOffline(const RenderJob&);
    
    private:
        void addNodeToCycles(Node*);

    private:
        std::unique_ptr<class PathtracerEnginePrivate> _this;

    };
}

