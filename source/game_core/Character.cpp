#include "Character.h"


using namespace game_core;
using namespace graphics_engine;

Character::Character()
{

}

Character::~Character()
{
    unload();
}

void Character::initialize(RasterizerEngineInterface* gei)
{
    GameObject::initialize(gei);

    mAnimationPlayer.initialize(gei);
}

void Character::update(float dt)
{
    mTotalTime += dt;
    if (mImportedNode == nullptr) return;

    if (running && mAnimationPlayer.peek().compare("run") != 0)
    {
        mAnimationPlayer.push("run", 0.25f, true);
    }
    else if (!running && mAnimationPlayer.peek().compare("run") == 0)
    {
        mAnimationPlayer.popSlowly();
    }

    const MouseState& ms = mREI->getMouse();

    if (ms.rmb)
        mAnimationPlayer.update(dt);


    mImportedNode->children[2]->weights = { 0.5f + 0.5f * sin(mTotalTime) };
    mImportedNode->children[1]->weights = { 0.5f + 0.5f * sin(mTotalTime * 2.0f) };


    auto pos = getPosition();
    mREI->updateNodeLocalTransform(mImportedNode, pos, mImportedNode->localScale, mImportedNode->localRotation);
}

RayCastHit Character::rayCast(const ray3& ray)
{
    return mREI->rayCast(mImportedNode, ray);
}

void Character::load()
{
    unload();
    // Load the mesh
    Model m;
    auto gi = GameInstance::getGameInstance();
    auto lp = LoadParameters{ gi->getAssetsRootDir() + L"\\test\\morph_targets.glb" };
    //auto lp = LoadParameters{ gi->getAssetsRootDir() + L"\\people\\human_male.glb" };
    lp.allAnimations = true;
    mREI->loadModel(lp, &m);
    mModelScene = m.scenes[0];

    //// Load animations
    //lp.filepath = gi->getAssetsRootDir() + L"\\people\\animations\\male_idle_00.glb";
    //mREI->loadModel(lp, &m);
    //auto idle00AT = m.animationTargets[0];
    //lp.filepath = gi->getAssetsRootDir() + L"\\people\\animations\\male_walk.glb";
    //mREI->loadModel(lp, &m);
    //auto walkingAT = m.animationTargets[0];
    //lp.filepath = gi->getAssetsRootDir() + L"\\people\\animations\\male_run.glb";
    //mREI->loadModel(lp, &m);
    //auto runAT = m.animationTargets[0];

    auto mainScene = mREI->getActiveScene();
    mImportedNode = mREI->importScene(mainScene, mModelScene);

    mImportedNode->children[2]->weights = { 0.5f };
    mImportedNode->children[1]->weights = { 0.5f };

    auto pos = vec3(0.0f, 0.0f, 0.0f);
    mREI->updateNodeLocalTransform(mImportedNode, pos, mImportedNode->localScale, mImportedNode->localRotation);

    //mREI->retargetAnimation(mAnimationPlayer.get("idle_00"), mImportedNode, idle00AT);
    //mREI->retargetAnimation(mAnimationPlayer.get("walk"), mImportedNode, walkingAT);
    //mREI->retargetAnimation(mAnimationPlayer.get("run"), mImportedNode, runAT);
    //mAnimationPlayer.push("idle_00", 0.0f, true);


    // create the capsule rigid body
    auto ps = gi->getPhysicsSimulator();
    float radius = 0.25f;
    float totalHeight = 1.8f;
    float height = totalHeight - 2.0f * radius;
    btVector3 btPos;
    convertVec3(&btPos, pos + vec3(0.0f, totalHeight * 0.5f, 0.0f));
    mRB.shape = std::make_unique<btCapsuleShape>(radius, height);
    mRB.motionState = std::make_unique<btDefaultMotionState>(btTransform(btQuaternion(0, 0, 0, 1), btPos));
    btRigidBody::btRigidBodyConstructionInfo bodyCI(0, mRB.motionState.get(), mRB.shape.get(), btVector3(0, 0, 0));
    mRB.rigidBody = std::make_unique<btRigidBody>(bodyCI);
    mRB.rigidBody->setUserPointer(this);
    ps->addRigidBody(mRB.rigidBody.get());
}

void Character::unload()
{
    if (mRB.rigidBody)
    {
        auto gi = GameInstance::getGameInstance();
        auto ps = gi->getPhysicsSimulator();
        ps->removeRigidBody(mRB.rigidBody.get());
    }
}
