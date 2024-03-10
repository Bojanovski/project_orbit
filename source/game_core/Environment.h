#pragma once
#include "EnvironmentBase.h"
#include <NavigationMesh.h>
#include <AgentManager.h>


namespace game_core
{
	class Environment : public EnvironmentBase
	{
	private:
		struct Segment
		{
			uint id;
			graphics_engine::Node* rootNode;
			mat4x4 worldTransform;

			// Physics
			std::unique_ptr<btDefaultMotionState> mGroundMotionState;
			std::unique_ptr<btRigidBody> mGroundRigidBody;
			
			// Pathfinding
			std::unique_ptr<NavigationMesh> mNavMesh;
			std::unique_ptr<AgentManager> mAgentManager;
		};

	public:
		Environment();
		virtual ~Environment();

		virtual void initialize(graphics_engine::RasterizerEngineInterface*) override;
		virtual void update(float) override;

		void loadSegment(const std::wstring&);
		void unloadSegments();
		void addAgent(Agent*);

	private:
		std::vector<Segment> mSegments;
		uint mSegmentIdCounter;
		std::vector<graphics_engine::Material*> mNavMeshWalkableMaterials;
		graphics_engine::Mesh* mNavMeshWalkableMesh = nullptr;
		graphics_engine::Node* mNavMeshWalkableNode = nullptr;

		// Physics
		std::unique_ptr<btCollisionShape> mGroundShape;
	};
}