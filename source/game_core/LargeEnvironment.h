#pragma once
#include "EnvironmentBase.h"
#include <NavigationMesh.h>
#include <AgentManager.h>


namespace game_core
{
	class LargeEnvironment : public EnvironmentBase
	{
	private:
		struct Object
		{
			graphics_engine::Node* rootNodeRasterizer;
			graphics_engine::Node* rootNodePathtracer;
		};

	public:
		LargeEnvironment();
		virtual ~LargeEnvironment();

		virtual void initialize(graphics_engine::RasterizerEngineInterface*) override;
		virtual void initialize(graphics_engine::PathtracerEngineInterface*) override;
		virtual void update(float) override;

		void loadPlanet(const std::wstring&);
		void loadStation(const std::wstring&);

		graphics_engine::Scene* getBlueprintScene() { return mBlueprintScene; }

	private:
		graphics_engine::Scene *mBlueprintScene;
		Object mStation;

		//// Physics
		//std::unique_ptr<btCollisionShape> mGroundShape;
	};
}