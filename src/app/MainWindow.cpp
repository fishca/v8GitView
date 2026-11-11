#include "app/MainWindow.h"

#include "git/GitRepository.h"
#include "history/CommitModel.h"
#include "history/HistoryLoader.h"
#include "ui/CommitView.h"
#include "ui/Theme.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QScreen>
#include <QSettings>
#include <QShowEvent>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#endif

#include <git2.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    qRegisterMetaType<QVector<HistoryRow>>("QVector<HistoryRow>");

    setWindowTitle(QStringLiteral("v8GitView"));
    resize(1280, 820);
    if (const QScreen *screen = QGuiApplication::primaryScreen()) {
        const QSize available = screen->availableGeometry().size();
        resize(qMin(1280, available.width() - 60),
               qMin(820, available.height() - 60));
    }

    m_model = new CommitModel(this);
    m_view = new CommitView(this);
    m_view->setModel(m_model);
    setCentralWidget(m_view);

    createActions();
    createMenus();
    createToolBar();
    createStatusBar();

    QSettings settings;
    if (settings.contains(QStringLiteral("window/geometry")))
        restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
    if (settings.contains(QStringLiteral("window/state")))
        restoreState(settings.value(QStringLiteral("window/state")).toByteArray());

    // Background history loader.
    m_loader = new HistoryLoader;
    m_loader->moveToThread(&m_loaderThread);
    m_loaderThread.setObjectName(QStringLiteral("v8GitView-history"));

    connect(&m_loaderThread, &QThread::finished, m_loader, &QObject::deleteLater);
    connect(m_loader, &HistoryLoader::opened, this, &MainWindow::onOpened);
    connect(m_loader, &HistoryLoader::batch, this, &MainWindow::onBatch);
    connect(m_loader, &HistoryLoader::failed, this, &MainWindow::onFailed);
    connect(m_model, &CommitModel::needMore, this, &MainWindow::requestMore);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &MainWindow::updateStatus);
    connect(m_model, &QAbstractItemModel::modelReset, this, &MainWindow::updateStatus);

    m_loaderThread.start();

    updateStatus();

    const QString lastRepository = settings.value(QStringLiteral("repo/last")).toString();
    if (settings.value(QStringLiteral("repo/reopen"), true).toBool()
        && !lastRepository.isEmpty()) {
        QTimer::singleShot(0, this, [this, lastRepository] {
            if (m_currentPath.isEmpty() && QFileInfo::exists(lastRepository))
                openPath(lastRepository);
        });
    }
}

MainWindow::~MainWindow()
{
    if (!m_loader)
        return;

    m_loader->cancel();

    if (m_loaderThread.isRunning()) {
        QMetaObject::invokeMethod(m_loader, "shutdown", Qt::BlockingQueuedConnection);
        m_loaderThread.quit();
        m_loaderThread.wait(5000);
    }
}

void MainWindow::createActions()
{
    m_openAction = new QAction(tr("&Open Repository…"), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::chooseRepository);

    m_reloadAction = new QAction(tr("&Reload"), this);
    m_reloadAction->setShortcut(QKeySequence::Refresh);
    m_reloadAction->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_reloadAction->setEnabled(false);
    connect(m_reloadAction, &QAction::triggered, this, &MainWindow::reloadRepository);

    m_quitAction = new QAction(tr("E&xit"), this);
    m_quitAction->setShortcut(QKeySequence::Quit);
    connect(m_quitAction, &QAction::triggered, this, &QWidget::close);

    m_aboutAction = new QAction(tr("&About v8GitView"), this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAction);
    fileMenu->addAction(m_reloadAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_quitAction);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_aboutAction);
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main"));
    toolBar->setObjectName(QStringLiteral("mainToolBar"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->addAction(m_openAction);
    toolBar->addAction(m_reloadAction);
}

void MainWindow::createStatusBar()
{
    m_repoLabel = new QLabel(tr("No repository opened"), this);
    m_repoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_repoLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    m_busyLabel = new QLabel(tr("Loading…"), this);
    m_busyLabel->setStyleSheet(QStringLiteral("color: #CC7832;"));
    m_busyLabel->hide();

    m_countLabel = new QLabel(this);
    m_countLabel->setMinimumWidth(110);
    m_countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_countLabel->setStyleSheet(QStringLiteral("color: #9DA0A8;"));

    statusBar()->addWidget(m_repoLabel, 1);
    statusBar()->addPermanentWidget(m_busyLabel);
    statusBar()->addPermanentWidget(m_countLabel);
    statusBar()->setSizeGripEnabled(true);
}

void MainWindow::chooseRepository()
{
    QSettings settings;
    const QString startDir =
        m_currentPath.isEmpty()
            ? settings.value(QStringLiteral("repo/lastDir"), QDir::homePath()).toString()
            : m_currentPath;

    const QString path = QFileDialog::getExistingDirectory(
        this, tr("Open Git repository"), startDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (path.isEmpty())
        return;

    settings.setValue(QStringLiteral("repo/lastDir"), path);
    openPath(path);
}

void MainWindow::reloadRepository()
{
    if (!m_currentPath.isEmpty())
        openPath(m_currentPath);
}

void MainWindow::openPath(const QString &path)
{
    if (path.isEmpty())
        return;

    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("v8GitView"),
                             tr("Path does not exist:\n%1").arg(path));
        return;
    }

    m_currentPath = path;
    m_model->clearRepository();
    m_reloadAction->setEnabled(false);
    m_repoLabel->setText(QDir::toNativeSeparators(path));
    setBusy(true);
    updateWindowTitle();

    // Interrupt a walk that may still be running, then queue the new one.
    m_loader->cancel();
    QMetaObject::invokeMethod(m_loader, "openRepository", Qt::QueuedConnection,
                              Q_ARG(QString, path));
}

