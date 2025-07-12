#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDir>
#include <QListWidgetItem>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QShortcut>
#include <QKeySequence>
#include <QDockWidget>
#include <QLabel>
#include <QRegularExpression>
#include <QToolTip>
#include <QTimer>

#include "mainwindow.hpp"
#include "AppOutputRedirector.hpp"
#include "CodeEditor.hpp"
#include "FilesList.hpp"

template<typename Func>
void runOnUiThread(Func&& func) {
    QMetaObject::invokeMethod(qApp, std::forward<Func>(func), Qt::QueuedConnection);
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    tabWidget = new QTabWidget;
    setCentralWidget(tabWidget);

    auto* dockWidget = new QWidget;
    auto* dockLayout = new QVBoxLayout(dockWidget);
    filesList = new FilesList(this);
    dockLayout->setContentsMargins(0,0,0,0);
    dockLayout->addWidget(filesList);
    connect(filesList, &FilesList::fileSelected, this, &MainWindow::openFileInTab);

    dock = new QDockWidget(tr("Project Files"), this);
    dock->setWidget(dockWidget);
    addDockWidget(Qt::LeftDockWidgetArea, dock);


    toolbar = addToolBar("Main Toolbar");
    openDirAction = toolbar->addAction(tr("Open Dir"));
    closeDirAction = toolbar->addAction(tr("Close Dir"));
    closeTabAction = toolbar->addAction(tr("Close Tab"));
    showDebugAction = toolbar->addAction(tr("Debug"));
    showDebugAction->setCheckable(true);
    clearDebugAction = toolbar->addAction(tr("Clear Debug"));
    quitAction = toolbar->addAction(tr("Quit"));

    outputEdit = new QTextEdit(this);
    outputEdit->setReadOnly(true);
    outputEdit->setAcceptRichText(false);

    outputDock = new QDockWidget(tr("Output"), this);
    outputDock->setWidget(outputEdit);
    addDockWidget(Qt::RightDockWidgetArea, outputDock);

    outputRedirector = new AppOutputRedirector(this);
    connect(outputRedirector, &AppOutputRedirector::newStdout, this, &MainWindow::appendStdout);
    connect(outputRedirector, &AppOutputRedirector::newStderr, this, &MainWindow::appendStderr);

    connect(openDirAction, &QAction::triggered, this, &MainWindow::onOpenDirClicked);
    connect(closeDirAction, &QAction::triggered, this, &MainWindow::onCloseDirClicked);
    connect(closeTabAction, &QAction::triggered, this, &MainWindow::onCloseTabClicked);
    connect(quitAction, &QAction::triggered, this, &MainWindow::onQuitClicked);
    connect(showDebugAction, &QAction::toggled, this, [this](bool toggled){
        qDebug() << "Toggle debug = " << toggled;
        this->lspClient.debugIO(toggled);
    });
    connect(clearDebugAction, &QAction::triggered, this, [this](bool toggled){
        this->outputEdit->clear();
    });

    closeTabShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    connect(closeTabShortcut, &QShortcut::activated, this, &MainWindow::closeCurrentTab);

    lspClient.startClangd();
    openDirectory();
}

void MainWindow::openDirectory() {
    auto dir = QFileDialog::getExistingDirectory(this, tr("Open Project Directory"));
    if (!dir.isEmpty()) {
        projectDir = dir + "/";
        loadFiles(dir);
        dock->setWindowTitle(tr("Project: %1").arg(QFileInfo(dir).fileName()));

        lspClient.setDocumentRoot(projectDir.toStdString());
    }
}

void MainWindow::closeDirectory() {
    tabWidget->clear();
    projectDir.clear();
    dock->setWindowTitle(tr("Project Files"));
}

void MainWindow::loadFiles(const QString& dirPath) {
    tabWidget->clear();
    filesList->setDir(dirPath);
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

void MainWindow::openFileInTab(const QString& relPath) {
    if (projectDir.isEmpty())
        return;
    QFile file(QDir(projectDir).filePath(relPath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&file);
    auto text = in.readAll();
    auto editor = new CodeEditor;
    auto path = (projectDir + relPath).toStdString();

    editor->setPlainText(text);
    editor->setReadOnly(false);
    connect(editor, &CodeEditor::hoveredWordTooltip, editor, [path, this, editor](const QString& word, int line, int column, const QPoint& globalPos){
        Q_UNUSED(globalPos);
        lspClient.hover(path, line, column, [line, column, word, this, globalPos, editor](auto result) {
            runOnUiThread([=](){
                if (result.isNull()) {
                    QToolTip::hideText();
                    return;
                }
                auto tooltip = QString("OK - Line=%1, column=%2, word=%3").arg(line).arg(column).arg(word);
                auto &contents = result->contents;
                std::visit([&](const auto& value) {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, lsp::MarkupContent>) {
                        tooltip = QString::fromStdString(value.value);
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        tooltip = QString::fromStdString(value);
                    } else if constexpr (std::is_same_v<T, lsp::MarkedString_Language_Value>) {
                        tooltip = QString("[%1] %2").arg(QString::fromStdString(value.language),
                                                         QString::fromStdString(value.value));
                    } else if constexpr (std::is_same_v<T, std::vector<lsp::MarkedString>>) {
                        QStringList parts;
                        for (const auto& item : value) {
                            std::visit([&](const auto& inner) {
                                using InnerT = std::decay_t<decltype(inner)>;
                                if constexpr (std::is_same_v<InnerT, std::string>) {
                                    parts << QString::fromStdString(inner);
                                } else if constexpr (std::is_same_v<InnerT, lsp::MarkedString_Language_Value>) {
                                    parts << QString("[%1] %2").arg(QString::fromStdString(inner.language),
                                                                    QString::fromStdString(inner.value));
                                }
                            }, item);
                        }
                        tooltip = parts.join("\n");
                    }
                }, contents);

                QToolTip::showText(globalPos, tooltip, editor, {}, 5000);
            });
        });
    });

    auto tabIdx = tabWidget->addTab(editor, relPath);
    tabWidget->setCurrentIndex(tabIdx);

    auto contents = text.toStdString();
    lspClient.openDocument(path, contents);
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


void MainWindow::appendStdout(const QString& text) {
    outputEdit->moveCursor(QTextCursor::End);
    auto html = QString("<span style=\"color:blue;\">") + text.toHtmlEscaped().replace("\n", "<br/>") + "</span>";
    outputEdit->insertHtml(html);
    outputEdit->moveCursor(QTextCursor::End);
}

void MainWindow::appendStderr(const QString& text) {
    outputEdit->moveCursor(QTextCursor::End);
    auto html = QString("<span style=\"color:red;\">") + text.toHtmlEscaped().replace("\n", "<br/>") + "</span>";
    outputEdit->insertHtml(html);
    outputEdit->moveCursor(QTextCursor::End);
}
