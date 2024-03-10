#pragma once
#include "GameWidget.h"
#include <QGraphicsView>


QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE


struct MapViewerPrivate;

class CMapViewer : public QGraphicsView, public game_core::GameWidget
{
	Q_OBJECT

public:
	using Super = QGraphicsView;

	explicit CMapViewer(QWidget* parent = nullptr);
	virtual ~CMapViewer();

public Q_SLOTS:
	void open();
	void zoomIn();
	void zoomOut();
	void normalSize();
	void fitToWindow();
	void animationUpdate();

	// game instance stuff
	void onGameSegmentLoaded();

protected:
	virtual void resizeEvent(QResizeEvent* ResizeEvent);
	virtual void mousePressEvent(QMouseEvent* Event);
	virtual void mouseReleaseEvent(QMouseEvent* Event);
	virtual void mouseMoveEvent(QMouseEvent* Event);
	virtual void wheelEvent(QWheelEvent* Event);

	virtual void onObjectVisualizationDone(const QImage*);

private:
	void createActions();

	MapViewerPrivate* _this;
	friend MapViewerPrivate;
};
