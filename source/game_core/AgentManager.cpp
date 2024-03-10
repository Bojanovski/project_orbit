#include "AgentManager.h"
#include "Agent.h"
#include <qdebug.h>

#define RC_DISABLE_ASSERTS
#include <recastnavigation/DetourCrowd.h>


using namespace game_core;
using namespace graphics_engine;

AgentManager::AgentManager()
{

}

AgentManager::~AgentManager()
{
	deinitialize();
}

void AgentManager::initialize(NavigationMesh* navmesh)
{
	mNavMesh = navmesh;
	mCrowd = dtAllocCrowd();
	mCrowd->init(maxAgents(), Agent::maxRadius(), navmesh->getNavMeshObject());




	dtObstacleAvoidanceParams params;
	//float velBias;
	//float weightDesVel;
	//float weightCurVel;
	//float weightSide;
	//float weightToi;
	//float horizTime;
	//unsigned char gridSize;	///< grid
	//unsigned char adaptiveDivs;	///< adaptive
	//unsigned char adaptiveRings;	///< adaptive
	//unsigned char adaptiveDepth;	///< adaptive


	//mCrowd->setObstacleAvoidanceParams(0, &params);





	//mCrowd->qu(0, &params);
}

void AgentManager::deinitialize()
{
	if (mCrowd)
	{
		dtFreeCrowd(mCrowd);
		mCrowd = nullptr;
	}
}

void AgentManager::update(float dt)
{
	assert(mCrowd);
	dtCrowdAgentDebugInfo di;

	memset(&di, 0, sizeof(di));
	di.idx = -1;
	di.vod = nullptr;

	mCrowd->update(dt, &di);
}

void AgentManager::addAgent(Agent* agent)
{
	assert(mCrowd);
	dtCrowdAgentParams cap;
	agent->getCrowdParams(&cap);
	float pos[] = { -3.0f, 0.5f, 0.0f };
	int agentIndex = mCrowd->addAgent(pos, &cap);
	assert(agentIndex != -1);
	assert(agent->mAgent == nullptr);
	agent->mAgent = mCrowd->getAgent(agentIndex);


	float dest[] = { 3.0f, 0.5f, 0.0f };
	const dtQueryFilter* filter = mCrowd->getFilter(0);
	const float* halfExtents = mCrowd->getQueryExtents();


	float m_targetPos[3];
	dtPolyRef m_targetRef;


	mNavMesh->getNavMeshQuery()->findNearestPoly(dest, halfExtents, filter, &m_targetRef, m_targetPos);
	mCrowd->requestMoveTarget(agentIndex, m_targetRef, m_targetPos);
}
