cmake_minimum_required(VERSION 3.11)

project(KikiEngine)

include(FetchContent)

### Dependencies ###
# volk
FetchContent_Declare(
	volk
	GIT_REPOSITORY	"https://github.com/zeux/volk.git"
	GIT_TAG 	"master"
)

FetchContent_MakeAvailable(volk)

# GLFW
FetchContent_Declare(
	glfw
	GIT_REPOSITORY	"https://github.com/glfw/glfw.git"
	GIT_TAG 	"master"
)

FetchContent_MakeAvailable(glfw)

# glm
FetchContent_Declare(
	glm
	GIT_REPOSITORY	"https://github.com/g-truc/glm.git"
	GIT_TAG 	"0af55ccecd98d4e5a8d1fad7de25ba429d60e863" #refs/tags/1.0.1
)

FetchContent_MakeAvailable(glm)

# Kiki Engine
add_subdirectory(kiki-engine)

# Asset baking
add_subdirectory(asset-baking)

# Demo game
add_subdirectory(games/demo)
