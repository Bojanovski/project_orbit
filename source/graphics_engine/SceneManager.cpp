#include "SceneManager.h"
#include "MeshShaderProgram.h"
#include "mikktspace.h"

// Make sure our type codes match the openGL type codes
static_assert(TYPE_BYTE             == GL_BYTE);
static_assert(TYPE_UNSIGNED_BYTE    == GL_UNSIGNED_BYTE);
static_assert(TYPE_SHORT            == GL_SHORT);
static_assert(TYPE_UNSIGNED_SHORT   == GL_UNSIGNED_SHORT);
static_assert(TYPE_INT              == GL_INT);
static_assert(TYPE_UNSIGNED_INT     == GL_UNSIGNED_INT);
static_assert(TYPE_FLOAT            == GL_FLOAT);
static_assert(TYPE_2_BYTES          == GL_2_BYTES);
static_assert(TYPE_3_BYTES          == GL_3_BYTES);
static_assert(TYPE_4_BYTES          == GL_4_BYTES);
static_assert(TYPE_DOUBLE           == GL_DOUBLE);


using namespace graphics_engine;

void convertValueFromTinyGltf(graphics_engine::Value* dest, const tinygltf::Value& src)
{
    switch (src.Type())
    {
    case tinygltf::NULL_TYPE:
        // Nothing to do
        break;

    case tinygltf::BOOL_TYPE:
        *dest = graphics_engine::Value(src.Get<bool>());
        break;

    case tinygltf::INT_TYPE:
        *dest = graphics_engine::Value(src.Get<int>());
        break;

    case tinygltf::REAL_TYPE:
        *dest = graphics_engine::Value(src.Get<double>());
        break;

    case tinygltf::STRING_TYPE:
        *dest = graphics_engine::Value(src.Get<std::string>());
        break;

    case tinygltf::BINARY_TYPE:
        *dest = graphics_engine::Value(src.Get<std::vector<unsigned char>>());
        break;

    case tinygltf::ARRAY_TYPE:
    {
        *dest = graphics_engine::Value(graphics_engine::Value::Array());
        auto arr = dest->getArray();
        arr->resize(src.ArrayLen());
        for (size_t i = 0; i < src.ArrayLen(); i++)
        {
            convertValueFromTinyGltf(&(*arr)[i], src.Get((int)i));
        }
        break;
    }

    case tinygltf::OBJECT_TYPE:
    {
        *dest = graphics_engine::Value(graphics_engine::Value::Object());
        auto obj = dest->getObject();
        obj->clear();
        auto keys = src.Keys();
        for (size_t i = 0; i < keys.size(); i++)
        {
            auto& key = keys[i];
            (*obj)[key] = graphics_engine::Value();
            convertValueFromTinyGltf(&(*obj)[key], src.Get(key));
        }
        break;
    }

    default:
        throw;
    }
}

SceneManager::SceneManager()
{

}

SceneManager::~SceneManager()
{
    mScenes.clear();
    mNodes.clear();
    mMeshes.clear();
    mMaterials.clear();
    mImages.clear();
}

void SceneManager::create(GraphicsEngineInterface* gei)
{
    mGEI = gei;

    mDefaultMaterial = mGEI->createMaterial(
        "default",
        color(0.8f, 0.6f, 0.4f, 1.0f), Texture(),
        0.1f, 0.1f, Texture(),
        1.0f, Texture(),
        color(0.0f, 0.0f, 0.0f, 1.0f), Texture(),
        false, 0.0f,
        false, 0.0f, Texture(),
        false,
        TextureTransform(),
        TextureTransform(),
        TextureTransform(),
        TextureTransform(),
        TextureTransform(),
        TextureTransform());
}

void SceneManager::addModel(const LoadedFile* lf, bool allAnimations, bool allVariants, const std::set<std::string>& variantFilter, Model* outModel)
{
    // Prepare
    mWorkingModel = dynamic_cast<const GLTFModelLoader::Model*>(lf);
    mTypeIndexToObject.clear();

    // Variants
    GLTFModelLoader::KHR_materials_variants::Model m;
    outModel->uses_KHR_materials_variants = GLTFModelLoader::LoadExtension(mWorkingModel, &m);
    if (allVariants)
    {
        outModel->variants = m.variants;
        for (size_t i = 0; i < m.variants.size(); i++) // add all
        {
            mVariantFilter.insert(i);
        }
    }
    else
    {
        for (size_t i = 0; i < m.variants.size(); i++)
        {
            const std::string& variantName = m.variants[i];
            if (variantFilter.find(variantName) != variantFilter.end())
            {
                mVariantFilter.insert(i);
                outModel->variants.push_back(variantName);
            }
        }
    }

    // Scenes
    outModel->scenes.resize(mWorkingModel->scenes.size());
    for (int i = 0; i < (int)mWorkingModel->scenes.size(); ++i)
    {
        auto& gltfObj = mWorkingModel->scenes[i];
        outModel->scenes[i] = addScene(gltfObj);
    }

    // Animations
    if (allAnimations)
    {
        outModel->animationTargets.resize(mWorkingModel->animations.size());
        for (int i = 0; i < (int)mWorkingModel->animations.size(); ++i)
        {
            auto& gltfObj = mWorkingModel->animations[i];
            outModel->animationTargets[i] = addAnimationTarget(gltfObj);
        }
    }

    // Extra
    convertValueFromTinyGltf(&outModel->extra, mWorkingModel->extras);

    // Clean
    mWorkingModel = nullptr;
}

Scene* SceneManager::addScene(const tinygltf::Scene &gltfObj)
{
    int index = GLTFModelLoader::GetIndex(gltfObj, mWorkingModel->scenes);
    auto ite = mTypeIndexToObject.find({ typeid(Scene), index });
    if (ite != mTypeIndexToObject.end()) return (Scene*)ite->second;

    auto ret = mGEI->createScene(gltfObj.name);
    mTypeIndexToObject[{ typeid(Scene), index }] = ret;
    ret->name = gltfObj.name;

    // Extra
    convertValueFromTinyGltf(&ret->extra, gltfObj.extras);

    // Nodes
    ret->nodes.reserve(gltfObj.nodes.size());
    for (int i = 0; i < (int)gltfObj.nodes.size(); ++i)
    {
        auto& gltfNode =  mWorkingModel->nodes[gltfObj.nodes[i]];
        auto node = addNode(gltfNode);
        ret->nodes[i] = node;
    }
    return ret;
}

Node* SceneManager::addNode(const tinygltf::Node &gltfObj)
{
    int index = GLTFModelLoader::GetIndex(gltfObj, mWorkingModel->nodes);
    auto ite = mTypeIndexToObject.find({ typeid(Node), index });
    if (ite != mTypeIndexToObject.end()) return (Node*)ite->second;

    // Get parent or scene info
    NodeContainer* belongsTo = nullptr;
    int sceneIndex, parentIndex;
    if (mWorkingModel->IsRoot(index, &parentIndex, &sceneIndex))
    {
        auto& gltfScene = mWorkingModel->scenes[sceneIndex];
        belongsTo = addScene(gltfScene);
    }
    else
    {
        auto& gltfParent = mWorkingModel->nodes[parentIndex];
        belongsTo = addNode(gltfParent);
    }

    vec3 localPosition, localScale;
    quat localRotation;
    // Move the transformation matrix to pos, scale and rot
    if (gltfObj.matrix.size() > 0)
    {
        auto& matrixVec = gltfObj.matrix;
        mat4x4 m;
        m << (float)matrixVec[0 + 0], (float)matrixVec[0 + 1], (float)matrixVec[0 + 2], (float)matrixVec[0 + 3],
             (float)matrixVec[4 + 0], (float)matrixVec[4 + 1], (float)matrixVec[4 + 2], (float)matrixVec[4 + 3],
             (float)matrixVec[8 + 0], (float)matrixVec[8 + 1], (float)matrixVec[8 + 2], (float)matrixVec[8 + 3],
             (float)matrixVec[12 + 0], (float)matrixVec[12 + 1], (float)matrixVec[12 + 2], (float)matrixVec[12 + 3];
        transform t;
        t.matrix() = m;

        localPosition = t.translation();
        localScale = t.linear().colwise().norm();
        localRotation = t.linear();
    }
    else
    {
        localPosition = GLTFModelLoader::VectorToTranslation(gltfObj.translation);
        localScale = GLTFModelLoader::VectorToScale(gltfObj.scale);
        localRotation = GLTFModelLoader::VectorToRotation(gltfObj.rotation);
    }

    auto ret = mGEI->createNode(gltfObj.name, belongsTo, localPosition, localScale, localRotation);
    mTypeIndexToObject[{ typeid(Node), index }] = ret;

    if (gltfObj.mesh != -1)
    {
        auto& gltfMesh = mWorkingModel->meshes[gltfObj.mesh];
        ret->mesh = addMesh(gltfMesh);
    }

    if (gltfObj.skin != -1)
    {
        assert(ret->mesh != nullptr);
        assert(ret->mesh->skinned);
        auto& gltfSkin = mWorkingModel->skins[gltfObj.skin];
        ret->skin = addSkin(gltfSkin);
    }

    if (gltfObj.weights.size() > 0)
    {
        assert(ret->mesh != nullptr);
        assert(ret->mesh->morphTargetCount == gltfObj.weights.size());
        ret->weights.resize(gltfObj.weights.size());
        for (size_t i = 0; i < gltfObj.weights.size(); i++)
            ret->weights[i] = (float)gltfObj.weights[i];
    }

    int skinIndex, positionIndex;
    if (mWorkingModel->IsJoint(index, &skinIndex, &positionIndex))
    {
        auto& gltfSkin = mWorkingModel->skins[skinIndex];
        auto skin = addSkin(gltfSkin);
        assert(skin->joints[positionIndex] == nullptr);
        skin->joints[positionIndex] = ret;
    }

    std::vector<int> skeletonSkins;
    if (mWorkingModel->IsSkeleton(index, &skeletonSkins))
    {
        for (size_t i = 0; i < skeletonSkins.size(); i++)
        {
            int skinIndex = skeletonSkins[i];
            auto& gltfSkin = mWorkingModel->skins[skinIndex];
            auto skin = addSkin(gltfSkin);
            assert(skin->skeleton == nullptr);
            skin->skeleton = ret;
        }
    }

    // Extra
    convertValueFromTinyGltf(&ret->extra, gltfObj.extras);

    // Children
    ret->children.reserve(gltfObj.children.size());
    for (int i = 0; i < (int)gltfObj.children.size(); ++i)
    {
        auto& gltfChild =  mWorkingModel->nodes[gltfObj.children[i]];
        addNode(gltfChild);
    }

    return ret;
}

