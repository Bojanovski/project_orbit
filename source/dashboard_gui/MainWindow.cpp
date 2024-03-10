
/*******************************************************************************
** Qt Advanced Docking System
** Copyright (C) 2017 Uwe Kindler
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


//============================================================================
/// \file   MainWindow.cpp
/// \author Uwe Kindler
/// \date   13.02.2018
/// \brief  Implementation of CMainWindow demo class
//============================================================================


//============================================================================
//                                   INCLUDES
//============================================================================
#include <MainWindow.h>
#include "ui_mainwindow.h"

#include <iostream>

#include <QTime>
#include <QLabel>
#include <QTextEdit>
#include <QCalendarWidget>
#include <QFrame>
#include <QTreeView>
#include <QFileSystemModel>
#include <QBoxLayout>
#include <QSettings>
#include <QDockWidget>
#include <QDebug>
#include <QResizeEvent>
#include <QAction>
#include <QWidgetAction>
#include <QComboBox>
#include <QInputDialog>
#include <QRubberBand>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QScreen>
#include <QStyle>
#include <QMessageBox>
#include <QMenu>
#include <QToolButton>
#include <QToolBar>
#include <QPointer>
#include <QMap>
#include <QElapsedTimer>
#include <QQuickWidget>


#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif

#ifdef Q_OS_WIN
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QAxWidget>
#endif
#endif

#include "DockManager.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"
#include "DockAreaTitleBar.h"
#include "DockAreaTabBar.h"
#include "FloatingDockContainer.h"
#include "DockComponentsFactory.h"
#include "StatusDialog.h"
#include "DockSplitter.h"
#include "StarSystemViewer.h"

#include <DarkStyle.h>
#include <GameInstance.h>
#include <RasterizerEngineWidget.h>
#include <PathtracerEngineWidget.h>


/**
 * Returns a random number from 0 to highest - 1
 */
int randomNumberBounded(int highest)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	return QRandomGenerator::global()->bounded(highest);
#else
	qsrand(QTime::currentTime().msec());
	return qrand() % highest;
#endif
}


/**
 * Function returns a features string with closable (c), movable (m) and floatable (f)
 * features. i.e. The following string is for a not closable but movable and floatable
 * widget: c- m+ f+
 */
static QString featuresString(ads::CDockWidget* DockWidget)
{
	auto f = DockWidget->features();
	return QString("c%1 m%2 f%3")
		.arg(f.testFlag(ads::CDockWidget::DockWidgetClosable) ? "+" : "-")
		.arg(f.testFlag(ads::CDockWidget::DockWidgetMovable) ? "+" : "-")
		.arg(f.testFlag(ads::CDockWidget::DockWidgetFloatable) ? "+" : "-");
}


/**
 * Appends the string returned by featuresString() to the window title of
 * the given DockWidget
 */
static void appendFeaturStringToWindowTitle(ads::CDockWidget* DockWidget)
{
	DockWidget->setWindowTitle(DockWidget->windowTitle()
		+  QString(" (%1)").arg(featuresString(DockWidget)));
}

/**
 * Helper function to create an SVG icon
 */
static QIcon svgIcon(const QString& File)
{
	// This is a workaround, because in item views SVG icons are not
	// properly scaled and look blurry or pixelate
	QIcon SvgIcon(File);
	SvgIcon.addPixmap(SvgIcon.pixmap(92));
	return SvgIcon;
}


//============================================================================
class CCustomComponentsFactory : public ads::CDockComponentsFactory
{
public:
	using Super = ads::CDockComponentsFactory;
	ads::CDockAreaTitleBar* createDockAreaTitleBar(ads::CDockAreaWidget* DockArea) const override
	{
		auto TitleBar = new ads::CDockAreaTitleBar(DockArea);
		auto CustomButton = new QToolButton(DockArea);
		CustomButton->setToolTip(QObject::tr("Help"));
		CustomButton->setIcon(svgIcon(":/adsdemo/images/help_outline.svg"));
		CustomButton->setAutoRaise(true);
		int Index = TitleBar->indexOf(TitleBar->button(ads::TitleBarButtonTabsMenu));
		TitleBar->insertWidget(Index + 1, CustomButton);
		return TitleBar;
	}
};



//===========================================================================
/**
 * Custom QTableWidget with a minimum size hint to test CDockWidget
 * setMinimumSizeHintMode() function of CDockWidget
 */
