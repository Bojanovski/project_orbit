#pragma once
#include "GraphicsEngineTypes.h"
#include "GraphicsEngineInterface.h"


namespace graphics_engine
{
	class AnimationPlayer
	{
		struct AnimationTrack
		{
			std::string name;
			AnimationTarget* animationTarget;
			float elapsedTime;
			float loopTime;
			float blendTime;
			bool loop;
			bool pop;
			float timeLeft;
		};

	public:
		AnimationPlayer();

		void initialize(GraphicsEngineInterface* gei);
		void update(float);
		AnimationTarget* get(const std::string& name);
		void push(const std::string& name, float blendTime, bool loop);
		void pop();
		void popSlowly();
		std::string peek();

	public:
		float PlaybackSpeed = 1.0f;

	private:
		GraphicsEngineInterface* mGEI;

		std::map<std::string, AnimationTarget> mLibrary;


		std::vector<AnimationTrack> mStack;
	};
}