uint getIndex(const uchar* data, uint index, GLenum componentType, GLsizei stride)
{
    switch (componentType)
    {
    case GL_INT:
        return *(int*)(data + index * stride);
    case GL_UNSIGNED_INT:
        return *(uint*)(data + index * stride);
    case GL_SHORT:
        return *(short*)(data + index * stride);
    case GL_UNSIGNED_SHORT:
        return *(ushort*)(data + index * stride);
    case GL_BYTE:
        return *(char*)(data + index * stride);
    case GL_UNSIGNED_BYTE:
        return *(uchar*)(data + index * stride);
    default:
        throw;
    }
}

ushort getJoint(const uchar* data, uint vertexIndex, GLenum componentType, GLsizei stride, uint jointIndex)
{
    switch (componentType)
    {
    case GL_UNSIGNED_BYTE:
        return *(uchar*)(data + vertexIndex * stride + jointIndex * sizeof(uchar));
    case GL_UNSIGNED_SHORT:
        return *(ushort*)(data + vertexIndex * stride + jointIndex * sizeof(ushort));
    default:
        throw;
    }
}

struct Primitive
{
    const Mesh::Buffer* indexBuffer;
    const Mesh::Buffer* positionBuffer;
    const Mesh::Buffer* uvBuffer;
    const Mesh::Buffer* normalBuffer;
    uchar* tangentBuffer;
    uchar* bitangentBuffer;
    bool tangentHasSign = true;
    const Mesh::Buffer* weightsBuffer;
    const Mesh::Buffer* jointsBuffer;

    static int getNumFaces(const SMikkTSpaceContext * pContext)
    {
        auto primitive = (Primitive*)pContext->m_pUserData;
        return primitive->indexBuffer->count / 3;
    }

    static int getNumVerticesOfFace(const SMikkTSpaceContext * pContext, const int iFace)
    {
        (void)pContext;
        (void)iFace;
        return 3;
    }

    static void getPosition(const SMikkTSpaceContext * pContext, float fvPosOut[], const int iFace, const int iVert)
    {
        auto primitive = (Primitive*)pContext->m_pUserData;
        auto ib = primitive->indexBuffer;
        auto i = getIndex(ib->data, iFace * 3 + iVert, ib->componentType, ib->stride);
        auto& pb = primitive->positionBuffer;
        auto cs = pb->componentSize;
        memcpy_s(fvPosOut, cs * 3, pb->data + i * pb->stride, cs * 3);
    }

    static void getNormal(const SMikkTSpaceContext * pContext, float fvNormOut[], const int iFace, const int iVert)
    {
        auto primitive = (Primitive*)pContext->m_pUserData;
        auto ib = primitive->indexBuffer;
        auto i = getIndex(ib->data, iFace * 3 + iVert, ib->componentType, ib->stride);
        auto& nb = primitive->normalBuffer;
        auto cs = nb->componentSize;
        memcpy_s(fvNormOut, cs * 3, nb->data + i * nb->stride, cs * 3);
    }

    static void getTexCoord(const SMikkTSpaceContext * pContext, float fvTexcOut[], const int iFace, const int iVert)
    {
        auto primitive = (Primitive*)pContext->m_pUserData;
        auto ib = primitive->indexBuffer;
        auto i = getIndex(ib->data, iFace * 3 + iVert, ib->componentType, ib->stride);
        auto& uvb = primitive->uvBuffer;
        auto cs = uvb->componentSize;
        memcpy_s(fvTexcOut, cs * 2, uvb->data + i * uvb->stride, cs * 2);
    }