class CMinSizeTableWidget : public QTableWidget
{
public:
	using QTableWidget::QTableWidget;
	virtual QSize minimumSizeHint() const override
	{
		return QSize(300, 100);
	}
};



//============================================================================
/**
 * Private data class pimpl
 */
struct MainWindowPrivate
{
	CMainWindow* _this;
	Ui::MainWindow ui;
	QAction* SavePerspectiveAction = nullptr;
	QWidgetAction* PerspectiveListAction = nullptr;
	QComboBox* PerspectiveComboBox = nullptr;
	ads::CDockManager* DockManager = nullptr;
	ads::CDockWidget* WindowTitleTestDockWidget = nullptr;
	QPointer<ads::CDockWidget> LastDockedEditor;
	QPointer<ads::CDockWidget> LastCreatedFloatingEditor;

	MainWindowPrivate(CMainWindow* _public) : _this(_public) {}

	/**
	 * Creates the toolbar actions
	 */
	void createActions();

	/**
	 * Fill the dock manager with dock widgets
	 */
	void createContent();

	/**
	 * Saves the dock manager state and the main window geometry
	 */
	void saveState();

	/**
	 * Save the list of perspectives
	 */
	void savePerspectives();

	/**
	 * Restores the dock manager state
	 */
	void restoreState();

	/**
	 * Restore the perspective listo of the dock manager
	 */
    void restorePerspectives();

