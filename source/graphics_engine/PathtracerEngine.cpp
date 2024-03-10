#include "PathtracerEngine.h"
#include "GLTFModelLoader.h"
#include "SceneManager.h"
#include <interactive_cycles.h>
#include <offline_cycles.h>

#include <thread>
#include <mutex>
#include <atomic>

#include <QDebug>


namespace graphics_engine
{
    struct PathtracerScene : public Scene
    {
        // no data members needed
    };

    struct PathtracerNode : public Node
    {
        cycles_wrapper::Node* cyclesHandle;
    };

    struct PathtracerMesh : public Mesh
    {
        virtual std::vector<bounds3>* getBounds() { return &submeshBounds; }
        virtual std::vector<uchar>* getTriangleData() { return &triangleData; }

        cycles_wrapper::Mesh* cyclesHandle;

        std::vector<bounds3> submeshBounds;
        std::vector<uchar> triangleData;
    };

    struct PathtracerImage : public Image
    {
        virtual uint getWidth() const { return width; }
        virtual uint getHeight() const { return height; }
        virtual int getCount() const { return 1; }

        cycles_wrapper::Texture* cyclesHandle;

        // Cycles expects us to keep the image data so we store it here
        std::vector<uchar> data;
        size_t dataSize;
        uint width, height;
        std::string mimeType;
    };

    struct PathtracerMaterial : public Material
    {
        cycles_wrapper::Material* cyclesHandle;
    };


    class PathtracerEnginePrivate
    {
    public:
        PathtracerEnginePrivate(PathtracerEngine* ptr)
            : _this(ptr)
        {
            mInteractiveCycles.SetLogFunction(_this, PathtracerEnginePrivate::logFunction);
        }

        ~PathtracerEnginePrivate()
        {
            if (mRenderJobThread.joinable()) {
                mRenderJobThread.join();
            }

            //_this->mInteractiveCycles.SetSuspended(true);
            mInteractiveCycles.SessionExit();
            mInteractiveCycles.DeinitializeOpenGL();
        }

        void beginUpdate()
        {
            if (mUpdateCounter == 0)
                mInteractiveCycles.SetSuspended(true);
            mUpdateCounter++;
        }

        void endUpdate()
        {
            mUpdateCounter--;
            if (mUpdateCounter == 0)
                mInteractiveCycles.SetSuspended(false);
            assert(mUpdateCounter >= 0);
        }

        static void logFunction(void* obj, int type, const char* msg)
        {
            (void)obj;
            return;
            switch (type)
            {
            case LOG_TYPE_INFO:
                qInfo() << msg;
                break;

            case LOG_TYPE_WARNING:
                qWarning() << msg;
                break;

            case LOG_TYPE_ERROR:
                qWarning() << msg;
                break;

            case LOG_TYPE_DEBUG:
            default:
                qDebug() << msg;
                break;
            }
        }

        // Offline rendering
        void renderOffline_threadWorker(const RenderJob&);
        void renderOffline_addScene(const PathtracerScene*);
        void renderOffline_addNode(const PathtracerNode*, cycles_wrapper::Node*);

    public:
        PathtracerEngine* const _this;

        cycles_wrapper::InteractiveCycles mInteractiveCycles;
        GLTFModelLoader mModelLoader;
        SceneManager mSceneManager;
        int mUpdateCounter = 0;
        Camera mViewportCameraCache;

        // Offline rendering
        std::thread mRenderJobThread;
        std::atomic<bool> mRenderJobThreadRunning;
        std::vector<RenderJob> mRenderJobs;
        std::unique_ptr<cycles_wrapper::OfflineCycles> mOfflineCycles;
    };

    struct UpdateGuard
    {
        UpdateGuard(PathtracerEnginePrivate* p)
            : ptr(p)
        {
            ptr->beginUpdate();
        }
    
        ~UpdateGuard()
        {
            ptr->endUpdate();
        }
    
    private:
        PathtracerEnginePrivate* const ptr;
    };
}

