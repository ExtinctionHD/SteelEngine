cmake_minimum_required(VERSION 3.7.0)

find_package(Vulkan REQUIRED)

set(TARGET_NAME VulkanRayTracing)
project(${TARGET_NAME})

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/Source)
file(GLOB_RECURSE SOURCE_FILES LIST_DIRECTORIES false
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.hpp"
    "${SOURCE_DIR}/*.c"
    "${SOURCE_DIR}/*.h"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/")

add_executable(${TARGET_NAME} ${SOURCE_FILES})

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
    External/gli/
    External/imgui/
    Source/
)

target_link_libraries(${TARGET_NAME}
    general ${Vulkan_LIBRARIES}

    debug ${PROJECT_SOURCE_DIR}/External/glfw/src/Debug/glfw3.lib
    debug ${PROJECT_SOURCE_DIR}/External/glslang/SPIRV/Debug/SPIRVd.lib
    debug ${PROJECT_SOURCE_DIR}/External/glslang/glslang/Debug/glslangd.lib
    debug ${PROJECT_SOURCE_DIR}/External/glslang/hlsl/Debug/HLSLd.lib
    debug ${PROJECT_SOURCE_DIR}/External/glslang/OGLCompilersDLL/Debug/OGLCompilerd.lib
    debug ${PROJECT_SOURCE_DIR}/External/glslang/glslang/OSDependent/Windows/Debug/OSDependentd.lib

    optimized ${PROJECT_SOURCE_DIR}/External/glfw/src/Release/glfw3.lib
    optimized ${PROJECT_SOURCE_DIR}/External/glslang/SPIRV/Release/SPIRV.lib
    optimized ${PROJECT_SOURCE_DIR}/External/glslang/glslang/Release/glslang.lib
    optimized ${PROJECT_SOURCE_DIR}/External/glslang/hlsl/Release/HLSL.lib
    optimized ${PROJECT_SOURCE_DIR}/External/glslang/OGLCompilersDLL/Release/OGLCompiler.lib
    optimized ${PROJECT_SOURCE_DIR}/External/glslang/glslang/OSDependent/Windows/Release/OSDependent.lib
)
file(GLOB PRECOMPILE_HEADERS
    "Source/External/*.hpp"
)

target_precompile_headers(${TARGET_NAME}
    PUBLIC
    ${PRECOMPILE_HEADERS}
)
