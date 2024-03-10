#pragma once
#include "GameCoreTypes.h"


struct dtCrowdAgentParams;
struct dtCrowdAgent;

namespace game_core
{
	class Agent
	{
		friend class AgentManager;

	public:
		//Agent();
		//virtual ~Agent();

		////void initialize(NavigationMesh*);
		//void deinitialize();
		//void update(float dt);

		vec3 getPosition() const;
		void getCrowdParams(dtCrowdAgentParams*);

		static float maxRadius() { return 0.25f; }
		static float maxHeight() { return 1.8f; }
		static float maxClimb() { return 0.2f; }

	private:
		const dtCrowdAgent* mAgent = nullptr;
	};
}