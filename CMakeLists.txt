cmake_minimum_required(VERSION 3.24)
project(lsp_client_demo_qt LANGUAGES CXX)

# set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(LSP_USE_SANITIZERS OFF CACHE BOOL "Disable sanitizers")

include(FetchContent)


find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

FetchContent_Declare(
    lsp-framework
    GIT_REPOSITORY https://github.com/leon-bckl/lsp-framework.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(lsp-framework)

add_subdirectory(src)