#define UPDATE_GUARD UpdateGuard ug(_this.get());

using namespace graphics_engine;

PathtracerEngine::PathtracerEngine()
    : _this(nullptr)
{

}

PathtracerEngine::~PathtracerEngine()
{

}

void PathtracerEngine::initialize(void* windowHWND)
{
    _this.reset();
    _this = std::make_unique<PathtracerEnginePrivate>(this);

    // Cycles
    bool isOk = true;
    isOk &= _this->mInteractiveCycles.InitializeOpenGL(windowHWND);
    isOk &= _this->mInteractiveCycles.SessionInit();

    UPDATE_GUARD;
    if (isOk)
    {
        //SetCamera(vec3::UnitZ * -4.0f, vec3::UnitZ, vec3::UnitY);
        cycles_wrapper::BackgroundSettings bs;
        bs.mType = cycles_wrapper::BackgroundSettings::Type::Color;
        bs.mColor[0] = 0.0f;
        bs.mColor[1] = 0.0f;
        bs.mColor[2] = 0.0f;
        //bs.mSky.mSunDirection[0] = 1.0f;
        //bs.mSky.mSunDirection[1] = 0.0f;
        //bs.mSky.mSunDirection[2] = 0.0f;
        _this->mInteractiveCycles.SetSceneBackground(bs);
        setCameraLookAt(vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 1, 0));
    }

    // The rest
    _this->mSceneManager.create(this);
    _this->mRenderJobThreadRunning = false;
}

void PathtracerEngine::deinitialize()
{
    _this.reset();
}

void PathtracerEngine::resize(int w, int h)
{
    _this->mInteractiveCycles.Resize(w, h);
}

void PathtracerEngine::update(float dt)
{
    (void)dt;
    if (_this->mRenderJobs.size() > 0 && !_this->mRenderJobThreadRunning)
    {
        // Make sure we are really done
        if (_this->mRenderJobThread.joinable()) {
            _this->mRenderJobThread.join();
        }




        // Start a new render job
        _this->mRenderJobThreadRunning = true;
        auto newRenderJob = _this->mRenderJobs.front();
        _this->mRenderJobs.erase(_this->mRenderJobs.begin());
        _this->mRenderJobThread = std::thread([this, newRenderJob]() {
            _this->renderOffline_threadWorker(newRenderJob);
            });
    }
}

void PathtracerEngine::draw()
{
    _this->mInteractiveCycles.Draw();
}

const Camera& PathtracerEngine::getCamera()
{
    float n, f, fov, aspect;
    vec3 p, d, u;
    _this->mInteractiveCycles.GetCamera(p.data(), d.data(), u.data(), &n, &f, &fov, &aspect);
    _this->mViewportCameraCache.setLookAt(p, d, u);
    _this->mViewportCameraCache.setPerspective(fov, aspect);
    _this->mViewportCameraCache.setPlaneDistances(n, f);
    return _this->mViewportCameraCache;
}

void PathtracerEngine::setCameraLookAt(const vec3& pos, const vec3& dir, const vec3& up)
{
    UPDATE_GUARD;
    _this->mViewportCameraCache.setLookAt(pos, dir, up);
    float p[] = { pos.x(), pos.y(), pos.z()};
    float d[] = { dir.x(), dir.y(), dir.z()};
    float u[] = { up.x(), up.y(), up.z()};
    _this->mInteractiveCycles.SetCamera(cycles_wrapper::CameraType::Perspective, p, d, u);
}

Scene* PathtracerEngine::createScene(const std::string& name)
{
    UPDATE_GUARD;
    _this->mSceneManager.mScenes.push_back(std::make_unique<PathtracerScene>());
    auto ret = (PathtracerScene*)_this->mSceneManager.mScenes.back().get();
    ret->name = name;
    return ret;
}

