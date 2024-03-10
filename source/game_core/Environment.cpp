#include "Environment.h"
#include <GraphicsEngineInterface.h>


using namespace game_core;
using namespace graphics_engine;

Environment::Environment()
	: EnvironmentBase()
{
	mSegmentIdCounter = 0;
	mGroundShape = std::make_unique<btStaticPlaneShape>(btVector3(0, 1, 0), 0);
}

Environment::~Environment()
{
	unloadSegments();
}

void Environment::initialize(graphics_engine::RasterizerEngineInterface* rei)
{
	EnvironmentBase::initialize(rei);

#ifdef QT_DEBUG
	std::vector<color> navMeshColors = {
		color(0.2f, 0.2f, 0.2f, 1.0f),
		color(0.2f, 0.2f, 1.0f, 1.0f),
		color(0.2f, 1.0f, 0.2f, 1.0f),
		color(0.2f, 1.0f, 1.0f, 1.0f),
		color(1.0f, 0.2f, 0.2f, 1.0f),
		color(1.0f, 0.2f, 1.0f, 1.0f),
		color(1.0f, 1.0f, 0.2f, 1.0f),
		color(1.0f, 1.0f, 1.0f, 1.0f),
	};
	mNavMeshWalkableMaterials.resize(navMeshColors.size());
	for (size_t i = 0; i < navMeshColors.size(); i++)
	{
		mNavMeshWalkableMaterials[i] = mREI->createMaterial(
			"navmesh_walkable_" + std::to_string(i),
			color(0.0f, 0.0f, 0.0f, 0.5f), Texture(),
			0.0f, 1.0f, Texture(),
			1.0f, Texture(),
			navMeshColors[i], Texture(),
			false, 1.0f,
			false, 0.0f, Texture(),
			false,
			TextureTransform(),
			TextureTransform(),
			TextureTransform(),
			TextureTransform(),
			TextureTransform(),
			TextureTransform());
	}
#endif

	auto gi = GameInstance::getGameInstance();
	std::vector<Scene*> scenes;
	//mREI->loadModel(gi->getAssetsRootDir() + L"\\environment\\test.glb", &scenes, nullptr);
	//mFloor = scenes[0];
	//mREI->loadModel(gi->getAssetsRootDir() + L"\\environment\\hallway_wall_00.glb", &scenes, nullptr);
	//mWall = scenes[0];
	//mREI->loadModel(gi->getAssetsRootDir() + L"\\environment\\hallway_ceiling_00.glb", &scenes, nullptr);
	//mCeiling = scenes[0];
}

void Environment::update(float dt)
{
	EnvironmentBase::update(dt);
	for (auto& segment : mSegments)
	{
		segment.mAgentManager->update(dt);
	}
}

