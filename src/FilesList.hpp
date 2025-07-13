#ifndef FILESLIST_HPP
#define FILESLIST_HPP

#include <QLineEdit>
#include <QListWidget>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QWidget>

class FileScannerWorker;
class FileFilterWorker;

class FileScannerWorker : public QObject {
    Q_OBJECT
  public:
    explicit FileScannerWorker(QObject *parent = nullptr);
    void setRootDir(const QString &dir);

  public slots:
    void start();

  signals:

    void filesChunkFound(const QStringList &chunk);
    void finished(qint64 elapsedMs);

  private:
    void scanDir(const QString &rootPath, QStringList &buffer);
    QString rootDir;
};

class FilesList : public QWidget {
    Q_OBJECT
  public:
    explicit FilesList(QWidget *parent = nullptr);

    void setFiles(const QStringList &files);
    void setDir(const QString &dir);
    void clear();
    QStringList currentFilteredFiles() const;

  signals:
    void fileSelected(const QString &filename);
    void filtersChanged();
    void requestFiltering(const QStringList &files, const QStringList &excludePatterns,
                          const QStringList &showPatterns);

  private slots:
    void updateList();
    void scheduleUpdateList();
    void updateListChunked(const QStringList &newFiles);

  private:
    QListWidget *list = nullptr;
    QLineEdit *excludeEdit = nullptr;
    QLineEdit *showEdit = nullptr;

    QString directory;
    QStringList fullList;
    QStringList filteredList;

    QThread *filterThread = nullptr;
    FileFilterWorker *filterWorker = nullptr;
    QTimer *updateTimer = nullptr;
};

#endif // FILESLIST_HPP
