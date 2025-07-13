#include "FilesList.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QQueue>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

// Normalize path to use `/`
static QString normalizePath(const QString &path) { return QDir::fromNativeSeparators(path); }

// Convert glob to regex list
static QList<QRegularExpression> toRegexList(const QStringList &patterns) {
    QList<QRegularExpression> list;
    for (auto &pat : patterns) {
        QString rx;
        for (QChar ch : pat.trimmed()) {
            if (ch == '*') {
                rx += ".*";
            } else if (ch == '?') {
                rx += '.';
            } else if (QString("[](){}.+^$|\\").contains(ch)) {
                rx += '\\' + ch;
            } else {
                rx += ch;
            }
        }
        if (!rx.isEmpty()) {
            list << QRegularExpression("^" + rx + "$", QRegularExpression::CaseInsensitiveOption);
        }
    }
    return list;
}

// ========================= FileScannerWorker =========================

FileScannerWorker::FileScannerWorker(QObject *parent) : QObject(parent) {}

void FileScannerWorker::setRootDir(const QString &dir) { rootDir = normalizePath(dir); }

void FileScannerWorker::start() {
    QElapsedTimer timer;
    timer.start();
    QStringList collected;
    scanDir(rootDir, collected);
    emit filesFound(collected);
    emit finished(timer.elapsed());
}

void FileScannerWorker::scanDir(const QString &rootPath, QStringList &buf) {
    QQueue<QString> queue;
    queue.enqueue(rootPath);
    while (!queue.isEmpty()) {
        auto dirPath = queue.dequeue();
        QDir dir(dirPath);
        auto entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo &fi : entries) {
            if (fi.isDir()) {
                queue.enqueue(normalizePath(fi.absoluteFilePath()));
            } else {
                auto rel = QDir(rootDir).relativeFilePath(fi.absoluteFilePath());
                buf << normalizePath(rel);
            }
        }
    }
}

// ========================= FilesList =========================

FilesList::FilesList(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    list = new QListWidget;
    excludeEdit = new QLineEdit;
    showEdit = new QLineEdit;

    excludeEdit->setText("build;.vs;cbuild*");
    showEdit->setPlaceholderText("Show (e.g. *.cpp;*.h)");

    layout->addWidget(list);
    layout->addWidget(excludeEdit);
    layout->addWidget(showEdit);

    connect(list, &QListWidget::itemClicked, this,
            [=](auto *it) { emit fileSelected(it->text()); });
    connect(excludeEdit, &QLineEdit::textChanged, this, &FilesList::scheduleUpdateList);
    connect(showEdit, &QLineEdit::textChanged, this, &FilesList::scheduleUpdateList);

    updateTimer = new QTimer(this);
    updateTimer->setSingleShot(true);
    updateTimer->setInterval(300);
    connect(updateTimer, &QTimer::timeout, this, &FilesList::updateList);
}

void FilesList::setDir(const QString &dir) {
    clear();
    directory = normalizePath(dir);

    auto *thread = new QThread;
    auto *worker = new FileScannerWorker;
    worker->moveToThread(thread);
    worker->setRootDir(dir);

    connect(thread, &QThread::started, worker, &FileScannerWorker::start);
    connect(worker, &FileScannerWorker::filesFound, this, [=](const QStringList &files) {
        allFiles = files;
        scheduleUpdateList();
    });
    connect(worker, &FileScannerWorker::finished, this, [=](qint64 ms) {
        qDebug() << "Scan finished in" << ms << "ms";
        worker->deleteLater();
        thread->quit();
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void FilesList::setFiles(const QStringList &files) {
    allFiles = files;
    scheduleUpdateList();
}

void FilesList::clear() {
    allFiles.clear();
    list->clear();
    directory.clear();
}

QStringList FilesList::currentFilteredFiles() const {
    QStringList res;
    for (int i = 0; i < list->count(); ++i) {
        res << list->item(i)->text();
    }
    return res;
}

void FilesList::scheduleUpdateList() { updateTimer->start(); }

void FilesList::updateList() {
    list->clear();

    const auto excludeGlobs = toRegexList(excludeEdit->text().split(';', Qt::SkipEmptyParts));
    const auto showTokens = showEdit->text().toLower().split(';', Qt::SkipEmptyParts);

    QStringList chunk;
    int count = 0;

    for (const auto &rel : allFiles) {
        const auto normPath = normalizePath(rel);
        const auto segments = normPath.split('/', Qt::SkipEmptyParts);
        const auto fileName = QFileInfo(normPath).fileName();

        // Exclude: exact glob-style match per segment
        bool excluded = false;
        for (const auto &rx : excludeGlobs) {
            for (const auto &segment : segments) {
                if (rx.match(segment).hasMatch()) {
                    excluded = true;
                    break;
                }
            }
            if (excluded) {
                break;
            }
        }
        if (excluded) {
            continue;
        }

        // Show: partial substring match per segment (case-insensitive)
        if (!showTokens.isEmpty()) {
            bool matched = false;
            for (const auto &token : showTokens) {
                for (const auto &segment : segments) {
                    if (segment.toLower().contains(token)) {
                        matched = true;
                        break;
                    }
                }
                if (matched) {
                    break;
                }
            }
            if (!matched) {
                continue;
            }
        }

        chunk << rel;

        if (++count % 200 == 0) {
            QCoreApplication::processEvents();
        }
    }

    list->addItems(chunk);
    emit filtersChanged();
}