Node* PathtracerEngine::createNode(
    const std::string& name,
    NodeContainer* belongsTo,
    const vec3& p, const vec3& s, const quat& r,
    Mesh* mesh,
    Skin*)
{
    UPDATE_GUARD;
    _this->mSceneManager.mNodes.push_back(std::make_unique<PathtracerNode>());
    auto ret = _this->mSceneManager.mNodes.back().get();
    ret->name = name;
    ret->belongsTo = belongsTo;
    Node* node = dynamic_cast<Node*>(belongsTo);
    if (node != nullptr)
    {
        node->children.push_back(ret);
    }
    else
    {
        Scene* scene = dynamic_cast<Scene*>(belongsTo);
        scene->nodes.push_back(ret);
    }
    ret->localPosition = p;
    ret->localScale = s;
    ret->localRotation = r;
    ret->mesh = mesh;
    //ret->localPosition = t.translation();
    //t.computeRotationScaling(&ret->localRotation, &ret->localScale);
    return ret;
}

bool PathtracerEngine::loadModel(const LoadParameters& params, Model* model)
{
    UPDATE_GUARD;
    if (params.preloadedFile != nullptr)
    {
        model->loadedFile.reset();
        _this->mSceneManager.addModel(params.preloadedFile, params.allAnimations, params.allVariants, params.variantFilter, model);
    }
    else
    {
        model->loadedFile = _this->mModelLoader.LoadModel(params.filepath.c_str());
        if (model->loadedFile == nullptr)
            return false;
        _this->mSceneManager.addModel(model->loadedFile.get(), params.allAnimations, params.allVariants, params.variantFilter, model);
    }
    return true;
}

void PathtracerEngine::setActiveScene(Scene* scene)
{
    UPDATE_GUARD;
    for (size_t i = 0; i < scene->nodes.size(); i++)
    {
        addNodeToCycles(scene->nodes[i]);
    }
}

Scene* PathtracerEngine::getActiveScene()
{
    throw;
}

void setCyclesMeshBuffer(cycles_wrapper::Mesh::Buffer* dest, const Mesh::Buffer& src)
{
    dest->data = src.data;
    dest->count = src.count;
    dest->stride = src.stride;
    dest->componentSize = src.componentSize;
    dest->componentType = src.componentType;
}

Mesh* PathtracerEngine::createMesh(
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
    const std::vector<bounds3>& submeshBounds)
{
    // Skinning not supported yet
    assert(jointBuffers.size() == 0);
    assert(weightBuffers.size() == 0);

    UPDATE_GUARD;
    auto cyclesScene = _this->mInteractiveCycles.GetScene();
    auto primitiveCount = (uint)materials.size();
    std::vector<cycles_wrapper::Material*> cyclesMaterials(primitiveCount);
    std::vector<cycles_wrapper::Mesh::Buffer> cyclesPosBuffer(primitiveCount);
    std::vector<cycles_wrapper::Mesh::Buffer> cyclesNorBuffer(primitiveCount);
    std::vector<cycles_wrapper::Mesh::Buffer> cyclesTanBuffer(primitiveCount);
    std::vector<cycles_wrapper::Mesh::Buffer> cyclesUVBuffer(primitiveCount);
    std::vector<cycles_wrapper::Mesh::Buffer> cyclesIndBuffer(primitiveCount);
    for (size_t i = 0; i < primitiveCount; i++)
    {
        auto material = (PathtracerMaterial*)materials[i];
        cyclesMaterials[i] = material->cyclesHandle;
        setCyclesMeshBuffer(&cyclesPosBuffer[i], positionBuffers[i]);
        setCyclesMeshBuffer(&cyclesNorBuffer[i], normalBuffers[i]);
        setCyclesMeshBuffer(&cyclesTanBuffer[i], tangentBuffers[i]);
        setCyclesMeshBuffer(&cyclesUVBuffer[i], uvBuffers[i]);
        setCyclesMeshBuffer(&cyclesIndBuffer[i], indexBuffers[i]);
    }

    _this->mSceneManager.mMeshes.push_back(std::make_unique<PathtracerMesh>());
    auto ret = (PathtracerMesh*)_this->mSceneManager.mMeshes.back().get();
    ret->name = name;
    ret->materials = materials;
    ret->submeshBounds = submeshBounds;
    ret->skinned = false; // not supported
    ret->morphTargetCount = 0; // not supported
    ret->cyclesHandle = _this->mInteractiveCycles.AddMesh(
        cyclesScene, 
        name.c_str(),
        cyclesMaterials.data(),
        cyclesPosBuffer.data(),
        cyclesNorBuffer.data(),
        cyclesUVBuffer.data(),
        cyclesTanBuffer.data(),
        cyclesIndBuffer.data(),
        primitiveCount);
    return ret;
}

