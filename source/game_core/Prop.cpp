#include "Prop.h"


using namespace game_core;
using namespace graphics_engine;

Prop::Prop()
{
}

Prop::~Prop()
{
	unload();
}

void Prop::initialize(graphics_engine::RasterizerEngineInterface* gei)
{
	GameObject::initialize(gei);
}

void Prop::update(float dt)
{
	GameObject::update(dt);
}

RayCastHit Prop::rayCast(const ray3& ray)
{
    return mREI->rayCast(mImportedNode, ray);
}

void Prop::load()
{
    unload();
    // Load the mesh
    Model m;
    auto gi = GameInstance::getGameInstance();
    auto lp = LoadParameters{ gi->getAssetsRootDir() + L"\\test\\pair.glb" };
    mREI->loadModel(lp, &m);
    //mREI->loadModel(gi->getAssetsRootDir() + L"\\environment\\props\\martini_glass.glb", &scenes, nullptr);
    //mREI->loadModel(gi->getAssetsRootDir() + L"\\environment\\props\\regular_box.glb", &scenes, nullptr);
    //mREI->loadModel(gi->getAssetsRootDir() + L"\\test\\KHR_materials_transmission_0.glb", &scenes, nullptr);
    //mREI->loadModel(gi->getAssetsRootDir() + L"\\test\\modular_interior_0.glb", &scenes, nullptr);
    //mREI->loadModel(gi->getAssetsRootDir() + L"\\test\\platter.glb", &scenes, nullptr);
    //mREI->loadModel(gi->getAssetsRootDir() + L"\\test\\EmissiveStrengthTest_0.glb", &scenes, nullptr);
    mModelScene = m.scenes[0];
    auto mainScene = mREI->getActiveScene();
    mImportedNode = mREI->importScene(mainScene, mModelScene);
    auto pos = vec3(0.0f, 0.0f, 0.0f);
    mREI->updateNodeLocalTransform(mImportedNode, pos, mImportedNode->localScale, mImportedNode->localRotation);

    //mREI->loadModel(gi->getAssetsRootDir() + L"\\models\\modular_interior_01.glb", &scenes, nullptr);
    //auto importedNode = mREI->importScene(mainScene, scenes[0]);
    //pos = vec3(0.0f, 0.0f, 1.0f);
    //mREI->updateNodeLocalTransform(importedNode, pos, importedNode->localScale, importedNode->localRotation);

    // create the rigid body
    auto ps = gi->getPhysicsSimulator();
    float halfWidth = 0.5f;
    float halfHeight = 0.5f;
    float halfDepth = 0.5f;
    btVector3 btPos;
    convertVec3(&btPos, pos);
    mRB.shape = std::make_unique<btBoxShape>(btVector3(halfWidth, halfHeight, halfDepth));
    mRB.motionState = std::make_unique<btDefaultMotionState>(btTransform(btQuaternion(0, 0, 0, 1), btPos));
    btRigidBody::btRigidBodyConstructionInfo bodyCI(0, mRB.motionState.get(), mRB.shape.get(), btVector3(0, 0, 0));
    mRB.rigidBody = std::make_unique<btRigidBody>(bodyCI);
    mRB.rigidBody->setUserPointer(this);
    ps->addRigidBody(mRB.rigidBody.get());
}

void Prop::unload()
{
    if (mRB.rigidBody)
    {
        auto gi = GameInstance::getGameInstance();
        auto ps = gi->getPhysicsSimulator();
        ps->removeRigidBody(mRB.rigidBody.get());
    }
}
