#include <PathtracerEngineWidget.h>
#include <QOpenGLContext>
#include <QTimer>
#include <QDebug>
#include <qevent.h>
#include <mutex>


using namespace graphics_engine;

PathtracerEngineWidget::PathtracerEngineWidget(QWidget *parent)
    : QWidget(parent)
    , mInitialized(false)
{
    HWND hwnd = reinterpret_cast<HWND>(winId());
    mCyclesEngine.initialize(hwnd);

    mElapsedTimer.start();
    bool isOk = connect(&mTimer, &QTimer::timeout, this, &PathtracerEngineWidget::update);
    assert(isOk);
    mTimer.start(5);
}

PathtracerEngineWidget::~PathtracerEngineWidget()
{
    cleanup();
}

void PathtracerEngineWidget::cleanup()
{
    mTimer.stop();
    mCyclesEngine.deinitialize();
}

void PathtracerEngineWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    mCyclesEngine.resize(event->size().width(), event->size().height());
}

void PathtracerEngineWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (mInitialized)
    {
        mCyclesEngine.draw();
    }
}

void PathtracerEngineWidget::showEvent(QShowEvent*)
{
    if (!mInitialized)
    {
        mInitialized = true;

        // Let the listeners know that the gl engine has been initialized
        emit engineReady(&mCyclesEngine);
    }
}

void PathtracerEngineWidget::update()
{
    if (mInitialized)
    {
        auto elapsed_miliseconds = mElapsedTimer.restart();
        float delta_time = elapsed_miliseconds / 1000.0f;
        //qDebug() << "Update took " << elapsed_miliseconds << " ms";
        mCyclesEngine.update(delta_time);
        mCyclesEngine.draw();
    }
}
