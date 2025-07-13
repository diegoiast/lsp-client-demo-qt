#include "FilesList.hpp"

#include <QLabel>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QElapsedTimer>

FilesList::FilesList(QWidget* parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    list = new QListWidget;
    excludeEdit = new QLineEdit;
    showEdit = new QLineEdit;

    excludeEdit->setText("build");
    showEdit->setPlaceholderText(tr("Show (substring, empty=all)"));
    showEdit->setToolTip(tr("Show (substring, empty=all)"));
    excludeEdit->setPlaceholderText("Exclude (semicolon-separated substrings)");
    excludeEdit->setToolTip("Exclude (semicolon-separated substrings)");

    layout->addWidget(list);
    layout->addWidget(excludeEdit);
    layout->addWidget(showEdit);

    connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit fileSelected(item->text());
    });
    connect(excludeEdit, &QLineEdit::textChanged, this, &FilesList::updateList);
    connect(showEdit, &QLineEdit::textChanged, this, &FilesList::updateList);
}

void FilesList::setFiles(const QStringList& files) {
    allFiles = files;
    updateList();
}

void FilesList::setDir(const QString& dir) {
    directory = dir;
    allFiles.clear();

    static QRegularExpression rx(R"((\.h|\.hpp|\.c|\.cpp|\.txt|\.md)$)", QRegularExpression::CaseInsensitiveOption);

    std::function<void(const QString&)> addFilesRecursive = [&](const QString& currentDir) {
        QDir dir(currentDir);
        auto entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
        for (const auto& entry : entries) {
            if (entry.isDir()) {
                addFilesRecursive(entry.absoluteFilePath());
            } else {
                auto name = entry.fileName();
                if (rx.match(name).hasMatch() || name.compare("makefile", Qt::CaseInsensitive) == 0) {
                    auto relPath = QDir(directory).relativeFilePath(entry.absoluteFilePath());
                    allFiles << relPath;
                }
            }
        }
    };

    QElapsedTimer t;
    t.start();
    addFilesRecursive(directory);
    qDebug() << "Loading files took" << t.elapsed();
    allFiles.sort(Qt::CaseInsensitive);
    qDebug() << "Sorting files took" << t.elapsed();
    updateList();
}

void FilesList::clear() {
    allFiles.clear();
    list->clear();
    directory.clear();
}

QStringList FilesList::currentFilteredFiles() const {
    QStringList result;
    for (int i = 0; i < list->count(); ++i) {
        result << list->item(i)->text();
    }
    return result;
}


void FilesList::updateList() {
    QElapsedTimer timer;
    timer.start();

    list->clear();

    const QString excludeStr = excludeEdit->text().trimmed();
    const QString showStr = showEdit->text().trimmed();

    const QStringList excludePatterns = excludeStr.split(';', Qt::SkipEmptyParts);
    const QStringList showPatterns = showStr.split(';', Qt::SkipEmptyParts);

    auto toRegexList = [](const QStringList& patterns) {
        QList<QRegularExpression> regexList;
        for (const auto& pattern : patterns) {
            QString p = QRegularExpression::escape(pattern.trimmed());
            p.replace("\\*", ".*");
            p.replace("\\?", ".");
            regexList.append(QRegularExpression("^" + p + "$", QRegularExpression::CaseInsensitiveOption));
        }
        return regexList;
    };

    const auto excludeRegexes = toRegexList(excludePatterns);
    const auto showRegexes = toRegexList(showPatterns);

    for (const QString& rel : allFiles) {
        const QString relLower = rel.toLower();
        const QStringList components = relLower.split('/', Qt::SkipEmptyParts);
        const QFileInfo fi(rel);
        const QString fileNameLower = fi.fileName().toLower();

        // --- EXCLUDE FILTER ---
        bool excluded = false;
        for (const QString& ex : excludePatterns) {
            const QString trimmed = ex.trimmed().toLower();
            if (trimmed.isEmpty())
                continue;

            // Exact component match only (no substring)
            if (components.contains(trimmed)) {
                excluded = true;
                break;
            }

            // Exact full relative path match only
            if (relLower == trimmed) {
                excluded = true;
                break;
            }

            // Exact filename match only
            if (fileNameLower == trimmed) {
                excluded = true;
                break;
            }
        }

        // Glob pattern exclude match on full path or filename
        if (!excluded) {
            for (const auto& rx : excludeRegexes) {
                if (rx.match(rel).hasMatch() || rx.match(fi.fileName()).hasMatch()) {
                    excluded = true;
                    break;
                }
            }
        }

        if (excluded)
            continue;

        // --- SHOW FILTER ---
        if (!showPatterns.isEmpty()) {
            bool matched = false;

            for (const QString& pattern : showPatterns) {
                const QString trimmed = pattern.trimmed().toLower();
                if (trimmed.isEmpty())
                    continue;

                // Substring or exact match in full path (this part is substring as original, can adjust if needed)
                if (relLower.contains(trimmed) || relLower == trimmed) {
                    matched = true;
                    break;
                }
            }

            // Glob match on full path or filename
            if (!matched) {
                for (const auto& rx : showRegexes) {
                    if (rx.match(rel).hasMatch() || rx.match(fi.fileName()).hasMatch()) {
                        matched = true;
                        break;
                    }
                }
            }

            if (!matched)
                continue;
        }

        list->addItem(rel);
    }

    emit filtersChanged();

    qint64 elapsedMs = timer.elapsed();
    qDebug("FilesList::updateList took %lld ms", elapsedMs);
}
