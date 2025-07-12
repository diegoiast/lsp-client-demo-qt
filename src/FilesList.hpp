#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QStringList>

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