Image* PathtracerEngine::createImage(
    const std::string& name,
    const uchar* data,
    size_t dataSize,
    uint width,
    uint height,
    const std::string& mimeType,
    bool isLinearSpace)
{
    UPDATE_GUARD;
    _this->mSceneManager.mImages.push_back(std::make_unique<PathtracerImage>());
    auto ret = (PathtracerImage*)_this->mSceneManager.mImages.back().get();
    ret->name = name;
    ret->mLinearSpace = isLinearSpace;
    ret->data.resize(dataSize);
    memcpy_s(ret->data.data(), dataSize, data, dataSize);
    ret->dataSize = dataSize;
    ret->width = width;
    ret->height = height;
    ret->mimeType = mimeType;

    auto cyclesScene = _this->mInteractiveCycles.GetScene();
    ret->cyclesHandle = _this->mInteractiveCycles.AddTexture(
        cyclesScene,
        name.c_str(),
        ret->data.data(), ret->dataSize,
        mimeType.c_str(),
        !isLinearSpace);
    return ret;
}

void setCyclesTextureTransform(cycles_wrapper::TextureTransform* dest, const TextureTransform& src)
{
    dest->offset[0] = src.Offset.x();
    dest->offset[1] = src.Offset.y();
    dest->rotation = src.Rotation;
    dest->scale[0] = src.Scale.x();
    dest->scale[1] = src.Scale.y();
}

