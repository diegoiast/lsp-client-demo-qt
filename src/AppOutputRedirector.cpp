#include "AppOutputRedirector.hpp"
#include <QCoreApplication>
#include <QThread>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <QTimer>
#else
#include <unistd.h>
#include <QSocketNotifier>
#endif

AppOutputRedirector::AppOutputRedirector(QObject* parent)
    : QObject(parent)
{
    setup();
}

AppOutputRedirector::~AppOutputRedirector()
{
    cleanup();
}

void AppOutputRedirector::setup()
{
#ifdef _WIN32
    // stdout
    if (_pipe(outPipe, 4096, _O_BINARY) == 0) {
        _dup2(outPipe[1], _fileno(stdout));
        outTimer = new QTimer(this);
        connect(outTimer, &QTimer::timeout, this, [this]() {
            char buf[4096];
            int n = _read(outPipe[0], buf, sizeof(buf));
            if (n > 0)
                emit newStdout(QString::fromLocal8Bit(buf, n));
        });
        outTimer->start(100);
    }
    // stderr
    if (_pipe(errPipe, 4096, _O_BINARY) == 0) {
        _dup2(errPipe[1], _fileno(stderr));
        errTimer = new QTimer(this);
        connect(errTimer, &QTimer::timeout, this, [this]() {
            char buf[4096];
            int n = _read(errPipe[0], buf, sizeof(buf));
            if (n > 0)
                emit newStderr(QString::fromLocal8Bit(buf, n));
        });
        errTimer->start(100);
    }
#else
    // stdout
    if (::pipe(outPipe) == 0) {
        ::dup2(outPipe[1], STDOUT_FILENO);
        outNotifier = new QSocketNotifier(outPipe[0], QSocketNotifier::Read, this);
        connect(outNotifier, &QSocketNotifier::activated, this, [this](int) {
            char buf[4096];
            ssize_t n = ::read(outPipe[0], buf, sizeof(buf));
            if (n > 0)
                emit newStdout(QString::fromLocal8Bit(buf, n));
        });
    }
    // stderr
    if (::pipe(errPipe) == 0) {
        ::dup2(errPipe[1], STDERR_FILENO);
        errNotifier = new QSocketNotifier(errPipe[0], QSocketNotifier::Read, this);
        connect(errNotifier, &QSocketNotifier::activated, this, [this](int) {
            char buf[4096];
            ssize_t n = ::read(errPipe[0], buf, sizeof(buf));
            if (n > 0)
                emit newStderr(QString::fromLocal8Bit(buf, n));
        });
    }
#endif
}

void AppOutputRedirector::cleanup()
{
#ifdef _WIN32
    if (outTimer) outTimer->stop();
    if (errTimer) errTimer->stop();
    if (outPipe[0] != -1) _close(outPipe[0]);
    if (outPipe[1] != -1) _close(outPipe[1]);
    if (errPipe[0] != -1) _close(errPipe[0]);
    if (errPipe[1] != -1) _close(errPipe[1]);
#else
    if (outNotifier) outNotifier->setEnabled(false);
    if (errNotifier) errNotifier->setEnabled(false);
    if (outPipe[0] != -1) ::close(outPipe[0]);
    if (outPipe[1] != -1) ::close(outPipe[1]);
    if (errPipe[0] != -1) ::close(errPipe[0]);
    if (errPipe[1] != -1) ::close(errPipe[1]);
#endif
}
