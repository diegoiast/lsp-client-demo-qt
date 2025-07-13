qt_add_executable(lsp_client_demo_qt WIN32
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
    AppOutputRedirector.cpp
    AppOutputRedirector.hpp
    CodeEditor.cpp
    CodeEditor.hpp
    FilesList.cpp
    FilesList.hpp
    LspClientImpl.cpp
    LspClientImpl.hpp
)

target_link_libraries(lsp_client_demo_qt PRIVATE Qt6::Widgets lsp)
