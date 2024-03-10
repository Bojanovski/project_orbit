#include <RasterizerEngine.h>
#include <RasterizerEngineWidget.h>
#include <QApplication>
#include <QOpenGLContext>
#include <QTimer>
#include <QDebug>
#include <QtMath>
#include <Eigen/Dense>
#include <Eigen/SVD>
#include <unordered_map>

#include "RenderTarget.h"
#include "UserImage.h"
#include "CubeMap.h"
#include "BRDF_LUT.h"
#include "FullScreenQuad.h"
#include "GBuffer.h"
#include "ImageShaderPrograms.h"
#include "ImageDownscaler.h"
#include "BlurShaderProgram.h"
#include "MeshShaderProgram.h"
#include "PBRShaderProgram.h"
#include "FinalizePBRShaderProgram.h"
#include "GLTFModelLoader.h"
#include "SceneManager.h"
#include "RasterizerMesh.h"
#include "Camera.h"

#include <atomic>
#include <thread>
#include <mutex>


namespace graphics_engine
{
    struct RasterizerMaterial : public Material
    {

    };

    struct RasterizerSkin : public Skin
    {

    };

    struct RasterizerNode : public Node
    {
        transform worldTransform;
    };

    struct DrawContext
    {
        Scene* scene;
        mat4x4 view;
        mat4x4 projection;
        transform world;
        transform skinningTransform;
        MeshShaderProgramBase* currentSP;
        uint ssUpscaleFactor;
        uint seed;
        const std::vector<float>* morphTargetWeights;
    };

    struct AsyncModelLoader
    {
        bool isUsed = false;
        std::atomic<bool> threadDone = true;
        std::thread worker;
        LoadParameters parameters;
        GLTFModelLoader loader;
        std::unique_ptr<GLTFModelLoader::Model> model;
        LoadModelCallback callbackPtr = nullptr;
    };

    class DrawBuffers
    {
    public:
        virtual void initialize(uint, uint, uint);
        virtual void resize(uint, uint, uint);

    public:
        uint mSSUpscaleFactor;
        std::unique_ptr<GBuffer> mGBuffer;
        std::unique_ptr<RenderTarget> mSSAOBuffer;
        std::unique_ptr<ImageDownscaler> mEmissiveBufferDownscaler;
        std::unique_ptr<RenderTarget> mPreBlurEmissiveBuffer;
        std::unique_ptr<RenderTarget> mHBlurredEmissiveBuffer;
        std::unique_ptr<RenderTarget> mVBlurredEmissiveBuffer;
        std::unique_ptr<RenderTarget> mPBRBuffer;
        std::unique_ptr<ImageDownscaler> mPBRBufferDownscaler;
    };

    class RenderJobBuffers : public DrawBuffers
    {
    public:
        virtual void initialize(uint, uint, uint);
        virtual void resize(uint, uint, uint);
    
    public:
        std::unique_ptr<RenderTarget> mFinal;
    };

    class RasterizerEnginePrivate
    {
    public:
        RasterizerEnginePrivate(RasterizerEngine* ptr);

        void initialize(RasterizerEngineWidget* container);
        void draw(const Camera& cam, Scene* scene, DrawBuffers* buffers);
        void prepareForDrawing(const Scene* obj);
        void prepareForDrawing(const Node* obj);
        class MeshShaderProgramBase* getShaderProgram(const Material&, const Mesh&);
        void drawScene(const Scene*);
        void drawNode(const Node*);
        void drawStaticMesh(const Mesh*);
        void drawSkinnedMesh(const Mesh*, const Skin*);

    public:
        const uint mSSUpscaleFactor = 2;
        RasterizerEngine* const _this;

        RasterizerEngineWidget* mContainer;
        QOpenGLContext* mContext;

        // Scene management
        GLTFModelLoader mModelLoader;
        SceneManager mSceneManager;
        Scene* mCurrentlyActiveScene;
        Camera mViewportCamera;
        const static int mAsyncLoaderCount = 16;
        AsyncModelLoader mAsyncLoaders[mAsyncLoaderCount];

