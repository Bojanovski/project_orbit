#pragma once
#include <QOpenGLWidget>
#include <QElapsedTimer>
#include <QOpenGLDebugLogger>
#include "RasterizerEngine.h"


namespace graphics_engine
{
    class RasterizerEngineWidget : public QOpenGLWidget
    {
        Q_OBJECT

    public:
        RasterizerEngineWidget(QWidget *parent = nullptr);
        ~RasterizerEngineWidget();

        RasterizerEngineInterface* getInterface() { return &mGraphicsEngine; }

    signals:
        void engineReady(void*);
        void mouseDoubleClick();

    protected:
        void initializeGL() override;
        void cleanup();
        void resizeGL(int w, int h) override;
        void update();
        void paintGL() override;
        void enterEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent*) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    private:
        QElapsedTimer mElapsedTimer;
        RasterizerEngine mGraphicsEngine;
        QColor mBaseColor;
        QOpenGLDebugLogger mLogger;
    };
}

