#pragma once
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QDebug>


namespace graphics_engine
{
    class FullScreenQuad : protected QOpenGLFunctions
    {
    public:
        FullScreenQuad()
            : mVBO(0)
        {

        }

        virtual ~FullScreenQuad()
        {
            destroy();
        }

        bool create()
        {
            destroy();
            initializeOpenGLFunctions();
            bool isOk = true;
            float vertices[] = {
                -1.0f, -1.0f, 0.0f,
                1.0f, -1.0f, 0.0f,
                -1.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 0.0f,
            };

            isOk &= mVAO.create();
            glGenBuffers(1, &mVBO);
            mVAO.bind();
            glBindBuffer(GL_ARRAY_BUFFER, mVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            mVAO.release();
            return isOk;
        }

        bool destroy()
        {
            int count = 0;
            if (mVAO.isCreated())
            {
                mVAO.destroy();
                count++;
            }
            if (mVBO != 0)
            {
                glDeleteBuffers(1, &mVBO);
                mVBO = 0;
                count++;
            }
            return (count == 2);
        }

        bool isCreated() const
        {
            return mVAO.isCreated();
        }

        bool draw()
        {
            mVAO.bind();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            mVAO.release();
            return true;
        }

    private:
        GLuint mVBO;
        QOpenGLVertexArrayObject mVAO;
    };
}