	/**
	 * Create a dock widget with a QCalendarWidget
	 */
	ads::CDockWidget* createCalendarDockWidget()
	{
		static int CalendarCount = 0;
		QCalendarWidget* w = new QCalendarWidget();
		ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("Calendar %1").arg(CalendarCount++));
		// The following lines are for testing the setWidget() and takeWidget()
		// functionality
		DockWidget->setWidget(w);
		DockWidget->setWidget(w); // what happens if we set a widget if a widget is already set
		DockWidget->takeWidget(); // we remove the widget
		DockWidget->setWidget(w); // and set the widget again - there should be no error
		DockWidget->setToggleViewActionMode(ads::CDockWidget::ActionModeShow);
		DockWidget->setIcon(svgIcon(":/adsdemo/images/date_range.svg"));
		ui.menuView->addAction(DockWidget->toggleViewAction());
		auto ToolBar = DockWidget->createDefaultToolBar();
		ToolBar->addAction(ui.actionSaveState);
		ToolBar->addAction(ui.actionRestoreState);
		// For testing all calendar dock widgets have a the tool button style
		// Qt::ToolButtonTextUnderIcon
		DockWidget->setToolBarStyleSource(ads::CDockWidget::ToolBarStyleFromDockWidget);
		DockWidget->setToolBarStyle(Qt::ToolButtonTextUnderIcon, ads::CDockWidget::StateFloating);
		return DockWidget;
	}


	/**
	 * Create dock widget with a text label
	 */
	ads::CDockWidget* createLongTextLabelDockWidget()
	{
		static int LabelCount = 0;
		QLabel* l = new QLabel();
		l->setWordWrap(true);
		l->setAlignment(Qt::AlignTop | Qt::AlignLeft);
		l->setText(QString("Label %1 %2 - Lorem ipsum dolor sit amet, consectetuer adipiscing elit. "
			"Aenean commodo ligula eget dolor. Aenean massa. Cum sociis natoque "
			"penatibus et magnis dis parturient montes, nascetur ridiculus mus. "
			"Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem. "
			"Nulla consequat massa quis enim. Donec pede justo, fringilla vel, "
			"aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, "
			"imperdiet a, venenatis vitae, justo. Nullam dictum felis eu pede "
			"mollis pretium. Integer tincidunt. Cras dapibus. Vivamus elementum "
			"semper nisi. Aenean vulputate eleifend tellus. Aenean leo ligula, "
			"porttitor eu, consequat vitae, eleifend ac, enim. Aliquam lorem ante, "
			"dapibus in, viverra quis, feugiat a, tellus. Phasellus viverra nulla "
			"ut metus varius laoreet.")
			.arg(LabelCount)
			.arg(QTime::currentTime().toString("hh:mm:ss:zzz")));

		ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("Label %1").arg(LabelCount++));
		DockWidget->setWidget(l);
		DockWidget->setIcon(svgIcon(":/adsdemo/images/font_download.svg"));
		ui.menuView->addAction(DockWidget->toggleViewAction());
		return DockWidget;
	}


	/**
	 * Creates as imple editor widget
	 */
	ads::CDockWidget* createEditorWidget()
	{
		static int EditorCount = 0;
		QPlainTextEdit* w = new QPlainTextEdit();
		w->setPlaceholderText("This is an editor. If you close the editor, it will be "
			"deleted. Enter your text here.");
		w->setStyleSheet("border: none");
		ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("Editor %1").arg(EditorCount++));
		DockWidget->setWidget(w);
		DockWidget->setIcon(svgIcon(":/adsdemo/images/edit.svg"));
		DockWidget->setFeature(ads::CDockWidget::CustomCloseHandling, true);
		ui.menuView->addAction(DockWidget->toggleViewAction());

		QMenu* OptionsMenu = new QMenu(DockWidget);
		OptionsMenu->setTitle(QObject::tr("Options"));
		OptionsMenu->setToolTip(OptionsMenu->title());
		OptionsMenu->setIcon(svgIcon(":/adsdemo/images/custom-menu-button.svg"));
		auto MenuAction = OptionsMenu->menuAction();
		// The object name of the action will be set for the QToolButton that
		// is created in the dock area title bar. You can use this name for CSS
		// styling
		MenuAction->setObjectName("optionsMenu");
		DockWidget->setTitleBarActions({OptionsMenu->menuAction()});
		auto a = OptionsMenu->addAction(QObject::tr("Clear Editor"));
		w->connect(a, SIGNAL(triggered()), SLOT(clear()));

		return DockWidget;
	}

	/**
	 * Creates a simply image viewr
	 */
	ads::CDockWidget* createMapViewer()
	{
		static int ImageViewerCount = 0;
		auto w = new CStarSystemViewer();
		bool isOk;
		auto gi = game_core::GameInstance::getGameInstance();
		isOk = QObject::connect(gi, SIGNAL(segmentLoaded()),
			w, SLOT(onGameSegmentLoaded()));
		Q_ASSERT(isOk);

		ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("Image Viewer %1").arg(ImageViewerCount++));
		DockWidget->setIcon(svgIcon(":/adsdemo/images/photo.svg"));
		DockWidget->setWidget(w,ads:: CDockWidget::ForceNoScrollArea);
		auto ToolBar = DockWidget->createDefaultToolBar();
		ToolBar->addActions(w->actions());
		return DockWidget;
	}

	/**
	 * Create a table widget
	 */
	ads::CDockWidget* createTableWidget()
	{
		static int TableCount = 0;
		auto w = new CMinSizeTableWidget();
		ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("Table %1").arg(TableCount++));
		static int colCount = 5;
		static int rowCount = 30;
		w->setColumnCount(colCount);
		w->setRowCount(rowCount);
		for (int col = 0; col < colCount; ++col)
		{
		  w->setHorizontalHeaderItem(col, new QTableWidgetItem(QString("Col %1").arg(col+1)));
		  for (int row = 0; row < rowCount; ++row)
		  {
			 w->setItem(row, col, new QTableWidgetItem(QString("T %1-%2").arg(row + 1).arg(col+1)));
		  }
		}
		DockWidget->setWidget(w);
		DockWidget->setIcon(svgIcon(":/adsdemo/images/grid_on.svg"));
		DockWidget->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromContent);
		auto ToolBar = DockWidget->createDefaultToolBar();
		auto Action = ToolBar->addAction(svgIcon(":/adsdemo/images/fullscreen.svg"), "Toggle Fullscreen");
		QObject::connect(Action, &QAction::triggered, [=]()
			{
				if (DockWidget->isFullScreen())
				{
					DockWidget->showNormal();
				}
				else
				{
					DockWidget->showFullScreen();
				}
			});
		ui.menuView->addAction(DockWidget->toggleViewAction());
		return DockWidget;
	}

	/**
	 * Create QQuickWidget for test for OpenGL and QQuick
	 */
	ads::CDockWidget *createQQuickWidget()
	{
		QQuickWidget *widget = new QQuickWidget();
		ads::CDockWidget *dockWidget = new ads::CDockWidget("Quick");
		dockWidget->setWidget(widget);
		return dockWidget;
	}

    /**
     * Create the rasterizer
     */    
    ads::CDockWidget *createRasterizerEngineWidget()
    {
        auto widget = new graphics_engine::RasterizerEngineWidget();
        bool isOk = QObject::connect(widget, SIGNAL(engineReady(void*)),
			_this, SLOT(onRasterizerEngineReady(void*)));
        Q_ASSERT(isOk);
		auto gi = game_core::GameInstance::getGameInstance();
		isOk = QObject::connect(widget, SIGNAL(mouseDoubleClick()),
			gi, SLOT(onRasterizerMouseDoubleClick()));
		Q_ASSERT(isOk);
        ads::CDockWidget *dockWidget = new ads::CDockWidget("OpenGL Engine");
        dockWidget->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
        dockWidget->setWidget(widget);
        return dockWidget;
    }

    /**
     * Create the pathtracer
     */
    ads::CDockWidget *createPathtracerEngineWidget()
    {
        auto widget = new graphics_engine::PathtracerEngineWidget();
        bool isOk = QObject::connect(widget, SIGNAL(engineReady(void*)),
                                     _this, SLOT(onPathtracerEngineReady(void*)));
        Q_ASSERT(isOk);
        ads::CDockWidget *dockWidget = new ads::CDockWidget("Cycles Engine");
        //dockWidget->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
        dockWidget->setWidget(widget);
        return dockWidget;
    }


