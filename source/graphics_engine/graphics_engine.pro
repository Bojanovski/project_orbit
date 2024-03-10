
GRAPHICS_ENGINE_OUT_ROOT = $${OUT_PWD}/..
CONFIG += c++14
CONFIG +=   debug_and_release \
            staticlib
TARGET = $$qtLibraryTarget(graphicsengine)
DEFINES += QT_DEPRECATED_WARNINGS
TEMPLATE = lib
DESTDIR = $${GRAPHICS_ENGINE_OUT_ROOT}/out
QT += core gui widgets opengl


HEADERS     += \
            BlurShaderProgram.h \
            Camera.h \
            GBuffer.h \
            GLTFModelLoader.h \
            GraphicsEngineInterface.h \
            GraphicsEngineTypes.h \
            GraphicsEngineWidget.h \
            GraphicsEngine.h \
            Material.h \
            Mesh.h \
            MeshDrawingShaderProgram.h \
            PBRShaderProgram.h \
            Primitive.h \
            RandomTexture.h \
            SceneManager.h \
            Texture2D.h \
            TextureBase.h \
            TextureDrawingShaderPrograms.h \
            RenderTarget.h \
            FullScreenQuad.h \
            ShaderProgramBase.h

SOURCES     += \
            GLTFModelLoader.cpp \
            ../mikktspace/mikktspace.c \
            GraphicsEngineWidget.cpp\
            GraphicsEngine.cpp \
            Mesh.cpp \
            RandomTexture.cpp \
            RenderTarget.cpp \
            SceneManager.cpp \
            ShaderProgramBase.cpp \
            Texture2D.cpp

LIBS        += \
            -lOpengl32

INCLUDEPATH += \
            ../eigen \
            ../tinygltf \
            ../mikktspace


headers.path=$$PREFIX/include
headers.files=$$HEADERS
target.path=$$PREFIX/out
INSTALLS += headers target

DISTFILES +=