        // Rendering
        DrawContext mDC;
        BRDF_LUT mBRDF_LUT;
        FullScreenQuad mFullScreenQuad;
        DrawBuffers mDrawBuffers;

        // Offline rendering
        std::mutex mRenderJobsMutex;
        std::vector<RenderJob> mRenderJobs;

        //ImageTexture mTestTexture;
        EnvironmentMap mCubeTexture;
        IrradianceMap mIrradianceMap;
        PrefilterMap mPrefilterMap;

        // Shader programs
        std::map<MeshShaderParameters, std::unique_ptr<StaticMeshShaderProgram>> mStaticMeshSPs;
        std::map<MeshShaderParameters, std::unique_ptr<SkinnedMeshShaderProgram>> mSkinnedMeshSPs;
        SSAOShaderProgram mSSAOSP;
        TextureOperationProgram mEmissiveBlurPreparationSP;
        BlurShaderProgram mHorizontalBlurSP;
        BlurShaderProgram mVerticalBlurSP;
        PBRShaderProgram mPBRSP;
        FinalizePBRShaderProgram mFinalizePBRSP;

        GLuint mViewportWidth;
        GLuint mViewportHeight;
        GLuint mViewportBaseColor[3] = {0, 0, 0};
    };
}

using namespace graphics_engine;

void DrawBuffers::initialize(uint width, uint height, uint ssUpscaleFactor)
{
    mSSUpscaleFactor            = ssUpscaleFactor;
    mGBuffer                    = std::make_unique<GBuffer>();
    mSSAOBuffer                 = std::make_unique<RenderTarget>();
    mEmissiveBufferDownscaler   = std::make_unique<ImageDownscaler>(ssUpscaleFactor * 2); // emissive factor (empirical)
    mPreBlurEmissiveBuffer      = std::make_unique<RenderTarget>();
    mHBlurredEmissiveBuffer     = std::make_unique<RenderTarget>();
    mVBlurredEmissiveBuffer     = std::make_unique<RenderTarget>();
    mPBRBuffer                  = std::make_unique<RenderTarget>();
    mPBRBufferDownscaler        = std::make_unique<ImageDownscaler>(ssUpscaleFactor);

    uint gw = width * ssUpscaleFactor, gh = height * ssUpscaleFactor;
    mGBuffer->create(gw, gh);
    mSSAOBuffer->create(gw, gh, { RenderTarget::TextureDescriptor{ GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE }, });
    mPBRBuffer->create(gw, gh, { RenderTarget::TextureDescriptor{ GL_RGBA16F, GL_RGBA, GL_FLOAT }, });
    mEmissiveBufferDownscaler->create(gw, gh);
    mPBRBufferDownscaler->create(gw, gh);
    uint ew = gw / mEmissiveBufferDownscaler->getFactor(), eh = gh / mEmissiveBufferDownscaler->getFactor();
    mPreBlurEmissiveBuffer->create(ew, eh, { RenderTarget::TextureDescriptor{ GL_RGB16F, GL_RGB, GL_FLOAT }, });
    mHBlurredEmissiveBuffer->create(ew, eh, { RenderTarget::TextureDescriptor{ GL_RGB16F, GL_RGB, GL_FLOAT }, });
    mVBlurredEmissiveBuffer->create(ew, eh, { RenderTarget::TextureDescriptor{ GL_RGB16F, GL_RGB, GL_FLOAT }, });

}

void DrawBuffers::resize(uint width, uint height, uint ssUpscaleFactor)
{
    uint gw = width * ssUpscaleFactor, gh = height * ssUpscaleFactor;
    mGBuffer->resize(gw, gh);
    mSSAOBuffer->resize(gw, gh);
    mPBRBuffer->resize(gw, gh);
    mEmissiveBufferDownscaler->resize(gw, gh);
    mPBRBufferDownscaler->resize(gw, gh);
    uint ew = gw / mEmissiveBufferDownscaler->getFactor(), eh = gh / mEmissiveBufferDownscaler->getFactor();
    mPreBlurEmissiveBuffer->resize(ew, eh);
    mHBlurredEmissiveBuffer->resize(ew, eh);
    mVBlurredEmissiveBuffer->resize(ew, eh);
}

