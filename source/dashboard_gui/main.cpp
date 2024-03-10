#include <MainWindow.h>
#include <QString>
#include <QFile>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>
#include <QPalette>

#include <memory>

#include <DarkStyle.h>
#include <GameInstance.h>


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stdout, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }

    fflush(stderr);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= 0x050600
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#endif

	std::shared_ptr<int> b;
	QApplication a(argc, argv);

    // When Qt::AA_ShareOpenGLContexts is set, the widget's context never changes,
    // not even when reparenting because the widget's associated texture is going
    // to be accessible also from the new top-level's context.
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);

    // Style the application
    if(dark_style::shouldUseDarkStyle())
    {
        dark_style::useDarkStyleOnApplication();
    }
	a.setApplicationName("Advanced Docking System Demo");
	a.setQuitOnLastWindowClosed(true);
	a.setWindowIcon(QIcon(":/adsdemo/images/ads_icon2.svg"));
	qInstallMessageHandler(myMessageOutput);

    // Create the Game Core's Game Instance. The main class responsible for handling
    // all the game's logic and states across the GUI.
    auto gi = new game_core::GameInstance;
    gi->initialize();

    // Create the window instance
    auto mainWindow = new CMainWindow;
    mainWindow->show();

    //QFile StyleSheetFile(":/adsdemo/app.css");
    //StyleSheetFile.open(QIODevice::ReadOnly);
    //QTextStream StyleSheetStream(&StyleSheetFile);
    //a.setStyleSheet(StyleSheetStream.readAll());
    //StyleSheetFile.close();
	return a.exec();
}
