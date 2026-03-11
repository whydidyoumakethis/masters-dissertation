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

# compile shaders
set(SHADER_IN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kiki-engine/renderer/shaders)
set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders/compiled)

file(GLOB SHADERS CONFIGURE_DEPENDS
    ${SHADER_IN_DIR}/*.vert
    ${SHADER_IN_DIR}/*.frag
    ${SHADER_IN_DIR}/*.geom
)

set(COMPILED_SHADERS)

find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)

foreach(shader IN LISTS SHADERS)
    get_filename_component(FILENAME ${shader} NAME)
    set(SPV_FILE ${SHADER_OUT_DIR}/${FILENAME}.spv)

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
        COMMAND ${GLSLANG_VALIDATOR} -V ${shader} -o ${SPV_FILE}
        DEPENDS ${shader}
        COMMENT "Compiling shader: ${FILENAME}"
    )

    list(APPEND COMPILED_SHADERS ${SPV_FILE})
endforeach()

add_custom_target(shaders ALL DEPENDS ${COMPILED_SHADERS})
# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG "v1.17.0"
)
FetchContent_MakeAvailable(spdlog)

# fastGLTF
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY "https://github.com/spnda/fastgltf.git"
    GIT_TAG "v0.9.0"
)
FetchContent_MakeAvailable(fastgltf)

# stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY "https://github.com/nothings/stb.git"
    GIT_TAG "2fb8c5a3deb2110c89669f8d6f36e5833b556b44"
)
FetchContent_MakeAvailable(stb)

# -------- Create an interface library to propagate dependencies --------
# This helps your subdirectories automatically get volk, glfw, glm
add_library(KikiDeps INTERFACE)
target_link_libraries(KikiDeps INTERFACE 
	volk 
	glfw 
	GPUOpen::VulkanMemoryAllocator
	EnTT::EnTT
	spdlog::spdlog
    fastgltf::fastgltf
)
target_include_directories(KikiDeps INTERFACE 
    ${glm_SOURCE_DIR}
    ${stb_SOURCE_DIR}
)
target_compile_definitions(KikiDeps INTERFACE
    PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}"
    PROJECT_BINARY_DIR="${CMAKE_BINARY_DIR}"
)
# Kiki Engine
add_subdirectory(kiki-engine)

# Asset baking
add_subdirectory(asset-baking)

# Demo game
add_subdirectory(games/demo)
