#pragma once
#include "GameCoreTypes.h"
#include <NavigationMesh.h>


class dtCrowd;

namespace game_core
{
	class Agent;

	class AgentManager
	{
	public:
		AgentManager();
		virtual ~AgentManager();

		void initialize(NavigationMesh*);
		void deinitialize();
		void update(float dt);
		void addAgent(Agent* agent);

		static int maxAgents() { return 25; }

	private:
		NavigationMesh* mNavMesh = nullptr;
		dtCrowd* mCrowd = nullptr;
	};
}