#ifdef Q_OS_WIN
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	/**
	 * Creates an ActiveX widget on windows
	 */
	ads::CDockWidget* createActiveXWidget(QWidget* parent = nullptr)
	{
	   static int ActiveXCount = 0;
	   QAxWidget* w = new QAxWidget("{6bf52a52-394a-11d3-b153-00c04f79faa6}", parent);
	   ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("Active X %1").arg(ActiveXCount++));
	   DockWidget->setWidget(w);
	   ui.menuView->addAction(DockWidget->toggleViewAction());
	   return DockWidget;
	}
#endif
#endif
};

//============================================================================
void MainWindowPrivate::createContent()
{
	ads::CDockWidget* DockWidget;


	//// Test container docking
	//auto DockWidget = createCalendarDockWidget();
	//DockWidget->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	//auto SpecialDockArea = DockManager->addDockWidget(ads::LeftDockWidgetArea, DockWidget);

	//// For this Special Dock Area we want to avoid dropping on the center of it (i.e. we don't want this widget to be ever tabbified):
	//{
	//	//SpecialDockArea->setAllowedAreas(ads::OuterDockAreas);
 //       SpecialDockArea->setAllowedAreas({ads::LeftDockWidgetArea,
 //                                         ads::RightDockWidgetArea,
 //                                         ads::TopDockWidgetArea,
 //                                         ads::BottomDockWidgetArea}); // just for testing
 //   }

    //// Test visible floating dock widget
    //DockWidget = createCalendarDockWidget();
    //DockManager->addDockWidgetFloating(DockWidget);
    //DockWidget->setWindowTitle(QString("My " + DockWidget->windowTitle()));

    // Create map viewer
    DockWidget = createMapViewer();
    DockManager->addDockWidget(ads::LeftDockWidgetArea, DockWidget);

    DockWidget = createRasterizerEngineWidget();
	DockWidget->setFeature(ads::CDockWidget::DockWidgetClosable, true);
    DockManager->addDockWidget(ads::LeftDockWidgetArea, DockWidget);

    DockWidget = createPathtracerEngineWidget();
    DockWidget->setFeature(ads::CDockWidget::DockWidgetClosable, true);
    DockManager->addDockWidget(ads::LeftDockWidgetArea, DockWidget);



	//DockWidget = createQQuickWidget();
	//DockWidget->setFeature(ads::CDockWidget::DockWidgetClosable, true);
	//DockManager->addDockWidget(ads::LeftDockWidgetArea, DockWidget);
}


