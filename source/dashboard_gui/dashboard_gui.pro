ADS_OUT_ROOT = $${OUT_PWD}/..
DARK_STYLE_OUT_ROOT = $${OUT_PWD}/..
GRAPHICS_ENGINE_OUT_ROOT = $${OUT_PWD}/..
GAME_CORE_OUT_ROOT = $${OUT_PWD}/..

TARGET = dashboard_gui
DESTDIR = $${ADS_OUT_ROOT}/out
QT += core gui widgets quick quickwidgets opengl

include(../project_orbit.pri)

lessThan(QT_MAJOR_VERSION, 6) {
    win32 {
        QT += axcontainer
    }
}

CONFIG += c++14
CONFIG += debug_and_release
DEFINES += QT_DEPRECATED_WARNINGS
RC_FILE += resources/app.rc

adsBuildStatic {
    DEFINES += ADS_STATIC
}


HEADERS += \
	MainWindow.h \
	StatusDialog.h \
	ImageViewer.h \
	RenderWidget.h

SOURCES += \
	main.cpp \
	MainWindow.cpp \
	StatusDialog.cpp \
	ImageViewer.cpp \
	RenderWidget.cpp

FORMS += \
	mainwindow.ui \
	StatusDialog.ui
	
RESOURCES += resources/dashboard_gui.qrc


LIBS += \
        -lOpengl32 \
        -L$${ADS_OUT_ROOT}/out \
        -L$${DARK_STYLE_OUT_ROOT}/out \
        -L$${GRAPHICS_ENGINE_OUT_ROOT}/out \
        -L$${GAME_CORE_OUT_ROOT}/out


INCLUDEPATH += \
            ../ads \
            ../dark_style \
            ../eigen \
            ../graphics_engine \
            ../cycles_wrapper \
            ../game_core

DEPENDPATH += \
            ../ads \
            ../dark_style \
            ../graphics_engine \
            ../cycles_wrapper \
            ../game_core
