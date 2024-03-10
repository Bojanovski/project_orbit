#pragma once
#include "GameObject.h"
#include "Agent.h"
#include "AnimationPlayer.h"


namespace game_core
{
	class Character : public GameObject, public Agent
	{
	public:
		Character();
		virtual ~Character();

		virtual void initialize(graphics_engine::RasterizerEngineInterface* gei);
		virtual void update(float);
		virtual RayCastHit rayCast(const ray3&);

		void load();
		void unload();
		void setRunning(bool running) { this->running = running; }

	private:
		float mTotalTime = 0.0f;
		graphics_engine::Scene* mModelScene = nullptr;
		graphics_engine::AnimationPlayer mAnimationPlayer;
		graphics_engine::Node* mImportedNode = nullptr;

		bool running = false;


		RigidBody mRB;
	};
}