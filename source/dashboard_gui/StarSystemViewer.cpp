#include "StarSystemViewer.h"

#include <math.h>

#include <QLabel>
#include <QImageReader>
#include <QImageWriter>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>
#include <QAction>
#include <QScrollBar>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QMouseEvent>
#include <QWheelEvent>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsColorizeEffect>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QPropertyAnimation>
#include <QTimer>


#include "RenderWidget.h"

class AstronomicalObject
{
public:
	virtual void setViewScale(qreal scale) = 0;
};

class SphericalObject : public AstronomicalObject
{
public:
	void createSun(QGraphicsScene* scene, qreal radius)
	{
		assert(mBodyEllipse == nullptr);
		assert(mHitAreaEllipse == nullptr);
		assert(mTrajectory == nullptr);

		mCenter = QPointF(0.0, 0.0);
		mRadius = radius;
		mColor = Qt::white;

		// body
		QBrush fillBrush(mColor);
		QPen outlinePen(mColor);
		outlinePen.setWidth(0);
		mBodyEllipse = scene->addEllipse(mCenter.x() - radius, mCenter.y() - radius, radius * 2.0, radius * 2.0, outlinePen, fillBrush);
		// hit area
		QBrush hitAreaFillBrush(Qt::transparent);
		qreal hitAreaRadius = 8.0; // screen space
		QPen hitAreaOutlinePen(mColor);
		hitAreaOutlinePen.setWidth(2);
		mHitAreaEllipse = scene->addEllipse(-hitAreaRadius, -hitAreaRadius, hitAreaRadius * 2.0, hitAreaRadius * 2.0, hitAreaOutlinePen, hitAreaFillBrush);
		mHitAreaEllipse->resetTransform();
	}

	void createPlanet(QGraphicsScene* scene, const QPointF& center, qreal radius, const QColor& fillColor)
	{
		assert(mBodyEllipse == nullptr);
		assert(mHitAreaEllipse == nullptr);
		assert(mTrajectory == nullptr);

		mCenter = center;
		mRadius = radius;
		mColor = fillColor;

		// body
		QBrush fillBrush(mColor);
		QPen outlinePen(mColor);
		outlinePen.setWidth(0);
		mBodyEllipse = scene->addEllipse(mCenter.x() - radius, mCenter.y() - radius, radius * 2.0, radius * 2.0, outlinePen, fillBrush);
		// hit area
		qreal hitAreaRadius = 8.0; // screen space
		QPen hitAreaOutlinePen(mColor);
		hitAreaOutlinePen.setWidth(2);
		mHitAreaEllipse = scene->addEllipse(-hitAreaRadius, -hitAreaRadius, hitAreaRadius * 2.0, hitAreaRadius * 2.0, hitAreaOutlinePen, QBrush(Qt::transparent));
		mHitAreaEllipse->resetTransform();
		// trajectory







		QPen trajectoryPen(Qt::transparent);
		trajectoryPen.setColor(Qt::green);
		trajectoryPen.setWidth(2);



		QPainterPath path;
		// Move to a starting point
		path.moveTo(16, 0);


		// Add a line to another point
		//path.lineTo(8, 0);
		//// Add a quadratic Bezier curve
		//path.quadTo(200, 100, 150, 150);


		// Add an elliptical arc
		path.arcTo(0.0 - 32 * 0.5, 0.0 - 32 * 0.5, 32, 32, 0.0, 360.0);


		mTrajectory = scene->addPath(path, trajectoryPen, QBrush(Qt::transparent));
	}

	virtual void setViewScale(qreal scale)
	{
		mHitAreaEllipse->setTransform(QTransform::fromScale(1.0 / scale, 1.0 / scale) * QTransform::fromTranslate(mCenter.x(), mCenter.y()));

		if (mTrajectory)
		{
			QPen hitAreaOutlinePen(mColor);
			hitAreaOutlinePen.setWidth(2.0 / scale);
			//mTrajectory->setPen(hitAreaOutlinePen);
			mTrajectory->setTransform(QTransform::fromScale(1.0 / scale, 1.0 / scale));
		}
	}


private:
	QPointF mCenter;
	qreal mRadius;
	QColor mColor;
	QGraphicsEllipseItem* mBodyEllipse = nullptr;
	QGraphicsEllipseItem* mHitAreaEllipse = nullptr;
	QGraphicsPathItem* mTrajectory = nullptr;
};

