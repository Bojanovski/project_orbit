#include "Agent.h"
#include "AgentManager.h"
#include "Agent.h"
#include <qdebug.h>

#define RC_DISABLE_ASSERTS
#include <recastnavigation/DetourCrowd.h>


using namespace game_core;
using namespace graphics_engine;

vec3 Agent::getPosition() const
{
	return vec3(mAgent->npos[0], mAgent->npos[1], mAgent->npos[2]);
}

void Agent::getCrowdParams(dtCrowdAgentParams* params)
{
	params->radius = Agent::maxRadius();
	params->height = Agent::maxHeight();
	params->maxAcceleration = 1.0f;
	params->maxSpeed = 0.5f;

	params->collisionQueryRange = 3.0f;
	params->pathOptimizationRange = 1.0f;
	params->separationWeight = 1.0f;
	params->updateFlags =
		UpdateFlags::DT_CROWD_ANTICIPATE_TURNS |
		UpdateFlags::DT_CROWD_OBSTACLE_AVOIDANCE |
		UpdateFlags::DT_CROWD_SEPARATION;
	params->obstacleAvoidanceType = 0;


	/// The index of the query filter used by this agent.
	//params->queryFilterType = 


	params->userData = this;
}
