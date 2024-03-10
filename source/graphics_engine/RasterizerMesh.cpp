#include "RasterizerMesh.h"


using namespace graphics_engine;

RasterizerMeshBase::RasterizerMeshBase()
    : mCachedShaderProgram(nullptr)
{
}

RasterizerMesh::RasterizerMesh()
    : RasterizerMeshBase()
{

}

RasterizerMesh::~RasterizerMesh()
{
    uint submeshCount = (uint)mPositionBuffers.size();
    for (uint i = 0; i < submeshCount; i++)
    {
        deleteBuffer(&mPositionBuffers[i]);
        deleteBuffer(&mNormalBuffers[i]);
        deleteBuffer(&mTangentBuffers[i]);
        deleteBuffer(&mUvBuffers[i]);
        deleteBuffer(&mElementBuffers[i]);
        if (skinned)
        {
            deleteBuffer(&mJointBuffers[i]);
            deleteBuffer(&mWeightBuffers[i]);
        }
    }
    mPositionBuffers.clear();
    mUvBuffers.clear();
    mNormalBuffers.clear();
    mElementBuffers.clear();
}

bool RasterizerMesh::create(
    const std::vector<Mesh::Buffer>& positionBuffers,
    const std::vector<Mesh::Buffer>& uvBuffers,
    const std::vector<Mesh::Buffer>& normalBuffers,
    const std::vector<Mesh::Buffer>& tangentBuffers,
    const std::vector<Mesh::Buffer>& jointBuffers,
    const std::vector<Mesh::Buffer>& weightBuffers,
    const std::vector<Mesh::Buffer>& indexBuffers,
    const std::vector<Mesh::MorphTargets>& morphTargets)
{
    initializeOpenGLFunctions();

    bool isOk = true;
    uint submeshCount = positionBuffers.size();
    assert(positionBuffers.size() == submeshCount);
    assert(uvBuffers.size() == submeshCount);
    assert(normalBuffers.size() == submeshCount);
    assert(tangentBuffers.size() == submeshCount);
    assert(indexBuffers.size() == submeshCount);
    if (skinned)
    {
        assert(jointBuffers.size() == submeshCount);
        assert(weightBuffers.size() == submeshCount);
    }
    if (morphTargetCount > 0)
    {
        assert(morphTargets.size() == submeshCount);
    }

    // Position
    mPositionBuffers.resize(submeshCount);
    for (GLuint i = 0; i < submeshCount; i++)
    {
        mPositionBuffers[i] = positionBuffers[i];
        glGenBuffers(1, &mPositionBuffers[i].glID);
        glBindBuffer(GL_ARRAY_BUFFER, mPositionBuffers[i].glID);
        glBufferData(GL_ARRAY_BUFFER,
            positionBuffers[i].count * positionBuffers[i].stride,
            positionBuffers[i].data,
            GL_STATIC_DRAW);
    }

    // UV
    mUvBuffers.resize(submeshCount);
    for (GLuint i = 0; i < submeshCount; i++)
    {
        mUvBuffers[i] = uvBuffers[i];
        glGenBuffers(1, &mUvBuffers[i].glID);
        glBindBuffer(GL_ARRAY_BUFFER, mUvBuffers[i].glID);
        glBufferData(GL_ARRAY_BUFFER,
            uvBuffers[i].count * uvBuffers[i].stride,
            uvBuffers[i].data,
            GL_STATIC_DRAW);
    }

    // Normal
    mNormalBuffers.resize(submeshCount);
    for (GLuint i = 0; i < submeshCount; i++)
    {
        mNormalBuffers[i] = normalBuffers[i];
        glGenBuffers(1, &mNormalBuffers[i].glID);
        glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffers[i].glID);
        glBufferData(GL_ARRAY_BUFFER,
            normalBuffers[i].count * normalBuffers[i].stride,
            normalBuffers[i].data,
            GL_STATIC_DRAW);
    }

    // Tangent
    mTangentBuffers.resize(submeshCount);
    for (GLuint i = 0; i < submeshCount; i++)
    {
        mTangentBuffers[i] = tangentBuffers[i];
        glGenBuffers(1, &mTangentBuffers[i].glID);
        glBindBuffer(GL_ARRAY_BUFFER, mTangentBuffers[i].glID);
        glBufferData(GL_ARRAY_BUFFER,
            tangentBuffers[i].count * tangentBuffers[i].stride,
            tangentBuffers[i].data,
            GL_STATIC_DRAW);
    }

    // Skinning
    if (skinned)
    {
        // Joints
        mJointBuffers.resize(submeshCount);
        for (GLuint i = 0; i < submeshCount; i++)
        {
            mJointBuffers[i] = jointBuffers[i];
            glGenBuffers(1, &mJointBuffers[i].glID);
            glBindBuffer(GL_ARRAY_BUFFER, mJointBuffers[i].glID);
            glBufferData(GL_ARRAY_BUFFER,
                jointBuffers[i].count * jointBuffers[i].stride,
                jointBuffers[i].data,
                GL_STATIC_DRAW);
        }

        // Weights
        mWeightBuffers.resize(submeshCount);
        for (GLuint i = 0; i < submeshCount; i++)
        {
            mWeightBuffers[i] = weightBuffers[i];
            glGenBuffers(1, &mWeightBuffers[i].glID);
            glBindBuffer(GL_ARRAY_BUFFER, mWeightBuffers[i].glID);
            glBufferData(GL_ARRAY_BUFFER,
                weightBuffers[i].count * weightBuffers[i].stride,
                weightBuffers[i].data,
                GL_STATIC_DRAW);
        }
    }

    // Morph targets
    mMorphTargets.resize(submeshCount);
    for (uint i = 0; i < submeshCount; i++)
    {
        if (morphTargetCount > 0)
        {
            mMorphTargets[i].posDiffBuffers.resize(morphTargetCount);
            mMorphTargets[i].norDiffBuffers.resize(morphTargetCount);
            mMorphTargets[i].tanDiffBuffers.resize(morphTargetCount);
        }
        for (uint j = 0; j < morphTargetCount; j++)
        {
            // Position
            mMorphTargets[i].posDiffBuffers[j] = morphTargets[i].posDiffBuffers[j];
            glGenBuffers(1, &mMorphTargets[i].posDiffBuffers[j].glID);
            glBindBuffer(GL_ARRAY_BUFFER, mMorphTargets[i].posDiffBuffers[j].glID);
            glBufferData(GL_ARRAY_BUFFER,
                morphTargets[i].posDiffBuffers[j].count * morphTargets[i].posDiffBuffers[j].stride,
                morphTargets[i].posDiffBuffers[j].data,
                GL_STATIC_DRAW);

            // Normal
            mMorphTargets[i].norDiffBuffers[j] = morphTargets[i].norDiffBuffers[j];
            glGenBuffers(1, &mMorphTargets[i].norDiffBuffers[j].glID);
            glBindBuffer(GL_ARRAY_BUFFER, mMorphTargets[i].norDiffBuffers[j].glID);
            glBufferData(GL_ARRAY_BUFFER,
                morphTargets[i].norDiffBuffers[j].count* morphTargets[i].norDiffBuffers[j].stride,
                morphTargets[i].norDiffBuffers[j].data,
                GL_STATIC_DRAW);

            // Tangent
            mMorphTargets[i].tanDiffBuffers[j] = morphTargets[i].tanDiffBuffers[j];
            glGenBuffers(1, &mMorphTargets[i].tanDiffBuffers[j].glID);
            glBindBuffer(GL_ARRAY_BUFFER, mMorphTargets[i].tanDiffBuffers[j].glID);
            glBufferData(GL_ARRAY_BUFFER,
                morphTargets[i].tanDiffBuffers[j].count* morphTargets[i].tanDiffBuffers[j].stride,
                morphTargets[i].tanDiffBuffers[j].data,
                GL_STATIC_DRAW);
        }
    }

    // Index buffer(s)
    mElementBuffers.resize(submeshCount);
    for (GLuint i = 0; i < submeshCount; i++)
    {
        mElementBuffers[i] = indexBuffers[i];
        glGenBuffers(1, &mElementBuffers[i].glID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBuffers[i].glID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indexBuffers[i].count * indexBuffers[i].stride,
            (GLuint*)indexBuffers[i].data,
            GL_STATIC_DRAW);
    }

    return isOk;
}

