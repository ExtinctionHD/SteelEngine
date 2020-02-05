cmake_minimum_required(VERSION 3.7.0)

set(TARGET_NAME VulkanRayTracing)
project(${TARGET_NAME})

find_package(Vulkan REQUIRED)

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

add_subdirectory(External/glfw)
add_subdirectory(External/glslang)

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

    debug glfw3
    debug SPIRVd
    debug glslangd
    debug HLSLd
    debug OGLCompilerd
    debug OSDependentd

    optimized glfw3
    optimized SPIRV
    optimized glslang
    optimized HLSL
    optimized OGLCompiler
    optimized OSDependent
)
file(GLOB PRECOMPILE_HEADERS
    "Source/External/*.hpp"
)

target_precompile_headers(${TARGET_NAME}
    PUBLIC
    ${PRECOMPILE_HEADERS}
)
