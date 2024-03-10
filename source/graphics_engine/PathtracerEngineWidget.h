#pragma once
#include <memory>
#include <QObject>
#include <QOpenGLWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QOpenGLDebugLogger>
#include <PathtracerEngine.h>


namespace graphics_engine
{
    class PathtracerEngineWidget : public QWidget
    {
        Q_OBJECT

    public:
        PathtracerEngineWidget(QWidget *parent = nullptr);
        ~PathtracerEngineWidget();

        PathtracerEngineInterface* getInterface() { return &mCyclesEngine; }

    signals:
        void engineReady(void*);

    protected:
        void cleanup();
        virtual void resizeEvent(QResizeEvent*) override;
        virtual void paintEvent(QPaintEvent*) override;
        virtual void showEvent(QShowEvent*) override;
        void update();

    private:
        QTimer mTimer;
        QElapsedTimer mElapsedTimer;
        //QColor mBaseColor;
        PathtracerEngine mCyclesEngine;
        bool mInitialized = false;
    };
}
