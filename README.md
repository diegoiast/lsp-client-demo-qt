# LSP client demo for Qt 

This repo shows how to use [LSP framework](https://github.com/leon-bckl/lsp-framework)
to connect your application to an LSP server (like `clangd`). 
While the example here is for C/C++, the same exact mechanism can
be used to connect a Python LSP and get the same

To learn about LSP visit - 
https://microsoft.github.io/language-server-protocol/ and  https://en.wikipedia.org/wiki/Language_Server_Protocol.


## Building

Just clone the repo, and build using CMake

```
git clone https://github.com/diegoiast/lsp-client-demo-qt
cd lsp-client-demo-qt
cmake -S . -B build -G Ninja
```

Building using Ninja is not stricly needed, default is OK 
(MSBuild on Windows, and GNU Makefiles on Linux by default). 

You can just open the CMake in QtCreator, or the whole dir in 
VSCode. This will configure and build the project for you.


`lsp-framework` is pulled using CMake `FetchContent_Declare`, no
need to install it locally. Using `CPM` will work as well.

## Requirements

* A C++ 20 compiler (GCC 14 from Debian testing)
* CMake 3.24, or newer
* Qt 6.8.3 and above (for the UI only, read further on for
  more information).

## Status

1. Not working yet.
  1. Iniailize server is working
  1. Hover is WIP, non reliable yet.
  1. Server gets reset (?) sometimes.
2. Tested only on Linux, Windows will come soon.
3. Forking clang is using QProcess. I want to remove this,
   as I assume not all users of LSP will be using Qt. 
4. macOS is not tested. 

License MIT

Diego Iastrubni diegoiast@gmail.com