void RenderJobBuffers::initialize(uint width, uint height, uint ssUpscaleFactor)
{
    DrawBuffers::initialize(width, height, ssUpscaleFactor);
    mFinal = std::make_unique<RenderTarget>();
    mFinal->create(width, height, { RenderTarget::TextureDescriptor{ GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE }, });
}

void RenderJobBuffers::resize(uint width, uint height, uint ssUpscaleFactor)
{
    DrawBuffers::resize(width, height, ssUpscaleFactor);
    mFinal->resize(width, height);
}

RasterizerEnginePrivate::RasterizerEnginePrivate(RasterizerEngine* ptr)
    : _this(ptr)
    , mEmissiveBlurPreparationSP({
        {TextureOperationProgram::Operation::Type::Sub, vec4(1.0f, 1.0f, 1.0f, 0.0f)},
        {TextureOperationProgram::Operation::Type::Max, vec4(0.0f, 0.0f, 0.0f, 0.0f)}
        })
    , mHorizontalBlurSP(10, BlurShaderProgram::BlurType::GaussianHorizontal, 5.0f)
    , mVerticalBlurSP(10, BlurShaderProgram::BlurType::GaussianVertical, 5.0f)
{

}

void RasterizerEnginePrivate::initialize(RasterizerEngineWidget* container)
{
    mContainer = container;
    mContext = container->context();

    mCurrentlyActiveScene = nullptr;
    mSceneManager.create(_this);
    uint w = 512, h = 512;
    mDrawBuffers.initialize(w, h, mSSUpscaleFactor);
    mBRDF_LUT.create(512);
    mFullScreenQuad.create();
    mSSAOSP.create();
    mEmissiveBlurPreparationSP.create();
    mHorizontalBlurSP.create();
    mVerticalBlurSP.create();
    mPBRSP.create();
    mFinalizePBRSP.create();
    mViewportCamera.setPerspective(30.0f, 1.0f);
    mViewportCamera.setLookAt(
        vec3(0.0f, 50.0f, -50.0f),
        vec3(0.0f, -1.0f, 1.0f).normalized(),
        vec3(0.0f, 1.0f, 0.0f));



    //_this->mTestTexture.createFromFileHDR(L"C:\\Projects\\project_orbit\\assets\\textures\\interior_hdr.exr");
    mCubeTexture.createFromFileEquirectangularHDR(512, L"C:\\Projects\\project_orbit\\assets\\textures\\interior_hdr.exr");
    mIrradianceMap.createFromEnvironmentMap(32, &mCubeTexture);
    mPrefilterMap.createFromEnvironmentMap(128, &mCubeTexture);
}

