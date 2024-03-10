#include <GameInstance.h>
#include <Tools.h>
#include <Environment.h>
#include <LargeEnvironment.h>
#include <Character.h>
#include <Prop.h>

#include <QOpenGLContext>
#include <QTimer>
#include <QDebug>


using namespace game_core;
using namespace graphics_engine;

namespace game_core
{
    class GameInstancePrivate
    {
    public:
        GameInstancePrivate(GameInstance *ptr)
            : _this(ptr)
        {

        }

    public:
        GameInstance * const _this;
        double mRunningTime = 0.0;
        QTimer mTimer;
        QElapsedTimer mElapsedTimer;
        RasterizerEngineInterface* mREI;
        PathtracerEngineInterface* mPEI;

        // Physics
        std::unique_ptr<btBroadphaseInterface> mBroadphase;
        std::unique_ptr<btDefaultCollisionConfiguration> mCollisionConfiguration;
        std::unique_ptr<btCollisionDispatcher> mDispatcher;
        std::unique_ptr<btSequentialImpulseConstraintSolver> mSolver;
        std::unique_ptr<btDiscreteDynamicsWorld> mDynamicsWorld;

        // Game objects
        std::vector<std::unique_ptr<GameObject>> mGameObjects;




        Environment* mEnvironment;
        LargeEnvironment* mLargeScaleEnvironment;

        Character* mHumanMale;
        Prop* mBox;

    };
}

GameInstance *gGameInstance = nullptr;

GameInstance *GameInstance::getGameInstance()
{
    return gGameInstance;
}

PhysicsSimulator* GameInstance::getPhysicsSimulator()
{
    return _this->mDynamicsWorld.get();
}

GameInstance::GameInstance(QObject *parent)
    : QObject(parent)
    , _this(nullptr)
{
    assert(gGameInstance == nullptr);
    gGameInstance = this;
}

GameInstance::~GameInstance()
{
    deinitialize();
}

void GameInstance::initialize()
{
    _this.reset();
    _this = std::make_unique<GameInstancePrivate>(this);

    _this->mTimer.setTimerType(Qt::TimerType::PreciseTimer);
    connect(&_this->mTimer, &QTimer::timeout, this, &GameInstance::update);
    _this->mTimer.start(16.666666);
    _this->mElapsedTimer.start();

    //
    // Physics
    // 
    _this->mBroadphase = std::make_unique<btDbvtBroadphase>();
    _this->mCollisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
    _this->mDispatcher = std::make_unique<btCollisionDispatcher>(_this->mCollisionConfiguration.get());
    _this->mSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
    _this->mDynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        _this->mDispatcher.get(),
        _this->mBroadphase.get(),
        _this->mSolver.get(),
        _this->mCollisionConfiguration.get());
    _this->mDynamicsWorld->setGravity(btVector3(0, -10, 0));

    // Game objects
    _this->mGameObjects.push_back(std::make_unique<Character>());
    _this->mHumanMale = (Character*)_this->mGameObjects.back().get();

    _this->mGameObjects.push_back(std::make_unique<Prop>());
    _this->mBox = (Prop*)_this->mGameObjects.back().get();

    _this->mGameObjects.push_back(std::make_unique<Environment>());
    _this->mEnvironment = (Environment*)_this->mGameObjects.back().get();

    _this->mGameObjects.push_back(std::make_unique<LargeEnvironment>());
    _this->mLargeScaleEnvironment = (LargeEnvironment*)_this->mGameObjects.back().get();
}

void GameInstance::deinitialize()
{
    _this->mTimer.stop();

    // Game objects
    _this->mGameObjects.clear();

    // Physics
    _this->mDynamicsWorld.reset();
    _this->mSolver.reset();
    _this->mCollisionConfiguration.reset();
    _this->mDispatcher.reset();
    _this->mBroadphase.reset();

    _this.reset();
}


Mesh* sphereMesh;


void GameInstance::setRasterizer(graphics_engine::RasterizerEngineInterface* gei)
{
    _this->mREI = gei;

    //auto platter = _this->mRasterizerInterface->loadScenes(getAssetsRootDir() + L"\\models\\platter.glb");
    //auto emScenes = _this->mRasterizerInterface->loadScenes(getAssetsRootDir() + L"\\models\\EmissiveStrengthTest_2.glb");
    auto scene = _this->mREI->createScene("main_scene");
    _this->mREI->setActiveScene(scene);

    Model m;
    auto lp = LoadParameters{ getAssetsRootDir() + L"\\test\\metal_sphere.glb" };
    _this->mREI->loadModel(lp, &m);
    auto sphereNode = m.scenes[0]->nodes[0];
    sphereMesh = sphereNode->mesh;

    // Game objects
    for (auto& go : _this->mGameObjects)
    {
        go->initialize(gei);
    }
}


