#pragma once
#include "GameObject.h"


namespace game_core
{
	class EnvironmentBase : public GameObject
	{
	protected:
		struct LibraryAsset
		{
			void buildDictionaries();
			void buildDictionaries(graphics_engine::Node*);

			graphics_engine::Model mModel;
			std::map<std::string, graphics_engine::Node*> mNameToNode;
			std::map<std::string, graphics_engine::Material*> mNameToMaterial;
		};

		struct Context
		{
			bool enabled = true;
			bool loadAnimations = false;
			bool loadAllVariants = false;
			std::set<std::string> variantFilter;
			std::map<ulong, LibraryAsset> idToLibraryAsset;
		};

	public:
		EnvironmentBase();
		virtual ~EnvironmentBase();

		virtual void initialize(graphics_engine::RasterizerEngineInterface*) override;
		virtual void initialize(graphics_engine::PathtracerEngineInterface*) override;
		virtual void update(float) override;

	protected:
		const graphics_engine::Value* getLibraryData(graphics_engine::NodeContainer*);
		void loadLibraryAsset(ulong, Context*);
		void loadLibraryAssets(graphics_engine::NodeContainer*, Context*);
		void addLibraryAssets(graphics_engine::NodeContainer*, Context*);

	protected:
		//std::vector<graphics_engine::Material*> mNavMeshWalkableMaterials;

		//// Physics
		//std::unique_ptr<btCollisionShape> mGroundShape;

		// Library Asset Database contexts
		Context mRasterizer, mPathtracer;
		std::map<std::wstring, graphics_engine::LoadedFile*> mLoadedFiles;
	};
}