void RasterizerEnginePrivate::draw(const Camera& cam, Scene* scene, DrawBuffers* buffers)
{
    mDC.view = cam.getView().matrix();
    mDC.projection = cam.getProjection().matrix();
    mDC.scene = scene;
    mDC.currentSP = nullptr;
    mDC.ssUpscaleFactor = buffers->mSSUpscaleFactor;

    if (mDC.scene != nullptr)
    {
        prepareForDrawing(mDC.scene);
    }

    // Draw the scene
    buffers->mGBuffer->beginUse();
    //glClearColor(
    //            _this->mViewportBaseColor[0] / 255.0f,
    //            _this->mViewportBaseColor[1] / 255.0f,
    //            _this->mViewportBaseColor[2] / 255.0f,
    //            1.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW); // default
    if (mDC.scene != nullptr)
    {
        drawScene(mDC.scene);
    }
    buffers->mGBuffer->endUse();

    // Compute SSAO
    buffers->mSSAOBuffer->beginUse();
    mSSAOSP.bind();
    mSSAOSP.setTransforms(mDC.projection, mDC.view);
    mSSAOSP.setGBuffer(buffers->mGBuffer.get());
    mFullScreenQuad.draw();
    mSSAOSP.unbind();
    buffers->mSSAOBuffer->endUse();

    // Downscale Emissive
    buffers->mEmissiveBufferDownscaler->downscale(buffers->mGBuffer.get(), (int)GBuffer::BufferType::Emissive);

    // Preparation for blur Emissive
    buffers->mPreBlurEmissiveBuffer->beginUse();
    mEmissiveBlurPreparationSP.bind();
    mEmissiveBlurPreparationSP.setTextureSource(buffers->mEmissiveBufferDownscaler->getResult());
    mFullScreenQuad.draw();
    mEmissiveBlurPreparationSP.unbind();
    buffers->mPreBlurEmissiveBuffer->endUse();

    // Horizontal blur Emissive
    buffers->mHBlurredEmissiveBuffer->beginUse();
    mHorizontalBlurSP.bind();
    mHorizontalBlurSP.setTextureSource(buffers->mPreBlurEmissiveBuffer.get());
    mFullScreenQuad.draw();
    mHorizontalBlurSP.unbind();
    buffers->mHBlurredEmissiveBuffer->endUse();

    // Vertical blur Emissive
    buffers->mVBlurredEmissiveBuffer->beginUse();
    mVerticalBlurSP.bind();
    mVerticalBlurSP.setTextureSource(buffers->mHBlurredEmissiveBuffer.get());
    mFullScreenQuad.draw();
    mVerticalBlurSP.unbind();
    buffers->mVBlurredEmissiveBuffer->endUse();

    // PBR compute
    buffers->mPBRBuffer->beginUse();
    mPBRSP.bind();
    mPBRSP.setTransforms(mDC.projection, mDC.view);
    mPBRSP.setGBuffer(buffers->mGBuffer.get());
    mPBRSP.setSSAO(buffers->mSSAOBuffer.get());
    mPBRSP.setIBL(&mIrradianceMap, &mPrefilterMap, &mBRDF_LUT);
    mFullScreenQuad.draw();
    mPBRSP.unbind();
    buffers->mPBRBuffer->endUse();

    // Downscale PBR
    buffers->mPBRBufferDownscaler->downscale(buffers->mPBRBuffer.get());

    // Draw the full screen quad to the target or the back buffer
    auto rjb = dynamic_cast<RenderJobBuffers*>(buffers);
    if (rjb == nullptr)
    {
        // Draw to the back buffer
        glViewport(0, 0, mViewportWidth, mViewportHeight);
        mFinalizePBRSP.bind();
        mFinalizePBRSP.setPBR_BlurredEmissive(buffers->mPBRBufferDownscaler->getResult(), buffers->mVBlurredEmissiveBuffer.get());
        mFullScreenQuad.draw();
        mFinalizePBRSP.unbind();
    }
    else
    {
        // Draw to the target
        rjb->mFinal->beginUse();
        mFinalizePBRSP.bind();
        //glEnable(GL_BLEND);
        mFinalizePBRSP.setPBR_BlurredEmissive(rjb->mPBRBufferDownscaler->getResult(), rjb->mVBlurredEmissiveBuffer.get());
        mFullScreenQuad.draw();
        mFinalizePBRSP.unbind();
        rjb->mFinal->endUse();
    }
}

void RasterizerEnginePrivate::prepareForDrawing(const Scene* obj)
{
    for (int i = 0; i < (int)obj->nodes.size(); ++i)
    {
        mDC.world = transform::Identity();
        mDC.skinningTransform = transform::Identity();
        prepareForDrawing(obj->nodes[i]);
    }
}
void RasterizerEnginePrivate::prepareForDrawing(const Node* obj)
{
    auto castedObj = (RasterizerNode*)obj;
    transform oldWorldTransform = mDC.world;
    mDC.world = mDC.world * obj->getLocalTransform();
    castedObj->worldTransform = mDC.world;

    for (int i = 0; i < (int)obj->children.size(); ++i)
    {
        prepareForDrawing(obj->children[i]);
    }

    mDC.world = oldWorldTransform;
}