    static void setTSpaceBasic(const SMikkTSpaceContext * pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
    {
        auto primitive = (Primitive*)pContext->m_pUserData;
        auto ib = primitive->indexBuffer;
        auto i = getIndex(ib->data, iFace * 3 + iVert, ib->componentType, ib->stride);
        auto& tb = primitive->tangentBuffer;
        auto cs = sizeof(float);
        auto tbStride = cs * (primitive->tangentHasSign ? 4 : 3);
        memcpy_s(tb + i * tbStride, cs * 3, fvTangent, cs * 3); // xyz
        if (primitive->tangentHasSign)
            memcpy_s(tb + i * tbStride + cs * 3, cs * 1, &fSign, cs * 1); // w
    }

    static void setTSpace(const SMikkTSpaceContext*, const float*, const float*, const float, const float, const tbool, const int, const int)
    {
        return;
    }

    static void getWeights(const SMikkTSpaceContext* pContext, float fvWeights[], const int iFace, const int iVert)
    {
        auto primitive = (Primitive*)pContext->m_pUserData;
        auto ib = primitive->indexBuffer;
        auto i = getIndex(ib->data, iFace * 3 + iVert, ib->componentType, ib->stride);
        auto& wb = primitive->weightsBuffer;
        auto cs = wb->componentSize;
        memcpy_s(fvWeights, cs * 4, wb->data + i * wb->stride, cs * 4);
    }

    static void getJoints(const SMikkTSpaceContext* pContext, ushort fvJoints[], const int iFace, const int iVert)
    {
        auto primitive = (Primitive*)pContext->m_pUserData;
        auto ib = primitive->indexBuffer;
        auto i = getIndex(ib->data, iFace * 3 + iVert, ib->componentType, ib->stride);
        auto& jb = primitive->jointsBuffer;
        fvJoints[0] = getJoint(jb->data, i, jb->componentType, jb->stride, 0);
        fvJoints[1] = getJoint(jb->data, i, jb->componentType, jb->stride, 1);
        fvJoints[2] = getJoint(jb->data, i, jb->componentType, jb->stride, 2);
        fvJoints[3] = getJoint(jb->data, i, jb->componentType, jb->stride, 3);
    }
};

void SceneManager::fillIndxBuffer(
    int accessorIndex, 
    Mesh::Buffer* outBuffer)
{
    auto& accessor = mWorkingModel->accessors[accessorIndex];
    assert(accessor.type == TINYGLTF_TYPE_SCALAR);
    assert(
        //accessor.componentType == TINYGLTF_COMPONENT_TYPE_INT || // not supported by gltf 2.0
        accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ||
        accessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT ||
        accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
        accessor.componentType == TINYGLTF_COMPONENT_TYPE_BYTE ||
        accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);
    auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
    auto& buffer = mWorkingModel->buffers[bufferView.buffer];
    size_t byteOffset, byteLength, byteStride;
    GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
    int32_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    outBuffer->data = (uchar*)(buffer.data.data() + byteOffset);
    outBuffer->count = (uint)accessor.count;
    outBuffer->stride = (int)byteStride;
    outBuffer->componentSize = (GLsizei)componentSize;
    switch (accessor.componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_INT:
        outBuffer->componentType = GL_INT;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        outBuffer->componentType = GL_UNSIGNED_INT;
        break;
    case TINYGLTF_COMPONENT_TYPE_SHORT:
        outBuffer->componentType = GL_SHORT;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        outBuffer->componentType = GL_UNSIGNED_SHORT;
        break;
    case TINYGLTF_COMPONENT_TYPE_BYTE:
        outBuffer->componentType = GL_BYTE;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        outBuffer->componentType = GL_UNSIGNED_BYTE;
        break;
    default:
        break;
    }
}

void SceneManager::fillPosBuffer(
    int accessorIndex,
    Mesh::Buffer* outBuffer)
{
    assert(accessorIndex != -1);
    auto& accessor = mWorkingModel->accessors[accessorIndex];
    if (accessor.sparse.isSparse)
    {
        assert(accessor.type == TINYGLTF_TYPE_VEC3);
        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        fillBufferFromSparseAccessor<vec3>(accessor, outBuffer);
    }
    else // not sparse
    {
        assert(accessor.type == TINYGLTF_TYPE_VEC3);
        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        assert(accessor.bufferView != -1);
        int32_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
        size_t newElementStride = componentSize * 3;
        auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
        auto& buffer = mWorkingModel->buffers[bufferView.buffer];
        size_t byteOffset, byteLength, byteStride;
        GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
        outBuffer->data = (uchar*)(buffer.data.data() + byteOffset);
        outBuffer->count = (uint)accessor.count;
        outBuffer->stride = (GLsizei)byteStride;
        outBuffer->componentSize = (GLsizei)componentSize;
        outBuffer->componentType = GL_FLOAT;
    }
}

void SceneManager::fillUVBuffer(
    int accessorIndex, 
    const Mesh::Buffer* posBuffer,
    Mesh::Buffer* outBuffer)
{
    if (accessorIndex != -1)
    {
        auto& accessor = mWorkingModel->accessors[accessorIndex];
        assert(accessor.type == TINYGLTF_TYPE_VEC2 /*|| accessor.type == TINYGLTF_TYPE_VEC3*/);
        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        if (accessor.sparse.isSparse)
        {
            throw;
        }
        else // not sparse
        {
            assert(accessor.bufferView != -1);
            auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
            auto& buffer = mWorkingModel->buffers[bufferView.buffer];
            size_t byteOffset, byteLength, byteStride;
            GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
            int32_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
            outBuffer->data = (uchar*)(buffer.data.data() + byteOffset);
            outBuffer->count = (uint)accessor.count;
            outBuffer->stride = (GLsizei)byteStride;
            outBuffer->componentSize = (GLsizei)componentSize;
            outBuffer->componentType = GL_FLOAT;
        }
    }
    else // dummy UV will be provided
    {
        mWorkingMeshData.push_back(std::make_unique<MeshTools::DataElement>());
        auto computedUVs = mWorkingMeshData.back().get();
        auto& pb = *posBuffer;
        int uvcs = sizeof(float);
        int uvStride = uvcs * 2;
        computedUVs->resize(pb.count * uvStride);
        outBuffer->data = computedUVs->data();
        outBuffer->count = pb.count;
        outBuffer->stride = uvStride;
        outBuffer->componentSize = uvcs;
        outBuffer->componentType = GL_FLOAT;
    }
}

void SceneManager::fillNorBuffer(
    int accessorIndex, 
    const Mesh::Buffer* indxBuffer, 
    const Mesh::Buffer* posBuffer,
    Mesh::Buffer* outBuffer)
{
    if (accessorIndex != -1)
    {
        auto& accessor = mWorkingModel->accessors[accessorIndex];
        if (accessor.sparse.isSparse)
        {
            assert(accessor.type == TINYGLTF_TYPE_VEC3);
            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            fillBufferFromSparseAccessor<vec3>(accessor, outBuffer);
        }
        else // not sparse
        {
            assert(accessor.type == TINYGLTF_TYPE_VEC3);
            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
            auto& buffer = mWorkingModel->buffers[bufferView.buffer];
            size_t byteOffset, byteLength, byteStride;
            GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
            int32_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
            outBuffer->data = (uchar*)(buffer.data.data() + byteOffset);
            outBuffer->count = (uint)accessor.count;
            outBuffer->stride = (GLsizei)byteStride;
            outBuffer->componentSize = (GLsizei)componentSize;
            outBuffer->componentType = GL_FLOAT;
        }
    }
    else // no accessor - flat normals need to be computed
    {
        mWorkingMeshData.push_back(std::make_unique<MeshTools::DataElement>());
        auto computedNormals = mWorkingMeshData.back().get();
        auto& ib = *indxBuffer;
        uint triangleCount = ib.count / 3;
        auto& pb = *posBuffer;
        auto pcs = pb.componentSize;
        int ncs = sizeof(float);
        int nStride = ncs * 3;
        computedNormals->resize(pb.count * nStride);
        for (uint j = 0; j < triangleCount; ++j)
        {
            uint i0 = getIndex(ib.data, j * 3 + 0, ib.componentType, ib.stride);
            uint i1 = getIndex(ib.data, j * 3 + 1, ib.componentType, ib.stride);
            uint i2 = getIndex(ib.data, j * 3 + 2, ib.componentType, ib.stride);
            vec3 v0, v1, v2;
            memcpy_s(v0.data(), pcs * 3, pb.data + i0 * pb.stride, pcs * 3);
            memcpy_s(v1.data(), pcs * 3, pb.data + i1 * pb.stride, pcs * 3);
            memcpy_s(v2.data(), pcs * 3, pb.data + i2 * pb.stride, pcs * 3);
            vec3 e0 = v1 - v0;
            vec3 e1 = v2 - v0;
            vec3 n = e0.cross(e1).normalized();
            memcpy_s(computedNormals->data() + i0 * nStride, ncs * 3, n.data(), ncs * 3);
            memcpy_s(computedNormals->data() + i1 * nStride, ncs * 3, n.data(), ncs * 3);
            memcpy_s(computedNormals->data() + i2 * nStride, ncs * 3, n.data(), ncs * 3);
        }
        outBuffer->data = computedNormals->data();
        outBuffer->count = pb.count;
        outBuffer->stride = nStride;
        outBuffer->componentSize = ncs;
        outBuffer->componentType = GL_FLOAT;
    }
}

void SceneManager::fillTanBuffer(
    int accessorIndex,
    const Mesh::Buffer* indxBuffer,
    const Mesh::Buffer* posBuffer, 
    const Mesh::Buffer* uvBuffer, 
    const Mesh::Buffer* norBuffer,
    bool useHandedness,
    Mesh::Buffer* outBuffer)
{
    int requiredType = useHandedness ? TINYGLTF_TYPE_VEC4 : TINYGLTF_TYPE_VEC3;
    int numOfComponents = tinygltf::GetNumComponentsInType(requiredType);
    if (accessorIndex != -1)
    {
        auto& accessor = mWorkingModel->accessors[accessorIndex];
        if (accessor.sparse.isSparse)
        {
            assert(accessor.type == requiredType);
            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            if (useHandedness)  fillBufferFromSparseAccessor<vec4>(accessor, outBuffer);
            else                fillBufferFromSparseAccessor<vec3>(accessor, outBuffer);
        }
        else // not sparse
        {
            assert(accessor.type == requiredType);
            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
            auto& buffer = mWorkingModel->buffers[bufferView.buffer];
            size_t byteOffset, byteLength, byteStride;
            GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
            int32_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
            outBuffer->data = (uchar*)(buffer.data.data() + byteOffset);
            outBuffer->count = (uint)accessor.count;
            outBuffer->stride = (GLsizei)byteStride;
            outBuffer->componentSize = (GLsizei)componentSize;
            outBuffer->componentType = GL_FLOAT;
        }
    }
    else // no accessor - the tangents need to be computed (MikkTSpace algorithms)
    {
        // Create the computed data
        mWorkingMeshData.push_back(std::make_unique<MeshTools::DataElement>());
        auto computedTangents = mWorkingMeshData.back().get();
        auto tCount = posBuffer->count;
        auto cs = sizeof(float);
        auto tStride = cs * numOfComponents;
        computedTangents->resize(tCount * tStride);

        // Use MikkTSpace to compute tangents
        SMikkTSpaceInterface mikktspaceInterface;
        mikktspaceInterface.m_getNumFaces = &Primitive::getNumFaces;
        mikktspaceInterface.m_getNumVerticesOfFace = &Primitive::getNumVerticesOfFace;
        mikktspaceInterface.m_getPosition = &Primitive::getPosition;
        mikktspaceInterface.m_getNormal = &Primitive::getNormal;
        mikktspaceInterface.m_getTexCoord = &Primitive::getTexCoord;
        mikktspaceInterface.m_setTSpaceBasic = &Primitive::setTSpaceBasic;
        mikktspaceInterface.m_setTSpace = &Primitive::setTSpace;
        Primitive mikktspacePrimitive;
        mikktspacePrimitive.indexBuffer = indxBuffer;
        mikktspacePrimitive.positionBuffer = posBuffer;
        mikktspacePrimitive.uvBuffer = uvBuffer;
        mikktspacePrimitive.normalBuffer = norBuffer;
        mikktspacePrimitive.tangentBuffer = computedTangents->data();
        mikktspacePrimitive.tangentHasSign = useHandedness;
        SMikkTSpaceContext mikktspaceContext;
        mikktspaceContext.m_pInterface = &mikktspaceInterface;
        mikktspaceContext.m_pUserData = (void*)(&mikktspacePrimitive);
        genTangSpaceDefault(&mikktspaceContext);

        // Fill the data
        outBuffer->data = computedTangents->data();
        outBuffer->count = tCount;
        outBuffer->stride = (GLsizei)tStride;
        outBuffer->componentSize = (GLsizei)cs;
        outBuffer->componentType = GL_FLOAT;
    }
}

template<typename vecType>
void SceneManager::fillBufferFromSparseAccessor(const tinygltf::Accessor& accessor, Mesh::Buffer* outBuffer)
{
    // Initialize the array
    int32_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    int componentCount;
    if constexpr (std::is_same_v<vecType, vec2>)        componentCount = 2;
    else if constexpr (std::is_same_v<vecType, vec3>)   componentCount = 3;
    else if constexpr (std::is_same_v<vecType, vec4>)   componentCount = 4;
    else                                                throw; // either vec2, vec3 or vec4 supported as the sparse accessor type
    size_t newElementStride = componentSize * componentCount;

    mWorkingMeshData.push_back(std::make_unique<MeshTools::DataElement>());
    auto computedPositions = mWorkingMeshData.back().get();
    computedPositions->resize(accessor.count * newElementStride);
    if (accessor.bufferView != -1)
    {
        // copy from the original
        auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
        auto& buffer = mWorkingModel->buffers[bufferView.buffer];
        size_t byteOffset, byteLength, byteStride;
        GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
        for (uint i = 0; i < accessor.count; ++i)
        {
            memcpy_s(computedPositions->data() + i * newElementStride, newElementStride, buffer.data.data() + i * byteStride, newElementStride);
        }
    }
    else
    {
        // set to zeroes
        for (uint i = 0; i < accessor.count; ++i)
        {
            vecType v0 = vecType::Zero();
            memcpy_s(computedPositions->data() + i * newElementStride, newElementStride, v0.data(), newElementStride);
        }
    }
    outBuffer->data = computedPositions->data();
    outBuffer->count = (uint)accessor.count;
    outBuffer->stride = (GLsizei)newElementStride;
    outBuffer->componentSize = (GLsizei)componentSize;
    outBuffer->componentType = GL_FLOAT;

    // Apply sparse values
    const auto& sa = accessor.sparse;
    std::vector<uint> indices;
    { // Indices
        indices.resize(sa.count);
        assert(
            //sa.indices.componentType == TINYGLTF_COMPONENT_TYPE_INT || // not supported by gltf 2.0
            sa.indices.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ||
            sa.indices.componentType == TINYGLTF_COMPONENT_TYPE_SHORT ||
            sa.indices.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
            sa.indices.componentType == TINYGLTF_COMPONENT_TYPE_BYTE ||
            sa.indices.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);
        auto& bufferView = mWorkingModel->bufferViews[sa.indices.bufferView];
        auto& buffer = mWorkingModel->buffers[bufferView.buffer];
        size_t byteOffset, byteLength, byteStride;
        GLTFModelLoader::GetBufferAccessProperties(TINYGLTF_TYPE_SCALAR, sa.indices.componentType, sa.indices.byteOffset, bufferView,
            &byteOffset, &byteLength, &byteStride);
        for (size_t i = 0; i < sa.count; i++)
        {
            indices.at(i) = getIndex(buffer.data.data() + byteOffset, i, sa.indices.componentType, byteStride);
        }
    }

    { // Values
        auto& bufferView = mWorkingModel->bufferViews[sa.values.bufferView];
        auto& buffer = mWorkingModel->buffers[bufferView.buffer];
        size_t byteOffset, byteLength, byteStride;
        GLTFModelLoader::GetBufferAccessProperties(accessor.type, accessor.componentType, sa.values.byteOffset, bufferView,
            &byteOffset, &byteLength, &byteStride);
        for (size_t i = 0; i < sa.count; i++)
        {
            vecType* v0 = (vecType*)(buffer.data.data() + byteOffset + i * byteStride);
            uint i0 = indices[i];
            memcpy_s((void*)(outBuffer->data + i0 * outBuffer->stride), outBuffer->stride, v0->data(), outBuffer->stride);
        }
    }
}

Mesh *SceneManager::addMesh(const tinygltf::Mesh &gltfObj)
{
    int index = GLTFModelLoader::GetIndex(gltfObj, mWorkingModel->meshes);
    auto ite = mTypeIndexToObject.find({ typeid(Mesh), index });
    if (ite != mTypeIndexToObject.end()) return (Mesh*)ite->second;

    // Prepare
    mWorkingMeshData.clear();

    // Materials
    auto primitiveCount = (uint)gltfObj.primitives.size();
    std::vector<Material*> materials;
    materials.resize(primitiveCount);
    for (uint i = 0; i < primitiveCount; i++)
    {
        int materialIndex = gltfObj.primitives[i].material;
        if (materialIndex != -1)
        {
            auto& gltfMaterial =  mWorkingModel->materials[materialIndex];
            materials[i] = addMaterial(gltfMaterial);
        }
        else
        {
            materials[i] = mDefaultMaterial;
        }
    }

    // Skinning
    bool useSkinning = false;
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end() ||
            primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
        {
            useSkinning = true;
            break;
        }
    }