struct StarSystemViewerPrivate
{
	CStarSystemViewer* _this;

	qreal MaxScale = 1.0f;
	qreal MinScale = 1.0f;
	//QSize ImageSize;///< stores the image size to detect image size changes
	QPoint MouseMoveStartPos;
	//QLabel* ScalingLabel;///< label displays scaling factor
	//QList<QWidget*> OverlayTools;///< list of tool widget to overlay



	QGraphicsScene* scene;
	SphericalObject sun;
	SphericalObject mars;

	QTimer* timer;
	int animationFPS = 30;

	StarSystemViewerPrivate(CStarSystemViewer* _public) : _this(_public) {}
};


CStarSystemViewer::CStarSystemViewer(QWidget* parent)
	: Super(parent),
	_this(new StarSystemViewerPrivate(this))
{
	//d->RenderWidget = new CRenderWidget(this);

	//this->setBackgroundRole(QPalette::Base);
	//this->setAlignment(Qt::AlignCenter);
	//this->setWidget(d->RenderWidget);
	this->createActions();
	//this->setMouseTracking(false); // only produce mouse move events if mouse button pressed



	// Disable vertical and horizontal scroll bars
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);



	_this->scene = new QGraphicsScene(this);
	setScene(_this->scene);
	qreal Km = 1000.0;
	qreal MKm = 1000000.0 * Km;
	qreal BKm = 1000.0 * MKm;
	_this->MinScale = 512.0 / (3.0 * BKm);
	qreal w = (qreal)width();
	qreal h = (qreal)height();
	setSceneRect(-w * 0.5, -h * 0.5, w, h);
	setRenderHint(QPainter::Antialiasing, true);
	qreal scaleFac = 2.0e-10;
	setTransform(QTransform::fromScale(scaleFac, scaleFac));
	//setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

	//scale(2.0 / _this->MinScale, 2.0 / _this->MinScale);


	//setRenderHint(QPainter::SmoothPixmapTransform, true); // Enable smooth zooming
	//setViewportUpdateMode(QGraphicsView::FullViewportUpdate);


	setDragMode(QGraphicsView::ScrollHandDrag);



	//QPointF p;
	////_this->sun.create(_this->scene, p, 696340.0 * Km, Qt::white);
	//_this->sun.create(_this->scene, p, 200.0, Qt::white);
	////p.setX(21300.48 * Km);
	//p.setX(300.0);
	//_this->mars.create(_this->scene, p, 100.0, Qt::red);



	_this->sun.createSun(_this->scene, 696340.0 * Km);
	QPointF p;
	p.setX(213.48 * MKm);
	_this->mars.createPlanet(_this->scene, p, 3389.5 * Km, Qt::red);


	_this->sun.setViewScale(transform().m11());
	_this->mars.setViewScale(transform().m11());



	// Set up a timer to call animationUpdate
	_this->timer = new QTimer(this);
	bool isOk = connect(_this->timer, SIGNAL(timeout()), this, SLOT(animationUpdate()));
	_this->timer->start(1000 / _this->animationFPS);
	assert(isOk);

}

CStarSystemViewer::~CStarSystemViewer()
{
	delete _this;
}


static void initializeImageFileDialog(QFileDialog& dialog, QFileDialog::AcceptMode acceptMode)
{
	static bool firstDialog = true;

	if (firstDialog) {
		firstDialog = false;
		const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
		dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
	}

	QStringList mimeTypeFilters;
	const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
		? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
	for (const QByteArray& mimeTypeName : supportedMimeTypes)
		mimeTypeFilters.append(mimeTypeName);
	mimeTypeFilters.sort();
	dialog.setMimeTypeFilters(mimeTypeFilters);
	dialog.selectMimeTypeFilter("image/jpeg");
	if (acceptMode == QFileDialog::AcceptSave)
		dialog.setDefaultSuffix("jpg");
}


