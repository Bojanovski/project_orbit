#pragma once
#include "GameObject.h"


namespace game_core
{
	class Prop : public GameObject
	{
	public:
		Prop();
		virtual ~Prop();

		virtual void initialize(graphics_engine::RasterizerEngineInterface*);
		virtual void update(float);
		virtual RayCastHit rayCast(const ray3&);

		void load();
		void unload();

	private:
		graphics_engine::Scene* mModelScene = nullptr;
		graphics_engine::Node* mImportedNode = nullptr;

		RigidBody mRB;
	};
}