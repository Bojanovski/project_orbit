#pragma once
#include <QImage>


namespace game_core
{
	class GameWidget
	{
	public:
		void renderObjectVisualization(uint w, uint h);

		virtual void onObjectVisualizationDone(const QImage*) {}
	};
}