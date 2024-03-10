#pragma once
#include "GameCoreTypes.h"
#include "GameInstance.h"
#include <GraphicsEngineInterface.h>


namespace game_core
{
	class GameObject
	{
		friend class GameInstance;

	public:
		GameObject() {}
		virtual ~GameObject() = 0 {}

	protected:
		virtual void initialize(graphics_engine::RasterizerEngineInterface* rei) { mREI = rei; }
		virtual void initialize(graphics_engine::PathtracerEngineInterface* pei) { mPEI = pei; }
		virtual void update(float) {}
		virtual RayCastHit rayCast(const ray3&) { return RayCastHit(); };

	protected:
		graphics_engine::RasterizerEngineInterface* mREI = nullptr;
		graphics_engine::PathtracerEngineInterface* mPEI = nullptr;

	};
}