MeshShaderProgramBase* RasterizerEnginePrivate::getShaderProgram(const Material& material, const Mesh& mesh)
{
    MeshShaderParameters params;
    params.morphTargetCount = mesh.morphTargetCount;
    params.coverageKernelDim = mDC.ssUpscaleFactor;
    params.materialFlags = 0;
    if (material.albedoTexture.isValid())               params.materialFlags |= MAT_ALBEDO_TEX;
    if (material.metallicRoughnessTexture.isValid())    params.materialFlags |= MAT_METALLIC_ROUGHNESS_TEX;
    if (material.normalTexture.isValid())               params.materialFlags |= MAT_NORMAL_TEX;
    if (material.emissiveTexture.isValid())             params.materialFlags |= MAT_EMISSIVE_TEX;
    //if (material.occlusionTexture.isSet())            params.materialFlags |= MAT_OCCLUSION_TEX;

    if (mesh.skinned)
    {
        if (mSkinnedMeshSPs.find(params) == mSkinnedMeshSPs.end())
        {
            mSkinnedMeshSPs[params] = std::make_unique<SkinnedMeshShaderProgram>(params);
            mSkinnedMeshSPs[params]->create();
        }
        return mSkinnedMeshSPs[params].get();
    }
    else
    {
        if (mStaticMeshSPs.find(params) == mStaticMeshSPs.end())
        {
            mStaticMeshSPs[params] = std::make_unique<StaticMeshShaderProgram>(params);
            mStaticMeshSPs[params]->create();
        }
        return mStaticMeshSPs[params].get();
    }
}

void RasterizerEnginePrivate::drawScene(const Scene* obj)
{
    for (int i = 0; i < (int)obj->nodes.size(); ++i)
    {
        mDC.world = transform::Identity();
        drawNode(obj->nodes[i]);
    }
}

void RasterizerEnginePrivate::drawNode(const Node* obj)
{
    auto castedObj = (RasterizerNode*)obj;
    mDC.seed = (uint)reinterpret_cast<uintptr_t>(obj);
    mDC.world = castedObj->worldTransform;
    mDC.morphTargetWeights = &obj->weights;

    if (obj->mesh != nullptr)
    {
        if (obj->skin != nullptr)   drawSkinnedMesh(obj->mesh, obj->skin);
        else                        drawStaticMesh(obj->mesh);
    }
    for (int i = 0; i < (int)obj->children.size(); ++i)
    {
        drawNode(obj->children[i]);
    }
}

void RasterizerEnginePrivate::drawStaticMesh(const Mesh* obj)
{
    auto castedObj = (RasterizerMeshBase*)obj;
    assert(!castedObj->skinned);
    for (uint j = 0; j < (uint)castedObj->materials.size(); j++)
    {
        auto material = castedObj->materials[j];
        if (castedObj->mCachedShaderProgram == nullptr)
            castedObj->mCachedShaderProgram = getShaderProgram(*material, *castedObj);
        auto shader = castedObj->mCachedShaderProgram;
        if (mDC.currentSP != shader)
        {
            shader->bind();
            shader->setCameraTransforms(mDC.view, mDC.projection);
            mDC.currentSP = shader;
        }
        shader->setMorphTargetWeights(*mDC.morphTargetWeights);
        shader->setSeed(mDC.seed + j);
        shader->setMaterial(*material);
        auto castedShader = (StaticMeshShaderProgram*)shader;
        castedShader->setMeshTransform(mDC.world);
        castedObj->draw(j);
    }
}

void RasterizerEnginePrivate::drawSkinnedMesh(const Mesh* obj, const Skin* skin)
{
    auto castedObj = (RasterizerMeshBase*)obj;
    assert(castedObj->skinned);
    for (uint j = 0; j < (uint)castedObj->materials.size(); j++)
    {
        auto material = castedObj->materials[j];
        if (castedObj->mCachedShaderProgram == nullptr)
            castedObj->mCachedShaderProgram = getShaderProgram(*material, *castedObj);
        auto shader = castedObj->mCachedShaderProgram;
        if (mDC.currentSP != shader)
        {
            shader->bind();
            shader->setCameraTransforms(mDC.view, mDC.projection);
            mDC.currentSP = shader;
        }
        shader->setMorphTargetWeights(*mDC.morphTargetWeights);
        shader->setSeed(mDC.seed + j);
        shader->setMaterial(*material);
        auto boneCount = skin->inverseBindMatrices.size();
        std::vector<mat4x4> bones(boneCount);
        for (size_t i = 0; i < boneCount; i++)
        {
            auto joint = (RasterizerNode*)skin->joints[i];
            bones[i] = joint->worldTransform.matrix() * skin->inverseBindMatrices[i];
        }
        auto castedShader = (SkinnedMeshShaderProgram*)shader;
        castedShader->setBones(bones);
        castedObj->draw(j);
    }
}

