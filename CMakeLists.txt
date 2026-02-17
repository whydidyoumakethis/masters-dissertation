cmake_minimum_required(VERSION 3.11)

project(KikiEngine)

include(FetchContent)

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
