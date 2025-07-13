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
    void filesFound(const QStringList &files);
    void finished(qint64 elapsedMs);

  private:
    void scanDir(const QString &rootPath, QStringList &buffer);
    QString rootDir;
};

#if 0
class FileFilterWorker : public QObject {
    Q_OBJECT
  public:
    explicit FileFilterWorker(QObject *parent = nullptr);

  public slots:
    // void processFilter(const QStringList &files, const QStringList &excludePatterns,
    // const QStringList &showPatterns);

  signals:
    void filteredFilesChunk(const QStringList &chunk);
    void filteringFinished();
};
#endif

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

  private:
    QListWidget *list = nullptr;
    QLineEdit *excludeEdit = nullptr;
    QLineEdit *showEdit = nullptr;

    QString directory;
    QStringList allFiles;

    QThread *filterThread = nullptr;
    FileFilterWorker *filterWorker = nullptr;
    QTimer *updateTimer = nullptr;
};

#endif // FILESLIST_HPP