    // Indices
    std::vector<Mesh::Buffer> indexBuffers(primitiveCount);
    uint totalTriangleCount = 0;
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        int accessorIndex = primitive.indices;
        fillIndxBuffer(accessorIndex, &indexBuffers[i]);
        uint triangleCount = indexBuffers[i].count / 3;
        totalTriangleCount += triangleCount;
    }

    // Positions
    std::vector<Mesh::Buffer> positionBuffers(primitiveCount);
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        int accessorIndex = primitive.attributes.at("POSITION");
        fillPosBuffer(accessorIndex, &positionBuffers[i]);
    }

    // UVs
    std::vector<Mesh::Buffer> uvBuffers(primitiveCount);
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        int accessorIndex = (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
            ? primitive.attributes.at("TEXCOORD_0")
            : -1;
        fillUVBuffer(accessorIndex, &positionBuffers[i], &uvBuffers[i]);
    }

    // Normals
    std::vector<Mesh::Buffer> normalBuffers(primitiveCount);
    bool ignoreTangents = false;
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        int accessorIndex = (primitive.attributes.find("NORMAL") != primitive.attributes.end())
            ? primitive.attributes.at("NORMAL")
            : -1;
        ignoreTangents = (accessorIndex == -1); // TODO: make it per-primitive
        fillNorBuffer(accessorIndex, &indexBuffers[i], &positionBuffers[i], &normalBuffers[i]);
    }

    // Tangents
    std::vector<Mesh::Buffer> tangentBuffers(primitiveCount);
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        int accessorIndex = (!ignoreTangents && primitive.attributes.find("TANGENT") != primitive.attributes.end())
            ? primitive.attributes.at("TANGENT")
            : -1;
        fillTanBuffer(accessorIndex, &indexBuffers[i], &positionBuffers[i], &uvBuffers[i], &normalBuffers[i], true, &tangentBuffers[i]);
    }

    // Joints
    std::vector<Mesh::Buffer> jointBuffers;
    if (useSkinning) jointBuffers.resize(primitiveCount);
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
        {
            int accessorIndex = primitive.attributes.at("JOINTS_0");
            auto& accessor = mWorkingModel->accessors[accessorIndex];
            assert(accessor.type == TINYGLTF_TYPE_VEC4);
            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
                accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
            auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
            auto& buffer = mWorkingModel->buffers[bufferView.buffer];
            size_t byteOffset, byteLength, byteStride;
            GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
            int32_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
            jointBuffers[i].data = (uchar*)(buffer.data.data() + byteOffset);
            jointBuffers[i].count = (uint)accessor.count;
            jointBuffers[i].stride = (GLsizei)byteStride;
            jointBuffers[i].componentSize = (GLsizei)componentSize;
            switch (accessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                jointBuffers[i].componentType = GL_UNSIGNED_BYTE;
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                jointBuffers[i].componentType = GL_UNSIGNED_SHORT;
                break;
            default:
                break;
            }
        }
        else if (useSkinning)
        {
            // make up something here, like bind to 'common root'
            throw;
        }
    }

    // Weights
    std::vector<Mesh::Buffer> weightBuffers;
    if (useSkinning) weightBuffers.resize(primitiveCount);
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
        {
            int accessorIndex = primitive.attributes.at("WEIGHTS_0");
            auto& accessor = mWorkingModel->accessors[accessorIndex];
            assert(accessor.type == TINYGLTF_TYPE_VEC4);
            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ||
                accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
                accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
            auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
            auto& buffer = mWorkingModel->buffers[bufferView.buffer];
            size_t byteOffset, byteLength, byteStride;
            GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
            int32_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
            weightBuffers[i].data = (uchar*)(buffer.data.data() + byteOffset);
            weightBuffers[i].count = (uint)accessor.count;
            weightBuffers[i].stride = (GLsizei)byteStride;
            weightBuffers[i].componentSize = (GLsizei)componentSize;
            switch (accessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                weightBuffers[i].componentType = GL_FLOAT;
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                weightBuffers[i].componentType = GL_UNSIGNED_BYTE;
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                weightBuffers[i].componentType = GL_UNSIGNED_SHORT;
                break;
            default:
                break;
            }
        }
        else if (useSkinning)
        {
            // make up something here, like bind to 'common root'
            throw;
        }
    }

    // Morph Targets
    // from: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
    // All primitives MUST have the same number of morph targets in the same order.
    auto morphTargetsPerPrimitive = (primitiveCount > 0) ? (uint)gltfObj.primitives[0].targets.size() : 0;
    std::vector<Mesh::MorphTargets> morphTargets(primitiveCount);
    MeshTools meshTools;
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        assert(primitive.targets.size() == morphTargetsPerPrimitive);
        auto& mt = morphTargets[i];
        mt.posDiffBuffers.resize(morphTargetsPerPrimitive);
        mt.norDiffBuffers.resize(morphTargetsPerPrimitive);
        mt.tanDiffBuffers.resize(morphTargetsPerPrimitive);
        for (size_t j = 0; j < primitive.targets.size(); j++)
        {
            auto& target = primitive.targets[j];
            int posAccessorIndex = (target.find("POSITION") != target.end()) ? target.at("POSITION") : -1;
            int norAccessorIndex = (target.find("NORMAL") != target.end()) ? target.at("NORMAL") : -1;
            int tanAccessorIndex = (target.find("TANGENT") != target.end()) ? target.at("TANGENT") : -1;

            // position diffs
            fillPosBuffer(posAccessorIndex, &mt.posDiffBuffers[j]);
            // compute the morphed positions if needed
            Mesh::Buffer morphedPosBuffer;
            if (norAccessorIndex == -1 || tanAccessorIndex == -1)
            {
                // morphed positions
                mWorkingMeshData.push_back(std::make_unique<MeshTools::DataElement>());
                meshTools.bufferInitialization<vec3>(positionBuffers[j], MeshTools::Initialization::None, &morphedPosBuffer, mWorkingMeshData.back().get());
                meshTools.bufferOperation<vec3>(positionBuffers[j], mt.posDiffBuffers[j], MeshTools::Operation::Add, &morphedPosBuffer);
            }

            // normal diffs
            Mesh::Buffer morphedNorBuffer;
            if (norAccessorIndex == -1)
            {
                fillNorBuffer(-1, &indexBuffers[i], &morphedPosBuffer, &morphedNorBuffer);
                mWorkingMeshData.push_back(std::make_unique<MeshTools::DataElement>());
                meshTools.bufferInitialization<vec3>(morphedNorBuffer, MeshTools::Initialization::None, &mt.norDiffBuffers[j], mWorkingMeshData.back().get());
                meshTools.bufferOperation<vec3>(morphedNorBuffer, normalBuffers[j], MeshTools::Operation::Sub, &mt.norDiffBuffers[j]);
            }
            else
            {
                fillNorBuffer(norAccessorIndex, nullptr, nullptr, &mt.norDiffBuffers[j]);
                if (tanAccessorIndex == -1)
                {
                    // we still need to compute the morphed normals because the tangents are missing
                    mWorkingMeshData.push_back(std::make_unique<MeshTools::DataElement>());
                    meshTools.bufferInitialization<vec3>(normalBuffers[j], MeshTools::Initialization::None, &morphedNorBuffer, mWorkingMeshData.back().get());
                    meshTools.bufferOperation<vec3>(normalBuffers[j], mt.norDiffBuffers[j], MeshTools::Operation::Add, &morphedNorBuffer);
                }
            }

            // tangent diffs
            Mesh::Buffer morphedTanBuffer;
            if (tanAccessorIndex == -1)
            {
                fillTanBuffer(-1, &indexBuffers[i], &morphedPosBuffer, &uvBuffers[i], &morphedNorBuffer, false, &morphedTanBuffer);
                mWorkingMeshData.push_back(std::make_unique<MeshTools::DataElement>());
                meshTools.bufferInitialization<vec3>(morphedTanBuffer, MeshTools::Initialization::None, &mt.tanDiffBuffers[j], mWorkingMeshData.back().get());
                meshTools.bufferOperation<vec3>(morphedTanBuffer, tangentBuffers[j], MeshTools::Operation::Sub, &mt.tanDiffBuffers[j]);
            }
            else
            {
                // No handedness when morph-targeting TANGENT data since handedness cannot be displaced
                fillTanBuffer(tanAccessorIndex, nullptr, nullptr, nullptr, nullptr, false, &mt.tanDiffBuffers[j]);
            }
        }
    }

    // Bounds
    std::vector<bounds3> primitiveBounds(primitiveCount);
    for (uint i = 0; i < primitiveCount; i++)
    {
        auto& primitive = gltfObj.primitives[i];
        int accessorIndex = primitive.attributes.at("POSITION");
        auto& a = mWorkingModel->accessors[accessorIndex];
        assert(a.type == TINYGLTF_TYPE_VEC3);
        assert(a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        assert(a.minValues.size() == 3);
        primitiveBounds[i].min = vec3((float)a.minValues[0], (float)a.minValues[1], (float)a.minValues[2]);
        assert(a.maxValues.size() == 3);
        primitiveBounds[i].max = vec3((float)a.maxValues[0], (float)a.maxValues[1], (float)a.maxValues[2]);
    }

    auto ret = mGEI->createMesh(
        gltfObj.name, 
        materials, 
        positionBuffers, 
        uvBuffers, 
        normalBuffers,
        tangentBuffers, 
        jointBuffers, 
        weightBuffers,
        indexBuffers,
        morphTargets,
        primitiveBounds);
    mTypeIndexToObject[{ typeid(Mesh), index }] = ret;

    // Compute and store triangle data for ray casting
    if (useSkinning)
    {
        ret->getTriangleData()->resize(totalTriangleCount * sizeof(SkinnedTriangle));
        uint triangleIndex = 0;
        for (uint i = 0; i < primitiveCount; i++)
        {
            uint triangleCount = indexBuffers[i].count / 3;
            Primitive mikktspacePrimitive;
            mikktspacePrimitive.indexBuffer = &indexBuffers[i];
            mikktspacePrimitive.positionBuffer = &positionBuffers[i];
            mikktspacePrimitive.weightsBuffer = &weightBuffers[i];
            mikktspacePrimitive.jointsBuffer = &jointBuffers[i];
            SMikkTSpaceContext mikktspaceContext;
            mikktspaceContext.m_pInterface = nullptr;
            mikktspaceContext.m_pUserData = (void*)(&mikktspacePrimitive);

            for (uint j = 0; j < triangleCount; j++)
            {
                SkinnedTriangle triangle;
                for (int k = 0; k < 3; k++)
                {
                    vec3 v;
                    Primitive::getPosition(&mikktspaceContext, v.data(), j, k);
                    triangle.v[k] = v.homogeneous();
                    Primitive::getWeights(&mikktspaceContext, triangle.w[k], j, k);
                    Primitive::getJoints(&mikktspaceContext, triangle.j[k], j, k);

                    // sort based on weights (should be sorted already)
                    bool goAgain = true;
                    while (goAgain)
                    {
                        goAgain = false;
                        for (size_t i = 0; i < 3; i++)
                        {
                            if (triangle.w[k][i + 0] < triangle.w[k][i + 1])
                            {
                                goAgain = true;
                                std::swap(triangle.w[k][i + 0], triangle.w[k][i + 1]);
                                std::swap(triangle.j[k][i + 0], triangle.j[k][i + 1]);
                            }
                        }
                    }

                    // find last non-zero weight
                    for (uint l = 0; l < 4; l++)
                    {
                        if (triangle.w[k][l] > 0.0f)    triangle.wCount[k] = l + 1;
                        else                            break;
                    }
                }
                auto offset = triangleIndex * sizeof(SkinnedTriangle);
                memcpy_s(ret->getTriangleData()->data() + offset, sizeof(SkinnedTriangle), &triangle, sizeof(SkinnedTriangle));
                triangleIndex++;
            }
        }
    }
    else // static
    {
        ret->getTriangleData()->resize(totalTriangleCount * sizeof(Triangle));
        uint triangleIndex = 0;
        for (uint i = 0; i < primitiveCount; i++)
        {
            uint triangleCount = indexBuffers[i].count / 3;
            Primitive mikktspacePrimitive;
            mikktspacePrimitive.indexBuffer = &indexBuffers[i];
            mikktspacePrimitive.positionBuffer = &positionBuffers[i];
            SMikkTSpaceContext mikktspaceContext;
            mikktspaceContext.m_pInterface = nullptr;
            mikktspaceContext.m_pUserData = (void*)(&mikktspacePrimitive);

            for (uint j = 0; j < triangleCount; j++)
            {
                vec3 v[3];
                Primitive::getPosition(&mikktspaceContext, v[0].data(), j, 0);
                Primitive::getPosition(&mikktspaceContext, v[1].data(), j, 1);
                Primitive::getPosition(&mikktspaceContext, v[2].data(), j, 2);
                Triangle triangle = Triangle::fromPoints(v);
                auto offset = triangleIndex * sizeof(Triangle);
                memcpy_s(ret->getTriangleData()->data() + offset, sizeof(Triangle), &triangle, sizeof(Triangle));
                triangleIndex++;
            }
        }
    }

    // Extras
    ret->morphTargetNames.clear();
    if (gltfObj.extras.Has("targetNames"))
    {
        ret->morphTargetNames.resize(morphTargetsPerPrimitive);
        auto& targetNames = gltfObj.extras.Get("targetNames");
        assert(targetNames.ArrayLen() == morphTargetsPerPrimitive);
        for (size_t i = 0; i < targetNames.ArrayLen(); i++)
        {
            ret->morphTargetNames[i] = targetNames.Get(i).Get<std::string>();
        }
    }

    // Extensions
    ret->uses_KHR_materials_variants = false;
    std::map<int, std::vector<Material*>> variantToMaterialList;
    for (uint i = 0; i < primitiveCount; i++)
    {
        GLTFModelLoader::KHR_materials_variants::Primitive p;
        if (GLTFModelLoader::LoadExtension(&gltfObj.primitives[i], &p))
        {
            ret->uses_KHR_materials_variants = true;
            for (size_t j = 0; j < p.mappings.size(); j++)
            {
                auto& mapping = p.mappings[j];
                bool useVariant = false;
                for (size_t k = 0; k < mapping.variants.size(); k++)
                {
                    if (mVariantFilter.find(mapping.variants[k]) != mVariantFilter.end())
                    {
                        useVariant = true;
                        break;
                    }
                }
                if (!useVariant)
                {
                    continue;
                }
                int variantMaterialIndex = mapping.material;
                assert(variantMaterialIndex != -1);
                auto& gltfMaterial = mWorkingModel->materials[variantMaterialIndex];
                auto variantMaterial = addMaterial(gltfMaterial);
                for (size_t k = 0; k < mapping.variants.size(); k++)
                {
                    auto variant = mapping.variants[k];
                    if (variantToMaterialList.find(variant) == variantToMaterialList.end())
                    {
                        // Set the default materials as the starting point of the list
                        variantToMaterialList[variant] = materials;
                    }
                    // Set the variant-specific material for this primitive
                    variantToMaterialList[variant][i] = variantMaterial;
                }
            }
        }
    }
    ret->variants.clear();
    for (const auto& entry : variantToMaterialList)
    {
        auto variantIndex = entry.first;
        if (mVariantFilter.find(variantIndex) != mVariantFilter.end())
        {
            ret->variants.push_back(Mesh::Variant());
            auto& v = ret->variants.back();
            v.index = variantIndex;
            auto& materials = entry.second;
            v.instance = mGEI->createMeshVariant(materials, ret);
        }
    }

    return ret;
}

