#pragma once

#include "history/HistoryRow.h"

#include <QMainWindow>
#include <QThread>
#include <QVector>

class QAction;
class QCloseEvent;
class QShowEvent;
class QLabel;
class CommitModel;
class CommitView;
class HistoryLoader;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openPath(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void chooseRepository();
    void reloadRepository();
    void onOpened(const QString &workdir);
    void onBatch(const QVector<HistoryRow> &rows, bool hasMore);
    void onFailed(const QString &error);
    void requestMore();
    void updateStatus();

private:
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void showAbout();
    void setBusy(bool busy);
    void updateWindowTitle();

    CommitView *m_view = nullptr;
    CommitModel *m_model = nullptr;
    HistoryLoader *m_loader = nullptr;
    QThread m_loaderThread;

    QAction *m_openAction = nullptr;
    QAction *m_reloadAction = nullptr;
    QAction *m_quitAction = nullptr;
    QAction *m_aboutAction = nullptr;

    QLabel *m_repoLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QLabel *m_busyLabel = nullptr;

    QString m_currentPath;
};
