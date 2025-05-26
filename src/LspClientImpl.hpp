#pragma once

#include <QProcess>
#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include <lsp/process.h>
#include <lsp/io/stream.h>
#include <lsp/connection.h>
#include <lsp/messagehandler.h>
#include <lsp/messages.h>


class LspClientImpl {
  public:
    explicit LspClientImpl();
    ~LspClientImpl();

    // Disable copy and move
    LspClientImpl(const LspClientImpl &) = delete;
    LspClientImpl &operator=(const LspClientImpl &) = delete;
    LspClientImpl(LspClientImpl &&) = delete;
    LspClientImpl &operator=(LspClientImpl &&) = delete;

    void debugIO(bool enable);

    void setDocumentRoot(const std::string &documentRoot);
    void openDocument(const std::string &fileName, const std::string &fileContents);
    void hover(const std::string &fileName, int line, int column, std::function<void(lsp::requests::TextDocument_Hover::Result &&result)> callback);

    void startClangd();
    void stopClangd();
    void initializeLspServer();
    void shutdownLspServer();

  private:
    void runLoop();
    std::string m_documentRoot;
    std::unique_ptr<lsp::Connection> m_connection;
    std::unique_ptr<lsp::MessageHandler> m_messageHandler;
    std::unique_ptr<lsp::Process> m_clandIO;

    std::thread m_workerThread;
    std::atomic_bool m_running{false};
};
