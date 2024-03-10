#include "MapViewer.h"

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

struct MapViewerPrivate
{
	CMapViewer* _this;

	//CRenderWidget* RenderWidget;///< renders the image to screen
	bool AutoFit;
	float MaxScale = 1.3f;
	float MinScale = 0.6f;
	//QSize ImageSize;///< stores the image size to detect image size changes
	QPoint MouseMoveStartPos;
	//QLabel* ScalingLabel;///< label displays scaling factor
	//QList<QWidget*> OverlayTools;///< list of tool widget to overlay



	QGraphicsScene* scene;
	QGraphicsEllipseItem* ellipse;
	QGraphicsRectItem* rectangle;
	QGraphicsTextItem* text;
	QGraphicsPixmapItem* pixmap;
	QGraphicsSvgItem* svgItem;

	QTimer* timer;
	int animationFPS = 30;

	MapViewerPrivate(CMapViewer* _public) : _this(_public) {}
};


CMapViewer::CMapViewer(QWidget* parent)
	: Super(parent),
	_this(new MapViewerPrivate(this))
{
	_this->AutoFit = true;
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
	setSceneRect(0, 0, 256, 256);
	//resetTransform();
	//setTransform(QTransform::fromScale(1.0f, 1.0f));
	//fitInView(_this->scene->sceneRect(), Qt::KeepAspectRatio);


	setRenderHint(QPainter::Antialiasing, true);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);



	//setRenderHint(QPainter::SmoothPixmapTransform, true); // Enable smooth zooming
	//setViewportUpdateMode(QGraphicsView::FullViewportUpdate);


	setDragMode(QGraphicsView::ScrollHandDrag);


	QBrush greenBrush(Qt::green);
	QBrush blueBrush(Qt::blue);
	QPen outlinePen(Qt::black);
	outlinePen.setWidth(2);

	//_this->rectangle = _this->scene->addRect(100, 0, 80, 100, outlinePen, blueBrush);

	//// addEllipse(x,y,w,h,pen,brush)
	//_this->ellipse = _this->scene->addEllipse(0, -100, 300, 60, outlinePen, greenBrush);

	//_this->text = _this->scene->addText("bogotobogo.com", QFont("Arial", 20));
	// movable text
	//_this->text->setFlag(QGraphicsItem::ItemIsMovable);




	//// Load the SVG file
	//QSvgRenderer* svgRenderer = new QSvgRenderer(QStringLiteral(":/adsdemo/images/panorama.svg"));
	//_this->svgItem = new QGraphicsSvgItem();
	//_this->svgItem->setSharedRenderer(svgRenderer);
	//auto defaultSize = svgRenderer->defaultSize();
	//_this->svgItem->setTransform(QTransform::fromScale(64.0f / defaultSize.width(), 64.0f / defaultSize.height()));
	//_this->scene->addItem(_this->svgItem);
	//if (svgRenderer->animated())
	//{
	//	svgRenderer->setFramesPerSecond(_this->animationFPS);
	//}

	// Set up a timer to call animationUpdate
	_this->timer = new QTimer(this);
	bool isOk = connect(_this->timer, SIGNAL(timeout()), this, SLOT(animationUpdate()));
	_this->timer->start(1000 / _this->animationFPS);
	assert(isOk);

}

CMapViewer::~CMapViewer()
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