//============================================================================
void MainWindowPrivate::createActions()
{
	ui.toolBar->addAction(ui.actionSaveState);
	ui.toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	ui.actionSaveState->setIcon(svgIcon(":/adsdemo/images/save.svg"));
	ui.toolBar->addAction(ui.actionRestoreState);
	ui.actionRestoreState->setIcon(svgIcon(":/adsdemo/images/restore.svg"));

	SavePerspectiveAction = new QAction("Create Perspective", _this);
	SavePerspectiveAction->setIcon(svgIcon(":/adsdemo/images/picture_in_picture.svg"));
	_this->connect(SavePerspectiveAction, SIGNAL(triggered()), SLOT(savePerspective()));
	PerspectiveListAction = new QWidgetAction(_this);
	PerspectiveComboBox = new QComboBox(_this);
	PerspectiveComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	PerspectiveComboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	PerspectiveListAction->setDefaultWidget(PerspectiveComboBox);
	ui.toolBar->addSeparator();
	ui.toolBar->addAction(PerspectiveListAction);
	ui.toolBar->addAction(SavePerspectiveAction);

	QAction* a = ui.toolBar->addAction("Create Floating Editor");
	a->setProperty("Floating", true);
	a->setToolTip("Creates floating dynamic dockable editor windows that are deleted on close");
	a->setIcon(svgIcon(":/adsdemo/images/note_add.svg"));
	_this->connect(a, SIGNAL(triggered()), SLOT(createEditor()));
	ui.menuTests->addAction(a);

	a = ui.toolBar->addAction("Create Docked Editor");
	a->setProperty("Floating", false);
	a->setToolTip("Creates a docked editor windows that are deleted on close");
	a->setIcon(svgIcon(":/adsdemo/images/docked_editor.svg"));
	_this->connect(a, SIGNAL(triggered()), SLOT(createEditor()));
	ui.menuTests->addAction(a);

	a = ui.toolBar->addAction("Create Editor Tab");
	a->setProperty("Floating", false);
	a->setToolTip("Creates a editor tab and inserts it as second tab into an area");
	a->setIcon(svgIcon(":/adsdemo/images/tab.svg"));
	a->setProperty("Tabbed", true);
	_this->connect(a, SIGNAL(triggered()), SLOT(createEditor()));
	ui.menuTests->addAction(a);

	a = ui.toolBar->addAction("Create Floating Table");
	a->setToolTip("Creates floating dynamic dockable table with millions of entries");
	a->setIcon(svgIcon(":/adsdemo/images/grid_on.svg"));
	_this->connect(a, SIGNAL(triggered()), SLOT(createTable()));
	ui.menuTests->addAction(a);

	a = ui.toolBar->addAction("Create Image Viewer");
	auto ToolButton = qobject_cast<QToolButton*>(ui.toolBar->widgetForAction(a));
	ToolButton->setPopupMode(QToolButton::InstantPopup);
	a->setToolTip("Creates floating, docked or pinned image viewer");
	a->setIcon(svgIcon(":/adsdemo/images/panorama.svg"));
	ui.menuTests->addAction(a);
	auto Menu = new QMenu();
	ToolButton->setMenu(Menu);
	a = Menu->addAction("Floating Image Viewer");
	_this->connect(a, SIGNAL(triggered()), SLOT(createImageViewer()));
	a = Menu->addAction("Docked Image Viewer");
	_this->connect(a, SIGNAL(triggered()), SLOT(createImageViewer()));
	a = Menu->addAction("Pinned Image Viewer");
	_this->connect(a, SIGNAL(triggered()), SLOT(createImageViewer()));


	ui.menuTests->addSeparator();
	a = ui.menuTests->addAction("Show Status Dialog");
	_this->connect(a, SIGNAL(triggered()), SLOT(showStatusDialog()));

	a = ui.menuTests->addAction("Toggle Label 0 Window Title");
	_this->connect(a, SIGNAL(triggered()), SLOT(toggleDockWidgetWindowTitle()));
	ui.menuTests->addSeparator();

	a = ui.toolBar->addAction("Apply VS Style");
	a->setToolTip("Applies a Visual Studio light style (visual_studio_light.css)." );
	a->setIcon(svgIcon(":/adsdemo/images/color_lens.svg"));
	QObject::connect(a, &QAction::triggered, _this, &CMainWindow::applyVsStyle);
	ui.menuTests->addAction(a);
}


//============================================================================
void MainWindowPrivate::saveState()
{
	QSettings Settings("Settings.ini", QSettings::IniFormat);
	Settings.setValue("mainWindow/Geometry", _this->saveGeometry());
	Settings.setValue("mainWindow/State", _this->saveState());
	Settings.setValue("mainWindow/DockingState", DockManager->saveState());
}


//============================================================================
void MainWindowPrivate::restoreState()
{
	QSettings Settings("Settings.ini", QSettings::IniFormat);
	_this->restoreGeometry(Settings.value("mainWindow/Geometry").toByteArray());
	_this->restoreState(Settings.value("mainWindow/State").toByteArray());
	DockManager->restoreState(Settings.value("mainWindow/DockingState").toByteArray());
}



//============================================================================
void MainWindowPrivate::savePerspectives()
{
	QSettings Settings("Settings.ini", QSettings::IniFormat);
	DockManager->savePerspectives(Settings);
}



