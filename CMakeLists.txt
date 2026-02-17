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

# VulkanMemoryAllocator
FetchContent_Declare(
	VulkanMemoryAllocator
	GIT_REPOSITORY	"https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
	GIT_TAG 	"e722e57c891a8fbe3cc73ca56c19dd76be242759"
)

FetchContent_MakeAvailable(VulkanMemoryAllocator)

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

# Kiki Engine
add_subdirectory(kiki-engine)

# Asset baking
add_subdirectory(asset-baking)

# Demo game
add_subdirectory(games/demo)
