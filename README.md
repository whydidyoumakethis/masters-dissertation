# Kiki Engine
*The sharp & cutting edge game engine*

`...`

## Building the Project
To build the project, first install [CMake](https://cmake.org/download/) version 4.2 or higher. Then, navigate to the `build` folder and run the relevant shell/batch file for your OS. If changes are made to any of the `CMakeLists.txt` files it is recommended to delete the `build/output` folder.

#### Windows
[Visual Studio 2026](https://visualstudio.microsoft.com/downloads/) must be installed first. `vs2026-cl.bat` uses the standard C++ compiler that should be included in your Visual Studio installation, however if you want to use Clang you need to install:

* C++ Clang Compiler for Windows 20+
* MSBuild support for LLVM (clang-cl) toolset

These can be installed using the Visual Studio installer.