//============================================================================
void MainWindowPrivate::restorePerspectives()
{
	QSettings Settings("Settings.ini", QSettings::IniFormat);
	DockManager->loadPerspectives(Settings);
	PerspectiveComboBox->clear();
	PerspectiveComboBox->addItems(DockManager->perspectiveNames());
}


//============================================================================
CMainWindow::CMainWindow(QWidget *parent) :
	QMainWindow(parent),
	d(new MainWindowPrivate(this))
{
	using namespace ads;
	d->ui.setupUi(this);
	setWindowTitle(QApplication::instance()->applicationName());
	d->createActions();

    // Style the window
    if(dark_style::shouldUseDarkStyle())
    {
        dark_style::useDarkStyleOnWidget(this);
    }

	// uncomment the following line if the tab close button should be
	// a QToolButton instead of a QPushButton
	// CDockManager::setConfigFlags(CDockManager::configFlags() | CDockManager::TabCloseButtonIsToolButton);

    // uncomment the following line if you want to use opaque undocking and
	// opaque splitter resizing
    //CDockManager::setConfigFlags(CDockManager::DefaultOpaqueConfig);

    // uncomment the following line if you want a fixed tab width that does
	// not change if the visibility of the close button changes
    //CDockManager::setConfigFlag(CDockManager::RetainTabSizeWhenCloseButtonHidden, true);

	// uncomment the following line if you don't want close button on DockArea's title bar
	//CDockManager::setConfigFlag(CDockManager::DockAreaHasCloseButton, false);

	// uncomment the following line if you don't want undock button on DockArea's title bar
	//CDockManager::setConfigFlag(CDockManager::DockAreaHasUndockButton, false);

	// uncomment the following line if you don't want tabs menu button on DockArea's title bar
	//CDockManager::setConfigFlag(CDockManager::DockAreaHasTabsMenuButton, false);

	// uncomment the following line if you don't want disabled buttons to appear on DockArea's title bar
	//CDockManager::setConfigFlag(CDockManager::DockAreaHideDisabledButtons, true);

	// uncomment the following line if you want to show tabs menu button on DockArea's title bar only when there are more than one tab and at least of them has elided title
	//CDockManager::setConfigFlag(CDockManager::DockAreaDynamicTabsMenuButtonVisibility, true);

	// uncomment the following line if you want floating container to always show application title instead of active dock widget's title
	//CDockManager::setConfigFlag(CDockManager::FloatingContainerHasWidgetTitle, false);

	// uncomment the following line if you want floating container to show active dock widget's icon instead of always showing application icon
	//CDockManager::setConfigFlag(CDockManager::FloatingContainerHasWidgetIcon, true);

	// uncomment the following line if you want a central widget in the main dock container (the dock manager) without a titlebar
	// If you enable this code, you can test it in the demo with the Calendar 0
	// dock widget.
	//CDockManager::setConfigFlag(CDockManager::HideSingleCentralWidgetTitleBar, true);

	// uncomment the following line to enable focus highlighting of the dock
	// widget that has the focus
    CDockManager::setConfigFlag(CDockManager::FocusHighlighting, true);

	// uncomment if you would like to enable dock widget auto hiding
    CDockManager::setAutoHideConfigFlags({CDockManager::DefaultAutoHideConfig});

	// uncomment if you would like to enable an equal distribution of the
	// available size of a splitter to all contained dock widgets
	// CDockManager::setConfigFlag(CDockManager::EqualSplitOnInsertion, true);
	
	// uncomment if you would like to close tabs with the middle mouse button, web browser style
	// CDockManager::setConfigFlag(CDockManager::MiddleMouseButtonClosesTab, true);

	// Now create the dock manager and its content
	d->DockManager = new CDockManager(this);
	d->DockManager->setDockWidgetToolBarStyle(Qt::ToolButtonIconOnly, ads::CDockWidget::StateFloating);

	bool isOk;
 #if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	isOk = connect(d->PerspectiveComboBox, SIGNAL(activated(QString)),
		d->DockManager, SLOT(openPerspective(QString)));
	Q_ASSERT(isOk);
#else
	isOk = connect(d->PerspectiveComboBox, SIGNAL(textActivated(QString)),
        d->DockManager, SLOT(openPerspective(QString)));
	Q_ASSERT(isOk);
