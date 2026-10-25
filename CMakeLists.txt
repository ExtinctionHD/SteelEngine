cmake_minimum_required(VERSION 3.22.1)

project("SteelEngine")

# Vulkan
find_package(Vulkan REQUIRED)

# glfw
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
add_subdirectory(External/glfw)

# glslang
option(ENABLE_GLSLANG_BINARIES "" OFF)
option(GLSLANG_TESTS "" OFF)
option(GLSLANG_ENABLE_INSTALL "" OFF)
option(ENABLE_HLSL "" OFF)
option(ENABLE_OPT "" OFF)
option(BUILD_EXTERNAL "Build glslang external dependencies" OFF)
add_subdirectory(External/glslang)

# tetgen
add_library(tetgen STATIC External/tetgen/tetgen.cxx External/tetgen/predicates.cxx)
set_target_properties(tetgen PROPERTIES FOLDER "tetgen")
target_compile_definitions(tetgen PRIVATE TETLIBRARY)

# imgui
file(GLOB IMGUI_HEADERS LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/External/imgui/misc/cpp/*.h"
    "${PROJECT_SOURCE_DIR}/External/imgui/examples/*glfw*.h"
    "${PROJECT_SOURCE_DIR}/External/imgui/examples/*vulkan*.h"
)
file(GLOB IMGUI_SOURCES LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/External/imgui/*.cpp"
    "${PROJECT_SOURCE_DIR}/External/imgui/misc/cpp/*.cpp"
    "${PROJECT_SOURCE_DIR}/External/imgui/backends/*glfw*.cpp"
    "${PROJECT_SOURCE_DIR}/External/imgui/backends/*vulkan*.cpp"
)

# spirv-reflect
file(GLOB SPIRV_REFLECT_FILES LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/External/SPIRV-Reflect/spirv_reflect.cpp"
    "${PROJECT_SOURCE_DIR}/External/SPIRV-Reflect/spirv_reflect.h"
)

# easy profiler
option(EASY_PROFILER_NO_GUI "" ON)
option(EASY_PROFILER_NO_SAMPLES "" ON)
add_subdirectory(External/easy_profiler)

# sources
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/Source)
file(GLOB_RECURSE SOURCES LIST_DIRECTORIES false
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.hpp"
    "${SOURCE_DIR}/*.c"
    "${SOURCE_DIR}/*.h"
)

# sources groups
source_group("Source\\External\\Imgui" FILES ${IMGUI_HEADERS})
source_group("Source\\External\\Imgui\\Private" FILES ${IMGUI_SOURCES})
source_group("Source\\External\\SPIRV-Reflect" FILES ${SPIRV_REFLECT_FILES})
foreach(source IN ITEMS ${SOURCES})
    get_filename_component(source_path "${source}" PATH)
    file(RELATIVE_PATH source_path_rel "${PROJECT_SOURCE_DIR}" "${source_path}")
    string(REPLACE "/" "\\" group_path "${source_path_rel}")
    source_group("${group_path}" FILES "${source}")
endforeach()

# SteelEngine
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${IMGUI_HEADERS}
    ${IMGUI_SOURCES}
    ${SPIRV_REFLECT_FILES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    USE_FOLDERS ON
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /MP)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror -Wno-missing-field-initializers)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    External/VulkanMemoryAllocator/include/
    External/glfw/include/
    External/glslang/
    External/glslang/glslang/Include
    External/glslang/glslang/Public
    External/glm/
    External/imgui/
    External/stb/
    External/tinygltf/
    External/tetgen/
    External/portable-file-dialogs/
    External/entt/src/
    External/easy_profiler/easy_profiler_core/include
    External/SPIRV-Reflect/
    External/inifile-cpp/include/
    External/cyCodeBase/
)

target_include_directories(${PROJECT_NAME} PRIVATE Source/)

target_link_libraries(${PROJECT_NAME} general
    Vulkan::Vulkan glfw glslang::glslang glslang::SPIRV tetgen easy_profiler
)

file(GLOB PRECOMPILE_HEADERS "Source/pch.hpp")
target_precompile_headers(${PROJECT_NAME} PRIVATE ${PRECOMPILE_HEADERS})

include(CMake/SetupRuntime.cmake)
steelengine_setup_runtime()
