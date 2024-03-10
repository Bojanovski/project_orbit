#include "ShaderProgramBase.h"
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QDebug>

#include <GL/gl.h>


using namespace graphics_engine;

std::uniform_real_distribution<float> ShaderProgramBase::sRandomFloats(0.0f, 1.0f);
std::default_random_engine ShaderProgramBase::sGenerator;

ShaderProgramBase::ShaderProgramBase()
    : mShaderProgram(0)
{}

ShaderProgramBase::~ShaderProgramBase()
{
    if (mShaderProgram != 0)
	{
		glDeleteProgram(mShaderProgram);
	}
}

bool ShaderProgramBase::create()
{
    initializeOpenGLFunctions();
	bool isOk = true;
	GLint success;
	GLchar infoLog[512];

	// Vertex
	const GLchar* vertexShaderSource = mVertexShaderSourceStr.c_str();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        qWarning() << "Error compiling vertex shader:\n" << infoLog;
		isOk = false;
	}

	// Fragmet
	const GLchar* fragmentShaderSource = mFragmentShaderSourceStr.c_str();
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        qWarning() << "Error compiling fragment shader:\n" << infoLog;
		isOk = false;
	}

	// Program
	mShaderProgram = glCreateProgram();
	glAttachShader(mShaderProgram, vertexShader);
	glAttachShader(mShaderProgram, fragmentShader);
	glLinkProgram(mShaderProgram);

	glGetProgramiv(mShaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(mShaderProgram, 512, NULL, infoLog);
        qWarning() << "Error linking shader program : \n" << infoLog;
		isOk = false;
	}

	glDetachShader(mShaderProgram, vertexShader);
	glDetachShader(mShaderProgram, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return isOk;
}

void ShaderProgramBase::bind()
{
	glUseProgram(mShaderProgram);
    mTextureUnitCounter = 0;
}

void ShaderProgramBase::unbind()
{
	glUseProgram(0);
}

int ShaderProgramBase::getUniformID(const char* name)
{   
    return glGetUniformLocation(mShaderProgram, name);
}

void ShaderProgramBase::setUniformValue_Matrix4x4(int uniformId, const mat4x4 &m)
{
    if (uniformId == -1) return;
    glUniformMatrix4fv(uniformId, 1, GL_FALSE, m.data());
}

void ShaderProgramBase::setUniformValue_Matrix4x4Array(int uniformId, const std::vector<mat4x4>& a)
{
	if (uniformId == -1) return;
	for (size_t i = 0; i < a.size(); ++i)
	{
		setUniformValue_Matrix4x4(uniformId + (int)i, a[i]);
	}
}

void ShaderProgramBase::setUniformValue_Transform(int uniformId, const transform& t)
{
    if (uniformId == -1) return;
    glUniformMatrix4fv(uniformId, 1, GL_FALSE, t.data());
}

void ShaderProgramBase::setUniformValue_Vector2(int uniformId, const vec2 &v)
{
    if (uniformId == -1) return;
    glUniform2fv(uniformId, 1, &v[0]);
}

void ShaderProgramBase::setUniformValue_Vector3(int uniformId, const vec3& v)
{
    if (uniformId == -1) return;
    glUniform3fv(uniformId, 1, &v[0]);
}

void ShaderProgramBase::setUniformValue_Vector3Array(int uniformId, const std::vector<vec3> &a)
{
    if (uniformId == -1) return;
    for (size_t i = 0; i < a.size(); ++i)
    {
        setUniformValue_Vector3(uniformId + (int)i, a[i]);
    }
}

void ShaderProgramBase::setUniformValue_Color(int uniformId, const color &c)
{
    if (uniformId == -1) return;
    glUniform4fv(uniformId, 1, c.data());
}

void ShaderProgramBase::setUniformValue_Int(int uniformId, int v)
{
    if (uniformId == -1) return;
    glUniform1i(uniformId, v);
}

void ShaderProgramBase::setUniformValue_Float(int uniformId, float v)
{
    if (uniformId == -1) return;
    glUniform1f(uniformId, v);
}

void ShaderProgramBase::setUniformValue_FloatArray(int uniformId, const std::vector<float> &a)
{
    if (uniformId == -1) return;
    glUniform1fv(uniformId, (GLsizei)a.size(), &a[0]);
}

void ShaderProgramBase::setUniformValue_Texture(int uniformId, const RasterizerImage* i, int index)
{
    if (uniformId == -1) return;
    assert(mTextureUnitCounter < 32);
    int textureUnitIndex = mTextureUnitCounter;
    GLenum textureUnit = GL_TEXTURE0 + textureUnitIndex;
    ++mTextureUnitCounter;
    if (i != nullptr)
    {
        glActiveTexture(textureUnit);
		glBindTexture(i->getGLType(), i->getGLID(index));
        //// Set texture minification and magnification filters to GL_LINEAR
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        setUniformValue_Int(uniformId, textureUnitIndex);
    }
}