Skin* SceneManager::addSkin(const tinygltf::Skin& gltfObj)
{
    int index = GLTFModelLoader::GetIndex(gltfObj, mWorkingModel->skins);
    auto ite = mTypeIndexToObject.find({ typeid(Skin), index });
    if (ite != mTypeIndexToObject.end()) return (Skin*)ite->second;

    auto jointsCount = gltfObj.joints.size();
    std::vector<mat4x4> ibm(jointsCount);
    if (gltfObj.inverseBindMatrices != -1)
    {
        auto& accessor = mWorkingModel->accessors[gltfObj.inverseBindMatrices];
        assert(accessor.sparse.isSparse == false);
        assert(accessor.type == TINYGLTF_TYPE_MAT4);
        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
        auto& buffer = mWorkingModel->buffers[bufferView.buffer];
        size_t byteOffset, byteLength, byteStride;
        GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
        auto data = byteOffset + buffer.data.data();
        auto dataSize = accessor.count * byteStride;
        memcpy_s(ibm.data(), dataSize, data, dataSize);
    }
    else
    {
        // if not present, just generate identity matrices
        for (size_t i = 0; i < jointsCount; i++)
        {
            ibm[i] = mat4x4::Identity();
        }
    }

    auto ret = mGEI->createSkin(ibm);
    mTypeIndexToObject[{ typeid(Skin), index }] = ret;
    return ret;
}

