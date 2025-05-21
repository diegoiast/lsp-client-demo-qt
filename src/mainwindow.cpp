#include "mainwindow.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDir>
#include <QPlainTextEdit>
#include <QListWidgetItem>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QShortcut>
#include <QKeySequence>
#include <QDockWidget>
#include <QLabel>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    tabWidget = new QTabWidget;
    setCentralWidget(tabWidget);

    sidebar = new QListWidget;
    excludeEdit = new QLineEdit;
    showEdit = new QLineEdit;
    excludeEdit->setText("build");
    showEdit->setPlaceholderText("Show (substring, empty=all)");
    excludeEdit->setPlaceholderText("Exclude (semicolon-separated substrings)");

    auto* dockWidget = new QWidget;
    auto* dockLayout = new QVBoxLayout(dockWidget);
    dockLayout->setContentsMargins(0,0,0,0);
    dockLayout->addWidget(sidebar);

    auto* excludeLabel = new QLabel("Exclude (use ; to separate):");
    auto* showLabel = new QLabel("Show:");
    dockLayout->addWidget(excludeLabel);
    dockLayout->addWidget(excludeEdit);
    dockLayout->addWidget(showLabel);
    dockLayout->addWidget(showEdit);
    // dockLayout->addStretch();

    dock = new QDockWidget(tr("Project Files"), this);
    dock->setWidget(dockWidget);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(sidebar, &QListWidget::itemClicked, this, &MainWindow::onSidebarItemClicked);
    connect(excludeEdit, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(showEdit, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);

    toolbar = addToolBar("Main Toolbar");
    openDirAction = toolbar->addAction(tr("Open Dir"));
    closeDirAction = toolbar->addAction(tr("Close Dir"));
    closeTabAction = toolbar->addAction(tr("Close Tab"));
    quitAction = toolbar->addAction(tr("Quit"));

    connect(openDirAction, &QAction::triggered, this, &MainWindow::onOpenDirClicked);
    connect(closeDirAction, &QAction::triggered, this, &MainWindow::onCloseDirClicked);
    connect(closeTabAction, &QAction::triggered, this, &MainWindow::onCloseTabClicked);
    connect(quitAction, &QAction::triggered, this, &MainWindow::onQuitClicked);

    closeTabShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    connect(closeTabShortcut, &QShortcut::activated, this, &MainWindow::closeCurrentTab);

    lspClient.startClangd();
    openDirectory();
}

void MainWindow::openDirectory() {
    auto dir = QFileDialog::getExistingDirectory(this, tr("Open Project Directory"));
    if (!dir.isEmpty()) {
        projectDir = dir;
        loadFiles(dir);
        dock->setWindowTitle(tr("Project: %1").arg(QFileInfo(dir).fileName()));
        
        lspClient.setDocumentRoot(projectDir.toStdString());
    }
}

void MainWindow::closeDirectory() {
    sidebar->clear();
    tabWidget->clear();
    projectDir.clear();
    allFiles.clear();
    dock->setWindowTitle(tr("Project Files"));
}

void MainWindow::loadFiles(const QString& dirPath) {
    sidebar->clear();
    tabWidget->clear();
    allFiles.clear();
    addFilesRecursive(dirPath, dirPath, allFiles);
    allFiles.sort(Qt::CaseInsensitive);
    updateFileList();
}

void MainWindow::addFilesRecursive(const QString& baseDir, const QString& currentDir, QStringList& files) {
    QDir dir(currentDir);
    auto entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
    static QRegularExpression rx(R"((\.h|\.hpp|\.c|\.cpp|\.txt|\.md)$)", QRegularExpression::CaseInsensitiveOption);
    for (const auto& entry : entries) {
        if (entry.isDir()) {
            addFilesRecursive(baseDir, entry.absoluteFilePath(), files);
        } else {
            auto name = entry.fileName();
            if (rx.match(name).hasMatch() || name.compare("makefile", Qt::CaseInsensitive) == 0) {
                auto relPath = QDir(baseDir).relativeFilePath(entry.absoluteFilePath());
                files << relPath;
            }
        }
    }
}

void MainWindow::onSidebarItemClicked(QListWidgetItem* item) {
    openFileInTab(item->text());
}

void MainWindow::openFileInTab(const QString& relPath) {
    if (projectDir.isEmpty())
        return;
    QFile file(QDir(projectDir).filePath(relPath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&file);
    auto text = in.readAll();
    auto editor = new QPlainTextEdit;
    editor->setPlainText(text);
    editor->setReadOnly(false);
    auto tabIdx = tabWidget->addTab(editor, relPath);
    tabWidget->setCurrentIndex(tabIdx);
}

void MainWindow::onOpenDirClicked() {
    openDirectory();
}

void MainWindow::onCloseDirClicked() {
    closeDirectory();
}

void MainWindow::onQuitClicked() {
    close();
}

void MainWindow::onCloseTabClicked() {
    closeCurrentTab();
}

void MainWindow::closeCurrentTab() {
    auto idx = tabWidget->currentIndex();
    if (idx != -1)
        tabWidget->removeTab(idx);
}

void MainWindow::onFilterChanged() {
    updateFileList();
}

void MainWindow::updateFileList() {
    sidebar->clear();
    auto excludeStr = excludeEdit->text().trimmed();
    auto showStr = showEdit->text().trimmed();

    QStringList excludeList;
    if (!excludeStr.isEmpty())
        excludeList = excludeStr.split(';', Qt::SkipEmptyParts);

    for (const auto& rel : allFiles) {
        bool excluded = false;
        for (const auto& ex : excludeList) {
            if (!ex.trimmed().isEmpty() && rel.contains(ex.trimmed(), Qt::CaseInsensitive)) {
                excluded = true;
                break;
            }
        }
        if (excluded)
            continue;
        if (!showStr.isEmpty() && !rel.contains(showStr, Qt::CaseInsensitive))
            continue;
        sidebar->addItem(rel);
    }
}