RasterizerEngine::RasterizerEngine()
    : _this(nullptr)
{
    mMS.x = 0;
    mMS.y = 0;
    mMS.wheel = 0;
    mMS.lmb = false;
    mMS.rmb = false;
    mMS.inside = false;
}

RasterizerEngine::~RasterizerEngine()
{

}

void RasterizerEngine::initialize(RasterizerEngineWidget* container)
{
    // Make sure we are using seamless cube map sampling for various cube map creation shaders
    // as well as for the rendering pass
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    _this.reset();
    _this = std::make_unique<RasterizerEnginePrivate>(this);
    _this->initialize(container);
}

void RasterizerEngine::deinitialize()
{
    for (int i = 0; i < _this->mAsyncLoaderCount; i++)
    {
        if (_this->mAsyncLoaders[i].worker.joinable())
            _this->mAsyncLoaders[i].worker.join();
    }
    _this.reset();
}

void RasterizerEngine::setBaseColor(unsigned int red, unsigned int green, unsigned int blue)
{
    _this->mViewportBaseColor[0] = red;
    _this->mViewportBaseColor[1] = green;
    _this->mViewportBaseColor[2] = blue;
}

const Camera& RasterizerEngine::getCamera()
{
    return _this->mViewportCamera;
}

void RasterizerEngine::setCameraLookAt(const vec3& pos, const vec3& dir, const vec3& up)
{
    _this->mViewportCamera.setLookAt(pos, dir, up);
}

RayCastHit RasterizerEngine::rayCast(NodeContainer* container, const ray3& ray)
{
    return _this->mSceneManager.rayCast(container, ray);
}

Scene* RasterizerEngine::createScene(const std::string& name)
{
    _this->mSceneManager.mScenes.push_back(std::make_unique<Scene>());
    auto ret = _this->mSceneManager.mScenes.back().get();
    ret->name = name;
    return ret;
}

Node* RasterizerEngine::importScene(NodeContainer* container, Scene* src)
{
    auto destNode = createNode(src->name, container, vec3::Zero(), vec3::Ones(), quat::Identity(), nullptr, nullptr);
    destNode->extra = src->extra;
    for (size_t i = 0; i < src->nodes.size(); i++)
    {
        importNode(destNode, src->nodes[i]);
    }
    return destNode;
}

Node* RasterizerEngine::importNode(NodeContainer* container, Node* src)
{
    auto importedNode = _this->mSceneManager.importNode(container, src);
    _this->mSceneManager.mapNodeToNode(importedNode, src);
    _this->mSceneManager.remapImportedSkins(importedNode, src);
    return importedNode;
}

Node* RasterizerEngine::createNode(
    const std::string &name, 
    NodeContainer* belongsTo,
    const vec3& p, const vec3& s, const quat& r, 
    Mesh* mesh,
    Skin* skin)
{
    _this->mSceneManager.mNodes.push_back(std::make_unique<RasterizerNode>());
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
    ret->skin = skin;
    //ret->localPosition = t.translation();
    //t.computeRotationScaling(&ret->localRotation, &ret->localScale);
    return ret;
}

void RasterizerEngine::updateNodeLocalTransform(Node* target, const vec3& position, const vec3& scale, const quat& rotation)
{
    target->localPosition = position;
    target->localScale = scale;
    target->localRotation = rotation;
}

