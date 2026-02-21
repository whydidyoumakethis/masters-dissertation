cmake_minimum_required(VERSION 3.13)

project(KikiEngine)

set(CMAKE_CXX_STANDARD 23)

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

# entt
FetchContent_Declare(
    entt
    GIT_REPOSITORY "https://github.com/skypjack/entt.git"
    GIT_TAG "v3.16.0"
)

FetchContent_MakeAvailable(entt)

# Reflect
FetchContent_Declare(
    reflect
    GIT_REPOSITORY "https://github.com/qlibs/reflect.git"
    GIT_TAG "96b9548a6e114368a3ab22a674a0feab7da7cfd8"
)

FetchContent_MakeAvailable(reflect)

# -------- Create an interface library to propagate dependencies --------
# This helps your subdirectories automatically get volk, glfw, glm
add_library(KikiDeps INTERFACE)
target_link_libraries(KikiDeps INTERFACE 
	volk 
	glfw 
	VulkanMemoryAllocator
	EnTT::EnTT
)
target_include_directories(KikiDeps INTERFACE ${glm_SOURCE_DIR})

# Kiki Engine
add_subdirectory(kiki-engine)

# Asset baking
add_subdirectory(asset-baking)

# Demo game
add_subdirectory(games/demo)