void MainWindow::onOpened(const QString &workdir)
{
    m_currentPath = workdir;
    m_reloadAction->setEnabled(true);
    m_repoLabel->setText(QDir::toNativeSeparators(workdir));
    setBusy(true);

    QSettings settings;
    settings.setValue(QStringLiteral("repo/last"), workdir);

    updateWindowTitle();
}

void MainWindow::onBatch(const QVector<HistoryRow> &rows, bool hasMore)
{
    m_model->appendBatch(rows, hasMore);
    setBusy(false);
    updateStatus();
}

void MainWindow::onFailed(const QString &error)
{
    setBusy(false);
    m_reloadAction->setEnabled(false);
    m_currentPath.clear();
    m_repoLabel->setText(tr("No repository opened"));
    updateWindowTitle();

    QMessageBox::warning(this, tr("v8GitView"),
                         tr("Cannot open repository:\n%1").arg(error));
}

void MainWindow::requestMore()
{
    if (!m_loader)
        return;

    setBusy(true);
    QMetaObject::invokeMethod(m_loader, "requestMore", Qt::QueuedConnection);
}

void MainWindow::updateStatus()
{
    const int commits = m_model->rowCount();
    const int lanes = m_model->maxLaneCount();
    m_countLabel->setText(tr("%1 commits · %2 lanes")
                              .arg(QLocale().toString(commits),
                                   QLocale().toString(lanes)));
}

void MainWindow::updateWindowTitle()
{
    if (m_currentPath.isEmpty()) {
        setWindowTitle(QStringLiteral("v8GitView"));
        return;
    }

    setWindowTitle(QStringLiteral("v8GitView — %1")
                       .arg(QDir::toNativeSeparators(m_currentPath)));
}

void MainWindow::setBusy(bool busy)
{
    m_busyLabel->setVisible(busy);
}

void MainWindow::showAbout()
{
    int major = 0;
    int minor = 0;
    int revision = 0;
    git_libgit2_version(&major, &minor, &revision);

    QMessageBox::about(
        this, tr("About v8GitView"),
        tr("<h3>v8GitView %1</h3>"
           "<p>A fast Git history viewer tuned for large 1C:Enterprise "
           "repositories.</p>"
           "<p>Qt %2 · libgit2 %3.%4.%5</p>"
           "<p>This program is distributed under the MIT License.</p>")
            .arg(QStringLiteral(V8GITVIEW_VERSION), QStringLiteral(QT_VERSION_STR))
            .arg(major)
            .arg(minor)
            .arg(revision));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/state"), saveState());
    if (!m_currentPath.isEmpty())
        settings.setValue(QStringLiteral("repo/last"), m_currentPath);

    QMainWindow::closeEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

#ifdef Q_OS_WIN
    // Dark title bar on Windows 10 20H1+ and Windows 11.
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL dark = TRUE;
    if (FAILED(DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark))))
        DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));

    // Override the accent colour some Windows 11 themes apply to captions.
    const COLORREF caption = RGB(30, 31, 34);
    DwmSetWindowAttribute(hwnd, 35, &caption, sizeof(caption));
    const COLORREF captionText = RGB(223, 225, 229);
    DwmSetWindowAttribute(hwnd, 36, &captionText, sizeof(captionText));
    const COLORREF border = RGB(58, 61, 65);
    DwmSetWindowAttribute(hwnd, 34, &border, sizeof(border));
#endif
}
