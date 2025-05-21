qt_add_executable(qt6_dir_viewer
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
)

target_link_libraries(qt6_dir_viewer PRIVATE Qt6::Widgets)
