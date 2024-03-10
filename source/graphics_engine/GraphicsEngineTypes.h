#pragma once

#include <vector>
#include <set>
#include <map>
#include <memory>
#include <cassert>
#include <Eigen/Dense>

#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>

#define M_PI_F ((float)M_PI)

#define TYPE_BYTE                           0x1400
#define TYPE_UNSIGNED_BYTE                  0x1401
#define TYPE_SHORT                          0x1402
#define TYPE_UNSIGNED_SHORT                 0x1403
#define TYPE_INT                            0x1404
#define TYPE_UNSIGNED_INT                   0x1405
#define TYPE_FLOAT                          0x1406
#define TYPE_2_BYTES                        0x1407
#define TYPE_3_BYTES                        0x1408
#define TYPE_4_BYTES                        0x1409
#define TYPE_DOUBLE                         0x140A

namespace graphics_engine
{
    // Generic types
    typedef void* Handle;
    typedef unsigned char uchar;
    typedef unsigned short ushort;
    typedef unsigned int uint;
    typedef unsigned int ulong;
    typedef Eigen::Vector2f vec2;
    typedef Eigen::Vector3f vec3;
    typedef Eigen::Vector4f vec4;
    typedef Eigen::Quaternionf quat;
    typedef Eigen::Vector4f color;
    typedef Eigen::Matrix4f mat4x4;
    typedef Eigen::Affine3f transform;
    typedef Eigen::Vector3i vec3i;
    typedef Eigen::Vector4i vec4i;
    typedef Eigen::Hyperplane<float, 3> plane3;
    struct bounds2 
    { 
        void update(const vec2& p)
        {
            for (int i = 0; i < 2; i++)
            {
                min[i] = std::min<float>(min[i], p[i]);
                max[i] = std::max<float>(max[i], p[i]);
            }
        }

        vec2 min, max; 
    };
    struct bounds3 
    {
        void update(const vec3& p)
        {
            for (int i = 0; i < 3; i++)
            {
                min[i] = std::min<float>(min[i], p[i]);
                max[i] = std::max<float>(max[i], p[i]);
            }
        }

        vec3 min, max;
    };

    struct ray3 
    {
        vec3 getPosition(float t) const { return origin + direction * t; }
        void transform(const mat4x4& m)
        { 
            origin = (m * origin.homogeneous()).head<3>();
            direction = m.block<3, 3>(0, 0) * direction;
        }

        vec3 origin; 
        vec3 direction;
    };

    struct Value
    {
        enum class Type { Null, Bool, Integer, Real, String, Binary, Array, Object };
        typedef std::vector<Value> Array;
        typedef std::map<std::string, Value> Object;

        Value() = default;
        explicit Value(bool b) : type(Type::Bool) { boolean_val = b; }
        explicit Value(int i) : type(Type::Integer) { int_val = i; real_val = i; }
        explicit Value(double n) : type(Type::Real) { real_val = n; }
        explicit Value(const std::string& s) : type(Type::String) { string_val = s; }
        explicit Value(const std::vector<unsigned char>& v) : type(Type::Binary) { binary_val = v; }
        explicit Value(const Array& a) : type(Type::Array) { array_val = a; }
        explicit Value(const Object& o) : type(Type::Object) { object_val = o; }

        Type getType() const { return type; }
        template <typename T> const T& get() const;
        const Value& get(int idx) const
        {
            static Value null_value;
            assert((type == Type::Array) && idx >= 0);
            return (static_cast<size_t>(idx) < array_val.size()) ? array_val[static_cast<size_t>(idx)] : null_value;
        }
        const Value& get(const std::string& key) const
        {
            static Value null_value;
            assert(type == Type::Object);
            Object::const_iterator it = object_val.find(key);
            return (it != object_val.end()) ? it->second : null_value;
        }
        size_t arrayLen() const
        {
            if (type != Type::Array) return 0;
            return array_val.size();
        }
        bool has(const std::string& key) const
        {
            if (type != Type::Object) return false;
            auto it = object_val.find(key);
            return (it != object_val.end()) ? true : false;
        }
        std::vector<std::string> keys() const
        {
            std::vector<std::string> keys;
            if (type != Type::Object) return keys;  // empty
            for (auto it = object_val.begin(); it != object_val.end(); ++it) keys.push_back(it->first);
            return keys;
        }

        Array* getArray() { return &array_val; }
        Object* getObject() { return &object_val; }

    private:
        Type type = Type::Null;
        bool boolean_val = false;
        int int_val = 0;
        double real_val = 0.0;
        std::string string_val;
        std::vector<unsigned char> binary_val;
        Array array_val;
        Object object_val;
    };

    template <> inline const bool& Value::get<bool>() const { return boolean_val; }
    template <> inline const int& Value::get<int>() const { return int_val; }
    template <> inline const double& Value::get<double>() const { return real_val; }
    template <> inline const std::string& Value::get<std::string>() const { return string_val; }
    template <> inline const std::vector<unsigned char>& Value::get<std::vector<unsigned char>>() const { return binary_val; }
    template <> inline const Value::Array& Value::get<Value::Array>() const { return array_val; }
    template <> inline const Value::Object& Value::get< Value::Object>() const { return object_val; }

    struct Triangle
    {
        static Triangle fromPoints(vec3 v[3])
        {
            Triangle t;
            t.v0 = v[0];
            t.e1 = v[1] - v[0];
            t.e2 = v[2] - v[0];
            t.n = t.e1.cross(t.e2);
            t.n.normalize();
            return t;
        }

