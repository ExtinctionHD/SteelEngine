cmake_minimum_required(VERSION 3.7.0)

set(TARGET_NAME SteelEngine)
project(${TARGET_NAME})

find_package(Vulkan REQUIRED)
find_package(Python REQUIRED COMPONENTS Interpreter)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/Source)
file(GLOB_RECURSE SOURCE_FILES LIST_DIRECTORIES false
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.hpp"
    "${SOURCE_DIR}/*.c"
    "${SOURCE_DIR}/*.h"
)
file(GLOB IMGUI_HEADER_FILES LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/External/imgui/examples/*glfw*.h"
    "${PROJECT_SOURCE_DIR}/External/imgui/examples/*vulkan*.h"
)
file(GLOB IMGUI_SOURCE_FILES LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/External/imgui/*.cpp"
    "${PROJECT_SOURCE_DIR}/External/imgui/examples/*glfw*.cpp"
    "${PROJECT_SOURCE_DIR}/External/imgui/examples/*vulkan*.cpp"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Bin/")

add_executable(${TARGET_NAME} ${SOURCE_FILES} ${IMGUI_HEADER_FILES} ${IMGUI_SOURCE_FILES})

add_subdirectory(External/glfw)

option(ENABLE_SPVREMAPPER "" OFF)
option(ENABLE_GLSLANG_BINARIES "" OFF)
option(ENABLE_CTEST "" OFF)
SET(ENABLE_HLSL OFF CACHE BOOL "")
add_subdirectory(External/glslang)

source_group("Source\\External\\Imgui\\" FILES ${IMGUI_HEADER_FILES})
source_group("Source\\External\\Imgui\\Private" FILES ${IMGUI_SOURCE_FILES})
foreach(source IN ITEMS ${SOURCE_FILES})
    get_filename_component(source_path "${source}" PATH)
    file(RELATIVE_PATH source_path_rel "${PROJECT_SOURCE_DIR}" "${source_path}")
    string(REPLACE "/" "\\" group_path "${source_path_rel}")
    source_group("${group_path}" FILES "${source}")
endforeach()

set_property(TARGET ${TARGET_NAME} PROPERTY USE_FOLDERS ON)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

target_include_directories(${TARGET_NAME}
    PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    External/VulkanExtensions/
    External/glfw/include/
    External/glslang/
    External/glm/
    External/stb/
    External/Imgui
    Source/
)

target_compile_definitions(${TARGET_NAME}
    PRIVATE NOMINMAX
)

target_link_libraries(${TARGET_NAME}
    general ${Vulkan_LIBRARIES} glfw glslang SPIRV
)

file(GLOB PRECOMPILE_HEADERS
    "Source/External/Headers.hpp"
)

target_precompile_headers(${TARGET_NAME}
    PUBLIC
    <string>
    <vector>
    <list>
    <set>
    <memory>
    <optional>
    <variant>
    <iostream>
    <cassert>
    <functional>
    ${PRECOMPILE_HEADERS}
)

execute_process(COMMAND ${Python_EXECUTABLE} Setup.py ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})