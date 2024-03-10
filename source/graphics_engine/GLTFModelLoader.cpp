#include <QDebug>
#include "GLTFModelLoader.h"
#include <codecvt> // codecvt_utf8
#include <locale>  // wstring_convert


#pragma warning(push)
#pragma warning(disable: 4018)
#pragma warning(disable: 4018)
#pragma warning(disable: 4267)
// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "..\tinygltf\tiny_gltf.h"
#include "..\tinygltf\json.hpp"
#pragma warning(pop)


using namespace graphics_engine;

const char* GLTFModelLoader::KHR_materials_transmission::extensionName = "KHR_materials_transmission";
const char* GLTFModelLoader::KHR_materials_ior::extensionName = "KHR_materials_ior";
const char* GLTFModelLoader::KHR_materials_volume::extensionName = "KHR_materials_volume";
const char* GLTFModelLoader::KHR_materials_emissive_strength::extensionName = "KHR_materials_emissive_strength";
const char* GLTFModelLoader::KHR_lights_punctual::extensionName = "KHR_lights_punctual";
const char* GLTFModelLoader::KHR_texture_transform::extensionName = "KHR_texture_transform";
const char* GLTFModelLoader::KHR_materials_variants::extensionName = "KHR_materials_variants";

const std::set<std::string> GLTFModelLoader::mExtensionsSupported = {
    KHR_materials_transmission::extensionName,
    //KHR_materials_ior::extensionName,
    //KHR_materials_volume::extensionName,
    KHR_materials_emissive_strength::extensionName,
    //KHR_lights_punctual::extensionName,
    //KHR_texture_transform::extensionName,
    KHR_materials_variants::extensionName,
};


GLTFModelLoader::GLTFModelLoader()
{

}

GLTFModelLoader::~GLTFModelLoader()
{

}

std::string string_to_utf8(const std::wstring& wstr)
{
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    return utf8_conv.to_bytes(wstr);
}

std::string getstr(const std::wstring& wstr)
{
    return string_to_utf8(wstr);
}

std::string getstr(const wchar_t* ws)
{
    std::wstring wstr = (ws != nullptr) ? std::wstring(ws) : L"";
    return getstr(wstr);
}

std::unique_ptr<GLTFModelLoader::Model> GLTFModelLoader::LoadModel(const wchar_t *file)
{
    std::string path = getstr(file);
    auto model = std::make_unique<GLTFModelLoader::Model>();
    model->SetFilePath(file);
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    // Check the file type
    bool ret = true;
    bool isBinary = false;
    std::string fileType = tinygltf::GetFilePathExtension(path);
    if (fileType.empty())
    {
        ret = false;
        err = "Unrecognized file type\n";
    }
    else if (fileType.compare("glb") == 0)
    {
        isBinary = true;
    }

    if (ret)
    {
        if (isBinary)
        {
            ret = loader.LoadBinaryFromFile(model.get(), &err, &warn, path); // for binary glTF (.glb)
        }
        else
        {
            ret = loader.LoadASCIIFromFile(model.get(), &err, &warn, path); // for ASCII glTF (.gltf)
        }
    }

    if (!warn.empty())
    {
        qWarning() << warn.c_str();
    }

    if (!err.empty())
    {
        qWarning() << err.c_str();
    }

    if (!ret)
    {
        qWarning() << "Failed to parse glTF\n";
        return nullptr;
    }

    // Extra processing
    model->SetNodeHierarchy();
    model->SetLinearTextureIndexSet();
    model->SetSkinMaps();
    UpdateAndValidateExtensions(model.get());

    return model;
}

