#include "NavigationMesh.h"
#include "Agent.h"
#include <qdebug.h>

#define RC_DISABLE_ASSERTS
#include <recastnavigation/Recast.h>
#include <recastnavigation/DetourNavMesh.h>
#include <recastnavigation/DetourNavMeshBuilder.h>
#include <recastnavigation/DetourNavMeshQuery.h>
#include <recastnavigation/DetourTileCacheBuilder.h>
#include <recastnavigation/DetourTileCache.h>


using namespace game_core;
using namespace graphics_engine;

enum SamplePolyAreas
{
	SAMPLE_POLYAREA_GROUND,
	SAMPLE_POLYAREA_WATER,
	SAMPLE_POLYAREA_ROAD,
	SAMPLE_POLYAREA_DOOR,
	SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP
};

enum SamplePolyFlags
{
	SAMPLE_POLYFLAGS_WALK = 0x01,		// Ability to walk (ground, grass, road)
	SAMPLE_POLYFLAGS_SWIM = 0x02,		// Ability to swim (water).
	SAMPLE_POLYFLAGS_DOOR = 0x04,		// Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP = 0x08,		// Ability to jump.
	SAMPLE_POLYFLAGS_DISABLED = 0x10,	// Disabled polygon
	SAMPLE_POLYFLAGS_ALL = 0xffff		// All abilities.
};

class BuildContext : public rcContext
{
public:
	BuildContext() 
		: rcContext(false)
	{
		enableLog(true);
	}

	virtual ~BuildContext()
	{

	}

protected:
	//virtual void doResetLog() {}
	virtual void doLog(const rcLogCategory, const char* msg, const int) override
	{
		qDebug() << msg;
	}
};

static const int MAX_CONVEXVOL_PTS = 12;
struct ConvexVolume
{
	float verts[MAX_CONVEXVOL_PTS * 3];
	float hmin, hmax;
	int nverts;
	int area;
};

NavigationMesh::NavigationMesh()
{
	mCtx = std::make_unique<BuildContext>();
}

NavigationMesh::~NavigationMesh()
{
	deinitialize();
}

