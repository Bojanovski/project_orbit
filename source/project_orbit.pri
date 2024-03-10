
CONFIG(debug, debug|release){
    win32-g++ {
    	versionAtLeast(QT_VERSION, 5.15.0) {
                LIBS += -lqtadvanceddocking
                LIBS += -ldarkstyle
                LIBS += -lgraphicsengine
                LIBS += -lgamecore
                LIBS += -lcycleswrapper
        }
    	else {
    		LIBS += -lqtadvanceddockingd
                LIBS += -ldarkstyled
                LIBS += -lgraphicsengined
                LIBS += -lgamecored
                LIBS += -lcycleswrapperd
        }
    }
    else:msvc {
        LIBS += -lqtadvanceddockingd
        LIBS += -ldarkstyled
        LIBS += -lgraphicsengined
        LIBS += -lgamecored
        LIBS += -lcycleswrapperd
    }
    else:mac {
        LIBS += -lqtadvanceddocking_debug
        LIBS += -ldarkstyle_debug
        LIBS += -lgraphicsengine_debug
        LIBS += -lgamecore_debug
        LIBS += -lcycleswrapper_debug
    }
    else {
        LIBS += -lqtadvanceddocking
        LIBS += -ldarkstyle
        LIBS += -lgraphicsengine
        LIBS += -lgamecore
        LIBS += -lcycleswrapper
    }
}
else{
    LIBS += -lqtadvanceddocking
    LIBS += -ldarkstyle
    LIBS += -lgraphicsengine
    LIBS += -lgamecore
    LIBS += -lcycleswrapper
}


unix:!macx {
    LIBS += -lxcb
    #LIBS += -ldarkstyle
}
