GRAPHICS_ENGINE_OUT_ROOT = $${OUT_PWD}/..
GAME_CORE_OUT_ROOT = $${OUT_PWD}/..
CONFIG += c++14
CONFIG +=   debug_and_release \
            staticlib
TARGET = $$qtLibraryTarget(gamecore)
DEFINES += QT_DEPRECATED_WARNINGS
TEMPLATE = lib
DESTDIR = $${GAME_CORE_OUT_ROOT}/out
QT += core


HEADERS     += \
            GameCoreTypes.h \
            GameInstance.h

SOURCES     += \
            GameInstance.cpp 


LIBS += \
        -lOpengl32 \
        -L$${ADS_OUT_ROOT}/out \
        -L$${DARK_STYLE_OUT_ROOT}/out \
        -L$${GRAPHICS_ENGINE_OUT_ROOT}/out \
        -L$${GAME_CORE_OUT_ROOT}/out


INCLUDEPATH += \
            ../eigen \
            ../graphics_engine \
            ../cycles_wrapper

DEPENDPATH += \
            ../graphics_engine


headers.path=$$PREFIX/include
headers.files=$$HEADERS
target.path=$$PREFIX/out
INSTALLS += headers target

DISTFILES +=
