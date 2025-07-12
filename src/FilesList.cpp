#include "FilesList.hpp"

#include <QLabel>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

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

    addFilesRecursive(directory);
    allFiles.sort(Qt::CaseInsensitive);
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
    list->clear();
    auto excludeStr = excludeEdit->text().trimmed();
    auto showStr = showEdit->text().trimmed();

    QStringList excludeList;
    if (!excludeStr.isEmpty())
        excludeList = excludeStr.split(';', Qt::SkipEmptyParts);

    for (const auto& rel : allFiles) {
        auto excluded = false;
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
        list->addItem(rel);
    }

    emit filtersChanged();
}
