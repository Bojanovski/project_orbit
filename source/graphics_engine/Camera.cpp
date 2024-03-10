#include "Camera.h"

using namespace graphics_engine;

Camera::Camera()
{
    mType = Type::Perspective;
    mView = transform::Identity();
    mNearPlaneDistance = 0.1f;
    mFarPlaneDistance = 1000.0f;
}

void Camera::setLookAt(const vec3& pos, const vec3& dir, const vec3& up)
{
    mPos = pos;
    mDir = dir.normalized();
    mUp = up.normalized();
    mView = lookAt(pos, pos + dir, up);
}

void Camera::setPerspective(float yfov, float aspect)
{
    mType = Type::Perspective;
    mYFOV = yfov;
    mAspect = aspect;
    mProjection = perspective(mYFOV, mAspect, mNearPlaneDistance, mFarPlaneDistance);
}

void Camera::setPanoramic()
{
    mType = Type::Panoramic;
    mAspect = 0.5f;
}

void Camera::setPlaneDistances(float n, float f)
{
    mNearPlaneDistance = n;
    mFarPlaneDistance = f;
    switch (mType)
    {
    case graphics_engine::Camera::Perspective:
        mProjection = perspective(mYFOV, mAspect, mNearPlaneDistance, mFarPlaneDistance);
        break;
    case graphics_engine::Camera::Panoramic:
        // ...
        break;
    default:
        break;
    }
}

float Camera::computeXFOV() const
{
    auto radf = (float)degToRad(mYFOV);
    float tanHalfFovy = tanf(radf / 2.0f);
    float xFOV_radians = 2.0f * std::atan(tanHalfFovy * mAspect);
    return radToDeg(xFOV_radians);
}

frustum Camera::computeFrustum()
{
    float np, fp;
    getPlaneDistances(&np, &fp);
    auto pos = getPosition();
    auto dir = getDirection();
    auto up = getUp();
    auto yfov = (float)degToRad(getYFOV());
    auto xfov = (float)degToRad(computeXFOV());

    frustum ret;
    ret.set(pos, dir, up, np, fp, xfov, yfov);
    return ret;
}

ray3 Camera::computeRayFromNDC(const vec2& pos) const
{
    assert(mType == Type::Perspective);
    ray3 ray;
    ray.origin = mPos;
    vec4 planePos = vec4(pos.x(), pos.y(), 0.0f, 1.0f);
    vec4 viewPos = mProjection.inverse() * planePos;
    viewPos /= viewPos.w();
    // Multiply to make sure the direction is big enough so that there is no floating point precision errors.
    // It will be normalized later in the world space.
    viewPos *= (ray.origin.norm() + 1.0f);
    viewPos[3] = 1.0f;
    vec4 worldPos = mView.inverse() * viewPos;
    // Subtract and normalize for direction
    ray.direction = worldPos.head<3>() - ray.origin;
    ray.direction.normalize();
    return ray;
}

mat4x4 Camera::perspective(float fovy, float aspect, float zNear, float zFar)
{
    assert(aspect > 0);
    assert(zFar > zNear);
    auto radf = (float)degToRad(fovy);
    float tanHalfFovy = tanf(radf / 2.0f);
    mat4x4 res = mat4x4::Zero();
    res(0, 0) = 1.0f / (aspect * tanHalfFovy);
    res(1, 1) = 1.0f / (tanHalfFovy);
    res(2, 2) = -(zFar + zNear) / (zFar - zNear);
    res(3, 2) = -1.0f;
    res(2, 3) = -(2.0f * zFar * zNear) / (zFar - zNear);
    return res;
}

transform Camera::lookAt(vec3 const& eye, vec3 const& center, vec3 const& up)
{
    vec3 f = (center - eye).normalized();
    vec3 u = up.normalized();
    vec3 s = f.cross(u).normalized();
    u = s.cross(f);
    mat4x4 res;
    res << s.x(), s.y(), s.z(), -s.dot(eye),
        u.x(), u.y(), u.z(), -u.dot(eye),
        -f.x(), -f.y(), -f.z(), f.dot(eye),
        0, 0, 0, 1;
    return transform(res);
}
