#pragma once
#include "GraphicsEngineTypes.h"
#include "GraphicsEngineInterface.h"
#include "GLTFModelLoader.h"
#include <typeindex>


namespace graphics_engine
{
    class SceneManager
    {
        friend class RasterizerEngine;
        friend class PathtracerEngine;

    public:
        SceneManager();
        ~SceneManager();

        void create(GraphicsEngineInterface* gei);
        void addModel(const LoadedFile*, bool, bool, const std::set<std::string>&, Model*);

    private:
        Scene* addScene(const tinygltf::Scene&);
        Node* addNode(const tinygltf::Node&);
        Mesh* addMesh(const tinygltf::Mesh&);
        Skin* addSkin(const tinygltf::Skin&);
        Material* addMaterial(const tinygltf::Material&);
        Image* addImage(const tinygltf::Image&, bool);
        Animation* addAnimation(const tinygltf::Animation&);
        AnimationTarget addAnimationTarget(const tinygltf::Animation&);

        void fillIndxBuffer(int, Mesh::Buffer*);
        void fillPosBuffer(int, Mesh::Buffer*);
        void fillUVBuffer(int, const Mesh::Buffer*, Mesh::Buffer*);
        void fillNorBuffer(int, const Mesh::Buffer*, const Mesh::Buffer*, Mesh::Buffer*);
        void fillTanBuffer(int, const Mesh::Buffer*, const Mesh::Buffer*, const Mesh::Buffer*, const Mesh::Buffer*, bool, Mesh::Buffer*);
        template<typename vecType> void fillBufferFromSparseAccessor(const tinygltf::Accessor&, Mesh::Buffer*);

        void retargetAnimation(AnimationTarget*, Node*, const AnimationTarget&);
        void setAnimationTime(const AnimationTarget&, float, float);

        Node* importNode(NodeContainer*, Node*);
        void mapNodeToNode(Node* dest, Node* src, bool clean = true);
        void mapNameToNode(Node* dest, bool clean = true);
        void remapImportedSkins(Node* dest, Node* src);

        // Graph operations
        Node* findCommonRoot(const std::vector<Node*>&);
        RayCastHit rayCast(NodeContainer*, const ray3&);
        RayCastHit rayCastNode(Node*, const transform&, const ray3&);
        void computeAndStoreWorldTransforms(Node*);

    private:
        GraphicsEngineInterface* mGEI;
        const GLTFModelLoader::Model *mWorkingModel;
        std::set<int> mVariantFilter;
        Material* mDefaultMaterial;

        std::vector<std::unique_ptr<MeshTools::DataElement>> mWorkingMeshData;

        std::vector<std::unique_ptr<Scene>> mScenes;
        std::vector<std::unique_ptr<Node>> mNodes;
        std::vector<std::unique_ptr<Skin>> mSkins;
        std::vector<std::unique_ptr<Mesh>> mMeshes;
        std::vector<std::unique_ptr<Material>> mMaterials;
        std::vector<std::unique_ptr<Image>> mImages;
        std::vector<std::unique_ptr<Animation>> mAnimations;

        std::map<Node*, Node*> mNodeToNodeMap;
        std::map<std::string, Node*> mNameToNodeMap;
        std::map<Node*, transform> mNodeToWorldTransform;

        struct TypeIndexKey
        {
            std::type_index type;
            int index;

            bool operator<(const TypeIndexKey& other) const
            {
                return (type < other.type) || ((type == other.type) && (index < other.index));
            }
        };
        std::map<TypeIndexKey, void*> mTypeIndexToObject;
    };
}
