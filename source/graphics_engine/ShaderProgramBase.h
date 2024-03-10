#pragma once
#include "GraphicsEngineTypes.h"
#include "RasterizerImage.h"
#include <QOpenGLFunctions>
#include <random>


namespace graphics_engine
{
    class ShaderProgramBase : protected QOpenGLFunctions
    {
    public:
        ShaderProgramBase();
        virtual ~ShaderProgramBase();

        virtual bool create();
        virtual void bind();
        virtual void unbind();
        
        bool isCreated() const { return (mShaderProgram > 0); }

    protected:
        int getUniformID(const char*);
        void setUniformValue_Matrix4x4(int, const mat4x4&);
        void setUniformValue_Matrix4x4Array(int, const std::vector<mat4x4>&);
        void setUniformValue_Transform(int, const transform&);
        void setUniformValue_Vector2(int, const vec2&);
        void setUniformValue_Vector3(int, const vec3&);
        void setUniformValue_Vector3Array(int, const std::vector<vec3>&);
        void setUniformValue_Color(int, const color&);
        void setUniformValue_Int(int, int);
        void setUniformValue_Float(int, float);
        void setUniformValue_FloatArray(int, const std::vector<float>&);
        void setUniformValue_Texture(int, const RasterizerImage*, int);

    protected:
        std::string mVertexShaderSourceStr;
        std::string mFragmentShaderSourceStr;
        int mTextureUnitCounter;

        static std::uniform_real_distribution<float> sRandomFloats;
        static std::default_random_engine sGenerator;

    private:
        GLuint mShaderProgram;
    };
}