bool RasterizerEngine::loadModel(const LoadParameters& params, Model* model)
{
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

void RasterizerEngine::loadModel_Async(const LoadParameters& params, LoadModelCallback callbackPtr)
{
    int freeLoaderIndex = -1;
    while (freeLoaderIndex == -1)
    {
        for (size_t i = 0; i < _this->mAsyncLoaderCount; i++)
        {
            if (_this->mAsyncLoaders[i].isUsed == false)
            {
                freeLoaderIndex = (int)i;
                break;
            }
        }
    }
    auto& freeLoader = _this->mAsyncLoaders[freeLoaderIndex];
    freeLoader.callbackPtr = callbackPtr;
    freeLoader.isUsed = true;
    freeLoader.threadDone = false;
    freeLoader.parameters = params;
    freeLoader.worker = std::thread([&freeLoader]() {
        freeLoader.model = freeLoader.loader.LoadModel(freeLoader.parameters.filepath.c_str());
        freeLoader.threadDone = true;
        });
}

void RasterizerEngine::setActiveScene(Scene* scene)
{
    _this->mCurrentlyActiveScene = scene;
}

Scene* RasterizerEngine::getActiveScene()
{
    return _this->mCurrentlyActiveScene;
}

Skin* RasterizerEngine::createSkin(const std::vector<mat4x4>& inverseBindMatrices)
{
    _this->mSceneManager.mSkins.push_back(std::make_unique<RasterizerSkin>());
    auto ret = (RasterizerSkin*)_this->mSceneManager.mSkins.back().get();
    auto jointsCount = inverseBindMatrices.size();
    ret->inverseBindMatrices = inverseBindMatrices;
    ret->joints.resize(jointsCount);
    for (size_t i = 0; i < jointsCount; i++)
    {
        ret->joints[i] = nullptr;
    }
    ret->skeleton = nullptr;
    return ret;
}

Mesh* RasterizerEngine::createMesh(
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
    _this->mSceneManager.mMeshes.push_back(std::make_unique<RasterizerMesh>());
    auto ret = (RasterizerMesh*)_this->mSceneManager.mMeshes.back().get();
    ret->name = name;
    ret->materials = materials;
    *ret->getBounds() = submeshBounds;
    ret->skinned = !jointBuffers.empty();
    ret->morphTargetCount = (morphTargets.size() > 0) ? (uint)morphTargets[0].posDiffBuffers.size() : 0;
    ret->create(positionBuffers, uvBuffers, normalBuffers, tangentBuffers, jointBuffers, weightBuffers, indexBuffers, morphTargets);
    return ret;
}

Mesh* RasterizerEngine::createMeshVariant(const std::vector<Material*>& materials, Mesh* src)
{
    _this->mSceneManager.mMeshes.push_back(std::make_unique<RasterizerMeshProxy>());
    auto ret = (RasterizerMeshProxy*)_this->mSceneManager.mMeshes.back().get();
    ret->name = src->name;
    ret->materials = materials;
    ret->skinned = src->skinned;
    auto castedSrc = dynamic_cast<RasterizerMesh*>(src);
    assert(castedSrc);
    ret->create(castedSrc);
    return ret;
}

Image* RasterizerEngine::createImage(
    const std::string& name,
    const uchar* data, 
    size_t dataSize,
    uint width,
    uint height, 
    const std::string& mimeType,
    bool isLinearSpace)
{
    (void)width;
    (void)height;
    _this->mSceneManager.mImages.push_back(std::make_unique<UserImage>());
    auto ret = (UserImage*)_this->mSceneManager.mImages.back().get();
    ret->name = name;
    ret->mLinearSpace = isLinearSpace;
    ret->createFromMemory(data, dataSize, mimeType);
    return ret;
}

Material* RasterizerEngine::createMaterial(
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
    _this->mSceneManager.mMaterials.push_back(std::make_unique<RasterizerMaterial>());
    auto ret = (RasterizerMaterial*)_this->mSceneManager.mMaterials.back().get();
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
    return ret;
}

void RasterizerEngine::retargetAnimation(AnimationTarget* atDest, Node* target, const AnimationTarget& atSrc)
{
    return _this->mSceneManager.retargetAnimation(atDest, target, atSrc);
}

void RasterizerEngine::setAnimationTime(const AnimationTarget& animationTarget, float time, float blend)
{
    _this->mSceneManager.setAnimationTime(animationTarget, time, blend);
}

void RasterizerEngine::renderOffline(const RenderJob& renderJob)
{
    std::lock_guard<std::mutex> lock(_this->mRenderJobsMutex);
    _this->mRenderJobs.push_back(renderJob);
}

const MouseState& RasterizerEngine::getMouse()
{
    return mMS;
}

void RasterizerEngine::enableContext()
{
    //bool isEnabled = _this->mContext->isValid() && _this->mContext == _this->mContainer->context();
    _this->mContainer->makeCurrent();
}

void RasterizerEngine::disableContext()
{
    _this->mContainer->doneCurrent();
}

bool RasterizerEngine::checkForErrors()
{
    bool noError = true;
    GLenum error = GL_NO_ERROR;
    do {
        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            const char* errorMessage = nullptr;
            switch (error) {
                case GL_INVALID_ENUM:
                    errorMessage = "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument.";
                    break;
                case GL_INVALID_VALUE:
                    errorMessage = "GL_INVALID_VALUE: A numeric argument is out of range.";
                    break;
                case GL_INVALID_OPERATION:
                    errorMessage = "GL_INVALID_OPERATION: The specified operation is not allowed in the current state.";
                    break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:
                    errorMessage = "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete.";
                    break;
                case GL_OUT_OF_MEMORY:
                    errorMessage = "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command.";
                    break;
                default:
                    errorMessage = "Unknown OpenGL Error";
                    break;
            }
            qWarning() << "Error: " << errorMessage;
            noError = false;
        }
    } while (error != GL_NO_ERROR);
    return noError;
}

