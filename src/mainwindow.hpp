#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QAction>
#include <QShortcut>
#include <QDockWidget>
#include <QLineEdit>
#include <QStringList>
#include <QVBoxLayout>
#include <QTextEdit>

#include "LspClientImpl.hpp"

class AppOutputRedirector;
class FilesList;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private:
    QTabWidget* tabWidget;
    QToolBar* toolbar;
    QAction* openDirAction;
    QAction* closeDirAction;
    QAction* quitAction;
    QAction* closeTabAction;
    QAction* showDebugAction;
    QAction* clearDebugAction;
    QShortcut* closeTabShortcut;
    QDockWidget* dock;
    QString projectDir;
    QDockWidget* outputDock;
    QTextEdit* outputEdit;

    FilesList* filesList = nullptr;
    AppOutputRedirector* outputRedirector = nullptr;
    LspClientImpl lspClient;

    void openDirectory();
    void loadFiles(const QString& dirPath);
    void addFilesRecursive(const QString& baseDir, const QString& currentDir, QStringList& files);
    void openFileInTab(const QString& relPath);
    void closeDirectory();
    void closeCurrentTab();
    void appendStdout(const QString& text);
    void appendStderr(const QString& text);

private slots:
    void onOpenDirClicked();
    void onCloseDirClicked();
    void onQuitClicked();
    void onCloseTabClicked();
};
