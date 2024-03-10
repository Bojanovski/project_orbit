#include "Tools.h"

using namespace graphics_engine;

void frustum::set(const vec3& pos, const vec3& dir, const vec3& up, float np, float fp, float xFOV, float yFOV)
{
    auto dir_corrected = dir.normalized();
    auto right = up.cross(dir).normalized();
    auto up_corrected = dir.cross(right).normalized();
    float xfov_div2 = xFOV * 0.5f;
    float yfov_div2 = yFOV * 0.5f;
    vec3 normal;
    // near plane
    planes[frustum::Near] = plane3(dir_corrected, pos + dir_corrected * np);
    // far plane
    planes[frustum::Far] = plane3(-dir_corrected, pos + dir_corrected * fp);
    // left plane
    normal = right + dir * tan(xfov_div2);
    planes[frustum::Left] = plane3(normal.normalized(), pos);
    // right plane
    normal = -right + dir * tan(xfov_div2);
    planes[frustum::Right] = plane3(normal.normalized(), pos);
    // bottom plane
    normal = up_corrected + dir * tan(yfov_div2);
    planes[frustum::Bottom] = plane3(normal.normalized(), pos);
    // top plane
    normal = -up_corrected + dir * tan(yfov_div2);
    planes[frustum::Top] = plane3(normal.normalized(), pos);
}

bounds3 SceneTools::computeEnclosingBounds(NodeContainer* nc)
{
    mBoundsCache.b.min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    mBoundsCache.b.max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    assert(nc);
    auto node = dynamic_cast<graphics_engine::Node*>(nc);
    if (node)
    {
        const auto& nodeWorldTransform = node->computeWorldTransform();
        for (int i = 0; i < node->children.size(); i++)
        {
            if (mUseVertexDataForCompute)
                computeEnclosingBoundsVertexData(node->children[i], transform::Identity());
            else
                computeEnclosingBoundsFast(node->children[i], transform::Identity());
        }
    }
    auto scene = dynamic_cast<graphics_engine::Scene*>(nc);
    if (scene)
    {
        for (int i = 0; i < scene->nodes.size(); i++)
        {
            if (mUseVertexDataForCompute)
                computeEnclosingBoundsVertexData(scene->nodes[i], transform::Identity());
            else
                computeEnclosingBoundsFast(scene->nodes[i], transform::Identity());
        }
    }
    return mBoundsCache.b;
}

void SceneTools::computeEnclosingBoundsVertexData(Node* node, const transform& parentWorldTransform)
{
    const auto& nodeWorldTransform = parentWorldTransform * node->getLocalTransform();
    if (node->mesh)
    {
        assert(!node->mesh->skinned);
        auto data = node->mesh->getTriangleData()->data();
        auto triangleCount = (uint)(node->mesh->getTriangleData()->size() / sizeof(Triangle));
        auto vectorSize = (rsize_t)(sizeof(float) * 3);
        auto triangleSize = (rsize_t)(vectorSize * 3);
        auto indexOffset = (int)0;

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

            // transform to world space and update bounds
            v0 = (nodeWorldTransform * v0.homogeneous()).head<3>();
            v1 = (nodeWorldTransform * v1.homogeneous()).head<3>();
            v2 = (nodeWorldTransform * v2.homogeneous()).head<3>();
            mBoundsCache.b.update(v0);
            mBoundsCache.b.update(v1);
            mBoundsCache.b.update(v2);
        }
    }

    for (int i = 0; i < node->children.size(); i++)
    {
        computeEnclosingBoundsVertexData(node->children[i], nodeWorldTransform);
    }
}