void NavigationMesh::initialize(const std::vector<Node*>& nodes)
{
	deinitialize();
	
	// Compute the geometry data
	for (size_t i = 0; i < nodes.size(); i++)
	{
		mat4x4 nodeWorldTransform = nodes[i]->computeWorldTransform().matrix();
		addNodeGeometry(nodeWorldTransform, nodes[i]);
	}

	// Step 1. Initialize build config
	rcConfig m_cfg;
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = mCellSize;
	m_cfg.ch = mCellHeight;
	m_cfg.walkableSlopeAngle = 30.0;
	m_cfg.walkableHeight = (int)ceilf(Agent::maxHeight() / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(Agent::maxClimb() / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(Agent::maxRadius() / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(mEdgeMaxLen / mCellSize);
	m_cfg.maxSimplificationError = mEdgeMaxError;
	m_cfg.minRegionArea = (int)rcSqr(mRegionMinSurfaceArea);
	m_cfg.mergeRegionArea = (int)rcSqr(mRegionMinSurfaceArea);
	m_cfg.maxVertsPerPoly = 6;
	m_cfg.detailSampleDist = mDetailSampleDist < 0.9f ? 0.0f : mCellSize * mDetailSampleDist;
	m_cfg.detailSampleMaxError = mCellHeight * mDetailSampleMaxError;
	rcVcopy(m_cfg.bmin, mBounds.min.data());
	rcVcopy(m_cfg.bmax, mBounds.max.data());
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	// Step 2. Rasterize input polygon soup
	mSolid = rcAllocHeightfield();
	assert(mSolid);
	bool isOk = rcCreateHeightfield(mCtx.get(), *mSolid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch);
	assert(isOk);
	// Allocate array that can hold triangle area types.
	auto vertexCount = (int)(mVertices.size() / 3);
	auto triangleCount = (int)(mTriangles.size() / 3);
	std::vector<uchar> triangleAreas(triangleCount);
	// Find triangles which are walkable based on their slope and rasterize them.
	memset(triangleAreas.data(), 0, triangleCount * sizeof(uchar));
	rcMarkWalkableTriangles(
		mCtx.get(), m_cfg.walkableSlopeAngle,
		mVertices.data(), vertexCount,
		mTriangles.data(), triangleCount,
		triangleAreas.data());
	isOk = rcRasterizeTriangles(
		mCtx.get(),
		mVertices.data(), vertexCount,
		mTriangles.data(), triangleAreas.data(), triangleCount,
		*mSolid, m_cfg.walkableClimb);
	assert(isOk);

	// Step 3. Filter walkable surfaces.
	if (mFilterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(mCtx.get(), m_cfg.walkableClimb, *mSolid);
	if (mFilterLedgeSpans)
		rcFilterLedgeSpans(mCtx.get(), m_cfg.walkableHeight, m_cfg.walkableClimb, *mSolid);
	if (mFilterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(mCtx.get(), m_cfg.walkableHeight, *mSolid);

	// Step 4. Partition walkable surface to simple regions.
	mChf = rcAllocCompactHeightfield();
	assert(mChf);
	isOk = rcBuildCompactHeightfield(mCtx.get(), m_cfg.walkableHeight, m_cfg.walkableClimb, *mSolid, *mChf);
	assert(isOk);
	// we no longer need the old data
	rcFreeHeightField(mSolid);
	mSolid = nullptr;
	// Erode the walkable area by agent radius.
	isOk = rcErodeWalkableArea(mCtx.get(), m_cfg.walkableRadius, *mChf);
	assert(isOk);

	//// (Optional) Mark areas.
	//const ConvexVolume* vols = m_geom->getConvexVolumes();
	//for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
	//	rcMarkConvexPolyArea(mCtx.get(), vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *mChf);

	// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
	// There are 3 partitioning methods, each with some pros and cons:
	// 1) Watershed partitioning
	//   - the classic Recast partitioning
	//   - creates the nicest tessellation
	//   - usually slowest
	//   - partitions the heightfield into nice regions without holes or overlaps
	//   - the are some corner cases where this method creates produces holes and overlaps
	//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
	//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
	//   * generally the best choice if you precompute the navmesh, use this if you have large open areas
	// 2) Monotone partitioning
	//   - fastest
	//   - partitions the heightfield into regions without holes and overlaps (guaranteed)
	//   - creates long thin polygons, which sometimes causes paths with detours
	//   * use this if you want fast navmesh generation
	// 3) Layer partitoining
	//   - quite fast
	//   - partitions the heighfield into non-overlapping regions
	//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
	//   - produces better triangles than monotone partitioning
	//   - does not have the corner cases of watershed partitioning
	//   - can be slow and create a bit ugly tessellation (still better than monotone)
	//     if you have large open areas with small obstacles (not a problem if you use tiles)
	//   * good choice to use for tiled navmesh with medium and small sized tiles
	switch (mPartitionType)
	{
	case PartitionType::Watershed:
		isOk = rcBuildDistanceField(mCtx.get(), *mChf);
		assert(isOk);
		isOk = rcBuildRegions(mCtx.get(), *mChf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea);
		assert(isOk);
		break;
	case PartitionType::Monotone:
		isOk = rcBuildRegionsMonotone(mCtx.get(), *mChf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea);
		assert(isOk);
		break;
	case PartitionType::Layers:
	default:
		isOk = rcBuildLayerRegions(mCtx.get(), *mChf, 0, m_cfg.minRegionArea);
		assert(isOk);
		break;
	}

	// Step 5. Trace and simplify region contours.
	mCset = rcAllocContourSet(); // create contours
	assert(mCset);
	isOk = rcBuildContours(mCtx.get(), *mChf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *mCset);
	assert(isOk);

	// Step 6. Build polygons mesh from contours.
	mMesh = rcAllocPolyMesh();
	assert(mMesh);
	isOk = rcBuildPolyMesh(mCtx.get(), *mCset, m_cfg.maxVertsPerPoly, *mMesh);

	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	mDmesh = rcAllocPolyMeshDetail();
	assert(mDmesh);
	isOk = rcBuildPolyMeshDetail(mCtx.get(), *mMesh, *mChf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *mDmesh);
	assert(isOk);

	// these are no longer needed
	rcFreeCompactHeightfield(mChf);
	mChf = nullptr;
	rcFreeContourSet(mCset);
	mCset = nullptr;


	// Step 8. Create Detour data from Recast poly mesh.
	assert(m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON);
	int navDataSize = 0;
	for (int i = 0; i < mMesh->npolys; ++i) // update poly flags from areas.
	{
		if (mMesh->areas[i] == RC_WALKABLE_AREA) mMesh->areas[i] = SAMPLE_POLYAREA_GROUND;

		if (mMesh->areas[i] == SAMPLE_POLYAREA_GROUND || mMesh->areas[i] == SAMPLE_POLYAREA_GRASS || mMesh->areas[i] == SAMPLE_POLYAREA_ROAD)
		{
			mMesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
		}
		else if (mMesh->areas[i] == SAMPLE_POLYAREA_WATER)
		{
			mMesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
		}
		else if (mMesh->areas[i] == SAMPLE_POLYAREA_DOOR)
		{
			mMesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
		}
	}
	dtNavMeshCreateParams params;
	memset(&params, 0, sizeof(params));
	params.verts = mMesh->verts;
	params.vertCount = mMesh->nverts;
	params.polys = mMesh->polys;
	params.polyAreas = mMesh->areas;
	params.polyFlags = mMesh->flags;
	params.polyCount = mMesh->npolys;
	params.nvp = mMesh->nvp;
	params.detailMeshes = mDmesh->meshes;
	params.detailVerts = mDmesh->verts;
	params.detailVertsCount = mDmesh->nverts;
	params.detailTris = mDmesh->tris;
	params.detailTriCount = mDmesh->ntris;
	//params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
	//params.offMeshConRad = m_geom->getOffMeshConnectionRads();
	//params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
	//params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
	//params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
	//params.offMeshConUserID = m_geom->getOffMeshConnectionId();
	//params.offMeshConCount = m_geom->getOffMeshConnectionCount();
	params.walkableHeight = Agent::maxHeight();
	params.walkableRadius = Agent::maxRadius();
	params.walkableClimb = Agent::maxClimb();
	rcVcopy(params.bmin, mMesh->bmin);
	rcVcopy(params.bmax, mMesh->bmax);
	params.cs = m_cfg.cs;
	params.ch = m_cfg.ch;
	params.buildBvTree = true;
	// navData will be deallocated when the mesh is freed
	uchar* navData = nullptr;
	isOk = dtCreateNavMeshData(&params, &navData, &navDataSize); 
	assert(isOk);
	mNavMesh = dtAllocNavMesh();
	assert(mNavMesh);
	dtStatus status;
	status = mNavMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
	isOk = !dtStatusFailed(status);
	assert(isOk);
	mNavQuery = dtAllocNavMeshQuery();
	assert(mNavQuery);
	status = mNavQuery->init(mNavMesh, 2048);
	isOk = !dtStatusFailed(status);
	assert(isOk);
}

void NavigationMesh::deinitialize()
{
	dtFreeNavMeshQuery(mNavQuery);
	mNavQuery = nullptr;
	rcFreeHeightField(mSolid);
	mSolid = nullptr;
	rcFreeCompactHeightfield(mChf);
	mChf = nullptr;
	rcFreeContourSet(mCset);
	mCset = nullptr;
	rcFreePolyMesh(mMesh);
	mMesh = nullptr;
	rcFreePolyMeshDetail(mDmesh);
	mDmesh = nullptr;
	dtFreeNavMesh(mNavMesh);
	mNavMesh = nullptr;

	mVertices.clear();
	mTriangles.clear();
	mBounds.min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	mBounds.max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

void NavigationMesh::getTiles(std::vector<const dtMeshTile*>* tiles) const
{
	tiles->clear();
	tiles->reserve(mNavMesh->getMaxTiles());
	for (int i = 0; i < mNavMesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = ((const dtNavMesh*)mNavMesh)->getTile(i);
		if (tile->header) tiles->push_back(tile);
	}
}

int NavigationMesh::getTileGeometryPolyCount(const dtMeshTile* tile) const
{
	return tile->header->polyCount;
}

void NavigationMesh::getTileGeometry(
	const dtMeshTile* tile,
	int polyIndex,
	std::vector<ushort>* indices,
	std::vector<vec3>* vertices,
	std::vector<vec2>* uvs,
	std::vector<vec3>* normals,
	std::vector<vec3>* tangents,
	bounds3* bounds) const
{
	indices->clear();
	vertices->clear();
	uvs->clear();
	normals->clear();
	tangents->clear();
	bounds->min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	bounds->max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	ushort indexCounter = 0;
	const dtPoly* p = &tile->polys[polyIndex];
	if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)	// Skip off-mesh links.
		return;

	const dtPolyDetail* pd = &tile->detailMeshes[polyIndex];
	for (int j = 0; j < pd->triCount; ++j)
	{
		const unsigned char* t = &tile->detailTris[(pd->triBase + j) * 4];
		for (int k = 0; k < 3; ++k)
		{
			vec3 v;
			if (t[k] < p->vertCount)
			{
				int i = p->verts[t[k]] * 3;
				float* vf = &tile->verts[i];
				v = vec3(vf[0], vf[1], vf[2]);
			}
			else
			{
				int i = (pd->vertBase + t[k] - p->vertCount) * 3;
				float* vf = &tile->detailVerts[i];
				v = vec3(vf[0], vf[1], vf[2]);
			}

			indices->push_back(indexCounter++);
			indices->push_back(indexCounter++);
			indices->push_back(indexCounter++);
			vertices->push_back(v);
			uvs->push_back(vec2(v.x(), v.z()));
			normals->push_back(vec3(0.0f, 1.0f, 0.0f));
			tangents->push_back(vec3(1.0f, 0.0f, 0.0f));
			bounds->update(v);
		}
	}
}

void NavigationMesh::addNodeGeometry(const mat4x4& nodeWorldTransform, Node* node)
{
	if (node->mesh && !node->mesh->skinned) // only static meshes are supported
	{
		auto data = node->mesh->getTriangleData()->data();
		auto triangleCount = (uint)(node->mesh->getTriangleData()->size() / sizeof(Triangle));
		auto vectorSize = (rsize_t)(sizeof(float) * 3);
		auto triangleSize = (rsize_t)(vectorSize * 3);
		auto indexOffset = (int)mTriangles.size();
		std::vector<float> vertices(triangleCount * 3 * 3);
		std::vector<int> triangles(triangleCount * 3);
		for (uint i = 0; i < triangleCount; i++)
		{
			// get vertices
			auto offset = (uint)(sizeof(Triangle) * i);
			auto triangle = (Triangle*)(data + offset);
			vec3 v0 = triangle->v0;
			vec3 v1 = v0 + triangle->e1;
			vec3 v2 = v0 + triangle->e2;

			// transform to world space
			v0 = (nodeWorldTransform * v0.homogeneous()).head<3>();
			v1 = (nodeWorldTransform * v1.homogeneous()).head<3>();
			v2 = (nodeWorldTransform * v2.homogeneous()).head<3>();

			// store
			memcpy_s((uchar*)vertices.data() + triangleSize * i + vectorSize * 0, vectorSize, v0.data(), vectorSize);
			memcpy_s((uchar*)vertices.data() + triangleSize * i + vectorSize * 1, vectorSize, v1.data(), vectorSize);
			memcpy_s((uchar*)vertices.data() + triangleSize * i + vectorSize * 2, vectorSize, v2.data(), vectorSize);
			triangles[i * 3 + 0] = indexOffset + i * 3 + 0;
			triangles[i * 3 + 1] = indexOffset + i * 3 + 1;
			triangles[i * 3 + 2] = indexOffset + i * 3 + 2;

			// update bounds
			mBounds.update(v0);
			mBounds.update(v1);
			mBounds.update(v2);
		}
		mVertices.insert(mVertices.end(), vertices.begin(), vertices.end());
		mTriangles.insert(mTriangles.end(), triangles.begin(), triangles.end());
	}

	for (size_t i = 0; i < node->children.size(); i++)
	{
		mat4x4 childWorldTransform = nodeWorldTransform * node->children[i]->getLocalTransform().matrix();
		addNodeGeometry(childWorldTransform, node->children[i]);
	}
}
