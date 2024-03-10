#include "EnvironmentBase.h"
#include <set>
#include <sstream>
#include <string>
#include <iomanip>


using namespace game_core;
using namespace graphics_engine;

EnvironmentBase::EnvironmentBase()
	: GameObject()
{

}

EnvironmentBase::~EnvironmentBase()
{
}

void EnvironmentBase::initialize(graphics_engine::RasterizerEngineInterface* rei)
{
	GameObject::initialize(rei);
}

void EnvironmentBase::initialize(graphics_engine::PathtracerEngineInterface* pei)
{
	GameObject::initialize(pei);
}

void EnvironmentBase::update(float dt)
{
	GameObject::update(dt);
}

const Value* EnvironmentBase::getLibraryData(graphics_engine::NodeContainer* nc)
{
	assert(nc);
	auto node = dynamic_cast<graphics_engine::Node*>(nc);
	if (node)
	{
		assert(node->extra.has("project_orbit_data"));
		auto& poData = node->extra.get("project_orbit_data");
		return &poData.get("library_objects");
	}
	auto scene = dynamic_cast<graphics_engine::Scene*>(nc);
	if (scene)
	{
		assert(scene->extra.has("project_orbit_data"));
		auto& poData = scene->extra.get("project_orbit_data");
		return &poData.get("library_objects");
	}

	throw;
}

void EnvironmentBase::loadLibraryAsset(ulong id, Context* context)
{
	if (!context->enabled)
		return;

	if (context->idToLibraryAsset.find(id) != context->idToLibraryAsset.end())
		return; // already loaded

	auto gi = GameInstance::getGameInstance();
	std::wstringstream wss;
	wss << std::setw(8) << std::setfill(L'0') << id;
	std::wstring filepath = gi->getAssetsRootDir() + L"\\database\\" + wss.str() + L"\\asset.glb";
	auto lp = LoadParameters{ filepath };
	if (mLoadedFiles.find(filepath) != mLoadedFiles.end())
		lp.preloadedFile = mLoadedFiles[filepath];
	context->idToLibraryAsset[id] = LibraryAsset();
	auto retVal = &mRasterizer.idToLibraryAsset[id];
	lp.allAnimations = mRasterizer.loadAnimations;
	lp.allVariants = mRasterizer.loadAllVariants;
	lp.variantFilter = mRasterizer.variantFilter;
	mREI->loadModel(lp, &retVal->mModel);
	if (mLoadedFiles.find(filepath) == mLoadedFiles.end())
		mLoadedFiles[filepath] = retVal->mModel.loadedFile.get();
	retVal->buildDictionaries();
}

void EnvironmentBase::loadLibraryAssets(graphics_engine::NodeContainer* nc, Context* context)
{
	// Get the libraries to load
	auto libraryObjects = getLibraryData(nc);
	std::set<ulong> librariesToLoad;
	for (size_t i = 0; i < libraryObjects->arrayLen(); i++)
	{
		auto& libraryObject = libraryObjects->get(i);
		//auto& name = libraryObject.get("name").get<std::string>();
		auto& mesh = libraryObject.get("mesh");
		auto assetId = mesh.get("asset_id").get<int>();
		librariesToLoad.insert(assetId);
		if (libraryObject.has("library_materials_override"))
		{
			auto& matOverrides = libraryObject.get("library_materials_override");
			for (size_t j = 0; j < matOverrides.arrayLen(); j++)
			{
				auto mat = matOverrides.get(j);
				auto matAssetId = mat.get("asset_id").get<int>();
				librariesToLoad.insert(matAssetId);
			}
		}
	}

	// Iterate over the library set and load the library assets
	for (auto it = librariesToLoad.begin(); it != librariesToLoad.end(); ++it)
	{
		loadLibraryAsset(*it, context);
	}
}

void EnvironmentBase::addLibraryAssets(graphics_engine::NodeContainer* nc, Context* context)
{
	// Add objects from libraries
	auto libraryObjects = getLibraryData(nc);
	for (size_t i = 0; i < libraryObjects->arrayLen(); i++)
	{
		auto& libraryObject = libraryObjects->get(i);
		auto& name = libraryObject.get("name").get<std::string>();
		auto& mesh = libraryObject.get("mesh");
		auto& assetId = mesh.get("asset_id").get<int>();
		auto& libraryAsset = context->idToLibraryAsset[assetId];

		auto& nodeName = mesh.get("node_name").get<std::string>();
		auto nodeToImport = libraryAsset.mNameToNode[nodeName];

		auto& p = libraryObject.get("location");
		auto& r = libraryObject.get("rotation");
		auto& s = libraryObject.get("scale");
		vec3 pos((float)p.get(0).get<double>(), (float)p.get(1).get<double>(), (float)p.get(2).get<double>());
		quat rot((float)r.get(0).get<double>(), (float)r.get(1).get<double>(), (float)r.get(2).get<double>(), (float)r.get(3).get<double>());
		vec3 scale((float)s.get(0).get<double>(), (float)s.get(1).get<double>(), (float)s.get(2).get<double>());

		// Import the node and set the transform
		auto importedNode = mREI->importNode(nc, nodeToImport);
		importedNode->name = name;
		mREI->updateNodeLocalTransform(importedNode, pos, scale, rot);

		// Apply library material overrides if needed
		if (libraryObject.has("library_materials_override"))
		{
			assert(importedNode->mesh);
			auto& matOverrides = libraryObject.get("library_materials_override");
			std::vector<graphics_engine::Material*> replacementMaterials(matOverrides.arrayLen());
			for (size_t j = 0; j < matOverrides.arrayLen(); j++)
			{
				auto mat = matOverrides.get(j);
				auto originalName = mat.get("original_name").get<std::string>();
				auto replacementName = mat.get("replacement_name").get<std::string>();
				auto matAssetId = mat.get("asset_id").get<int>();
				if (originalName.compare(replacementName) != 0 || assetId != matAssetId)
				{
					// We need to replace this material
					auto &matLibraryAsset = context->idToLibraryAsset[matAssetId];
					auto replacementMaterial = matLibraryAsset.mNameToMaterial.find(replacementName)->second;
					replacementMaterials[j] = replacementMaterial;
				}
				else
				{
					auto originalMaterial = importedNode->mesh->materials[j];
					replacementMaterials[j] = originalMaterial;
				}
			}

			auto meshVariant = mREI->createMeshVariant(replacementMaterials, importedNode->mesh);
			importedNode->mesh = meshVariant;
		}
	}
}

void EnvironmentBase::LibraryAsset::buildDictionaries()
{
	mNameToNode.clear();
	mNameToMaterial.clear();
	for (size_t i = 0; i < mModel.scenes.size(); i++)
	{
		auto scene = mModel.scenes[i];
		for (size_t j = 0; j < scene->nodes.size(); j++)
		{
			buildDictionaries(scene->nodes[j]);
		}
	}
}

void EnvironmentBase::LibraryAsset::buildDictionaries(graphics_engine::Node* node)
{
	mNameToNode[node->name] = node;

	if (node->mesh)
	{
		for (size_t i = 0; i < node->mesh->materials.size(); i++)
		{
			auto& mat = node->mesh->materials[i];
			assert(!mat->name.empty());
			mNameToMaterial[mat->name] = mat;
		}
	}

	for (size_t i = 0; i < node->children.size(); i++)
	{
		buildDictionaries(node->children[i]);
	}
}
