#pragma once
#include "GameCoreTypes.h"


class rcContext;
class dtNavMeshQuery;
struct rcHeightfield;
struct rcCompactHeightfield;
struct rcContourSet;
struct rcPolyMesh;
struct rcPolyMeshDetail;
class dtNavMesh;
struct dtMeshTile;

namespace game_core
{
	class NavigationMesh
	{
	public:
		enum class PartitionType
		{
			Watershed,
			Monotone,
			Layers
		};

	public:
		NavigationMesh();
		virtual ~NavigationMesh();

		void initialize(const std::vector<graphics_engine::Node*>&);
		void deinitialize();

		dtNavMesh* getNavMeshObject() { return mNavMesh; }
		dtNavMeshQuery* getNavMeshQuery() { return mNavQuery; }
		void getTiles(std::vector<const dtMeshTile*>*) const;
		int getTileGeometryPolyCount(const dtMeshTile*) const;
		void getTileGeometry(
			const dtMeshTile*,
			int,
			std::vector<ushort>*,
			std::vector<vec3>*,
			std::vector<vec2>*,
			std::vector<vec3>*, 
			std::vector<vec3>*,
			bounds3*) const;

	private:
		void addNodeGeometry(const mat4x4&, graphics_engine::Node*);

	private:
		float mCellSize = 0.1f;
		float mCellHeight = 0.5f;
		float mEdgeMaxLen = 5.0f;
		float mEdgeMaxError = 1.0f;
		float mRegionMinSurfaceArea = 4.0f;
		float mDetailSampleDist = 3.0f;
		float mDetailSampleMaxError = 0.1f;
		bool mFilterLowHangingObstacles = true;
		bool mFilterLedgeSpans = true;
		bool mFilterWalkableLowHeightSpans = true;
		PartitionType mPartitionType = PartitionType::Watershed;

		std::unique_ptr<rcContext> mCtx;
		dtNavMeshQuery* mNavQuery = nullptr;
		rcHeightfield* mSolid = nullptr;
		rcCompactHeightfield* mChf = nullptr;
		rcContourSet* mCset = nullptr;
		rcPolyMesh* mMesh = nullptr;
		rcPolyMeshDetail* mDmesh = nullptr;
		dtNavMesh* mNavMesh = nullptr;

		std::vector<float> mVertices;
		std::vector<int> mTriangles;
		bounds3 mBounds;
	};
}