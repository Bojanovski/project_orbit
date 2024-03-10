#pragma once
#include "GraphicsEngineTypes.h"
#include <tiny_gltf.h>
#include <set>
#include <memory>


namespace graphics_engine
{
    class GLTFModelLoader
    {
    public:
        class Model : public tinygltf::Model, public graphics_engine::LoadedFile
        {
            friend class GLTFModelLoader;

        public:
            bool IsInLinearSpace(int textureIndex) const;
            bool IsRoot(int nodeIndex, int* parentNodeIndex, int* sceneIndex) const;
            bool IsJoint(int nodeIndex, int* skinIndex, int* position) const;
            bool IsSkeleton(int nodeIndex, std::vector<int>* skins) const;

        private:
            void SetNodeHierarchy();
            void SetNodeHierarchyRecursive(int nodeIndex, int parentIndex);
            void SetLinearTextureIndexSet();
            void SetSkinMaps();
            void SetFilePath(const std::wstring filepath) { mFilePath = filepath; }

        private:
            std::map<int, int> mNodeToScene;
            std::map<int, int> mNodeToParent;
            std::set<int> mLinearTextureIndexSet;
            std::map<int, int> mJointToSkin;
            std::map<int, int> mJointToPosition;
            std::map<int, std::vector<int>> mSkeletonToSkins;
        };

        // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_transmission/README.md
        struct KHR_materials_transmission
        {
            static const char* extensionName;
            struct Material
            {
                static const char* GetExtensionName() { return extensionName; }
                double transmissionFactor;
                tinygltf::TextureInfo transmissionTexture;
            };
        };

        // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_ior/README.md
        struct KHR_materials_ior
        {
            static const char* extensionName;
            struct Material
            {
                static const char* GetExtensionName() { return extensionName; }
                double ior;
            };
        };

        // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_volume/README.md
        struct KHR_materials_volume
        {
            static const char* extensionName;
            struct Material
            {
                static const char* GetExtensionName() { return extensionName; }
                double thicknessFactor;
                tinygltf::TextureInfo thicknessTexture;
                double attenuationDistance;
                std::vector<double> attenuationColor;
            };
        };

        // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_emissive_strength/README.md
        struct KHR_materials_emissive_strength
        {
            static const char* extensionName;
            struct Material
            {
                static const char* GetExtensionName() { return extensionName; }
                double emissiveStrength;
            };
        };

        // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/README.md
        struct KHR_lights_punctual
        {
            static const char* extensionName;
            struct Node
            {
                static const char* GetExtensionName() { return extensionName; }
                int light;
            };
        };

        // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_texture_transform/README.md
        struct KHR_texture_transform
        {
            static const char* extensionName;
            struct TextureInfo
            {
                static const char* GetExtensionName() { return extensionName; }
                double offset[2];
                double rotation;
                double scale[2];
                int texCoord;
            };
        };

        // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_variants/README.md
        struct KHR_materials_variants
        {
            static const char* extensionName;
            struct Model
            {
                static const char* GetExtensionName() { return extensionName; }
                std::vector<std::string> variants;
            };
            struct Primitive
            {
                struct Mapping
                {
                    int material;
                    std::vector<int> variants;
                };
                static const char* GetExtensionName() { return extensionName; }
                std::vector<Mapping> mappings;
            };
        };

    public:
        GLTFModelLoader();
        ~GLTFModelLoader();

        std::unique_ptr<Model> LoadModel(const wchar_t* file);

        static bool LoadExtension(const tinygltf::Material* src, KHR_materials_transmission::Material* dest);
        static bool LoadExtension(const tinygltf::Material* src, KHR_materials_ior::Material* dest);
        static bool LoadExtension(const tinygltf::Material* src, KHR_materials_volume::Material* dest);
        static bool LoadExtension(const tinygltf::Material* src, KHR_materials_emissive_strength::Material* dest);
        static bool LoadExtension(const tinygltf::Node* src, KHR_lights_punctual::Node* dest);
        static bool LoadExtension(const tinygltf::TextureInfo* src, KHR_texture_transform::TextureInfo* dest);
        static bool LoadExtension(const tinygltf::NormalTextureInfo* src, KHR_texture_transform::TextureInfo* dest);
        static bool LoadExtension(const tinygltf::OcclusionTextureInfo* src, KHR_texture_transform::TextureInfo* dest);
        static bool LoadExtension(const tinygltf::Model* src, KHR_materials_variants::Model* dest);
        static bool LoadExtension(const tinygltf::Primitive* src, KHR_materials_variants::Primitive* dest);

        static color VectorToColor(const std::vector<double>& in);
        static vec3 VectorToTranslation(const std::vector<double>& in);
        static vec3 VectorToScale(const std::vector<double>& in);
        static quat VectorToRotation(const std::vector<double>& in);
        static void GetBufferAccessProperties(const tinygltf::Accessor& a, const tinygltf::BufferView& bv, size_t* byteOffset, size_t* byteLength, size_t* byteStride);
        static void GetBufferAccessProperties(int type, int componentType, size_t offset, const tinygltf::BufferView& bv, size_t* byteOffset, size_t* byteLength, size_t* byteStride);
        
        template<typename T> static int GetIndex(const T& o, const std::vector<T>& v) { return &o - v.data(); }

    private:
        static void LoadExtensionHelper_TextureInfoExtension(const tinygltf::Value& extValue, KHR_texture_transform::TextureInfo* dest);
        static void ValueToTextureInfo(const tinygltf::Value& textureInfoValue, tinygltf::TextureInfo* textureInfo);

        bool UpdateAndValidateExtensions(Model *model);

    private:
        static const std::set<std::string> mExtensionsSupported;

    };
}

