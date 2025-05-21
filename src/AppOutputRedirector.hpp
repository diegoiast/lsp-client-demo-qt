#pragma once
#include <QObject>
#include <QString>

class AppOutputRedirector : public QObject {
    Q_OBJECT
public:
    explicit AppOutputRedirector(QObject* parent = nullptr);
    ~AppOutputRedirector();

signals:
    void newStdout(const QString&);
    void newStderr(const QString&);

private:
#ifdef _WIN32
    int outPipe[2] = {-1, -1};
    int errPipe[2] = {-1, -1};
    class QTimer* outTimer = nullptr;
    class QTimer* errTimer = nullptr;
#else
    int outPipe[2] = {-1, -1};
    int errPipe[2] = {-1, -1};
    class QSocketNotifier* outNotifier = nullptr;
    class QSocketNotifier* errNotifier = nullptr;
#endif
    void setup();
    void cleanup();
};
