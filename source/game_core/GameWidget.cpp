#include "GameWidget.h"
#include <GameInstance.h>


using namespace game_core;

void GameWidget::renderObjectVisualization(uint w, uint h)
{
	auto gi = game_core::GameInstance::getGameInstance();
	gi->renderObjectVisualization(this, w, h);
}

