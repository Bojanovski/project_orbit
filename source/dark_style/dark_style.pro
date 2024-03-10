
DARK_STYLE_OUT_ROOT = $${OUT_PWD}/..
CONFIG += c++14
CONFIG +=   debug_and_release \
            staticlib
TARGET = $$qtLibraryTarget(darkstyle)
DEFINES += QT_DEPRECATED_WARNINGS
TEMPLATE = lib
DESTDIR = $${DARK_STYLE_OUT_ROOT}/out
QT += core gui widgets


RESOURCES   += resources/darkstyle.qrc

HEADERS     += DarkStyle.h \
               WinDark.h

SOURCES     += DarkStyle.cpp \
               WinDark.cpp

headers.path=$$PREFIX/include
headers.files=$$HEADERS
target.path=$$PREFIX/out
INSTALLS += headers target

DISTFILES +=
