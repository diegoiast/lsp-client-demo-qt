#include <cstring>
#include <iostream>

#include "LspClientImpl.hpp"
#include "lsp/fileuri.h"

LspClientImpl::LspClientImpl() {}

void LspClientImpl::debugIO(bool enable) { (void)enable; }

void LspClientImpl::setDocumentRoot(const std::string &newRoot) {
    m_documentRoot = newRoot;
    initializeLspServer();
}

void LspClientImpl::openDocument(const std::string &fileName, const std::string &fileContents) {
    if (!m_running) {
        return;
    }
    // When opening a file:
    lsp::notifications::TextDocument_DidOpen::Params params{
        .textDocument = {
            .uri = lsp::FileUri::fromPath(fileName),
            .languageId = "cpp", // or "c", "python", etc.
            .version = 1,
            .text = fileContents // The full text of the opened file
        }};
    m_messageHandler->sendNotification<lsp::notifications::TextDocument_DidOpen>(std::move(params));
}

void LspClientImpl::hover(
    const std::string &fileName, int line, int column,
    std::function<void(lsp::requests::TextDocument_Hover::Result &&result)> callback) {

    if (!m_running) {
        return;
    }

    lsp::HoverParams params;
    params.textDocument.uri = lsp::FileUri::fromPath(fileName);
    params.position.line = line;
    params.position.character = column;
    // params.workDoneToken

    m_messageHandler->sendRequest<lsp::requests::TextDocument_Hover>(
        std::move(params),
        [callback = std::move(callback)](auto result) { callback(std::move(result)); },
        [](const lsp::Error &error) {
            std::cerr << "Failed to get response from LSP server: " << error.what() << std::endl;
        });
}

LspClientImpl::~LspClientImpl() {
    shutdownLspServer();
    stopClangd();
}

void LspClientImpl::startClangd() {
    try {
#if defined(WIN32)
        m_clandIO = std::make_unique<lsp::Process>("C :\\Program Files\\LLVM\\bin\\clangd.exe");
#elif defined(POSIX)
        m_clandIO = std::make_unique<lsp::Process>("/usr/bin/clangd");
#endif
        m_connection = std::make_unique<lsp::Connection>(m_clandIO->stdIO());
        m_messageHandler = std::make_unique<lsp::MessageHandler>(*m_connection);
        m_running = true;
        m_workerThread = std::thread(&LspClientImpl::runLoop, this);
    } catch (lsp::ProcessError e) {
    }
}

void LspClientImpl::stopClangd() {
    m_running = false;
    // Send shutdown/exit message if needed here
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void LspClientImpl::initializeLspServer() {
    if (!m_running) {
        return;
    }

    auto initializeParams = lsp::requests::Initialize::Params{};
    initializeParams.rootUri = lsp::FileUri::fromPath(m_documentRoot);
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

void LspClientImpl::shutdownLspServer() {}

void LspClientImpl::runLoop() {
    while (m_running) {
        m_messageHandler->processIncomingMessages();
    }
}
