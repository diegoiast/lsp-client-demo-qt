#ifndef FILESLIST_HPP
#define FILESLIST_HPP

#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QWidget>

class QLineEdit;
class QListWidget;
class FileScannerWorker;
class FileFilterWorker;
class LoadingWidget;

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
    void scheduleUpdateList();
    void updateList(const QStringList &files, bool clearList);

  private:
    LoadingWidget *loadingWidget = nullptr;
    QListWidget *list = nullptr;
    QLineEdit *excludeEdit = nullptr;
    QLineEdit *showEdit = nullptr;

    QString directory;
    QStringList fullList;

    QThread *filterThread = nullptr;
    FileFilterWorker *filterWorker = nullptr;
    QTimer *updateTimer = nullptr;
};

#endif // FILESLIST_HPP