void SceneTools::computeEnclosingBoundsFast(Node* node, const transform& parentWorldTransform)
{
    const auto& nodeWorldTransform = parentWorldTransform * node->getLocalTransform();
    if (node->mesh)
    {
        assert(!node->mesh->skinned);
        for (auto i = 0; i < node->mesh->getBounds()->size(); i++)
        {
            auto& sb = node->mesh->getBounds()->at(i);
            std::vector<vec3> points = {
                vec3(sb.min.x(), sb.min.y(), sb.min.z()),
                vec3(sb.min.x(), sb.min.y(), sb.max.z()),
                vec3(sb.min.x(), sb.max.y(), sb.min.z()),
                vec3(sb.min.x(), sb.max.y(), sb.max.z()),
                vec3(sb.max.x(), sb.min.y(), sb.min.z()),
                vec3(sb.max.x(), sb.min.y(), sb.max.z()),
                vec3(sb.max.x(), sb.max.y(), sb.min.z()),
                vec3(sb.max.x(), sb.max.y(), sb.max.z()),
            };

            for (auto j = 0; j < points.size(); j++)
            {
                // transform to world space and update bounds
                auto point = (nodeWorldTransform * points[j].homogeneous()).head<3>();
                mBoundsCache.b.update(point);
            }
        }
    }

    for (int i = 0; i < node->children.size(); i++)
    {
        computeEnclosingBoundsFast(node->children[i], nodeWorldTransform);
    }
}

vec3 SceneTools::computeEnclosingFrustumPosition(NodeContainer* nc, const vec3& dir, const vec3& up, float np, float fp, float xFOV, float yFOV)
{
    mFrustumCache.dir = dir;
    mFrustumCache.up = up;
    mFrustumCache.np = np;
    mFrustumCache.fp = fp;
    mFrustumCache.xFOV = xFOV;
    mFrustumCache.yFOV = yFOV;
    mFrustumCache.initialized = false;
    assert(nc);
    auto node = dynamic_cast<graphics_engine::Node*>(nc);
    if (node)
    {
        const auto& nodeWorldTransform = node->computeWorldTransform();
        for (int i = 0; i < node->children.size(); i++)
        {
            if (mUseVertexDataForCompute)
                computeEnclosingFrustumVertexData(node->children[i], transform::Identity());
            else
                computeEnclosingFrustumFast(node->children[i], transform::Identity());
        }
    }
    auto scene = dynamic_cast<graphics_engine::Scene*>(nc);
    if (scene)
    {
        for (int i = 0; i < scene->nodes.size(); i++)
        {
            if (mUseVertexDataForCompute)
                computeEnclosingFrustumVertexData(scene->nodes[i], transform::Identity());
            else
                computeEnclosingFrustumFast(scene->nodes[i], transform::Identity());
        }
    }

    // Compute the camera position
    bool isOk;
    auto cameraPlane = mFrustumCache.f.getPlane(frustum::Near);
    ray3 verticalIntersection, horizontalIntersection;
    isOk = intersectPlanePlane(mFrustumCache.f.getPlane(frustum::Left), mFrustumCache.f.getPlane(frustum::Right), &verticalIntersection);
    assert(isOk);
    if (cameraPlane.signedDistance(verticalIntersection.origin) < 0.0f)
    {
        cameraPlane = plane3(cameraPlane.normal(), verticalIntersection.origin);
    }
    isOk = intersectPlanePlane(mFrustumCache.f.getPlane(frustum::Top), mFrustumCache.f.getPlane(frustum::Bottom), &horizontalIntersection);
    assert(isOk);
    if (cameraPlane.signedDistance(horizontalIntersection.origin) < 0.0f)
    {
        cameraPlane = plane3(cameraPlane.normal(), horizontalIntersection.origin);
    }
    ray3 cameraRay;
    isOk = intersectPlanePlane(
        plane3(verticalIntersection.direction.cross(dir).normalized(), verticalIntersection.origin),
        plane3(horizontalIntersection.direction.cross(dir).normalized(), horizontalIntersection.origin),
        &cameraRay);
    assert(isOk);
    vec3 cameraPos;
    isOk = intersectPlaneRay(cameraPlane, cameraRay, &cameraPos);
    assert(isOk);
    return cameraPos;
}