Material* PathtracerEngine::createMaterial(
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
    const TextureTransform& transmissionTransform)
{
    UPDATE_GUARD;
    _this->mSceneManager.mMaterials.push_back(std::make_unique<PathtracerMaterial>());
    auto ret = (PathtracerMaterial*)_this->mSceneManager.mMaterials.back().get();
    ret->name = name;
    ret->albedoColor = albedoColor;
    ret->albedoTexture = albedoTexture;
    ret->metallicFactor = metallicFactor;
    ret->roughnessFactor = roughnessFactor;
    ret->metallicRoughnessTexture = metallicRoughnessTexture;
    ret->normalScale = normalScale;
    ret->normalTexture = normalTexture;
    ret->emissiveFactor = emissiveFactor;
    ret->emissiveTexture = emissiveTexture;
    ret->uses_KHR_materials_emissive_strength = uses_KHR_materials_emissive_strength;
    ret->emissiveStrength = emissiveStrength;
    ret->uses_KHR_materials_transmission = uses_KHR_materials_transmission;
    ret->transmissionFactor = transmissionFactor;
    ret->transmissionTexture = transmissionTexture;
    ret->Uses_KHR_texture_transform = Uses_KHR_texture_transform;
    ret->albedoTransform = albedoTransform;
    ret->metallicRoughnessTransform = metallicRoughnessTransform;
    ret->normalTransform = normalTransform;
    ret->occlusionTransform = occlusionTransform;
    ret->emissiveTransform = emissiveTransform;
    ret->transmissionTransform = transmissionTransform;

    // Create the shader
    auto cyclesScene = _this->mInteractiveCycles.GetScene();
    cycles_wrapper::Texture* albedoTex = albedoTexture.isValid() ? ((PathtracerImage*)albedoTexture.image)->cyclesHandle : nullptr;
    cycles_wrapper::Texture* metallicRoughnessTex = metallicRoughnessTexture.isValid() ? ((PathtracerImage*)metallicRoughnessTexture.image)->cyclesHandle : nullptr;
    cycles_wrapper::Texture* normalTex = normalTexture.isValid() ? ((PathtracerImage*)normalTexture.image)->cyclesHandle : nullptr;
    cycles_wrapper::Texture* emissiveTex = emissiveTexture.isValid() ? ((PathtracerImage*)emissiveTexture.image)->cyclesHandle : nullptr;
    cycles_wrapper::Texture* transmissionTex = emissiveTexture.isValid() ? ((PathtracerImage*)transmissionTexture.image)->cyclesHandle : nullptr;
    cycles_wrapper::TextureTransform albedoTrans, metallicRoughnessTrans, normalTrans, occlusionTrans, emissiveTrans, transmissionTrans;
    setCyclesTextureTransform(&albedoTrans, albedoTransform);
    setCyclesTextureTransform(&metallicRoughnessTrans, metallicRoughnessTransform);
    setCyclesTextureTransform(&normalTrans, normalTransform);
    setCyclesTextureTransform(&occlusionTrans, occlusionTransform);
    setCyclesTextureTransform(&emissiveTrans, emissiveTransform);
    setCyclesTextureTransform(&transmissionTrans, transmissionTransform);

    float volumeAttenuationColor[] = {
        1.0f,
        1.0f,
        1.0f };

    cycles_wrapper::Material cyclesMaterial;
    ret->cyclesHandle = _this->mInteractiveCycles.AddMaterial(
        cyclesScene,
        name.c_str(),
        albedoTex, albedoTrans, albedoColor.data(),
        metallicRoughnessTex, metallicRoughnessTrans, metallicFactor, roughnessFactor,
        normalTex, normalTrans, normalScale,
        emissiveTex, emissiveTrans, emissiveFactor.data(), emissiveStrength,
        0.0f,
        1.0f,
        volumeAttenuationColor, 0.0f, FLT_MAX);
    return ret;
}

void PathtracerEngine::renderOffline(const RenderJob& renderJob)
{
    _this->mRenderJobs.push_back(renderJob);
}

void PathtracerEngine::addNodeToCycles(Node* node)
{
    UPDATE_GUARD;
    auto castedNode = (PathtracerNode*)node;
    auto castedParent = dynamic_cast<PathtracerNode*>(node->belongsTo);
    auto handleParent = (castedParent != nullptr) ? (cycles_wrapper::Node*)castedParent->cyclesHandle : nullptr;

    float rotation[4] = {
        castedNode->localRotation.x(),
        castedNode->localRotation.y(),
        castedNode->localRotation.z(),
        castedNode->localRotation.w()
    };

    auto cyclesScene = _this->mInteractiveCycles.GetScene();
    castedNode->cyclesHandle = _this->mInteractiveCycles.AddNode(
        cyclesScene, node->name.c_str(),
        handleParent,
        castedNode->localPosition.data(),
        rotation,
        castedNode->localScale.data());

    if (node->mesh != nullptr)
    {
        auto castedMesh = (PathtracerMesh*)node->mesh;
        _this->mInteractiveCycles.AssignMeshToNode(
            cyclesScene,
            castedNode->cyclesHandle,
            castedMesh->cyclesHandle);
    }

    for (size_t i = 0; i < node->children.size(); i++)
    {
        addNodeToCycles(node->children[i]);
    }
}

