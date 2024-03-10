#pragma once
#include "GraphicsEngineTypes.h"
#include <QOpenGLExtraFunctions>


namespace graphics_engine
{
    struct RasterizerMeshBase : public Mesh
    {
        friend class RasterizerEnginePrivate;
    
    public:
        RasterizerMeshBase();

        virtual bool draw(int index) = 0;

    protected:
        class MeshShaderProgramBase* mCachedShaderProgram;
    };

    //
    // Used for rendering geometry
    //
    struct RasterizerMesh : public RasterizerMeshBase, protected QOpenGLExtraFunctions
    {
        friend struct RasterizerMeshProxy;

    public:
        struct Buffer
        {
            GLuint glID = 0;
            GLuint count;
            GLsizei stride;
            GLsizei componentSize;
            GLenum componentType;

            const RasterizerMesh::Buffer& operator=(const Mesh::Buffer& mesh);
        };

        struct MorphTargets
        {
            std::vector<RasterizerMesh::Buffer> posDiffBuffers;
            std::vector<RasterizerMesh::Buffer> norDiffBuffers;
            std::vector<RasterizerMesh::Buffer> tanDiffBuffers;
        };

    public:
        RasterizerMesh();
        virtual ~RasterizerMesh();

        bool create(
            const std::vector<Mesh::Buffer>& positionBuffers,
            const std::vector<Mesh::Buffer>& uvBuffers,
            const std::vector<Mesh::Buffer>& normalBuffers,
            const std::vector<Mesh::Buffer>& tangentBuffers,
            const std::vector<Mesh::Buffer>& jointBuffers,
            const std::vector<Mesh::Buffer>& weightBuffers,
            const std::vector<Mesh::Buffer>& indexBuffers,
            const std::vector<Mesh::MorphTargets>& morphTargets);
        virtual bool draw(int index);
        virtual std::vector<bounds3>* getBounds() { return &submeshBounds; }
        virtual std::vector<uchar>* getTriangleData() { return &triangleData; }

    private:
        void deleteBuffer(Buffer* buffer);

    private:
        std::vector<Buffer> mPositionBuffers;
        std::vector<Buffer> mUvBuffers;
        std::vector<Buffer> mNormalBuffers;
        std::vector<Buffer> mTangentBuffers;
        std::vector<Buffer> mJointBuffers;
        std::vector<Buffer> mWeightBuffers;
        std::vector<Buffer> mElementBuffers;
        std::vector<MorphTargets> mMorphTargets;

        std::vector<bounds3> submeshBounds;
        std::vector<uchar> triangleData;
    };

    //
    // Uses the geometry from mSource as a source mesh but it can have material overrides
    //
    struct RasterizerMeshProxy : public RasterizerMeshBase
    {
    public:
        RasterizerMeshProxy();
        virtual ~RasterizerMeshProxy();

        bool create(RasterizerMesh* src);
        virtual bool draw(int index) { return mSource->draw(index); }
        virtual std::vector<bounds3>* getBounds() { return &mSource->submeshBounds; }
        virtual std::vector<uchar>* getTriangleData() { return &mSource->triangleData; }

    private:
        RasterizerMesh* mSource;
    };
}
