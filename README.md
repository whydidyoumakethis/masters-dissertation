# Kiki Engine
*The sharp & cutting edge game engine*

`...`

## Building the Project
To build the project, first install [CMake](https://cmake.org/download/) version 3.13 or higher. Then, navigate to the `build` folder and run the relevant shell/batch file for your OS. If changes are made to any of the `CMakeLists.txt` files it is recommended to delete the `build/output` folder.

#### Windows
[Visual Studio](https://visualstudio.microsoft.com/downloads/) must be installed first. `vs2026-cl.bat` uses the standard C++ compiler that should be included in your Visual Studio installation, however if you want to use Clang you need to install:

Visual Studio 2026
* *C++ Clang Compiler for Windows 20+*
* *MSBuild support for LLVM (clang-cl) toolset*

Visual Studio 2022
* *C++ Clang Compiler for Windows 12.0.0+*
* *C++ Clang-cl for v143+ build tools (x64/x86)*

These can be installed using the Visual Studio installer.

#### MacOS/Linux
`./unix.sh [build type] [compiler]`

Defaults to `Debug` and clang++. Creates Makefiles in `output/[build type]`.

Example for Linux: `./unix.sh Debug g++`.


***


**Code for COMP5531M @ University of Leeds** \
*Markus' Voxel Soldiers (2026)*