void setTextureTransform(TextureTransform* ttPtr, const GLTFModelLoader::KHR_texture_transform::TextureInfo* ti)
{
    if (ti == nullptr)
    {
        ttPtr->Offset = vec2(0.0f, 0.0f);
        ttPtr->Rotation = 0.0f;
        ttPtr->Scale = vec2(1.0f, 1.0f);
    }
    else
    {
        ttPtr->Offset = vec2((float)ti->offset[0], (float)ti->offset[1]);
        ttPtr->Rotation = (float)ti->rotation;
        ttPtr->Scale = vec2((float)ti->scale[0], (float)ti->scale[1]);
    }
}

Material *SceneManager::addMaterial(const tinygltf::Material &gltfObj)
{
    int index = GLTFModelLoader::GetIndex(gltfObj, mWorkingModel->materials);
    auto ite = mTypeIndexToObject.find({ typeid(Material), index });
    if (ite != mTypeIndexToObject.end()) return (Material*)ite->second;

    color albedoColor = GLTFModelLoader::VectorToColor(gltfObj.pbrMetallicRoughness.baseColorFactor);
    float metallicFactor = (float)gltfObj.pbrMetallicRoughness.metallicFactor;
    float roughnessFactor = (float)gltfObj.pbrMetallicRoughness.roughnessFactor;
    float normalScale = (float)gltfObj.normalTexture.scale;
    color emissiveFactor = GLTFModelLoader::VectorToColor(gltfObj.emissiveFactor);

    GLTFModelLoader::KHR_materials_emissive_strength::Material emissiveStrengthMat;
    float emissiveStrength = 1.0f;
    bool uses_KHR_materials_emissive_strength = false;
    if (GLTFModelLoader::LoadExtension(&gltfObj, &emissiveStrengthMat))
    {
        emissiveStrength = (float)emissiveStrengthMat.emissiveStrength;
        uses_KHR_materials_emissive_strength = true;
    }

    GLTFModelLoader::KHR_materials_transmission::Material transmissionMat;
    float transmissionFactor = 0.0f;
    tinygltf::TextureInfo transmissionTextureInfo;
    bool uses_KHR_materials_transmission = false;
    if (GLTFModelLoader::LoadExtension(&gltfObj, &transmissionMat))
    {
        transmissionFactor = (float)transmissionMat.transmissionFactor;
        transmissionTextureInfo = transmissionMat.transmissionTexture;
        uses_KHR_materials_transmission = true;
    }

    bool result;
    TextureTransform
        albedoTransform,
        metallicRoughnessTransform,
        normalTransform,
        occlusionTransform,
        emissiveTransform,
        transmissionTransform;
    GLTFModelLoader::KHR_texture_transform::TextureInfo ti;
    bool Uses_KHR_texture_transform = false;

    result = GLTFModelLoader::LoadExtension(&gltfObj.pbrMetallicRoughness.baseColorTexture, &ti);
    Uses_KHR_texture_transform |= result;
    setTextureTransform(&albedoTransform, result ? &ti : nullptr);

    result = GLTFModelLoader::LoadExtension(&gltfObj.pbrMetallicRoughness.metallicRoughnessTexture, &ti);
    Uses_KHR_texture_transform |= result;
    setTextureTransform(&metallicRoughnessTransform, result ? &ti : nullptr);

    result = GLTFModelLoader::LoadExtension(&gltfObj.normalTexture, &ti);
    Uses_KHR_texture_transform |= result;
    setTextureTransform(&normalTransform, result ? &ti : nullptr);

    result = GLTFModelLoader::LoadExtension(&gltfObj.occlusionTexture, &ti);
    Uses_KHR_texture_transform |= result;
    setTextureTransform(&occlusionTransform, result ? &ti : nullptr);

    result = GLTFModelLoader::LoadExtension(&gltfObj.emissiveTexture, &ti);
    Uses_KHR_texture_transform |= result;
    setTextureTransform(&emissiveTransform, result ? &ti : nullptr);

    result = GLTFModelLoader::LoadExtension(&transmissionTextureInfo, &ti);
    Uses_KHR_texture_transform |= result;
    setTextureTransform(&transmissionTransform, result ? &ti : nullptr);

    Texture albedoTexture, metallicRoughnessTexture, normalTexture, emissiveTexture, transmissionTexture;
    auto albedoTexIndex = gltfObj.pbrMetallicRoughness.baseColorTexture.index;
    if (albedoTexIndex != -1)
    {
        auto& gltfTexture =  mWorkingModel->textures[albedoTexIndex];
        albedoTexture.name = gltfTexture.name;
        auto& gltfImage = mWorkingModel->images[gltfTexture.source];
        albedoTexture.image = addImage(gltfImage, mWorkingModel->IsInLinearSpace(albedoTexIndex));
    }

    auto normalTexIndex = gltfObj.normalTexture.index;
    if (normalTexIndex != -1)
    {
        auto& gltfTexture = mWorkingModel->textures[normalTexIndex];
        normalTexture.name = gltfTexture.name;
        auto& gltfImage = mWorkingModel->images[gltfTexture.source];
        normalTexture.image = addImage(gltfImage, mWorkingModel->IsInLinearSpace(normalTexIndex));
    }

    auto metallicRoughnessTexIndex = gltfObj.pbrMetallicRoughness.metallicRoughnessTexture.index;
    if (metallicRoughnessTexIndex != -1)
    {
        auto& gltfTexture = mWorkingModel->textures[metallicRoughnessTexIndex];
        metallicRoughnessTexture.name = gltfTexture.name;
        auto& gltfImage = mWorkingModel->images[gltfTexture.source];
        metallicRoughnessTexture.image = addImage(gltfImage, mWorkingModel->IsInLinearSpace(metallicRoughnessTexIndex));
    }

    auto emissiveTexIndex = gltfObj.emissiveTexture.index;
    if (emissiveTexIndex != -1)
    {
        auto& gltfTexture = mWorkingModel->textures[emissiveTexIndex];
        emissiveTexture.name = gltfTexture.name;
        auto& gltfImage = mWorkingModel->images[gltfTexture.source];
        emissiveTexture.image = addImage(gltfImage, mWorkingModel->IsInLinearSpace(emissiveTexIndex));
    }

    auto transmissionTexIndex = -1;
    if (transmissionTexIndex != -1)
    {
        auto& gltfTexture = mWorkingModel->textures[transmissionTexIndex];
        transmissionTexture.name = gltfTexture.name;
        auto& gltfImage = mWorkingModel->images[gltfTexture.source];
        transmissionTexture.image = addImage(gltfImage, mWorkingModel->IsInLinearSpace(transmissionTexIndex));
    }

    auto ret = mGEI->createMaterial(
        gltfObj.name,
        albedoColor,
        albedoTexture,
        metallicFactor,
        roughnessFactor,
        metallicRoughnessTexture,
        normalScale,
        normalTexture,
        emissiveFactor,
        emissiveTexture,
        uses_KHR_materials_emissive_strength,
        emissiveStrength,
        uses_KHR_materials_transmission,
        transmissionFactor,
        transmissionTexture,
        Uses_KHR_texture_transform,
        albedoTransform,
        metallicRoughnessTransform,
        normalTransform,
        occlusionTransform,
        emissiveTransform,
        transmissionTransform);
    mTypeIndexToObject[{ typeid(Material), index }] = ret;
    return ret;
}

