#pragma once
#include "GameWidget.h"
#include <QGraphicsView>


QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE


struct StarSystemViewerPrivate;

class CStarSystemViewer : public QGraphicsView, public game_core::GameWidget
{
	Q_OBJECT

public:
	using Super = QGraphicsView;

	explicit CStarSystemViewer(QWidget* parent = nullptr);
	virtual ~CStarSystemViewer();

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

private:
	void createActions();

	StarSystemViewerPrivate* _this;
	friend StarSystemViewerPrivate;
};
