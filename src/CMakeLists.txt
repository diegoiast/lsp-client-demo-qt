qt_add_executable(lsp_client_demo_qt
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
    LspClientImpl.cpp
    LspClientImpl.hpp
    AppOutputRedirector.cpp
    AppOutputRedirector.hpp
)

target_link_libraries(lsp_client_demo_qt PRIVATE Qt6::Widgets lsp)