void Environment::loadSegment(const std::wstring& filepath)
{
	unloadSegments(); // unload all for now !!!!!!!!!!

	assert(mREI);
	mSegments.push_back(Segment());
	auto& newSegment = mSegments.back();
	newSegment.id = mSegmentIdCounter++;
	Model m;
	auto lp = LoadParameters{ filepath };
	mREI->loadModel(lp, &m);
	assert(m.scenes.size() == 1);

	// Import the blueprint to the main scene and load the required assets
	auto scene = m.scenes[0];
	auto mainScene = mREI->getActiveScene();
	newSegment.rootNode = mREI->importScene(mainScene, scene);
	newSegment.rootNode->name = "segment_root_node_" + std::to_string(newSegment.id);
	loadLibraryAssets(newSegment.rootNode, &mRasterizer);
	addLibraryAssets(newSegment.rootNode, &mRasterizer);
	newSegment.worldTransform = newSegment.rootNode->computeWorldTransform().matrix();

	// Create a ground plane
	auto gi = GameInstance::getGameInstance();
	auto ps = gi->getPhysicsSimulator();
	newSegment.mGroundMotionState = std::make_unique<btDefaultMotionState>(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, newSegment.mGroundMotionState.get(), mGroundShape.get(), btVector3(0, 0, 0));
	newSegment.mGroundRigidBody = std::make_unique<btRigidBody>(groundRigidBodyCI);
	newSegment.mGroundRigidBody->setUserPointer(this);
	ps->addRigidBody(newSegment.mGroundRigidBody.get());

	// Build the navigation mesh
	newSegment.mNavMesh = std::make_unique<NavigationMesh>();
	newSegment.mNavMesh->initialize({ newSegment.rootNode });

	// Build the navigation mesh for drawing
#ifdef QT_DEBUG
	std::vector<const dtMeshTile*> tiles;
	newSegment.mNavMesh->getTiles(&tiles);
	int primitiveCount = 0;
	for (size_t i = 0; i < tiles.size(); i++)
	{
		auto tile = tiles[i];
		primitiveCount += newSegment.mNavMesh->getTileGeometryPolyCount(tile);
	}

	std::vector<std::vector<ushort>> tilesIndices(primitiveCount);
	std::vector<std::vector<vec3>> tilesVertices(primitiveCount);
	std::vector<std::vector<vec2>> tilesUVs(primitiveCount);
	std::vector<std::vector<vec3>> tilesNormals(primitiveCount);
	std::vector<std::vector<vec3>> tilesTangents(primitiveCount);

	std::vector<Material*> materials(primitiveCount);
	std::vector<Mesh::Buffer> indexBuffers(primitiveCount);
	std::vector<Mesh::Buffer> positionBuffers(primitiveCount);
	std::vector<Mesh::Buffer> uvBuffers(primitiveCount);
	std::vector<Mesh::Buffer> normalBuffers(primitiveCount);
	std::vector<Mesh::Buffer> tangentBuffers(primitiveCount);
	std::vector<Mesh::Buffer> jointBuffers; // ignore
	std::vector<Mesh::Buffer> weightBuffers; // ignore
	std::vector<Mesh::MorphTargets> morphTargets; // ignore
	std::vector<bounds3> primitiveBounds(primitiveCount);
	auto lvlInverse = newSegment.worldTransform.inverse();

	int pi = 0; // primitive index
	for (size_t i = 0; i < tiles.size(); i++)
	{
		auto tile = tiles[i];
		auto polyCount = newSegment.mNavMesh->getTileGeometryPolyCount(tile);
		for (size_t j = 0; j < polyCount; j++)
		{
			newSegment.mNavMesh->getTileGeometry(tile, j, &tilesIndices[pi], &tilesVertices[pi], &tilesUVs[pi], &tilesNormals[pi], &tilesTangents[pi], &primitiveBounds[pi]);

			// move from global space to level-node relative space
			auto vertexCount = tilesVertices[pi].size();
			assert(tilesVertices[pi].size() == vertexCount);
			assert(tilesUVs[pi].size() == vertexCount);
			assert(tilesNormals[pi].size() == vertexCount);
			assert(tilesTangents[pi].size() == vertexCount);
			for (size_t k = 0; k < vertexCount; k++)
			{
				tilesVertices[pi][k] = (lvlInverse * tilesVertices[pi][k].homogeneous()).head<3>();
				tilesNormals[pi][k] = lvlInverse.block<3, 3>(0, 0) * tilesNormals[pi][k];
				tilesTangents[pi][k] = lvlInverse.block<3, 3>(0, 0) * tilesTangents[pi][k];
			}

			// material
			materials[pi] = mNavMeshWalkableMaterials[pi % mNavMeshWalkableMaterials.size()];

			// index
			indexBuffers[pi].data = (uchar*)tilesIndices[pi].data();
			indexBuffers[pi].count = (uint)tilesIndices[pi].size();
			indexBuffers[pi].stride = (int)(sizeof(ushort));
			indexBuffers[pi].componentSize = (int)(sizeof(ushort));
			indexBuffers[pi].componentType = TYPE_UNSIGNED_SHORT;

			// position
			positionBuffers[pi].data = (uchar*)tilesVertices[pi].data();
			positionBuffers[pi].count = (uint)tilesVertices[pi].size();
			positionBuffers[pi].stride = (int)(sizeof(float) * 3);
			positionBuffers[pi].componentSize = (int)sizeof(float);
			positionBuffers[pi].componentType = TYPE_FLOAT;

			// uv
			uvBuffers[pi].data = (uchar*)tilesUVs[pi].data();
			uvBuffers[pi].count = (uint)tilesUVs[pi].size();
			uvBuffers[pi].stride = (int)(sizeof(float) * 2);
			uvBuffers[pi].componentSize = (int)sizeof(float);
			uvBuffers[pi].componentType = TYPE_FLOAT;

			// normal
			normalBuffers[pi].data = (uchar*)tilesNormals[pi].data();
			normalBuffers[pi].count = (uint)tilesNormals[pi].size();
			normalBuffers[pi].stride = (int)(sizeof(float) * 3);
			normalBuffers[pi].componentSize = (int)sizeof(float);
			normalBuffers[pi].componentType = TYPE_FLOAT;

			// tangent
			tangentBuffers[pi].data = (uchar*)tilesTangents[pi].data();
			tangentBuffers[pi].count = (uint)tilesTangents[pi].size();
			tangentBuffers[pi].stride = (int)(sizeof(float) * 3);
			tangentBuffers[pi].componentSize = (int)sizeof(float);
			tangentBuffers[pi].componentType = TYPE_FLOAT;

			// increase the index
			++pi;
		}
	}

	mNavMeshWalkableMesh = mREI->createMesh(
		"segment_navmesh_" + std::to_string(newSegment.id),
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
	mNavMeshWalkableNode = mREI->createNode(
		"segment_navmesh_" + std::to_string(newSegment.id),
		newSegment.rootNode,
		vec3::Zero(), vec3::Ones(), quat::Identity(),
		mNavMeshWalkableMesh,
		nullptr);
#endif

	// Initialize the agent manager (dtCrowd)
	newSegment.mAgentManager = std::make_unique<AgentManager>();
	newSegment.mAgentManager->initialize(newSegment.mNavMesh.get());
}

void Environment::unloadSegments()
{
	auto gi = GameInstance::getGameInstance();
	auto ps = gi->getPhysicsSimulator();
	for (size_t i = 0; i < mSegments.size(); i++)
	{
		ps->removeRigidBody(mSegments[i].mGroundRigidBody.get());
	}
	mSegments.clear();
}

void Environment::addAgent(Agent* agent)
{
	assert(mSegments.size() == 1); // needs to be just one (for now)
	mSegments[0].mAgentManager->addAgent(agent);
}