void PathtracerEnginePrivate::renderOffline_threadWorker(const RenderJob& renderJob)
{
    mOfflineCycles = std::make_unique<cycles_wrapper::OfflineCycles>();
    mOfflineCycles->SetLogFunction(_this, PathtracerEnginePrivate::logFunction);
    mOfflineCycles->SessionInit();

    auto scene = dynamic_cast<const PathtracerScene*>(renderJob.scene);
    assert(scene);
    renderOffline_addScene(scene);




    auto& cam = renderJob.camera;
    //auto rs = camera->RenderSettings;
    uint width = renderJob.imageWidth;
    uint height = width * cam.getAspect();
    mOfflineCycles->Resize(width, height);
    switch (cam.getType())
    {
    case Camera::Type::Perspective:
    default:
    {
        auto pos = cam.getPosition();
        auto dir = cam.getDirection();
        auto up = cam.getUp();
        float p[] = { pos.x(), pos.y(), pos.z() };
        float d[] = { dir.x(), dir.y(), dir.z() };
        float u[] = { up.x(), up.y(), up.z() };
        float yfov = cam.getYFOV();
        mOfflineCycles->SetCamera(cycles_wrapper::CameraType::Perspective, p, d, u, yfov);
    }
    break;

    case Camera::Type::Panoramic:
    {
        auto pos = cam.getPosition();
        auto dir = cam.getUp();
        auto up = cam.getDirection();
        float p[] = { pos.x(), pos.y(), pos.z() };
        float d[] = { dir.x(), dir.y(), dir.z() };
        float u[] = { up.x(), up.y(), up.z() };
        mOfflineCycles->SetCamera(cycles_wrapper::CameraType::Panoramic, p, d, u);
    }
    break;
    }





    cycles_wrapper::BackgroundSettings bs;
    bs.mType = cycles_wrapper::BackgroundSettings::Type::Color;
    bs.mColor[0] = 0.0f;
    bs.mColor[1] = 0.0f;
    bs.mColor[2] = 0.0f;
    //bs.mSky.mSunDirection[0] = 1.0f;
    //bs.mSky.mSunDirection[1] = 0.0f;
    //bs.mSky.mSunDirection[2] = 0.0f;
    mOfflineCycles->SetSceneBackground(bs);
    //cycles_wrapper::DenoisingOptions denoising;
    //denoising.mEnable = rs->Denoise->Enable;
    //denoising.mPrefilter = rs->Denoise->Prefilter;
    //GetInstance()->SetDenoising(denoising);
    //for (int i = 0; i < params->Scene->Nodes->Length; i++)
    //{
    //    ApplyRenderMode(params->Scene->Nodes[i], cycles_wrapper::RenderMode::PBR);
    //}

    mOfflineCycles->SetSamples(renderJob.spp);
    mOfflineCycles->SetIsSingleChannelFloat(false);
    mOfflineCycles->PostSceneUpdate();

    mOfflineCycles->RenderScene(renderJob.outputFile.c_str(), false);



    // All done
    mOfflineCycles->SessionExit();
    mOfflineCycles.reset();
    mRenderJobThreadRunning = false;
}

void PathtracerEnginePrivate::renderOffline_addScene(const PathtracerScene* scene)
{
    for (size_t i = 0; i < scene->nodes.size(); i++)
    {
        auto node = (const PathtracerNode*)scene->nodes[i];
        renderOffline_addNode(node, nullptr);
    }
}

