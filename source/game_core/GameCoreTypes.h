#pragma once
#include "GraphicsEngineTypes.h"
#include <bullet/btBulletDynamicsCommon.h>


class btDiscreteDynamicsWorld;
class btCollisionShape;
struct btDefaultMotionState;
class btRigidBody;

namespace game_core
{
    // Generic types
    typedef graphics_engine::Handle Handle;
    typedef graphics_engine::uchar uchar;
    typedef graphics_engine::ushort ushort;
    typedef graphics_engine::uint uint;
    typedef graphics_engine::ulong ulong;
    typedef graphics_engine::vec2 vec2;
    typedef graphics_engine::vec3 vec3;
    typedef graphics_engine::vec4 vec4;
    typedef graphics_engine::quat quat;
    typedef graphics_engine::color color;
    typedef graphics_engine::mat4x4 mat4x4;
    typedef graphics_engine::transform transform;
    typedef graphics_engine::bounds2 bounds2;
    typedef graphics_engine::bounds3 bounds3;
    typedef graphics_engine::ray3 ray3;
    typedef graphics_engine::RayCastHit RayCastHit;

    // Physics
	typedef btDiscreteDynamicsWorld PhysicsSimulator;

    struct RigidBody
    {
        std::unique_ptr<btCollisionShape> shape;
        std::unique_ptr<btDefaultMotionState> motionState;
        std::unique_ptr<btRigidBody> rigidBody;
    };

	inline void convertVec3(btVector3* dest, const vec3& src) { *dest = btVector3(src.x(), src.y(), src.z()); }
    inline void convertVec3(vec3* dest, const btVector3& src) { *dest = vec3(src.x(), src.y(), src.z()); }
}
