#include <QProcess>

#include <cstring>
#include <iostream>
#include <stdexcept>

#include "LspClientImpl.hpp"


// internal class
class QProcessStream : public lsp::io::Stream {
public:
    bool debugIO  = false;
    
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
            if (debugIO) {
                std::cerr << "Read bytes " << size << ": " << std::string(buffer, size) << std::endl;
            }
            totalRead += bytesRead;
        }
    }

    void write(const char* buffer, std::size_t size) override {
        qint64 bytesWritten = m_process->write(buffer, size);
        if (bytesWritten < 0) {
            throw std::runtime_error("Failed to write to QProcess");
        }
        if (debugIO) {
            std::cerr << "Wrote bytes " << size << ": " << std::string(buffer, size) << std::endl;
        }
        if (!m_process->waitForBytesWritten(1)) {
            throw std::runtime_error("Timeout writing to QProcess");
        }
    }

private:
    QProcess* m_process;
};

LspClientImpl::LspClientImpl() {}

void LspClientImpl::debugIO(bool enable) {
    auto io = dynamic_cast<QProcessStream*>(this->m_clandIO.get());
    io->debugIO = enable;
}

void LspClientImpl::setDocumentRoot(const std::string &newRoot) {
    m_documentRoot = newRoot;
    initializeLspServer();
}

void LspClientImpl::openDocument(const std::string &fileName, const std::string &fileContents)
{
    // When opening a file:
     lsp::notifications::TextDocument_DidOpen::Params params{
        .textDocument = {
            .uri = "file://" + fileName, 
            .languageId = "cpp", // or "c", "python", etc.
            .version = 1, 
            .text = fileContents // The full text of the opened file
        }
    };
    m_messageHandler->sendNotification<lsp::notifications::TextDocument_DidOpen>(std::move(params));

}

void LspClientImpl::hover(const std::string &fileName, int line, int column, std::function<void(lsp::requests::TextDocument_Hover::Result &&result)> callback)
{
    lsp::HoverParams params;
    params.textDocument.uri = std::string("file://") + fileName;
    params.position.line = line;
    params.position.character = column;
    // params.workDoneToken
    
    m_messageHandler->sendRequest<lsp::requests::TextDocument_Hover>(
        std::move(params), [callback = std::move(callback)](auto result) {
            callback(std::move(result));
        },
        [](const lsp::Error &error) {
            std::cerr << "Failed to get response from LSP server: " << error.what() << std::endl;
        }
    );
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
        std::cerr << "Failed to start clangd!";
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
    delete clangdProcess;
    clangdProcess = nullptr;
}

void LspClientImpl::initializeLspServer() {
    auto initializeParams = lsp::requests::Initialize::Params{};
    initializeParams.rootUri = "file://" + m_documentRoot;
    initializeParams.capabilities = {};

    auto id = m_messageHandler->sendRequest<lsp::requests::Initialize>(
        std::move(initializeParams),
        [](lsp::requests::Initialize::Result &&result) {
            std::cout << " - Server initialized successfully\n";
            if (result.capabilities.textDocumentSync.has_value()) {
                std::cout << " - Text document sync supported\n";
            }
            if (result.capabilities.completionProvider.has_value()) {
                std::cout << " - Completion provider supported\n";
            }
            if (result.capabilities.hoverProvider.has_value()) {
                std::cout << " - Hover provider supported\n";
            }
            if (result.capabilities.documentHighlightProvider.has_value()) {
                std::cout << " - Document highlight provider provider supported\n";
            }
            if (result.capabilities.renameProvider.has_value()) {
                std::cout << " - Rename provider provider supported\n";
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
    while (m_running) {
        m_messageHandler->processIncomingMessages();
    }
}