bool RasterizerMesh::draw(int index)
{
    int attributeCounter = 0;

    // Positions
    glEnableVertexAttribArray(attributeCounter);
    glBindBuffer(GL_ARRAY_BUFFER, mPositionBuffers[index].glID);
    glVertexAttribPointer(
        attributeCounter,                           // attribute
        3,                                          // size
        mPositionBuffers[index].componentType,      // type
        GL_FALSE,                                   // normalized?
        mPositionBuffers[index].stride,             // stride
        (void*)0                                    // array buffer offset
    );
    ++attributeCounter;

    // UVs
    glEnableVertexAttribArray(attributeCounter);
    glBindBuffer(GL_ARRAY_BUFFER, mUvBuffers[index].glID);
    glVertexAttribPointer(
        attributeCounter,                           // attribute
        2,                                          // size
        mUvBuffers[index].componentType,            // type
        GL_FALSE,                                   // normalized?
        mUvBuffers[index].stride,                   // stride
        (void*)0                                    // array buffer offset
    );
    ++attributeCounter;

    // Normals
    glEnableVertexAttribArray(attributeCounter);
    glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffers[index].glID);
    glVertexAttribPointer(
        attributeCounter,                           // attribute
        3,                                          // size
        mNormalBuffers[index].componentType,        // type
        GL_FALSE,                                   // normalized?
        mNormalBuffers[index].stride,               // stride
        (void*)0                                    // array buffer offset
    );
    ++attributeCounter;

    // Tangents
    glEnableVertexAttribArray(attributeCounter);
    glBindBuffer(GL_ARRAY_BUFFER, mTangentBuffers[index].glID);
    glVertexAttribPointer(
        attributeCounter,                           // attribute
        4,                                          // size
        mTangentBuffers[index].componentType,       // type
        GL_FALSE,                                   // normalized?
        mTangentBuffers[index].stride,              // stride
        (void*)0                                    // array buffer offset
    );
    ++attributeCounter;

    // Skinning
    if (skinned)
    {
        // Joints
        glEnableVertexAttribArray(attributeCounter);
        glBindBuffer(GL_ARRAY_BUFFER, mJointBuffers[index].glID);
        glVertexAttribIPointer(
            attributeCounter,                           // attribute
            4,                                          // size
            mJointBuffers[index].componentType,         // type
            mJointBuffers[index].stride,                // stride
            (void*)0                                    // array buffer offset
        );
        ++attributeCounter;

        // Weights
        glEnableVertexAttribArray(attributeCounter);
        glBindBuffer(GL_ARRAY_BUFFER, mWeightBuffers[index].glID);
        glVertexAttribPointer(
            attributeCounter,                           // attribute
            4,                                          // size
            mWeightBuffers[index].componentType,        // type
            GL_FALSE,                                   // normalized?
            mWeightBuffers[index].stride,               // stride
            (void*)0                                    // array buffer offset
        );
        ++attributeCounter;
    }

    // Morph targets
    for (uint j = 0; j < morphTargetCount; j++)
    {
        // Positions
        glEnableVertexAttribArray(attributeCounter);
        glBindBuffer(GL_ARRAY_BUFFER, mMorphTargets[index].posDiffBuffers[j].glID);
        glVertexAttribPointer(
            attributeCounter,                                           // attribute
            3,                                                          // size
            mMorphTargets[index].posDiffBuffers[j].componentType,       // type
            GL_FALSE,                                                   // normalized?
            mMorphTargets[index].posDiffBuffers[j].stride,              // stride
            (void*)0                                                    // array buffer offset
        );
        ++attributeCounter;

        // Normals
        glEnableVertexAttribArray(attributeCounter);
        glBindBuffer(GL_ARRAY_BUFFER, mMorphTargets[index].norDiffBuffers[j].glID);
        glVertexAttribPointer(
            attributeCounter,                                           // attribute
            3,                                                          // size
            mMorphTargets[index].norDiffBuffers[j].componentType,       // type
            GL_FALSE,                                                   // normalized?
            mMorphTargets[index].norDiffBuffers[j].stride,              // stride
            (void*)0                                                    // array buffer offset
        );
        ++attributeCounter;

        // Tangents
        glEnableVertexAttribArray(attributeCounter);
        glBindBuffer(GL_ARRAY_BUFFER, mMorphTargets[index].tanDiffBuffers[j].glID);
        glVertexAttribPointer(
            attributeCounter,                                           // attribute
            3,                                                          // size, morph target tangents are vec3 - handness (w coord) is not morph-able
            mMorphTargets[index].tanDiffBuffers[j].componentType,       // type
            GL_FALSE,                                                   // normalized?
            mMorphTargets[index].tanDiffBuffers[j].stride,              // stride
            (void*)0                                                    // array buffer offset
        );
        ++attributeCounter;
    }

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBuffers[index].glID);

    // Draw the triangles
    glDrawElements(
        GL_TRIANGLES,                               // mode
        mElementBuffers[index].count,               // count
        mElementBuffers[index].componentType,       // type
        (void*)0                                    // element array buffer offset
    );

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    return true;
}

void RasterizerMesh::deleteBuffer(Buffer* buffer)
{
    if (buffer->glID != 0)
    {
        glDeleteBuffers(1, &buffer->glID);
        buffer->glID = 0;
    }
}

const RasterizerMesh::Buffer& RasterizerMesh::Buffer::operator=(const Mesh::Buffer& mesh)
{
    glID = 0;
    count = mesh.count;
    stride = mesh.stride;
    componentSize = mesh.componentSize;
    componentType = mesh.componentType;
    return *this;
}

RasterizerMeshProxy::RasterizerMeshProxy()
    : RasterizerMeshBase()
{

}

RasterizerMeshProxy::~RasterizerMeshProxy()
{

}

bool RasterizerMeshProxy::create(RasterizerMesh* src)
{
    mSource = src;
    return true;
}