bool goGo = true;

void GameInstance::setPathtracer(graphics_engine::PathtracerEngineInterface* gei)
{
    _this->mPEI = gei;

    //Model m;
    //auto lp = LoadParameters{ getAssetsRootDir() + L"\\test\\modular_interior_0.glb" };
    //_this->mPEI->loadModel(lp, &m);
    //auto scene = m.scenes[0];
    //auto& rootNodes = scenes[0]->nodes;
    //auto& childNodes = rootNodes[0]->children;
    //auto ceilingNode = childNodes[0];
    //auto floorNode = childNodes[1];
    //auto wallNode = childNodes[2];
    //auto mainScene = _this->mPathtracerInterface->createNewScene("MainScene");
    //for (int x = 0; x < 3; ++x)
    //{
    //    //for (int y = 0; y < 3; ++y)
    //    {
    //        for (int z = 0; z < 3; ++z)
    //        {
    //            vec3 pos = vec3(x - 1.0f, 0.0f, z - 1.0f) * 2.0f;
    //            auto ceiling = _this->mPathtracerInterface->createNewNode("CeilingNode", mainScene, pos, vec3::Ones(), quat::Identity(), ceilingNode->mesh);
    //            auto floor = _this->mPathtracerInterface->createNewNode("FloorNode", mainScene, pos, vec3::Ones(), quat::Identity(), floorNode->mesh);

    //            if (x == 0)
    //            {
    //                auto someNode = _this->mPathtracerInterface->createNewNode("WallNode", mainScene, pos, vec3::Ones(), quat::Identity(), wallNode->mesh);
    //            }

    //            if (x == 2)
    //            {
    //                quat rot(Eigen::AngleAxisf(-1.0f * M_PI_F, vec3::UnitY()));
    //                auto someNode = _this->mPathtracerInterface->createNewNode("WallNode", mainScene, pos, vec3::Ones(), rot, wallNode->mesh);
    //            }

    //            if (z == 0)
    //            {
    //                quat rot(Eigen::AngleAxisf(-0.5f * M_PI_F, vec3::UnitY()));
    //                auto someNode = _this->mPathtracerInterface->createNewNode("WallNode", mainScene, pos, vec3::Ones(), rot, wallNode->mesh);
    //            }

    //            if (z == 2)
    //            {
    //                quat rot(Eigen::AngleAxisf(0.5f * M_PI_F, vec3::UnitY()));
    //                auto someNode = _this->mPathtracerInterface->createNewNode("WallNode", mainScene, pos, vec3::Ones(), rot, wallNode->mesh);
    //            }
    //        }
    //    }
    //}
    //_this->mPEI->setActiveScene(scene);
    _this->mPEI->setCameraLookAt(-vec3(-0.5f, -0.1f, -1.0f) * 3.0f, -vec3(0.5f, -0.2f, 1.0f) * 100.0, vec3(0, 1, 0));



    //if (goGo)
    //{
    //    RenderJob rj;
    //    rj.camera = _this->mPathtracerInterface->getCamera();
    //    rj.camera.setPanoramic();
    //    rj.camera.setLookAt(vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f));
    //    rj.scene = mainScene;
    //    rj.imageWidth = 2048;
    //    rj.outputFile = "output_interior.exr";
    //    rj.spp = 30;
    //    _this->mPathtracerInterface->renderOffline(rj);
    //    goGo = false;
    //}

    // Game objects
    for (auto& go : _this->mGameObjects)
    {
        go->initialize(gei);
    }
}



//std::vector<Scene*> scenes;



void GameInstance::loadGame(const std::wstring&)
{
    // Rasterizer stuff
    ContextGuard cg(_this->mREI);
    _this->mLargeScaleEnvironment->loadStation(getAssetsRootDir() + L"\\environment\\space_stations\\STA_01.glb");
    _this->mEnvironment->loadSegment(getAssetsRootDir() + L"\\environment\\test.glb");
    _this->mHumanMale->load();

    _this->mEnvironment->addAgent(_this->mHumanMale);

    //_this->mBox->load();


    //scenes.clear();
    //Model m;
    //_this->mREI->loadModel(getAssetsRootDir() + L"\\test\\asset.glb", &m);
    //_this->mREI->loadModel(getAssetsRootDir() + L"\\database\\00000006\\asset.glb", &m);
    //_this->mREI->loadModel(getAssetsRootDir() + L"\\test\\KHR_materials_transmission_0.glb", &m);

    //scenes = m.scenes;

    emit segmentLoaded();
}