        vec3 v0;
        vec3 e1;
        vec3 e2;
        vec3 n;
    };

    struct SkinnedTriangle
    {
        vec4 v[3];
        float w[3][4];
        ushort j[3][4];
        uint wCount[3];
    };

    // Model
    enum class CameraType { Perspective, Orthographic, Panoramic, None };
    enum class LightType { Directional, Spot, Point, None };
    
    struct Node;

    struct RayCastHit
    {
        explicit operator bool() const
        {
            return (node != nullptr);
        }

        Node* node = nullptr;
        float distance = FLT_MAX;
    };

    struct Animation
    {
        enum class InterpolationType { Linear, Step, CubicSpline, UserDefined };
        enum class PathType { Translation, Rotation, Scale, Weights };

        struct Sampler
        {
            InterpolationType interpolation;
            std::map<float, uint> input;
            std::vector<uchar> output;
        };

        struct Channel
        {
            Sampler* sampler;
            PathType targetPath;
        };

        std::string name;
        std::vector<Channel> channels;
        std::vector<Sampler> samplers;
        float inputMin, inputMax, inputDuration;
    };

    struct AnimationTarget
    {
        Animation* animation;
        std::vector<Node*> nodes;
    };

    struct Skin
    {
        std::vector<mat4x4> inverseBindMatrices;
        std::vector<Node*> joints;
        Node* skeleton;
    };

    struct Mesh
    {
        struct Buffer
        {
            uchar* data = nullptr;
            uint count;
            int stride;
            int componentSize;
            uint componentType;
        };

        struct MorphTargets
        {
            std::vector<Buffer> posDiffBuffers;
            std::vector<Buffer> norDiffBuffers;
            std::vector<Buffer> tanDiffBuffers;
        };

        struct Variant
        {
            int index;
            Mesh* instance;
        };

        virtual ~Mesh() {}
        virtual std::vector<bounds3>* getBounds() = 0;
        virtual std::vector<uchar>* getTriangleData() = 0;

        std::string name;
        std::vector<struct Material*> materials;
        bool skinned;
        uint morphTargetCount;

        std::vector<std::string> morphTargetNames;

        bool uses_KHR_materials_variants = false;
        std::vector<Variant> variants;
    };

    struct NodeContainer
    {
        virtual ~NodeContainer() = 0 {}
        virtual transform getLocalTransform() const { return transform::Identity(); }
        virtual transform computeWorldTransform() const { return transform::Identity(); }
    };

    struct Scene : public NodeContainer
    {
        std::string name;
        std::vector<Node*> nodes;
        Value extra;
    };

    struct Light {};

    struct Node : public NodeContainer
    {
        transform getLocalTransform() const override
        {
            transform ret = transform::Identity();
            ret.translate(localPosition);
            ret.rotate(localRotation);
            ret.scale(localScale);
            return ret;
        }

        transform computeWorldTransform() const override
        {
            auto parentNode = dynamic_cast<Node*>(belongsTo);
            transform parentWorld = parentNode ? parentNode->computeWorldTransform() : transform::Identity();
            return parentWorld * getLocalTransform();
        }

        std::string name;
        NodeContainer* belongsTo = nullptr;
        std::vector<Node*> children;
        vec3 localPosition;
        vec3 localScale;
        quat localRotation;
        bool visible = true;
        Mesh* mesh;

        // The skin referenced by this node.
        Skin* skin;

        // The weights of the instantiated morph target.
        // The number of array elements MUST match the number of morph targets of the referenced mesh.
        // When defined, mesh MUST also be defined.
        std::vector<float> weights;

        // Extra data
        Value extra;
    };

    struct Image
    {
        virtual ~Image() = 0 {}

        virtual uint getWidth() const = 0;
        virtual uint getHeight() const = 0;
        virtual int getCount() const = 0;

        bool mLinearSpace;
        std::string name;
    };

    struct Texture
    {
        bool isValid() const { return image != nullptr; }
        bool inLinearSpace() const { return image->mLinearSpace; }

        Image* image = nullptr;
        //Sampler sampler;
        std::string name;
    };

    struct TextureTransform
    {
        vec2 Offset = vec2(0.0f, 0.0f);
        float Rotation = 0.0f;
        vec2 Scale = vec2(1.0f, 1.0f);
    };

    struct Material
    {
        std::string name;

        color albedoColor;
        Texture albedoTexture;

        float metallicFactor;
        float roughnessFactor;
        Texture metallicRoughnessTexture;

        float normalScale;
        Texture normalTexture;

        color emissiveFactor;
        Texture emissiveTexture;

        bool uses_KHR_materials_emissive_strength = false;
        float emissiveStrength;

        bool uses_KHR_materials_transmission = false;
        float transmissionFactor;
        Texture transmissionTexture;

        bool Uses_KHR_texture_transform = false;
        TextureTransform albedoTransform;
        TextureTransform metallicRoughnessTransform;
        TextureTransform normalTransform;
        TextureTransform occlusionTransform;
        TextureTransform emissiveTransform;
        TextureTransform transmissionTransform;
    };

    class LoadedFile
    {
    public:
        virtual ~LoadedFile() {}
        const std::wstring& getFilepath() const { return mFilePath; }

    protected:
        std::wstring mFilePath;
    };

    struct Model
    {
        std::vector<Scene*> scenes;
        std::vector<AnimationTarget> animationTargets;

        bool uses_KHR_materials_variants = false;
        std::vector<std::string> variants;

        Value extra;

        std::unique_ptr<LoadedFile> loadedFile;
    };
}