void CStarSystemViewer::createActions()
{
	QAction* a;

	a = new QAction(tr("&Open..."));
	a->setIcon(QIcon(":/adsdemo/images/perm_media.svg"));
	connect(a, &QAction::triggered, this, &CStarSystemViewer::open);
	a->setShortcut(QKeySequence::Open);
	this->addAction(a);

	a = new QAction(tr("Fit on Screen"));
	a->setIcon(QIcon(":/adsdemo/images/zoom_out_map.svg"));
	connect(a, &QAction::triggered, this, &CStarSystemViewer::fitToWindow);
	this->addAction(a);

	a = new QAction(tr("Actual Pixels"));
	a->setIcon(QIcon(":/adsdemo/images/find_in_page.svg"));
	connect(a, &QAction::triggered, this, &CStarSystemViewer::normalSize);
	this->addAction(a);

	a = new QAction(this);
	a->setSeparator(true);
	this->addAction(a);

	a = new QAction(tr("Zoom In (25%)"));
	a->setIcon(QIcon(":/adsdemo/images/zoom_in.svg"));
	connect(a, &QAction::triggered, this, &CStarSystemViewer::zoomIn);
	this->addAction(a);

	a = new QAction(tr("Zoom Out (25%)"));
	a->setIcon(QIcon(":/adsdemo/images/zoom_out.svg"));
	connect(a, &QAction::triggered, this, &CStarSystemViewer::zoomOut);
	this->addAction(a);

	this->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void CStarSystemViewer::open()
{
	//renderObjectVisualization(1024, 512);

	//d->AutoFit = false;
	//d->RenderWidget->zoomIn();
}

void CStarSystemViewer::zoomIn()
{
	//d->AutoFit = false;
	//d->RenderWidget->zoomIn();
}


void CStarSystemViewer::zoomOut()
{
	//d->AutoFit = false;
	//d->RenderWidget->zoomOut();
}

void CStarSystemViewer::normalSize()
{
	//d->AutoFit = false;
	//d->RenderWidget->normalSize();
}


void CStarSystemViewer::fitToWindow()
{
	//d->AutoFit = true;
	//d->RenderWidget->scaleToSize(this->maximumViewportSize());
}

void CStarSystemViewer::animationUpdate()
{
	//if (_this->svgItem->renderer()->animated())
	//	_this->svgItem->update();

}

void CStarSystemViewer::onGameSegmentLoaded()
{
	//renderObjectVisualization(1024, 512);
}

void CStarSystemViewer::resizeEvent(QResizeEvent* e)
{
	Super::resizeEvent(e);
	auto newWidth = (qreal)width();
	auto newHeight = (qreal)height();
	QRectF rect = sceneRect();
	qreal wDiff = newWidth - rect.width();
	qreal hDiff = newHeight - rect.height();
	setSceneRect(rect.x() - wDiff * 0.5, rect.y() - hDiff * 0.5, newWidth, newHeight);
}

void CStarSystemViewer::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		setCursor(Qt::ClosedHandCursor);
		_this->MouseMoveStartPos = e->pos();
	}
	Super::mousePressEvent(e);
}

void CStarSystemViewer::mouseReleaseEvent(QMouseEvent* e)
{
	//d->RenderWidget->setCursor(Qt::OpenHandCursor);
	Super::mouseReleaseEvent(e);
}

void CStarSystemViewer::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		QPointF delta = mapToScene(_this->MouseMoveStartPos) - mapToScene(e->pos());
		setSceneRect(sceneRect().translated(delta));
		_this->MouseMoveStartPos = e->pos();
	}
	Super::mouseMoveEvent(e);
}

void CStarSystemViewer::wheelEvent(QWheelEvent* e)
{
	int delta = e->angleDelta().y();
	qreal factor = 1.5;  // Adjust the zoom factor as needed
	qreal currentScale = transform().m11();

	if (delta > 0 && currentScale < _this->MaxScale)
	{
		// Zoom in
		scale(factor, factor);
	}
	else if (delta < 0 && currentScale > _this->MinScale)
	{
		// Zoom out
		scale(1.0 / factor, 1.0 / factor);
	}

	e->accept();
	//Super::wheelEvent(e);

	_this->sun.setViewScale(transform().m11());
	_this->mars.setViewScale(transform().m11());
}