void RasterizerEngine::resize(int w, int h)
{
    _this->mViewportWidth = w;
    _this->mViewportHeight = h;
    _this->mDrawBuffers.resize(w, h, _this->mSSUpscaleFactor);

    _this->mViewportCamera.setPerspective(30.0f, (float)_this->mViewportWidth / _this->mViewportHeight);
}

void RasterizerEngine::update(float)
{
    // Check if some of the loaders are done and the callbacks need to be called
    for (int i = 0; i < _this->mAsyncLoaderCount; i++)
    {
        auto& loader = _this->mAsyncLoaders[i];
        if (loader.isUsed == true && loader.threadDone == true)
        {
            loader.worker.join();
            loader.isUsed = false;
            if (loader.model == nullptr)
            {
                // Failed to load
                loader.callbackPtr(nullptr);
            }
            else
            {
                // Finish loading and send to the GPU
                auto& params = loader.parameters;
                Model model;
                std::vector<Scene*> scenes;
                std::vector<AnimationTarget> animationTargets;
                _this->mSceneManager.addModel(loader.model.get(), params.allAnimations, params.allVariants, params.variantFilter, &model);
                loader.callbackPtr(&model);
            }
        }
    }
}

void RasterizerEngine::draw()
{
    // Draw to the backbuffer
    _this->draw(_this->mViewportCamera, _this->mCurrentlyActiveScene, &_this->mDrawBuffers);

    // Run any render job that is in the queue
    if (!_this->mRenderJobs.empty())
    {
        std::lock_guard<std::mutex> lock(_this->mRenderJobsMutex);
        auto job = _this->mRenderJobs.back();
        _this->mRenderJobs.pop_back();
        RenderJobBuffers jobBuffers;
        jobBuffers.initialize(job.imageWidth, job.imageHeight, 4);
        _this->draw(job.camera, job.scene, &jobBuffers);

        switch (job.outputType)
        {
        case RenderJob::OutputType::File:
        {
            auto img = jobBuffers.mFinal->getImage(0);
            img.save(job.outputFile.c_str());
            if (job.callback)
                job.callback(job);
        }
            break;

        case RenderJob::OutputType::QImage:
        {
            job.outputImage = jobBuffers.mFinal->getImage(0);
            if (job.callback) 
                job.callback(job);
        }
        break;

        default:
            throw;
            break;
        }
    }

    checkForErrors();
}
