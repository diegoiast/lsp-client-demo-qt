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
    emit finished(timer.elapsed());
}

void FileScannerWorker::scanDir(const QString &rootPath, QStringList &buf) {
    QStringList chunk;
    const int chunkSize = 200;
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
                chunk << normalizePath(rel);
                if (chunk.size() >= chunkSize) {
                    emit filesChunkFound(chunk);
                    chunk.clear();
                    QThread::msleep(20);
                }
            }
        }
    }
    if (!chunk.isEmpty()) {
        emit filesChunkFound(chunk);
    }
}

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
    connect(worker, &FileScannerWorker::filesChunkFound, this, [=](const QStringList &chunk) {
        fullList.append(chunk);
        updateListChunked(chunk);
    });
    connect(worker, &FileScannerWorker::finished, this, [=](qint64 ms) {
        connect(worker, &FileScannerWorker::finished, this, [=](qint64 ms) {
            qDebug() << "Scan finished in" << ms << "ms";
            updateList();
            worker->deleteLater();
            thread->quit();
        });
    });
    connect(thread, &QThread::started, worker, &FileScannerWorker::start);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void FilesList::setFiles(const QStringList &files) {
    fullList = files;
    scheduleUpdateList();
}

void FilesList::clear() {
    fullList.clear();
    filteredList.clear();
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

void FilesList::updateListChunked(const QStringList &newFiles) {
    auto excludes = toRegexList(excludeEdit->text().split(';', Qt::SkipEmptyParts));
    auto shows = toRegexList(showEdit->text().split(';', Qt::SkipEmptyParts));
    auto filteredChunk = QStringList();

    for (const auto &rel : newFiles) {
        auto normPath = normalizePath(rel).toLower();
        auto segments = normPath.split('/', Qt::SkipEmptyParts);
        auto excluded = false;

        for (auto const &rx : excludes) {
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

        if (!shows.isEmpty()) {
            bool matched = false;
            for (auto const &rx : shows) {
                for (auto const &segment : segments) {
                    if (rx.match(segment).hasMatch() ||
                        segment.contains(rx.pattern(), Qt::CaseInsensitive)) {
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

        filteredChunk << rel;
    }

    filteredChunk.sort(Qt::CaseInsensitive);
    for (auto const &rel : filteredChunk) {
        auto *item = new QListWidgetItem(QDir::toNativeSeparators(rel));
        item->setToolTip(QDir::toNativeSeparators(directory + rel));
        list->addItem(item);
    }

    filteredList.append(filteredChunk);
}

void FilesList::updateList() {
    auto const excludeGlobs = toRegexList(excludeEdit->text().split(';', Qt::SkipEmptyParts));
    auto const showTokens = showEdit->text().toLower().split(';', Qt::SkipEmptyParts);
    auto chunk = QStringList();
    auto count = 0;

    list->clear();
    for (const auto &rel : fullList) {
        auto const normPath = normalizePath(rel);
        auto const segments = normPath.split('/', Qt::SkipEmptyParts);
        auto const fileName = QFileInfo(normPath).fileName();

        auto excluded = false;
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

    chunk.sort(Qt::CaseInsensitive);
    for (auto const &rel : chunk) {
        auto *item = new QListWidgetItem(QDir::toNativeSeparators(rel));
        item->setToolTip(QDir::toNativeSeparators(directory + rel));
        list->addItem(item);
    }
    emit filtersChanged();
}