void SceneTools::computeEnclosingFrustumVertexData(Node* node, const transform& parentWorldTransform)
{
    const auto& nodeWorldTransform = parentWorldTransform * node->getLocalTransform();
    if (node->mesh)
    {
        assert(!node->mesh->skinned);
        auto data = node->mesh->getTriangleData()->data();
        auto triangleCount = (uint)(node->mesh->getTriangleData()->size() / sizeof(Triangle));
        auto vectorSize = (rsize_t)(sizeof(float) * 3);
        auto triangleSize = (rsize_t)(vectorSize * 3);
        auto indexOffset = (int)0;

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

            // transform to world space and update bounds
            v0 = (nodeWorldTransform * v0.homogeneous()).head<3>();
            v1 = (nodeWorldTransform * v1.homogeneous()).head<3>();
            v2 = (nodeWorldTransform * v2.homogeneous()).head<3>();

            if (!mFrustumCache.initialized)
            {
                mFrustumCache.f.set(v0, mFrustumCache.dir, mFrustumCache.up, mFrustumCache.np, mFrustumCache.fp, mFrustumCache.xFOV, mFrustumCache.yFOV);
                mFrustumCache.initialized = true;
            }
            updateFrustumPlanes(v0);
            updateFrustumPlanes(v1);
            updateFrustumPlanes(v2);
        }
    }

    for (int i = 0; i < node->children.size(); i++)
    {
        computeEnclosingFrustumVertexData(node->children[i], nodeWorldTransform);
    }
}

void SceneTools::computeEnclosingFrustumFast(Node* node, const transform& parentWorldTransform)
{
    const auto& nodeWorldTransform = parentWorldTransform * node->getLocalTransform();
    if (node->mesh)
    {
        assert(!node->mesh->skinned);
        for (auto i = 0; i < node->mesh->getBounds()->size(); i++)
        {
            auto& sb = node->mesh->getBounds()->at(i);
            std::vector<vec3> points = {
                vec3(sb.min.x(), sb.min.y(), sb.min.z()),
                vec3(sb.min.x(), sb.min.y(), sb.max.z()),
                vec3(sb.min.x(), sb.max.y(), sb.min.z()),
                vec3(sb.min.x(), sb.max.y(), sb.max.z()),
                vec3(sb.max.x(), sb.min.y(), sb.min.z()),
                vec3(sb.max.x(), sb.min.y(), sb.max.z()),
                vec3(sb.max.x(), sb.max.y(), sb.min.z()),
                vec3(sb.max.x(), sb.max.y(), sb.max.z()),
            };

            for (auto j = 0; j < points.size(); j++)
            {
                auto point = (nodeWorldTransform * points[j].homogeneous()).head<3>(); // to world space 
                if (!mFrustumCache.initialized)
                {
                    mFrustumCache.f.set(point, mFrustumCache.dir, mFrustumCache.up, mFrustumCache.np, mFrustumCache.fp, mFrustumCache.xFOV, mFrustumCache.yFOV);
                    mFrustumCache.initialized = true;
                }
                updateFrustumPlanes(point);
            }
        }
    }

    for (int i = 0; i < node->children.size(); i++)
    {
        computeEnclosingFrustumFast(node->children[i], nodeWorldTransform);
    }
}

void SceneTools::updateFrustumPlanes(const vec3& pos)
{
    for (int i = 0; i < 6; i++)
    {
        plane3* p = &mFrustumCache.f.planes[i];
        float dist = p->signedDistance(pos);
        if (dist < 0.0f)
        {
            *p = plane3(p->normal(), pos);
        }
    }
}

