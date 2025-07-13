#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QStringList>

class FileScannerWorker : public QObject {
    Q_OBJECT

public:
    explicit FileScannerWorker(QObject* parent = nullptr);
    void setRootDir(const QString& dir);
    void start();

signals:
    void filesFound(const QStringList& files);  // emits after each dir
    void finished(qint64 elapsedMs);  // include time
private:
    void scanDir(const QString& path, QStringList& buffer);

    QString rootDir;
    QRegularExpression fileFilter;
};

class FilesList : public QWidget {
    Q_OBJECT

public:
    explicit FilesList(QWidget* parent = nullptr);

    void setFiles(const QStringList& files);
    void setDir(const QString& dir);
    void clear();
    QStringList currentFilteredFiles() const;

signals:
    void fileSelected(const QString& relPath);
    void filtersChanged();

private:
    void updateList();

    QStringList allFiles;
    QString directory;

    QListWidget* list;
    QLineEdit* excludeEdit;
    QLineEdit* showEdit;
};