Image* SceneManager::addImage(const tinygltf::Image& gltfObj, bool isLinearSpace)
{
    int index = GLTFModelLoader::GetIndex(gltfObj, mWorkingModel->images);
    auto ite = mTypeIndexToObject.find({ typeid(Image), index });
    if (ite != mTypeIndexToObject.end()) return (Image*)ite->second;

    std::string mimeType = gltfObj.mimeType.substr(6);
    const unsigned char* data = nullptr;
    size_t dataSize = 0;
    if (gltfObj.uri.empty())
    {
        int bufferViewIndex = gltfObj.bufferView;
        auto& bufferView = mWorkingModel->bufferViews[bufferViewIndex];
        int bufferIndex = bufferView.buffer;
        auto& buffer = mWorkingModel->buffers[bufferIndex];
        data = buffer.data.data() + bufferView.byteOffset;
        dataSize = bufferView.byteLength;
    }
    else
    {
        // TODO: Handle this case
        throw;
    }

    auto ret = mGEI->createImage(gltfObj.name, data, dataSize, gltfObj.width, gltfObj.height, mimeType, isLinearSpace);
    mTypeIndexToObject[{ typeid(Image), index }] = ret;
    return ret;
}

Animation::PathType getPathType(const std::string& str)
{
    static const std::unordered_map<std::string, Animation::PathType> typeMap = {
        {"translation", Animation::PathType::Translation},
        {"rotation", Animation::PathType::Rotation},
        {"scale", Animation::PathType::Scale},
        {"weights", Animation::PathType::Weights}
    };

    auto it = typeMap.find(str);
    assert(it != typeMap.end());
    return it->second;
}

Animation::InterpolationType getInterpolationType(const std::string& str)
{
    static const std::unordered_map<std::string, Animation::InterpolationType> typeMap = {
        {"LINEAR", Animation::InterpolationType::Linear},
        {"STEP", Animation::InterpolationType::Step},
        {"CUBICSPLINE", Animation::InterpolationType::CubicSpline}
    };

    auto it = typeMap.find(str);
    assert(it != typeMap.end());
    return it->second;
}

Animation* SceneManager::addAnimation(const tinygltf::Animation& gltfObj)
{
    int index = GLTFModelLoader::GetIndex(gltfObj, mWorkingModel->animations);
    auto ite = mTypeIndexToObject.find({ typeid(Animation), index });
    if (ite != mTypeIndexToObject.end()) return (Animation*)ite->second;

    mAnimations.push_back(std::make_unique<Animation>());
    auto ret = (Animation*)mAnimations.back().get();
    ret->name = gltfObj.name;
    ret->inputMin = FLT_MAX;
    ret->inputMax = -FLT_MAX;

    // Samplers
    auto samplerCount = (uint)gltfObj.samplers.size();
    ret->samplers.resize(samplerCount);
    for (uint i = 0; i < samplerCount; i++)
    {
        auto samplerIndex = gltfObj.channels[i].sampler;
        auto& gltfSampler = gltfObj.samplers[samplerIndex];
        auto& newSampler = ret->samplers[i];
        newSampler.interpolation = getInterpolationType(gltfSampler.interpolation);

        // Input (time)
        {
            auto& accessor = mWorkingModel->accessors[gltfSampler.input];
            assert(accessor.sparse.isSparse == false);
            assert(accessor.type == TINYGLTF_TYPE_SCALAR);
            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
            auto& buffer = mWorkingModel->buffers[bufferView.buffer];
            size_t byteOffset, byteLength, byteStride;
            GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
            auto data = byteOffset + buffer.data.data();
            for (size_t i = 0; i < accessor.count; i++)
            {
                float* time = (float*)(data + i * byteStride);
                newSampler.input[*time] = (uint)i;
            }

            ret->inputMin = std::min(ret->inputMin, (float)accessor.minValues[0]);
            ret->inputMax = std::max(ret->inputMax, (float)accessor.maxValues[0]);
        }

        // Output (position, rotation, scale or weights)
        {
            auto& accessor = mWorkingModel->accessors[gltfSampler.output];
            assert(accessor.sparse.isSparse == false);
            assert(accessor.sparse.isSparse == false);
            assert(accessor.type == TINYGLTF_TYPE_SCALAR ||
                accessor.type == TINYGLTF_TYPE_VEC3 ||
                accessor.type == TINYGLTF_TYPE_VEC4);
            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            auto& bufferView = mWorkingModel->bufferViews[accessor.bufferView];
            auto& buffer = mWorkingModel->buffers[bufferView.buffer];
            size_t byteOffset, byteLength, byteStride;
            GLTFModelLoader::GetBufferAccessProperties(accessor, bufferView, &byteOffset, &byteLength, &byteStride);
            auto data = byteOffset + buffer.data.data();
            auto dataSize = accessor.count * byteStride;
            newSampler.output.resize(dataSize);
            memcpy_s(newSampler.output.data(), dataSize, data, dataSize);
        }
    }
    ret->inputDuration = ret->inputMax - ret->inputMin;

    // Channels
    auto channelCount = (uint)gltfObj.channels.size();
    ret->channels.resize(channelCount);
    for (uint i = 0; i < channelCount; i++)
    {
        auto& newChannel = ret->channels[i];
        newChannel.targetPath = getPathType(gltfObj.channels[i].target_path);
        auto samplerIndex = gltfObj.channels[i].sampler;
        newChannel.sampler = &ret->samplers[samplerIndex];
    }

    mTypeIndexToObject[{ typeid(Animation), index }] = ret;
    return ret;
}

AnimationTarget SceneManager::addAnimationTarget(const tinygltf::Animation& gltfObj)
{
    AnimationTarget at;
    at.animation = addAnimation(gltfObj);
    auto channelCount = (uint)gltfObj.channels.size();
    at.nodes.resize(channelCount);
    for (uint i = 0; i < channelCount; i++)
    {
        auto nodeIndex = gltfObj.channels[i].target_node;
        auto nodeIt = mTypeIndexToObject.find({ typeid(Node), nodeIndex });
        assert(nodeIt != mTypeIndexToObject.end());
        at.nodes[i] = (Node*)nodeIt->second;
    }
    return at;
}

void getInerpolationValues(
    const Animation::Sampler* sampler,
    float time,
    float* interpolationFactor,
    uint* segmentBegin,
    uint* segmentEnd)
{
    auto segmentEndIt = sampler->input.upper_bound(time);
    if (segmentEndIt == sampler->input.end())
    {
        segmentEndIt--;
    }
    else if (segmentEndIt == sampler->input.begin())
    {
        segmentEndIt++;
    }
    auto segmentBeginIt = std::prev(segmentEndIt);

    float segDuration = segmentEndIt->first - segmentBeginIt->first;
    *interpolationFactor = (time - segmentBeginIt->first) / segDuration;
    *interpolationFactor = std::clamp(*interpolationFactor, 0.0f, 1.0f);
    *segmentBegin = segmentBeginIt->second;
    *segmentEnd = segmentEndIt->second;
}

vec3 interpolateVec3(const Animation::Sampler* sampler, float time)
{
    float interpolationFactor;
    uint i0, i1;
    getInerpolationValues(sampler, time, &interpolationFactor, &i0, &i1);
    auto v0 = *(vec3*)(sampler->output.data() + i0 * sizeof(float) * 3);
    auto v1 = *(vec3*)(sampler->output.data() + i1 * sizeof(float) * 3);
    switch (sampler->interpolation)
    {
    case Animation::InterpolationType::Step:
        return v0;
    case Animation::InterpolationType::Linear:
        return (1.0f - interpolationFactor) * v0 + interpolationFactor * v1;
    default:
        throw;
    }
}

quat interpolateQuat(const Animation::Sampler* sampler, float time)
{
    float interpolationFactor;
    uint i0, i1;
    getInerpolationValues(sampler, time, &interpolationFactor, &i0, &i1);
    auto v0 = *(quat*)(sampler->output.data() + i0 * sizeof(float) * 4);
    auto v1 = *(quat*)(sampler->output.data() + i1 * sizeof(float) * 4);
    switch (sampler->interpolation)
    {
    case Animation::InterpolationType::Step:
        return v0;
    case Animation::InterpolationType::Linear:
        return v0.slerp(interpolationFactor, v1);
    default:
        throw;
    }
}

void SceneManager::retargetAnimation(AnimationTarget* atDest, Node* target, const AnimationTarget& atSrc)
{
    mapNameToNode(target, true);
    atDest->animation = atSrc.animation;
    atDest->nodes.resize(atSrc.nodes.size());
    for (size_t i = 0; i < atSrc.nodes.size(); i++)
    {
        auto srcNodeName = atSrc.nodes[i]->name;
        atDest->nodes[i] = mNameToNodeMap[srcNodeName];
    }
}

