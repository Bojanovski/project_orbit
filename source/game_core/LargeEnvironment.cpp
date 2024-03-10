#include "LargeEnvironment.h"


using namespace game_core;
using namespace graphics_engine;

LargeEnvironment::LargeEnvironment()
	: EnvironmentBase()
{

}

LargeEnvironment::~LargeEnvironment()
{

}

void LargeEnvironment::initialize(RasterizerEngineInterface* rei)
{
	EnvironmentBase::initialize(rei);
	mBlueprintScene = rei->createScene("large_environment_blueprint_scene");
}

void LargeEnvironment::initialize(PathtracerEngineInterface* pei)
{
	EnvironmentBase::initialize(pei);
}

void LargeEnvironment::update(float dt)
{
	EnvironmentBase::update(dt);

}

void LargeEnvironment::loadPlanet(const std::wstring& filepath)
{
	throw;
}

void LargeEnvironment::loadStation(const std::wstring& filepath)
{
	assert(mREI && mPEI);
	mRasterizer.enabled = true;
	mPathtracer.enabled = true;
	mPathtracer.loadAnimations = mRasterizer.loadAnimations = false;
	mPathtracer.loadAllVariants = mRasterizer.loadAllVariants = false;
	mRasterizer.variantFilter = { "blueprint" };
	//mPathtracer.variantFilter = { "white" };

	// Load the station blueprint
	Model rm;
	auto lp = LoadParameters{ filepath };
	mREI->loadModel(lp, &rm);
	assert(rm.scenes.size() == 1);
	// Import the blueprint to the main scene and load the required assets
	auto sceneRast = rm.scenes[0];
	mStation.rootNodeRasterizer = mREI->importScene(mBlueprintScene, sceneRast);
	mStation.rootNodeRasterizer->name = "object_root_node_" + std::to_string(123);
	loadLibraryAssets(mStation.rootNodeRasterizer, &mRasterizer);
	addLibraryAssets(mStation.rootNodeRasterizer, &mRasterizer);

	//// Now load the same mesh but for the pathtracer
	//auto pathtracerMainScene = mPEI->getActiveScene();
	//lp.preloadedFile = rm.loadedFile.get();
	//Model pm;
	//mPEI->loadModel(lp, &pm);
	//// Import the node
	//auto scenePathtracer = pm.scenes[0];
	//mStation.rootNodePathtracer = mPEI->importScene(pathtracerMainScene, scenePathtracer);
	//mStation.rootNodePathtracer->name = "object_root_node_" + std::to_string(123);
	//loadLibraryAssets(mStation.rootNodePathtracer, &mPathtracer);
	//addLibraryAssets(mStation.rootNodePathtracer, &mPathtracer);
}
