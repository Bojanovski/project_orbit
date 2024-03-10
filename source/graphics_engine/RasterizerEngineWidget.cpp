#include <RasterizerEngineWidget.h>
#include <QApplication>
#include <QStyle>
#include <QDebug>
#include <QtMath>
#include <QMouseEvent>
#include <QOpenGLFunctions>


using namespace graphics_engine;

RasterizerEngineWidget::RasterizerEngineWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , mLogger(this)
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapInterval(1);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    //format.setProfile(QSurfaceFormat::CoreProfile);
    //format.setVersion(3,0);
    format.setOption(QSurfaceFormat::DebugContext);
    setFormat(format);

    setUpdateBehavior(UpdateBehavior::NoPartialUpdate);
    //setUpdatesEnabled()

    //setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    //showFullScreen();

    setMouseTracking(true);
}

RasterizerEngineWidget::~RasterizerEngineWidget()
{
    cleanup();
}

void RasterizerEngineWidget::initializeGL()
{
    QOpenGLWidget::initializeGL();

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    mLogger.initialize();
    qDebug () << (const char*)ctx->functions()->glGetString(GL_VERSION);
    qDebug () << (const char*)ctx->functions()->glGetString(GL_EXTENSIONS);

    mGraphicsEngine.initialize(this);
    auto baseColor = QApplication::palette().color(QPalette::Base);
    mGraphicsEngine.setBaseColor(baseColor.red(), baseColor.green(), baseColor.blue());

    mElapsedTimer.start();
    connect(this, &QOpenGLWidget::frameSwapped, this, &RasterizerEngineWidget::update);

    // Let the listeners know that the engine has been initialized
    emit engineReady(&mGraphicsEngine);
}

void RasterizerEngineWidget::cleanup()
{
    makeCurrent();
    mGraphicsEngine.deinitialize();
    doneCurrent();
}

void RasterizerEngineWidget::resizeGL(int w, int h)
{
    QOpenGLWidget::resizeGL(w, h);
    mGraphicsEngine.resize(w, h);
}

void RasterizerEngineWidget::update()
{
    // Update the engine
    auto elapsed_miliseconds = mElapsedTimer.restart();
    float dt = elapsed_miliseconds / 1000.0f;
    //qDebug() << "Update took " << elapsed_miliseconds << " ms";
    mGraphicsEngine.update(dt);

    // Update the base
    QWidget::update();
}

void RasterizerEngineWidget::paintGL()
{
    QOpenGLWidget::paintGL();
    mGraphicsEngine.draw();

    const QList<QOpenGLDebugMessage> messages = mLogger.loggedMessages();
    for (const QOpenGLDebugMessage &message : messages)
    {
        qDebug() << message;
    }
}

void RasterizerEngineWidget::enterEvent(QEvent* e)
{
    mGraphicsEngine.mMS.inside = true;
    QWidget::enterEvent(e);
}

void RasterizerEngineWidget::mouseMoveEvent(QMouseEvent* e)
{
    float halfWidth = 0.5f * width();
    float halfHeight = 0.5f * height();
    float mousePosX_NDC = (e->x() - halfWidth) / halfWidth;
    float mousePosY_NDC = (e->y() - halfHeight) / halfHeight;
    mousePosY_NDC *= -1.0f; // flip the y
    mGraphicsEngine.mMS.x = mousePosX_NDC;
    mGraphicsEngine.mMS.y = mousePosY_NDC;
    QWidget::mouseMoveEvent(e);
}

void RasterizerEngineWidget::mousePressEvent(QMouseEvent* e)
{
    switch (e->button())
    {
    case Qt::LeftButton:
        mGraphicsEngine.mMS.lmb = true;
        break;
    case Qt::RightButton:
        mGraphicsEngine.mMS.rmb = true;
        break;
    default:
        break;
    }
    QWidget::mousePressEvent(e);
}

void RasterizerEngineWidget::mouseReleaseEvent(QMouseEvent* e)
{
    switch (e->button())
    {
    case Qt::LeftButton:
        mGraphicsEngine.mMS.lmb = false;
        break;
    case Qt::RightButton:
        mGraphicsEngine.mMS.rmb = false;
        break;
    default:
        break;
    }
    QWidget::mouseReleaseEvent(e);
}

void RasterizerEngineWidget::wheelEvent(QWheelEvent* e)
{
    mGraphicsEngine.mMS.wheel += e->angleDelta().y(); // vertical mouse wheel
    QWidget::wheelEvent(e);
}

void RasterizerEngineWidget::leaveEvent(QEvent* e)
{
    mGraphicsEngine.mMS.inside = false;
    QWidget::leaveEvent(e);
}

void RasterizerEngineWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    emit mouseDoubleClick();
    QWidget::mouseDoubleClickEvent(e);
}