bool GLTFModelLoader::LoadExtension(const tinygltf::Material* src, KHR_materials_transmission::Material* dest)
{
    if (src->extensions.find(KHR_materials_transmission::extensionName) != src->extensions.end())
    {
        const tinygltf::Value& extValue = src->extensions.at(KHR_materials_transmission::extensionName);
        assert(extValue.IsObject());

        dest->transmissionFactor = 0.0;
        if (extValue.Has("transmissionFactor"))
        {
            tinygltf::Value transmissionFactorValue = extValue.Get("transmissionFactor");
            assert(transmissionFactorValue.IsNumber());
            dest->transmissionFactor = transmissionFactorValue.GetNumberAsDouble();
        }
        dest->transmissionTexture = tinygltf::TextureInfo();
        if (extValue.Has("transmissionTexture"))
        {
            tinygltf::Value transmissionTextureValue = extValue.Get("transmissionTexture");
            ValueToTextureInfo(transmissionTextureValue, &dest->transmissionTexture);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool GLTFModelLoader::LoadExtension(const tinygltf::Material* src, KHR_materials_ior::Material* dest)
{
    if (src->extensions.find(dest->GetExtensionName()) != src->extensions.end())
    {
        const tinygltf::Value& extValue = src->extensions.at(dest->GetExtensionName());
        assert(extValue.IsObject());

        dest->ior = 1.5;
        if (extValue.Has("ior"))
        {
            tinygltf::Value iorValue = extValue.Get("ior");
            assert(iorValue.IsNumber());
            dest->ior = iorValue.GetNumberAsDouble();
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool GLTFModelLoader::LoadExtension(const tinygltf::Material* src, KHR_materials_volume::Material* dest)
{
    if (src->extensions.find(dest->GetExtensionName()) != src->extensions.end())
    {
        const tinygltf::Value& extValue = src->extensions.at(dest->GetExtensionName());
        assert(extValue.IsObject());

        dest->thicknessFactor = 0.0;
        if (extValue.Has("thicknessFactor"))
        {
            tinygltf::Value val = extValue.Get("thicknessFactor");
            assert(val.IsNumber());
            dest->thicknessFactor = val.GetNumberAsDouble();
        }
        dest->thicknessTexture = tinygltf::TextureInfo();
        if (extValue.Has("thicknessTexture"))
        {
            tinygltf::Value thicknessTextureValue = extValue.Get("thicknessTexture");
            ValueToTextureInfo(thicknessTextureValue, &dest->thicknessTexture);
        }

        dest->attenuationDistance = std::numeric_limits<double>::infinity();
        if (extValue.Has("attenuationDistance"))
        {
            tinygltf::Value val = extValue.Get("attenuationDistance");
            assert(val.IsNumber());
            dest->attenuationDistance = val.GetNumberAsDouble();
        }

        dest->attenuationColor = { 1.0, 1.0, 1.0 };
        if (extValue.Has("attenuationColor"))
        {
            tinygltf::Value val = extValue.Get("attenuationColor");
            assert(val.IsArray() && val.ArrayLen() == 3);
            assert(val.Get(0).IsNumber());
            assert(val.Get(1).IsNumber());
            assert(val.Get(2).IsNumber());

            dest->attenuationColor = {
                val.Get(0).GetNumberAsDouble(),
                val.Get(1).GetNumberAsDouble(),
                val.Get(2).GetNumberAsDouble()
            };
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool GLTFModelLoader::LoadExtension(const tinygltf::Material* src, KHR_materials_emissive_strength::Material* dest)
{
    if (src->extensions.find(dest->GetExtensionName()) != src->extensions.end())
    {
        const tinygltf::Value& extValue = src->extensions.at(dest->GetExtensionName());
        assert(extValue.IsObject());

        dest->emissiveStrength = 1.0;
        if (extValue.Has("emissiveStrength"))
        {
            auto val = extValue.Get("emissiveStrength");
            assert(val.IsNumber());
            dest->emissiveStrength = val.GetNumberAsDouble();
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool GLTFModelLoader::LoadExtension(const tinygltf::Node* src, KHR_lights_punctual::Node* dest)
{
    if (src->extensions.find(dest->GetExtensionName()) != src->extensions.end())
    {
        const tinygltf::Value& extValue = src->extensions.at(dest->GetExtensionName());
        assert(extValue.IsObject());

        dest->light = -1;
        if (extValue.Has("light"))
        {
            tinygltf::Value lightValue = extValue.Get("light");
            assert(lightValue.IsNumber());
            dest->light = lightValue.GetNumberAsInt();
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool GLTFModelLoader::LoadExtension(const tinygltf::TextureInfo* src, KHR_texture_transform::TextureInfo* dest)
{
    if (src->extensions.find(dest->GetExtensionName()) != src->extensions.end())
    {
        const tinygltf::Value& extValue = src->extensions.at(dest->GetExtensionName());
        assert(extValue.IsObject());
        LoadExtensionHelper_TextureInfoExtension(extValue, dest);
        return true;
    }
    else
    {
        return false;
    }
}

bool GLTFModelLoader::LoadExtension(const tinygltf::NormalTextureInfo* src, KHR_texture_transform::TextureInfo* dest)
{
    if (src->extensions.find(dest->GetExtensionName()) != src->extensions.end())
    {
        const tinygltf::Value& extValue = src->extensions.at(dest->GetExtensionName());
        assert(extValue.IsObject());
        LoadExtensionHelper_TextureInfoExtension(extValue, dest);
        return true;
    }
    else
    {
        return false;
    }
}

bool GLTFModelLoader::LoadExtension(const tinygltf::OcclusionTextureInfo* src, KHR_texture_transform::TextureInfo* dest)
{
    if (src->extensions.find(dest->GetExtensionName()) != src->extensions.end())
    {
        const tinygltf::Value& extValue = src->extensions.at(dest->GetExtensionName());
        assert(extValue.IsObject());
        LoadExtensionHelper_TextureInfoExtension(extValue, dest);
        return true;
    }
    else
    {
        return false;
    }
}

bool GLTFModelLoader::LoadExtension(const tinygltf::Model* src, KHR_materials_variants::Model* dest)
{
    if (src->extensions.find(dest->GetExtensionName()) == src->extensions.end())
        return false;

    const auto& extValue = src->extensions.at(dest->GetExtensionName());
    assert(extValue.IsObject());
    dest->variants.clear();
    if (extValue.Has("variants"))
    {
        auto& variantsValue = extValue.Get("variants");
        assert(variantsValue.IsArray());
        dest->variants.resize(variantsValue.ArrayLen());
        for (auto i = 0; i < (int)variantsValue.ArrayLen(); i++)
        {
            auto& variantValue = variantsValue.Get(i);
            assert(variantValue.IsObject());
            dest->variants[i] = variantValue.Get("name").Get<std::string>();
        }
    }
    return true;
}

bool GLTFModelLoader::LoadExtension(const tinygltf::Primitive* src, KHR_materials_variants::Primitive* dest)
{
    if (src->extensions.find(dest->GetExtensionName()) == src->extensions.end())
        return false;

    const auto& extValue = src->extensions.at(dest->GetExtensionName());
    assert(extValue.IsObject());
    dest->mappings.clear();
    if (extValue.Has("mappings"))
    {
        auto& mappingsValue = extValue.Get("mappings");
        assert(mappingsValue.IsArray());
        dest->mappings.resize(mappingsValue.ArrayLen());
        for (auto i = 0; i < (int)mappingsValue.ArrayLen(); i++)
        {
            auto& mappingValue = mappingsValue.Get(i);
            assert(mappingValue.IsObject());
            dest->mappings[i].material = mappingValue.Get("material").Get<int>();
            auto& variantsValue = mappingValue.Get("variants");
            assert(variantsValue.IsArray());
            dest->mappings[i].variants.resize(variantsValue.ArrayLen());
            for (auto j = 0; j < (int)variantsValue.ArrayLen(); j++)
            {
                auto& variantValue = variantsValue.Get(j);
                dest->mappings[i].variants[j] = variantValue.Get<int>();
            }
        }
    }
    return true;
}

color GLTFModelLoader::VectorToColor(const std::vector<double>& in)
{
    color out;
    if (in.size() == 0)
    {
        out[0] = 0.0f; out[1] = 0.0f; out[2] = 0.0f;
        out[3] = 1.0f; // alpha
    }
    else if (in.size() == 3)
    {
        out[0] = (float)in[0]; out[1] = (float)in[1]; out[2] = (float)in[2];
        out[3] = 1.0f; // alpha
    }
    else if (in.size() == 4)
    {
        out[0] = (float)in[0]; out[1] = (float)in[1]; out[2] = (float)in[2];
        out[3] = (float)in[3]; // alpha
    }
    return out;
}

vec3 GLTFModelLoader::VectorToTranslation(const std::vector<double>& in)
{
    vec3 out;
    if (in.size() == 0)
    {
        out[0] = 0.0f; out[1] = 0.0f; out[2] = 0.0f;
    }
    else if (in.size() == 3)
    {
        out[0] = (float)in[0]; out[1] = (float)in[1]; out[2] = (float)in[2];
    }
    return out;
}

vec3 GLTFModelLoader::VectorToScale(const std::vector<double>& in)
{
    vec3 out;
    if (in.size() == 0)
    {
        out[0] = 1.0f; out[1] = 1.0f; out[2] = 1.0f;
    }
    else if (in.size() == 3)
    {
        out[0] = (float)in[0]; out[1] = (float)in[1]; out[2] = (float)in[2];
    }
    return out;
}

quat GLTFModelLoader::VectorToRotation(const std::vector<double>& in)
{
    // From gltf 2.0 spec:
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#transformations
    // Rotation is a unit quaternion value, XYZW, in the local coordinate system, where W is the scalar
    if (in.size() == 4)
    {
        return quat((float)in[3],
                    (float)in[0],
                    (float)in[1],
                    (float)in[2]);
    }
    else
    {
        return quat(1.0f, 0.0f, 0.0f, 0.0f);
    }
}

void GLTFModelLoader::GetBufferAccessProperties(const tinygltf::Accessor &a, const tinygltf::BufferView& bv, size_t *byteOffset, size_t *byteLength, size_t *byteStride)
{
    GetBufferAccessProperties(a.type, a.componentType, a.byteOffset, bv, byteOffset, byteLength, byteStride);
}

void GLTFModelLoader::GetBufferAccessProperties(int type, int componentType, size_t offset, const tinygltf::BufferView& bv, size_t* byteOffset, size_t* byteLength, size_t* byteStride)
{
    *byteLength = bv.byteLength;
    *byteStride = bv.byteStride;
    if (*byteStride == 0)
    {
        *byteStride = (size_t)(tinygltf::GetNumComponentsInType(type) * tinygltf::GetComponentSizeInBytes(componentType));
    }
    *byteOffset = bv.byteOffset;
    *byteOffset += offset;
    *byteLength -= offset;
}

void GLTFModelLoader::LoadExtensionHelper_TextureInfoExtension(const tinygltf::Value& extValue, KHR_texture_transform::TextureInfo* dest)
{
    // default values
    dest->offset[0] = dest->offset[1] = 0.0;
    dest->rotation = 0.0;
    dest->scale[0] = dest->scale[1] = 1.0;
    dest->texCoord = -1;
    if (extValue.Has("offset"))
    {
        tinygltf::Value val = extValue.Get("offset");
        assert(val.IsArray() && val.ArrayLen() == 2);
        assert(val.Get(0).IsNumber());
        assert(val.Get(1).IsNumber());
        dest->offset[0] = val.Get(0).GetNumberAsDouble();
        dest->offset[1] = val.Get(1).GetNumberAsDouble();
    }
    if (extValue.Has("rotation"))
    {
        tinygltf::Value val = extValue.Get("rotation");
        assert(val.IsNumber());
        dest->rotation = val.GetNumberAsDouble();
    }
    if (extValue.Has("scale"))
    {
        tinygltf::Value val = extValue.Get("scale");
        assert(val.IsArray() && val.ArrayLen() == 2);
        assert(val.Get(0).IsNumber());
        assert(val.Get(1).IsNumber());
        dest->scale[0] = val.Get(0).GetNumberAsDouble();
        dest->scale[1] = val.Get(1).GetNumberAsDouble();
    }
    if (extValue.Has("texCoord"))
    {
        tinygltf::Value val = extValue.Get("texCoord");
        assert(val.IsNumber());
        dest->texCoord = val.GetNumberAsInt();
    }
}

void GLTFModelLoader::ValueToTextureInfo(const tinygltf::Value& textureInfoValue, tinygltf::TextureInfo* textureInfo)
{
    assert(textureInfoValue.IsObject());
    (*textureInfo) = tinygltf::TextureInfo();
    if (textureInfoValue.Has("index"))
    {
        textureInfo->index = textureInfoValue.Get("index").GetNumberAsInt();
    }
    if (textureInfoValue.Has("texCoord"))
    {
        textureInfo->texCoord = textureInfoValue.Get("texCoord").GetNumberAsInt();
    }
}

bool GLTFModelLoader::UpdateAndValidateExtensions(Model *model)
{
    for (int i = 0; i < (int)model->extensionsUsed.size(); i++)
    {
        std::string extName = model->extensionsUsed[i];
        const bool is_in = mExtensionsSupported.find(extName) != mExtensionsSupported.end();
        if (!is_in)
        {
            std::string extNotSupported = "Extension " + extName + " is used by the model but it's not supported by qigltf library";
            qWarning() << extNotSupported.c_str();
        }
    }
    bool requiredExtensionUnsupported = false;
    for (int i = 0; i < (int)model->extensionsRequired.size(); i++)
    {
        std::string extName = model->extensionsRequired[i];
        const bool is_in = mExtensionsSupported.find(extName) != mExtensionsSupported.end();
        if (!is_in)
        {
            std::string extNotSupported = "Extension " + extName + " is required by the model but it's not supported by qigltf library";
            qWarning() << extNotSupported.c_str();
            requiredExtensionUnsupported = true;
        }
    }
    if (requiredExtensionUnsupported)
    {
        qWarning() << "The model failed to load because some required extensions are unsupported";
        return false;
    }

    return true;
}

void GLTFModelLoader::Model::SetNodeHierarchy()
{
    mNodeToScene.clear();
    mNodeToParent.clear();
    for (size_t i = 0; i < scenes.size(); i++)
    {
        for (size_t j = 0; j < scenes[i].nodes.size(); j++)
        {
            int nodeIndex = scenes[i].nodes[j];
            mNodeToScene[nodeIndex] = (int)i;
            SetNodeHierarchyRecursive(nodeIndex, -1);
        }
    }
}

void GLTFModelLoader::Model::SetNodeHierarchyRecursive(int nodeIndex, int parentIndex)
{
    mNodeToParent[nodeIndex] = parentIndex;
    for (size_t i = 0; i < nodes[nodeIndex].children.size(); i++)
    {
        int childIndex = nodes[nodeIndex].children[i];
        mNodeToScene[childIndex] = mNodeToScene[nodeIndex];
        SetNodeHierarchyRecursive(childIndex, nodeIndex);
    }
}

void GLTFModelLoader::Model::SetLinearTextureIndexSet()
{
    mLinearTextureIndexSet.clear();
    for (size_t i = 0; i < materials.size(); i++)
    {
        tinygltf::Material& material = materials[i];
        //mLinearTextureIndexSet.insert(material.pbrMetallicRoughness.baseColorTexture.index);
        mLinearTextureIndexSet.insert(material.pbrMetallicRoughness.metallicRoughnessTexture.index);
        mLinearTextureIndexSet.insert(material.normalTexture.index);
        mLinearTextureIndexSet.insert(material.occlusionTexture.index);
        //mLinearTextureIndexSet.insert(material.emissiveTexture.index);
    }
}

void GLTFModelLoader::Model::SetSkinMaps()
{
    mJointToSkin.clear();
    mJointToPosition.clear();
    mSkeletonToSkins.clear();
    for (size_t i = 0; i < skins.size(); i++)
    {
        int skeletonNodeIndex = skins[i].skeleton;
        if (skeletonNodeIndex >= 0)
        {
            mSkeletonToSkins[skeletonNodeIndex].push_back((int)i);
        }
        for (size_t j = 0; j < skins[i].joints.size(); j++)
        {
            int jointNodeIndex = skins[i].joints[j];
            mJointToSkin[jointNodeIndex] = (int)i;
            mJointToPosition[jointNodeIndex] = (int)j;
        }
    }
}

bool GLTFModelLoader::Model::IsInLinearSpace(int textureIndex) const
{
    return mLinearTextureIndexSet.find(textureIndex) != mLinearTextureIndexSet.end();
}

bool GLTFModelLoader::Model::IsRoot(int nodeIndex, int* parentNodeIndex, int* sceneIndex) const
{
    auto nodeSceneIte = mNodeToScene.find(nodeIndex);
    *sceneIndex = nodeSceneIte->second;
    auto nodeParentIte = mNodeToParent.find(nodeIndex);
    if (nodeParentIte->second >= 0)
    {
        *parentNodeIndex = nodeParentIte->second;
        return false;
    }
    else
    {
        *parentNodeIndex = -1;
        return true;
    }
}

bool GLTFModelLoader::Model::IsJoint(int nodeIndex, int* skinIndex, int* position) const
{
    auto skinIndexIt = mJointToSkin.find(nodeIndex);
    auto jointPositionIt = mJointToPosition.find(nodeIndex);
    if (skinIndexIt != mJointToSkin.end() && jointPositionIt != mJointToPosition.end())
    {
        *skinIndex = skinIndexIt->second;
        *position = jointPositionIt->second;
        return true;
    }
    else
    {
        return false;
    }
}

bool GLTFModelLoader::Model::IsSkeleton(int nodeIndex, std::vector<int>* skins) const
{
    auto skinIndexIt = mSkeletonToSkins.find(nodeIndex);
    if (skinIndexIt != mSkeletonToSkins.end())
    {
        *skins = skinIndexIt->second;
        return true;
    }
    else
    {
        return false;
    }
}