#endif

	isOk = connect(d->DockManager, &CDockManager::floatingWidgetCreated,
            this, &CMainWindow::onFloatingWidgetCreated);
	Q_ASSERT(isOk);

	d->createContent();
	// Default window geometry - center on screen
    resize(1280, 720);
    setGeometry(QStyle::alignedRect(
        Qt::LeftToRight, Qt::AlignCenter, frameSize(),
        QGuiApplication::primaryScreen()->availableGeometry()
    ));

	//d->restoreState();
	d->restorePerspectives();
}


//============================================================================
CMainWindow::~CMainWindow()
{
	delete d;
}


//============================================================================
void CMainWindow::closeEvent(QCloseEvent* event)
{
	// First end the game instance
	auto gi = game_core::GameInstance::getGameInstance();
	gi->deinitialize();

	d->saveState();
    // Delete dock manager here to delete all floating widgets. This ensures
    // that all top level windows of the dock manager are properly closed
    d->DockManager->deleteLater();
	QMainWindow::closeEvent(event);
}


//============================================================================
void CMainWindow::on_actionSaveState_triggered(bool)
{
	qDebug() << "MainWindow::on_actionSaveState_triggered";
	d->saveState();
}


//============================================================================
void CMainWindow::on_actionRestoreState_triggered(bool)
{
	qDebug() << "MainWindow::on_actionRestoreState_triggered";
	d->restoreState();
}


//============================================================================
void CMainWindow::savePerspective()
{
	QString PerspectiveName = QInputDialog::getText(this, "Save Perspective", "Enter unique name:");
	if (PerspectiveName.isEmpty())
	{
		return;
	}

	d->DockManager->addPerspective(PerspectiveName);
	QSignalBlocker Blocker(d->PerspectiveComboBox);
	d->PerspectiveComboBox->clear();
	d->PerspectiveComboBox->addItems(d->DockManager->perspectiveNames());
	d->PerspectiveComboBox->setCurrentText(PerspectiveName);

	d->savePerspectives();
}


//============================================================================
void CMainWindow::onViewToggled(bool Open)
{
	Q_UNUSED(Open);
	auto DockWidget = qobject_cast<ads::CDockWidget*>(sender());
	if (!DockWidget)
	{
		return;
	}

	//qDebug() << DockWidget->objectName() << " viewToggled(" << Open << ")";
}


//============================================================================
void CMainWindow::onViewVisibilityChanged(bool Visible)
{
	Q_UNUSED(Visible);
	auto DockWidget = qobject_cast<ads::CDockWidget*>(sender());
    if (!DockWidget)
    {
        return;
    }

    //qDebug() << DockWidget->objectName() << " visibilityChanged(" << Visible << ")";
}

void CMainWindow::onFloatingWidgetCreated(void* ptr)
{
    auto floatingWidget = static_cast<ads::CFloatingDockContainer*>(ptr);
    if (dark_style::shouldUseDarkStyle())
    {
        dark_style::useDarkStyleOnWidget(floatingWidget);
    }
}


//============================================================================
void CMainWindow::createEditor()
{
	QObject* Sender = sender();
	QVariant vFloating = Sender->property("Floating");
	bool Floating = vFloating.isValid() ? vFloating.toBool() : true;
	QVariant vTabbed = Sender->property("Tabbed");
	bool Tabbed = vTabbed.isValid() ? vTabbed.toBool() : true;
	auto DockWidget = d->createEditorWidget();
	DockWidget->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
	DockWidget->setFeature(ads::CDockWidget::DockWidgetForceCloseWithArea, true);
    connect(DockWidget, SIGNAL(closeRequested()), SLOT(onEditorCloseRequested()));

    if (Floating)
    {
		auto FloatingWidget = d->DockManager->addDockWidgetFloating(DockWidget);
        FloatingWidget->move(QPoint(20, 20));
		d->LastCreatedFloatingEditor = DockWidget;
		d->LastDockedEditor.clear();
		return;
    }


	ads::CDockAreaWidget* EditorArea = d->LastDockedEditor ? d->LastDockedEditor->dockAreaWidget() : nullptr;
	if (EditorArea)
	{
		if (Tabbed)
		{
			// Test inserting the dock widget tab at a given position instead
			// of appending it. This function inserts the new dock widget as
			// first tab
			d->DockManager->addDockWidgetTabToArea(DockWidget, EditorArea, 0);
		}
		else
		{
			d->DockManager->setConfigFlag(ads::CDockManager::EqualSplitOnInsertion, true);
			d->DockManager->addDockWidget(ads::RightDockWidgetArea, DockWidget, EditorArea);
		}
	}
	else
	{
		if (d->LastCreatedFloatingEditor)
		{
			d->DockManager->addDockWidget(ads::RightDockWidgetArea, DockWidget, d->LastCreatedFloatingEditor->dockAreaWidget());
		}
		else
		{
			d->DockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);
		}
	}
	d->LastDockedEditor = DockWidget;
}


