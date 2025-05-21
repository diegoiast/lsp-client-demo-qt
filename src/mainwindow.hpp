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
#include "LspClientImpl.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private:
    QListWidget* sidebar;
    QTabWidget* tabWidget;
    QToolBar* toolbar;
    QAction* openDirAction;
    QAction* closeDirAction;
    QAction* quitAction;
    QAction* closeTabAction;
    QShortcut* closeTabShortcut;
    QDockWidget* dock;
    QLineEdit* excludeEdit;
    QLineEdit* showEdit;
    QString projectDir;
    QStringList allFiles;
    
    LspClientImpl lspClient;

    void openDirectory();
    void loadFiles(const QString& dirPath);
    void addFilesRecursive(const QString& baseDir, const QString& currentDir, QStringList& files);
    void openFileInTab(const QString& relPath);
    void closeDirectory();
    void closeCurrentTab();
    void updateFileList();

private slots:
    void onSidebarItemClicked(QListWidgetItem* item);
    void onOpenDirClicked();
    void onCloseDirClicked();
    void onQuitClicked();
    void onCloseTabClicked();
    void onFilterChanged();
};
