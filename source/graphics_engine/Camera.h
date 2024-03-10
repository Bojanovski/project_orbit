#pragma once
#include "GraphicsEngineTypes.h"
#include "Tools.h"


namespace graphics_engine
{
    class Camera
    {
    public:
        enum Type { Perspective, Panoramic };

    public:
        Camera();

        void setLookAt(const vec3& pos, const vec3& dir, const vec3& up);
        void setPerspective(float yfov, float aspect);
        void setPanoramic();
        void setPlaneDistances(float n, float f);

        void getPlaneDistances(float* n, float* f) const { *n = mNearPlaneDistance; *f = mFarPlaneDistance; }
        mat4x4 getProjection() const { return mProjection; }
        transform getView() const { return mView; }
        float getYFOV() const { return mYFOV; }
        float getAspect() const { return mAspect; }
        vec3 getPosition() const { return mPos; }
        vec3 getDirection() const { return mDir; }
        vec3 getUp() const { return mUp; }
        Type getType() const { return mType; }

        ray3 computeRayFromNDC(const vec2& pos) const;
        float computeXFOV() const;
        frustum computeFrustum();
        vec3 computeRightDirection() const { return mUp.cross(mDir).normalized(); }

    private:
        mat4x4 perspective(float fovy, float aspect, float zNear, float zFar);
        transform lookAt(vec3 const& eye, vec3 const& center, vec3 const& up);

    private:
        Type mType;
        float mNearPlaneDistance;
        float mFarPlaneDistance;
        float mYFOV;
        float mAspect;
        vec3 mPos;
        vec3 mDir;
        vec3 mUp;
        transform mView;
        mat4x4 mProjection;
    };
}
