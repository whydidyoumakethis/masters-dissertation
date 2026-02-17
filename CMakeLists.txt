cmake_minimum_required(VERSION 3.11)

project(KikiEngine)

include(FetchContent)

### Dependencies ###
# volk
FetchContent_Declare(
	volk
	GIT_REPOSITORY	"https://github.com/zeux/volk.git"
	GIT_TAG 	"d261d2505214c553a9185f310a7dc822cfadc46b"
)

FetchContent_MakeAvailable(volk)

# GLFW
FetchContent_Declare(
	glfw
	GIT_REPOSITORY	"https://github.com/glfw/glfw.git"
	GIT_TAG 	"7c9ea952338b792cb2f70d7c663e498725073797"
)

FetchContent_MakeAvailable(glfw)

# glm
FetchContent_Declare(
	glm
	GIT_REPOSITORY	"https://github.com/g-truc/glm.git"
	GIT_TAG 	"0af55ccecd98d4e5a8d1fad7de25ba429d60e863" #refs/tags/1.0.1
)

FetchContent_MakeAvailable(glm)

# -------- Create an interface library to propagate dependencies --------
# This helps your subdirectories automatically get volk, glfw, glm
add_library(KikiDeps INTERFACE)
target_link_libraries(KikiDeps INTERFACE volk glfw)
target_include_directories(KikiDeps INTERFACE ${glm_SOURCE_DIR})

# Kiki Engine
add_subdirectory(kiki-engine)

# Asset baking
add_subdirectory(asset-baking)

# Demo game
add_subdirectory(games/demo)