void SceneManager::setAnimationTime(const AnimationTarget& animationTarget, float time, float blend)
{
    auto animation = animationTarget.animation;
    auto channelCount = (uint)animation->channels.size();
    for (uint i = 0; i < channelCount; i++)
    {
        auto sampler = animation->channels[i].sampler;
        auto node = animationTarget.nodes[i];
        switch (animation->channels[i].targetPath)
        {
        case Animation::PathType::Translation:
        {
            vec3 newPos = interpolateVec3(sampler, time);
            node->localPosition = (1.0f - blend) * node->localPosition + blend * newPos;
        }
        break;
        case Animation::PathType::Rotation:
        {
            quat newRotation = interpolateQuat(sampler, time);
            node->localRotation = node->localRotation.slerp(blend, newRotation);
        }
        break;
        case Animation::PathType::Scale:
        {
            vec3 newScale = interpolateVec3(sampler, time);
            node->localScale = (1.0f - blend) * node->localScale + blend * newScale;
        }
        break;
        default:
            throw;
            break;
        }
    }
}

Node* SceneManager::importNode(NodeContainer* container, Node* src)
{
    Skin* skin = nullptr;
    if (src->skin)
    {
        skin = mGEI->createSkin(src->skin->inverseBindMatrices);
    }

    auto destNode = mGEI->createNode(src->name, container, src->localPosition, src->localScale, src->localRotation, src->mesh, skin);
    destNode->extra = src->extra;
    for (size_t i = 0; i < src->children.size(); i++)
    {
        importNode(destNode, src->children[i]);
    }

    return destNode;
}

void SceneManager::mapNodeToNode(Node* dest, Node* src, bool clean)
{
    if (clean)
    {
        mNodeToNodeMap.clear();
    }

    mNodeToNodeMap[src] = dest;
    for (size_t i = 0; i < src->children.size(); i++)
    {
        mapNodeToNode(dest->children[i], src->children[i], false);
    }
}

void SceneManager::mapNameToNode(Node* dest, bool clean)
{
    if (clean)
    {
        mNameToNodeMap.clear();
    }

    mNameToNodeMap[dest->name] = dest;
    for (size_t i = 0; i < dest->children.size(); i++)
    {
        mapNameToNode(dest->children[i], false);
    }
}

void SceneManager::remapImportedSkins(Node* dest, Node* src)
{
    if (dest->skin)
    {
        size_t jointCount = src->skin->joints.size();
        for (size_t i = 0; i < jointCount; i++)
        {
            auto destSkin = const_cast<Skin*>(dest->skin);
            destSkin->joints[i] = mNodeToNodeMap[src->skin->joints[i]];
        }
    }

    size_t childCount = src->children.size();
    for (size_t i = 0; i < childCount; i++)
    {
        remapImportedSkins(dest->children[i], src->children[i]);
    }
}

Node* SceneManager::findCommonRoot(const std::vector<Node*>& nodes)
{
    std::map<Node*, uint> hitsMap;
    for (size_t i = 0; i < nodes.size(); i++)
    {
        Node* node = nodes[i];
        do
        {
            node = dynamic_cast<Node*>(node->belongsTo);
            if (hitsMap.find(node) == hitsMap.end()) hitsMap[node] = 0;
            hitsMap[node] += 1;
        } while (node);
    }
    for (auto it = hitsMap.begin(); it != hitsMap.end(); ++it)
    {
        Node* node = it->first;
        uint hits = it->second;
        if (node != nullptr && hits == nodes.size())
            return node;
    }
    return nullptr;
}

RayCastHit SceneManager::rayCast(NodeContainer* container, const ray3& ray)
{
    Node* node = dynamic_cast<Node*>(container);
    if (node != nullptr) // is node
    {
        transform parentTransform = node->belongsTo->computeWorldTransform();
        return rayCastNode(node, parentTransform, ray);
    }
    else // is scene
    {
        Scene* scene = dynamic_cast<Scene*>(container);
        RayCastHit closestHit;
        closestHit.node = nullptr;
        closestHit.distance = FLT_MAX;
        for (size_t i = 0; i < scene->nodes.size(); i++)
        {
            auto hit = rayCastNode(scene->nodes[i], transform::Identity(), ray);
            if (hit.distance < closestHit.distance)
            {
                closestHit = hit;
            }
        }
        return closestHit;
    }
}

bool rayIntersectsTriangle(const ray3& ray, const Triangle& triangle, float* t)
{
    // Möller–Trumbore intersection algorithm
    constexpr float epsilon = std::numeric_limits<float>::epsilon();
    vec3 ray_cross_e2 = ray.direction.cross(triangle.e2);
    float det = triangle.e1.dot(ray_cross_e2);

    if (det > -epsilon && det < epsilon)
        return false;    // This ray is parallel to this triangle.

    float inv_det = 1.0 / det;
    vec3 s = ray.origin - triangle.v0;
    float u = inv_det * s.dot(ray_cross_e2);

    if (u < 0 || u > 1)
        return false;

    vec3 s_cross_e1 = s.cross(triangle.e1);
    float v = inv_det * ray.direction.dot(s_cross_e1);

    if (v < 0 || u + v > 1)
        return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    *t = inv_det * triangle.e2.dot(s_cross_e1);

    if (*t > epsilon) // ray intersection
    {
        return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
    {
        return false;
    }
}

RayCastHit SceneManager::rayCastNode(Node* node, const transform& parentTransform, const ray3& ray)
{
    RayCastHit closestHit;
    transform nodeTransform = parentTransform * node->getLocalTransform();
    if (node->mesh && node->skin)
    {
        mNodeToWorldTransform.clear();
        auto boneCount = node->skin->inverseBindMatrices.size();
        std::vector<mat4x4> bones(boneCount);
        for (size_t i = 0; i < boneCount; i++)
        {
            auto joint = node->skin->joints[i];
            computeAndStoreWorldTransforms(joint); // to get up-to-date transforms
            bones[i] = mNodeToWorldTransform[joint].matrix() * node->skin->inverseBindMatrices[i];
        }
        // Find the closest hit
        float best_t = FLT_MAX;
        assert(node->mesh->skinned);
        uint triangleCount = (uint)(node->mesh->getTriangleData()->size() / sizeof(SkinnedTriangle));
        for (uint i = 0; i < triangleCount; i++)
        {
            auto offset = i * sizeof(SkinnedTriangle);
            auto& skinnedTriangle = *(SkinnedTriangle*)(node->mesh->getTriangleData()->data() + offset);
            vec3 v[3];
            for (uint j = 0; j < 3; j++) // for each triangle vertex
            {
                vec4 hV = vec4::Zero();
                auto wCount = skinnedTriangle.wCount[j];
                for (uint k = 0; k < wCount; ++k) // for each bone weight
                {
                    float w = skinnedTriangle.w[j][k];
                    auto boneIndex = skinnedTriangle.j[j][k];
                    auto& boneTransform = bones[boneIndex];
                    vec4 hV_i = boneTransform * skinnedTriangle.v[j];
                    hV = hV + hV_i * w;
                }
                v[j] = hV.head<3>();
            }
            Triangle triangle = Triangle::fromPoints(v);
            float t;
            if (rayIntersectsTriangle(ray, triangle, &t) && t < best_t)
            {
                best_t = t;
            }
        }
        if (best_t < FLT_MAX)
        {
            closestHit.distance = best_t;
            closestHit.node = node;
        }
    }
    else if (node->mesh)
    {
        // Transform the ray to local space
        ray3 rayLocalSpace = ray;
        rayLocalSpace.transform(nodeTransform.inverse().matrix());
        // Find the closest hit
        float best_t = FLT_MAX;
        assert(!node->mesh->skinned);
        uint triangleCount = (uint)(node->mesh->getTriangleData()->size() / sizeof(Triangle));
        for (uint i = 0; i < triangleCount; i++)
        {
            auto offset = i * sizeof(Triangle);
            auto& triangle = *(Triangle*)(node->mesh->getTriangleData()->data() + offset);
            float t;
            if (rayIntersectsTriangle(rayLocalSpace, triangle, &t) && t < best_t)
            {
                best_t = t;
            }
        }
        if (best_t < FLT_MAX)
        {
            // Go back to world space
            vec3 hitLocalSpace = rayLocalSpace.getPosition(best_t);
            vec3 hitWorldSpace = (nodeTransform * hitLocalSpace.homogeneous()).head<3>();
            float dirNorm = ray.direction.norm();
            float best_t_worldSpace = (hitWorldSpace - ray.origin).dot(ray.direction) / (dirNorm * dirNorm);
            closestHit.distance = best_t_worldSpace;
            closestHit.node = node;
        }
    }

    for (size_t i = 0; i < node->children.size(); i++)
    {
        auto hit = rayCastNode(node->children[i], nodeTransform, ray);
        if (hit.distance < closestHit.distance)
        {
            closestHit = hit;
        }
    }

    return closestHit;
}

void SceneManager::computeAndStoreWorldTransforms(Node* node)
{
    auto parentNode = dynamic_cast<Node*>(node->belongsTo);
    if (parentNode == nullptr) // it's a scene
    {
        mNodeToWorldTransform[node] = node->getLocalTransform();
    }
    else // it's a node
    {
        if (mNodeToWorldTransform.find(parentNode) == mNodeToWorldTransform.end())
        {
            computeAndStoreWorldTransforms(parentNode);
        }
        auto parentTransform = mNodeToWorldTransform[parentNode];
        mNodeToWorldTransform[node] = parentTransform * node->getLocalTransform();
    }
}
