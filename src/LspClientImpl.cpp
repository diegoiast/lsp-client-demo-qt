#include <QProcess>
#include <QSocketNotifier>
#include <QThread>
#include <cstring>
#include <fstream>
#include <iostream>
#include <qdebug.h>
#include <stdexcept>

#include "LspClientImpl.hpp"


// internal class
class QProcessStream : public lsp::io::Stream {
public:
    explicit QProcessStream(QProcess* process)
        : m_process(process)
    {
        Q_ASSERT(process);
    }

    void read(char* buffer, std::size_t size) override {
        std::size_t totalRead = 0;
        while (totalRead < size) {
            if (m_process->bytesAvailable() == 0 && !m_process->waitForReadyRead(-1)) {
                throw std::runtime_error("Timeout or error reading from QProcess");
            }

            qint64 bytesRead = m_process->read(buffer + totalRead, size - totalRead);
            if (bytesRead <= 0) {
                throw std::runtime_error("Failed to read from QProcess");
            }

            totalRead += bytesRead;
        }
    }

    void write(const char* buffer, std::size_t size) override {
        qint64 bytesWritten = m_process->write(buffer, size);
        if (bytesWritten < 0) {
            throw std::runtime_error("Failed to write to QProcess");
        }

        if (!m_process->waitForBytesWritten(1)) {
            throw std::runtime_error("Timeout writing to QProcess");
        }
    }

private:
    QProcess* m_process;
};

LspClientImpl::LspClientImpl() {}

void LspClientImpl::setDocumentRoot(const std::string &newRoot) {
    m_documentRoot = newRoot;
    initializeLspServer();
}

LspClientImpl::~LspClientImpl() {
    shutdownLspServer();
    stopClangd();
}

void LspClientImpl::startClangd() {
    clangdProcess = new QProcess();
    clangdProcess->setProgram("/usr/bin/clangd");
    clangdProcess->setProcessChannelMode(QProcess::SeparateChannels);
    clangdProcess->start();

    if (!clangdProcess->waitForStarted()) {
        qWarning() << "Failed to start clangd!";
        return;
    }

    m_clandIO  = std::make_unique<QProcessStream>(clangdProcess);
    m_connection = std::make_unique<lsp::Connection>(*m_clandIO);
    m_messageHandler = std::make_unique<lsp::MessageHandler>(*m_connection);
    m_running = true;
    m_workerThread = std::thread(&LspClientImpl::runLoop, this);
}

void LspClientImpl::stopClangd() {

    m_running = false;
    // Send shutdown/exit message if needed here
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void LspClientImpl::initializeLspServer() {
    auto initializeParams = lsp::requests::Initialize::Params{};
    initializeParams.rootUri = "file://" + m_documentRoot;
    initializeParams.capabilities = {};

    auto id = m_messageHandler->sendRequest<lsp::requests::Initialize>(
        lsp::requests::Initialize::Params{/* parameters */},
        [](lsp::requests::Initialize::Result &&result) {
            std::cerr << " - Server initialized successfully\n";
            if (result.capabilities.textDocumentSync.has_value()) {
                std::cerr << " - Text document sync supported\n";
            }
            if (result.capabilities.completionProvider.has_value()) {
                std::cerr << " - Completion provider supported\n";
            }
            if (result.capabilities.hoverProvider.has_value()) {
                std::cerr << " - Hover provider supported\n";
            }
        },
        [](const lsp::Error &error) {
            std::cerr << "Failed to get response from LSP server: " << error.what() << std::endl;
        });
    if (auto strPtr = std::get_if<lsp::json::String>(&id)) {
        std::cerr << "lsp::requests::Initialize - String ID: " << *strPtr << "\n";
    } else if (auto intPtr = std::get_if<lsp::json::Integer>(&id)) {
        std::cerr << "lsp::requests::Initialize - Integer ID: " << *intPtr << "\n";
    } else if (std::holds_alternative<lsp::json::Null>(id)) {
        std::cerr << "lsp::requests::Initialize - ID is null\n";
    }
#if 0
    auto [id, result] = m_messageHandler->sendRequest<lsp::requests::TextDocument_Diagnostic>(
        lsp::requests::TextDocument_Diagnostic::Params{/* parameters */});
#endif
}

void LspClientImpl::shutdownLspServer() {
}

void LspClientImpl::runLoop() {
    QThread::sleep(2);
    while (m_running) {
        m_messageHandler->processIncomingMessages();
    }
}
