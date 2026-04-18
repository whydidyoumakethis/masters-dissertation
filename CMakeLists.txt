cmake_minimum_required(VERSION 3.15)
project(KikiEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "")

include(FetchContent)

# --- Jolt Physics Configuration ---
# Must be set before FetchContent to ensure Jolt builds with correct settings
# Use dynamic MSVC runtime to match the rest of the engine (Fixes LNK2038)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE INTERNAL "")
# Prevent Jolt from overriding global compiler flags
set(OVERRIDE_CXX_FLAGS OFF CACHE INTERNAL "")
# Enable RTTI and Exceptions for compatibility with EnTT and fastgltf
set(CPP_RTTI_ENABLED ON CACHE INTERNAL "")
set(CPP_EXCEPTIONS_ENABLED ON CACHE INTERNAL "")

# Disable Jolt's internal tests and samples to speed up build
set(TARGET_UNIT_TESTS OFF CACHE INTERNAL "")
set(TARGET_SAMPLES OFF CACHE INTERNAL "")
set(TARGET_HELLO_WORLD OFF CACHE INTERNAL "")

# Jolt Physics
FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics.git"
    GIT_TAG        "v5.5.0"
    SOURCE_SUBDIR  "Build"
)

# volk (Vulkan loader)
FetchContent_Declare(
    volk
    GIT_REPOSITORY "https://github.com/zeux/volk.git"
    GIT_TAG        "d261d2505214c553a9185f310a7dc822cfadc46b"
)

# VulkanMemoryAllocator
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
    GIT_TAG        "e722e57c891a8fbe3cc73ca56c19dd76be242759"
)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw.git"
    GIT_TAG        "7c9ea952338b792cb2f70d7c663e498725073797"
)

# glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    GIT_TAG        "0af55ccecd98d4e5a8d1fad7de25ba429d60e863"
)

# entt (ECS)
FetchContent_Declare(
    entt
    GIT_REPOSITORY "https://github.com/skypjack/entt.git"
    GIT_TAG        "v3.16.0"
)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG        "v1.17.0"
)

# fastgltf
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY "https://github.com/spnda/fastgltf.git"
    GIT_TAG        "v0.9.0"
)

FetchContent_MakeAvailable(
    JoltPhysics volk VulkanMemoryAllocator glfw glm entt spdlog fastgltf
)

# --- Shader Compilation ---
set(SHADER_IN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kiki-engine/renderer/shaders)
set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders/compiled)

file(GLOB SHADERS CONFIGURE_DEPENDS ${SHADER_IN_DIR}/*.vert ${SHADER_IN_DIR}/*.frag ${SHADER_IN_DIR}/*.geom)
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

# --- KikiDeps Interface ---
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

#assimp
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(ASSIMP_ANDROID_JNIIOSYSTEM OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY "https://github.com/assimp/assimp.git"
    GIT_TAG "v6.0.4"
)
FetchContent_MakeAvailable(assimp)

# ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
    GIT_TAG "6c754ed2cb483488cf728a0d80f0152cbcb5eeb8"
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    # Define which source files to compile
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        # Include the specific backends your engine uses
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )

    # Set the include directories so #include <imgui.h> works everywhere
    target_include_directories(imgui PUBLIC 
        ${imgui_SOURCE_DIR} 
        ${imgui_SOURCE_DIR}/backends
    )

    # Link backends to their respective requirements
    # 'volk' and 'glfw' must be defined before this point
    target_link_libraries(imgui PUBLIC volk glfw spdlog::spdlog)
    
    # If using volk (Vulkan Loader), ImGui needs this definition
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
endif()

# -------- Create an interface library to propagate dependencies --------
# This helps your subdirectories automatically get volk, glfw, glm
add_library(KikiDeps INTERFACE)

target_link_libraries(KikiDeps INTERFACE 
    volk 
    glfw 
    VulkanMemoryAllocator
    EnTT::EnTT
    spdlog::spdlog
    fastgltf::fastgltf
    assimp::assimp
    Jolt
    imgui
)

target_include_directories(KikiDeps INTERFACE 
    ${glm_SOURCE_DIR}
    ${joltphysics_SOURCE_DIR} 
    ${stb_SOURCE_DIR}
)
target_compile_definitions(KikiDeps INTERFACE
    PROJECT_ROOT_PATH=\"${CMAKE_SOURCE_DIR}\"
    PROJECT_BINARY_DIR=\"${CMAKE_BINARY_DIR}\"
    
    # Synchronize Jolt debug/release modes
    # $<$<CONFIG:Debug>:JPH_DEBUG>
    # $<$<NOT:$<CONFIG:Debug>>:JPH_RELEASE>
    PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}"
    PROJECT_BINARY_DIR="${CMAKE_BINARY_DIR}"
    PROJECT_ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets"
    PROJECT_SHADER_PATH="${CMAKE_BINARY_DIR}/shaders/compiled"
)

# if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
#     target_compile_definitions(KikiDeps INTERFACE JPH_USE_NEON)
# else()
#     target_compile_definitions(KikiDeps INTERFACE JPH_USE_SSE4_1 JPH_USE_SSE4_2 JPH_USE_AVX JPH_USE_AVX2)
# endif()

# --- Subdirectories ---
add_subdirectory(kiki-engine)
add_subdirectory(asset-baking)
add_subdirectory(games/demo)