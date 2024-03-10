#pragma once
#include "GameWidget.h"
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QImage>
#include <GameCoreTypes.h>
#include <GraphicsEngineInterface.h>


namespace game_core
{
    class GameInstance : public QObject
    {
        Q_OBJECT

    public:
        GameInstance(QObject *parent = nullptr);
        ~GameInstance();

        void initialize();
        void deinitialize();
        void setRasterizer(graphics_engine::RasterizerEngineInterface*);
        void setPathtracer(graphics_engine::PathtracerEngineInterface*);
        void loadGame(const std::wstring&);
        void renderObjectVisualization(GameWidget* widget, uint w, uint h);

        std::wstring getAssetsRootDir() const { return L"C:\\Projects\\project_orbit\\assets"; }
        PhysicsSimulator* getPhysicsSimulator();

        static GameInstance* getGameInstance();

    signals:
        void segmentLoaded();

    public slots:
        void onRasterizerMouseDoubleClick();

    private:
        void update();

    private:
        std::unique_ptr<class GameInstancePrivate> _this;
    };
}