void PathtracerEnginePrivate::renderOffline_addNode(const PathtracerNode* node, cycles_wrapper::Node* parent)
{
    float rotation[4] = {
        node->localRotation.x(),
        node->localRotation.y(),
        node->localRotation.z(),
        node->localRotation.w()
    };
    
    auto offlineCyclesScene = mOfflineCycles->GetScene();
    auto cyclesHandle = mOfflineCycles->AddNode(
        offlineCyclesScene, node->name.c_str(),
        parent,
        node->localPosition.data(),
        rotation,
        node->localScale.data());
    
    if (node->mesh != nullptr)
    {
        auto mesh = (PathtracerMesh*)node->mesh;

        // Materials
        auto primitiveCount = (uint)mesh->materials.size();
        std::vector<cycles_wrapper::Material*> materials;
        materials.resize(primitiveCount);
        for (uint i = 0; i < primitiveCount; i++)
        {
            auto material = mesh->materials[i];
            cycles_wrapper::Texture* albedoTex = nullptr;
            if (material->albedoTexture.isValid())
            {
                albedoTex = mOfflineCycles->AddTexture(
                    offlineCyclesScene,
                    material->albedoTexture.name.c_str(),
                    ((PathtracerImage*)material->albedoTexture.image)->data.data(),
                    ((PathtracerImage*)material->albedoTexture.image)->dataSize,
                    ((PathtracerImage*)material->albedoTexture.image)->mimeType.c_str(),
                    !material->albedoTexture.inLinearSpace());
            }

            cycles_wrapper::Texture* metallicRoughnessTex = nullptr;
            if (material->metallicRoughnessTexture.isValid())
            {
                metallicRoughnessTex = mOfflineCycles->AddTexture(
                    offlineCyclesScene,
                    material->metallicRoughnessTexture.name.c_str(),
                    ((PathtracerImage*)material->metallicRoughnessTexture.image)->data.data(),
                    ((PathtracerImage*)material->metallicRoughnessTexture.image)->dataSize,
                    ((PathtracerImage*)material->metallicRoughnessTexture.image)->mimeType.c_str(),
                    !material->metallicRoughnessTexture.inLinearSpace());
            }

            cycles_wrapper::Texture* normalTex = nullptr;
            if (material->normalTexture.isValid())
            {
                normalTex = mOfflineCycles->AddTexture(
                    offlineCyclesScene,
                    material->normalTexture.name.c_str(),
                    ((PathtracerImage*)material->normalTexture.image)->data.data(),
                    ((PathtracerImage*)material->normalTexture.image)->dataSize,
                    ((PathtracerImage*)material->normalTexture.image)->mimeType.c_str(),
                    !material->normalTexture.inLinearSpace());
            }

            cycles_wrapper::Texture* emissiveTex = nullptr;
            if (material->emissiveTexture.isValid())
            {
                emissiveTex = mOfflineCycles->AddTexture(
                    offlineCyclesScene,
                    material->emissiveTexture.name.c_str(),
                    ((PathtracerImage*)material->emissiveTexture.image)->data.data(),
                    ((PathtracerImage*)material->emissiveTexture.image)->dataSize,
                    ((PathtracerImage*)material->emissiveTexture.image)->mimeType.c_str(),
                    !material->emissiveTexture.inLinearSpace());
            }

            cycles_wrapper::TextureTransform albedoTrans, metallicRoughnessTrans, normalTrans, occlusionTrans, emissiveTrans;
            setCyclesTextureTransform(&albedoTrans, material->albedoTransform);
            setCyclesTextureTransform(&metallicRoughnessTrans, material->metallicRoughnessTransform);
            setCyclesTextureTransform(&normalTrans, material->normalTransform);
            setCyclesTextureTransform(&occlusionTrans, material->occlusionTransform);
            setCyclesTextureTransform(&emissiveTrans, material->emissiveTransform);

            float volumeAttenuationColor[] = { 1.0f, 1.0f, 1.0f };

            materials[i] = mOfflineCycles->AddMaterial(
                offlineCyclesScene,
                material->name.c_str(),
                albedoTex, albedoTrans, material->albedoColor.data(),
                metallicRoughnessTex, metallicRoughnessTrans, material->metallicFactor, material->roughnessFactor,
                normalTex, normalTrans, material->normalScale,
                emissiveTex, emissiveTrans, material->emissiveFactor.data(), material->emissiveStrength,
                0.0f,
                1.0f,
                volumeAttenuationColor, 0.0f, FLT_MAX);
        }

        auto interactiveCyclesMesh = mesh->cyclesHandle;
        auto offlineCyclesMesh = mOfflineCycles->CopyMesh(offlineCyclesScene, materials.data(), interactiveCyclesMesh);
        mOfflineCycles->AssignMeshToNode(offlineCyclesScene, cyclesHandle, offlineCyclesMesh);
    }
    
    for (size_t i = 0; i < node->children.size(); i++)
    {
        auto child = (const PathtracerNode*)node->children[i];
        renderOffline_addNode(child, cyclesHandle);
    }
}