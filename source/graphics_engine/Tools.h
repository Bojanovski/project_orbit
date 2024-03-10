#pragma once
#include "GraphicsEngineTypes.h"

#define TOOLS_EPSILON 0.0001f


namespace graphics_engine
{
	struct frustum
	{
		enum Side { Near = 0, Far = 1, Left = 2, Right = 3, Bottom = 4, Top = 5, };
		// Computes the frustum volume from the camera parameters
		// - yFOV and xFOV are in radians
		void set(const vec3& pos, const vec3& dir, const vec3& up, float np, float fp, float xFOV, float yFOV);
		const plane3& getPlane(Side s) { return planes[(int)s]; }

		plane3 planes[6];
	};

	inline double degToRad(double degrees) { return degrees * M_PI / 180.0; }
	inline double radToDeg(double radians) { return radians * 180.0 / M_PI; }

	// Returns a point on the plane
	inline vec3 pointOnPlane(const plane3& p) { return -p.normal() * p.offset(); }

	// Intersection of 2-planes: a variation based on the 3-plane version (Graphics Gems 1 pg 305)
	// Note that the 'normal' components of the planes need not be unit length
	inline bool intersectPlanePlane(const plane3& p1, const plane3& p2, ray3* outRay)
	{
		// logically the 3rd plane, but we only use the normal component.
		const vec3 p3_normal = p1.normal().cross(p2.normal());
		const float det = p3_normal.squaredNorm();

		// If the determinant is 0, that means parallel planes, no intersection.
		if (abs(det) < TOOLS_EPSILON)
			return false;
		// calculate the final (point, normal)
		outRay->origin = ((p3_normal.cross(p2.normal()) * p1.offset()) + (p1.normal().cross(p3_normal) * p2.offset())) / det;
		outRay->direction = p3_normal.normalized();
		return true;
	}

	// Determines the point of intersection between a plane defined by a point and
	// a normal vector and a line defined by a point and a direction vector.
	inline bool intersectPlaneRay(const plane3& p, const ray3& r, vec3* outPoint)
	{
		vec3 lineDir = r.direction.normalized(); // must be normalized
		if (p.normal().dot(lineDir) == 0)
			return false;

		double t = (p.normal().dot(pointOnPlane(p)) - p.normal().dot(r.origin)) / p.normal().dot(lineDir);
		*outPoint = r.origin + lineDir * t;
		return true;
	}

	class SceneTools
	{
	public:
		// Will return an AABB (bounds3) that fits all the geometry inside
		bounds3 computeEnclosingBounds(NodeContainer* nc);
		// Will return a frustum with the same direction, yFOV and aspect ratio as the provided frustum,
		// but all the geometry will fit inside
		vec3 computeEnclosingFrustumPosition(NodeContainer* nc, const vec3& dir, const vec3& up, float np, float fp, float xFOV, float yFOV);

	private:
		void computeEnclosingBoundsVertexData(Node* node, const transform&);
		void computeEnclosingBoundsFast(Node* node, const transform&);
		void computeEnclosingFrustumVertexData(Node* node, const transform&);
		void computeEnclosingFrustumFast(Node* node, const transform&);
		void updateFrustumPlanes(const vec3& pos);

	public:
		bool mUseVertexDataForCompute = false;

	private:
		struct {
			bounds3 b;
		} mBoundsCache;

		struct {
			bool initialized = false;
			frustum f;
			vec3 dir, up;
			float np, fp, xFOV, yFOV;
		} mFrustumCache;
	};

	class MeshTools
	{
	public:
		typedef std::vector<uchar> DataElement;
		enum class Operation { Add, Sub };
		enum class Initialization { Zero, Copy, None };

	public:
		template<typename vecType> void bufferOperation(const Mesh::Buffer&, const Mesh::Buffer&, Operation, Mesh::Buffer*);
		template<typename vecType> void bufferInitialization(const Mesh::Buffer&, Initialization, Mesh::Buffer*, DataElement*);
	};
}