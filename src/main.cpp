#include "app/MainWindow.h"
#include "git/GitRepository.h"
#include "history/HistoryLoader.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QString>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<HistoryRow>>("QVector<HistoryRow>");

    GitRepository::globalInit();

    int exitCode = 0;
    {
        QApplication app(argc, argv);
        QApplication::setApplicationName(QStringLiteral("v8GitView"));
        QApplication::setOrganizationName(QStringLiteral("v8GitView"));
        QApplication::setApplicationVersion(QStringLiteral(V8GITVIEW_VERSION));

        Theme::applyDarkPalette(app);
        QApplication::setWindowIcon(Theme::appIcon());

        MainWindow window;
        window.show();

        if (argc > 1)
            window.openPath(QString::fromLocal8Bit(argv[1]));

        exitCode = app.exec();
    }

    GitRepository::globalShutdown();
    return exitCode;
}