template<typename vecType>
void MeshTools::bufferOperation(const Mesh::Buffer& b1, const Mesh::Buffer& b2, Operation op, Mesh::Buffer* outBuffer)
{
    auto count = b1.count;
    auto cs = b1.componentSize;
    assert(count == b2.count);
    assert(cs == b2.componentSize);
    //assert(stride == b2.stride); // it is ok if this is different
    assert(count == outBuffer->count);
    assert(cs == outBuffer->componentSize);
    //assert(stride == outBuffer->stride);
    int componentCount;
    if constexpr (std::is_same_v<vecType, vec2>)        componentCount = 2;
    else if constexpr (std::is_same_v<vecType, vec3>)   componentCount = 3;
    else if constexpr (std::is_same_v<vecType, vec4>)   componentCount = 4;
    else                                                throw; // either vec2, vec3 or vec4 supported as the sparse accessor type
    size_t size = cs * componentCount;
    for (uint j = 0; j < count; ++j)
    {
        vecType v1, v2;
        memcpy_s(v1.data(), size, b1.data + j * b1.stride, size);
        memcpy_s(v2.data(), size, b2.data + j * b2.stride, size);
        vecType v_final;
        switch (op)
        {
        case MeshTools::Operation::Add:
            v_final = v1 + v2;
            break;
        case MeshTools::Operation::Sub:
            v_final = v1 - v2;
            break;
        default:
            throw;
        }
        memcpy_s(outBuffer->data + j * outBuffer->stride, size, v_final.data(), size);
    }
}

template<typename vecType>
void MeshTools::bufferInitialization(const Mesh::Buffer& inBuffer, Initialization init, Mesh::Buffer* outBuffer, DataElement* outDataElement)
{
    outBuffer->count = inBuffer.count;
    outBuffer->stride = inBuffer.stride;
    outBuffer->componentSize = inBuffer.componentSize;
    outBuffer->componentType = inBuffer.componentType;
    outDataElement->resize(outBuffer->count * outBuffer->stride);
    outBuffer->data = outDataElement->data();
    auto cs = inBuffer.componentSize;
    int componentCount;
    if constexpr (std::is_same_v<vecType, vec2>)        componentCount = 2;
    else if constexpr (std::is_same_v<vecType, vec3>)   componentCount = 3;
    else if constexpr (std::is_same_v<vecType, vec4>)   componentCount = 4;
    else                                                throw; // either vec2, vec3 or vec4 supported as the sparse accessor type
    size_t size = cs * componentCount;
    switch (init)
    {
    case MeshTools::Initialization::Copy:
        for (uint i = 0; i < outBuffer->count; ++i)
        {
            memcpy_s(outBuffer->data + i * outBuffer->stride, size, inBuffer.data + i * inBuffer.stride, size);
        }
        break;

    case MeshTools::Initialization::Zero:
        for (uint i = 0; i < outBuffer->count; ++i)
        {
            vecType v0 = vecType::Zero();
            memcpy_s(outBuffer->data + i * outBuffer->stride, size, v0.data(), size);
        }
        break;

    case MeshTools::Initialization::None:
        // do nothing
        break;

    default:
        throw;
    }
}

//
// Instantiate methods
//

template void MeshTools::bufferOperation<vec2>(const Mesh::Buffer&, const Mesh::Buffer&, Operation, Mesh::Buffer*);
template void MeshTools::bufferOperation<vec3>(const Mesh::Buffer&, const Mesh::Buffer&, Operation, Mesh::Buffer*);
template void MeshTools::bufferOperation<vec4>(const Mesh::Buffer&, const Mesh::Buffer&, Operation, Mesh::Buffer*); 

template void MeshTools::bufferInitialization<vec2>(const Mesh::Buffer&, Initialization, Mesh::Buffer*, DataElement*);
template void MeshTools::bufferInitialization<vec3>(const Mesh::Buffer&, Initialization, Mesh::Buffer*, DataElement*);
template void MeshTools::bufferInitialization<vec4>(const Mesh::Buffer&, Initialization, Mesh::Buffer*, DataElement*);