//============================================================================
void CMainWindow::onEditorCloseRequested()
{
	auto DockWidget = qobject_cast<ads::CDockWidget*>(sender());
	int Result = QMessageBox::question(this, "Close Editor", QString("Editor %1 "
		"contains unsaved changes? Would you like to close it?")
		.arg(DockWidget->windowTitle()));
	if (QMessageBox::Yes == Result)
	{
		DockWidget->closeDockWidget();
	}
}


//============================================================================
void CMainWindow::onImageViewerCloseRequested()
{
	auto DockWidget = qobject_cast<ads::CDockWidget*>(sender());
	int Result = QMessageBox::question(this, "Close Image Viewer", QString("%1 "
		"contains unsaved changes? Would you like to close it?")
		.arg(DockWidget->windowTitle()));
	if (QMessageBox::Yes == Result)
	{
		DockWidget->closeDockWidget();
	}
}


//============================================================================
void CMainWindow::createTable()
{
	auto DockWidget = d->createTableWidget();
	DockWidget->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
	auto FloatingWidget = d->DockManager->addDockWidgetFloating(DockWidget);
    FloatingWidget->move(QPoint(40, 40));
}


//============================================================================
void CMainWindow::showStatusDialog()
{
	CStatusDialog Dialog(d->DockManager);
	Dialog.exec();
}


//============================================================================
void CMainWindow::toggleDockWidgetWindowTitle()
{
	QString Title = d->WindowTitleTestDockWidget->windowTitle();
	int i = Title.indexOf(" (Test)");
	if (-1 == i)
	{
		Title += " (Test)";
	}
	else
	{
		Title = Title.left(i);
	}
	d->WindowTitleTestDockWidget->setWindowTitle(Title);
}


//============================================================================
void CMainWindow::applyVsStyle()
{
	QFile StyleSheetFile(":adsdemo/res/visual_studio_light.css");
	StyleSheetFile.open(QIODevice::ReadOnly);
	QTextStream StyleSheetStream(&StyleSheetFile);
	auto Stylesheet = StyleSheetStream.readAll();
	StyleSheetFile.close();
	d->DockManager->setStyleSheet(Stylesheet);
}


//============================================================================
void CMainWindow::createImageViewer()
{
	QAction* a = qobject_cast<QAction*>(sender());
	qDebug() << "createImageViewer " << a->text();

	auto DockWidget = d->createMapViewer();
	DockWidget->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
	DockWidget->setFeature(ads::CDockWidget::DockWidgetForceCloseWithArea, true);
	DockWidget->setFeature(ads::CDockWidget::CustomCloseHandling, true);
	DockWidget->resize(QSize(640, 480));
	connect(DockWidget, &ads::CDockWidget::closeRequested, this,
		&CMainWindow::onImageViewerCloseRequested);

	if (a->text().startsWith("Floating"))
	{
		auto FloatingWidget = d->DockManager->addDockWidgetFloating(DockWidget);
		FloatingWidget->move(QPoint(20, 20));
	}
	else if (a->text().startsWith("Docked"))
	{
		d->DockManager->addDockWidget(ads::RightDockWidgetArea, DockWidget);
	}
	else if (a->text().startsWith("Pinned"))
	{
		d->DockManager->addAutoHideDockWidget(ads::SideBarLeft, DockWidget);
	}
}

bool rasterizerSet = false;
bool pathtracerSet = false;

void CMainWindow::onRasterizerEngineReady(void* gei)
{
	auto gi = game_core::GameInstance::getGameInstance();
	gi->setRasterizer((graphics_engine::RasterizerEngineInterface*)gei);

	rasterizerSet = true;
	if (rasterizerSet && pathtracerSet)
	{
		gi->loadGame(L"");
	}
}

void CMainWindow::onPathtracerEngineReady(void* gei)
{
	auto gi = game_core::GameInstance::getGameInstance();
	gi->setPathtracer((graphics_engine::PathtracerEngineInterface*)gei);

	pathtracerSet = true;
	if (rasterizerSet && pathtracerSet)
	{
		gi->loadGame(L"");
	}
}