void onObjectVisualizationDone_callback(const RenderJob& job)
{
    auto w = (GameWidget*)job.userData;
    w->onObjectVisualizationDone(&job.outputImage);
}

void GameInstance::renderObjectVisualization(GameWidget* widget, uint w, uint h)
{
    RenderJob job;
    job.scene = _this->mLargeScaleEnvironment->getBlueprintScene();// _this->mREI->getActiveScene();
    job.camera = _this->mREI->getCamera();

    job.camera.setPerspective(30.0f, (float)w / (float)h);

    SceneTools st;
    st.mUseVertexDataForCompute = false;

    float np, fp;
    job.camera.getPlaneDistances(&np, &fp);
    vec3 dir = -vec3(1.0f, 1.0f, 1.0f).normalized();
    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    auto yfov = (float)degToRad(job.camera.getYFOV());
    auto xfov = (float)degToRad(job.camera.computeXFOV());
    vec3 camPos = st.computeEnclosingFrustumPosition(job.scene, dir, up, np, fp, xfov, yfov);
    job.camera.setLookAt(camPos, dir, up);
    job.camera.setPlaneDistances(0.1f, 100000.0f); // FIX THE DISTANCE

    job.imageWidth = w;
    job.imageHeight = h;
    job.userData = widget;
    job.outputType = RenderJob::OutputType::QImage;
    job.callback = onObjectVisualizationDone_callback;
    _this->mREI->renderOffline(job);
}

void GameInstance::onRasterizerMouseDoubleClick()
{
    if (_this->mREI)
    {
        const MouseState& ms = _this->mREI->getMouse();
        auto cursorPosNDC = vec2(ms.x, ms.y);
        auto& camera = _this->mREI->getCamera();
        auto camRay = camera.computeRayFromNDC(cursorPosNDC);
        btVector3 rayFrom, rayTo;
        convertVec3(&rayFrom, camRay.origin);
        convertVec3(&rayTo, camRay.origin + camRay.direction * 100.0f);
        btCollisionWorld::AllHitsRayResultCallback rayCallback(rayFrom, rayTo);
        _this->mDynamicsWorld->rayTest(rayFrom, rayTo, rayCallback);
        if (rayCallback.hasHit())
        {
            float closestDistance = FLT_MAX;
            for (int i = 0; i < rayCallback.m_collisionObjects.size(); i++)
            {
                auto go = (GameObject*)rayCallback.m_collisionObjects[i]->getUserPointer();
                auto drawGeometryHit = go->rayCast(camRay);
                if (drawGeometryHit && drawGeometryHit.distance < closestDistance)
                {
                    closestDistance = drawGeometryHit.distance;
                }
            }
            if (closestDistance < FLT_MAX)
            {
                vec3 pos = camRay.getPosition(closestDistance);
                auto mainScene = _this->mREI->getActiveScene();
                _this->mREI->createNode("mouse_doubleclick", mainScene, pos, vec3::Ones() * 0.02f, quat::Identity(), sphereMesh);
            }
        }
    }
}

MouseState gOldMS;
float gPolarAngle = (float)M_PI_2, gAzimuthalAngle = 0.0f, gRadius = 10.0f;

void GameInstance::update()
{
    auto elapsed_miliseconds = _this->mElapsedTimer.restart();
    float delta_time = elapsed_miliseconds / 1000.0f;
    _this->mRunningTime += delta_time;
    float stable_delta_time = std::min(0.03333f, delta_time);

    if (_this->mREI)
    {
        const MouseState& ms = _this->mREI->getMouse();
        float diffX = ms.x - gOldMS.x;
        float diffY = ms.y - gOldMS.y;
        if (ms.lmb)
        {
            gPolarAngle     += diffY * 1.1f;
            gAzimuthalAngle += diffX * 1.1f;
        }
        gPolarAngle = std::clamp(gPolarAngle, 0.0001f, (float)(M_PI - 0.0001));
        int diffWheel = ms.wheel - gOldMS.wheel;
        gRadius -= diffWheel * 0.005f;
        if (gRadius < 0.0f) gRadius = 0.0f;
        gOldMS = ms;

        float x = gRadius * sin(gPolarAngle) * cos(gAzimuthalAngle);
        float y = gRadius * cos(gPolarAngle);
        float z = gRadius * sin(gPolarAngle) * sin(gAzimuthalAngle);
        auto camPos = vec3(x, y, z);
        auto camTar = vec3::Zero();
        _this->mREI->setCameraLookAt(camPos, (camTar - camPos).normalized(), vec3(0.0f, 1.0f, 0.0f));

    _this->mHumanMale->setRunning(true);
    }



    // Game objects
    for (auto& go : _this->mGameObjects)
    {
        go->update(stable_delta_time);
    }
}