void CMapViewer::createActions()
{
	QAction* a;

	a = new QAction(tr("&Open..."));
	a->setIcon(QIcon(":/adsdemo/images/perm_media.svg"));
	connect(a, &QAction::triggered, this, &CMapViewer::open);
	a->setShortcut(QKeySequence::Open);
	this->addAction(a);

	a = new QAction(tr("Fit on Screen"));
	a->setIcon(QIcon(":/adsdemo/images/zoom_out_map.svg"));
	connect(a, &QAction::triggered, this, &CMapViewer::fitToWindow);
	this->addAction(a);

	a = new QAction(tr("Actual Pixels"));
	a->setIcon(QIcon(":/adsdemo/images/find_in_page.svg"));
	connect(a, &QAction::triggered, this, &CMapViewer::normalSize);
	this->addAction(a);

	a = new QAction(this);
	a->setSeparator(true);
	this->addAction(a);

	a = new QAction(tr("Zoom In (25%)"));
	a->setIcon(QIcon(":/adsdemo/images/zoom_in.svg"));
	connect(a, &QAction::triggered, this, &CMapViewer::zoomIn);
	this->addAction(a);

	a = new QAction(tr("Zoom Out (25%)"));
	a->setIcon(QIcon(":/adsdemo/images/zoom_out.svg"));
	connect(a, &QAction::triggered, this, &CMapViewer::zoomOut);
	this->addAction(a);

	this->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void CMapViewer::open()
{
	renderObjectVisualization(1024, 512);

	//d->AutoFit = false;
	//d->RenderWidget->zoomIn();
}

void CMapViewer::zoomIn()
{
	//d->AutoFit = false;
	//d->RenderWidget->zoomIn();
}


void CMapViewer::zoomOut()
{
	//d->AutoFit = false;
	//d->RenderWidget->zoomOut();
}

void CMapViewer::normalSize()
{
	//d->AutoFit = false;
	//d->RenderWidget->normalSize();
}


void CMapViewer::fitToWindow()
{
	//d->AutoFit = true;
	//d->RenderWidget->scaleToSize(this->maximumViewportSize());
}

void CMapViewer::animationUpdate()
{
	//if (_this->svgItem->renderer()->animated())
	//	_this->svgItem->update();

}

void CMapViewer::onGameSegmentLoaded()
{
	renderObjectVisualization(1024, 512);
}

void CMapViewer::resizeEvent(QResizeEvent* e)
{
	Super::resizeEvent(e);
	if (_this->AutoFit)
	{
		setSceneRect(sceneRect().x(), sceneRect().y(), width(), height());
		//fitInView(_this->scene->sceneRect(), Qt::KeepAspectRatio);
	}
}

void CMapViewer::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton) 
	{
		setCursor(Qt::ClosedHandCursor);
		_this->MouseMoveStartPos = e->pos();
	}
	Super::mousePressEvent(e);
}

void CMapViewer::mouseReleaseEvent(QMouseEvent* e)
{
	//d->RenderWidget->setCursor(Qt::OpenHandCursor);
	Super::mouseReleaseEvent(e);
}

void CMapViewer::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::LeftButton) 
	{
		QPointF delta = mapToScene(_this->MouseMoveStartPos) - mapToScene(e->pos());
		setSceneRect(sceneRect().translated(delta));
		_this->MouseMoveStartPos = e->pos();
	}
	Super::mouseMoveEvent(e);
}

void CMapViewer::wheelEvent(QWheelEvent* e)
{
	int delta = e->angleDelta().y();
	qreal factor = 1.1;  // Adjust the zoom factor as needed

	if (delta > 0 && transform().m11() < _this->MaxScale)
	{
		// Zoom in
		scale(factor, factor);
	}
	else if (delta < 0 && transform().m11() > _this->MinScale)
	{
		// Zoom out
		scale(1.0 / factor, 1.0 / factor);
	}

	e->accept();
	//Super::wheelEvent(e);
}

void CMapViewer::onObjectVisualizationDone(const QImage* img)
{
	QImage* modifiedImage = const_cast<QImage*>(img);
	//for (int y = 0; y < modifiedImage->height(); ++y)
	//{
	//	for (int x = 0; x < modifiedImage->width(); ++x)
	//	{
	//		QColor pixelColor = modifiedImage->pixelColor(x, y);
	//		pixelColor.setAlphaF(std::min(pixelColor.alphaF() * 2.0, 1.0)); // increase the alpha
	//		//float lol = std::min(pixelColor.alphaF() * 2.0, 1.0);
	//		//pixelColor.setRgb(3 * lol, 184 * lol, 229 * lol);
	//		modifiedImage->setPixelColor(x, y, pixelColor);
	//	}
	//}
	
	// Set image
	auto pixMap = QPixmap::fromImage(*modifiedImage);
	_this->pixmap = _this->scene->addPixmap(pixMap);
	//_this->pixmap->setFlag(QGraphicsPixmapItem::ItemIgnoresTransformations, false);
	_this->pixmap->setTransformationMode(Qt::TransformationMode::SmoothTransformation);



	//QGraphicsColorizeEffect* brightnessEffect = new QGraphicsColorizeEffect;
	//brightnessEffect->setColor(QColor(255, 255, 255, 255));  // White color for brightness
	////brightnessEffect->setColor(QColor(3, 184, 229));  // White color for brightness
	////brightnessEffect->
	//brightnessEffect->setStrength(0.1);  // Adjust the strength for the desired brightness level
	//_this->pixmap->setGraphicsEffect(brightnessEffect);


	//QGraphicsOpacityEffect* multiplyAlphaEffect = new QGraphicsOpacityEffect;
	//multiplyAlphaEffect->setOpacity(50.5);  // Adjust the opacity for the desired alpha multiplication effect
	//_this->pixmap->setGraphicsEffect(multiplyAlphaEffect);

	//_this->pixmap->ant

	//d->RenderWidget->showImage(newImage);
	//this->adjustDisplaySize(newImage);
}
