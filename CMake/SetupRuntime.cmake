include_guard(GLOBAL)

# Prepare the project build directory for running SteelEngine.
function(steelengine_setup_runtime)
    set(source_dir "${PROJECT_SOURCE_DIR}")
    set(runtime_dir "${PROJECT_BINARY_DIR}")

    # Live resource directories: junctions on Windows, symlinks elsewhere.
    _steelengine_link_runtime_path("${source_dir}/Config"           "${runtime_dir}/Config")
    _steelengine_link_runtime_path("${source_dir}/Config/imgui.ini" "${runtime_dir}/imgui.ini")
    _steelengine_link_runtime_path("${source_dir}/Source/Shaders"   "${runtime_dir}/Shaders")
    _steelengine_link_runtime_path("${source_dir}/Assets"           "${runtime_dir}/Assets")
    _steelengine_link_runtime_path("${source_dir}/LocalAssets"      "${runtime_dir}/LocalAssets" OPTIONAL)

    # Share IDE settings with the source configuration, as in the original setup.
    if(MSVC)
        _steelengine_link_runtime_path(
            "${source_dir}/Config/SteelEngine.sln.DotSettings"
            "${runtime_dir}/SteelEngine.sln.DotSettings"
        )
    endif()
endfunction()

# _steelengine_link_runtime_path(<source> <destination> [OPTIONAL])
# Directories use live links; files use hard links on the same filesystem.
# Existing destinations are preserved, matching the original setup script.
function(_steelengine_link_runtime_path source destination)
    cmake_parse_arguments(PARSE_ARGV 2 link "OPTIONAL" "" "")
    if(link_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown runtime link arguments: ${link_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT EXISTS "${source}")
        if(link_OPTIONAL)
            message(VERBOSE "Skipping optional runtime path: ${source}")
            return()
        endif()
        message(FATAL_ERROR "Missing required runtime path: ${source}")
    endif()

    if(EXISTS "${destination}" OR IS_SYMLINK "${destination}")
        return()
    endif()

    get_filename_component(destination_dir "${destination}" DIRECTORY)
    file(MAKE_DIRECTORY "${destination_dir}")

    if(IS_DIRECTORY "${source}" AND WIN32)
        # Junctions do not require administrator privileges or Developer Mode.
        file(TO_NATIVE_PATH "${source}" native_source)
        file(TO_NATIVE_PATH "${destination}" native_destination)
        execute_process(
            COMMAND cmd /c mklink /J "${native_destination}" "${native_source}"
            RESULT_VARIABLE link_result
            OUTPUT_VARIABLE link_output
            ERROR_VARIABLE link_error
        )
        string(STRIP "${link_output}\n${link_error}" link_details)
    elseif(IS_DIRECTORY "${source}")
        file(CREATE_LINK "${source}" "${destination}" SYMBOLIC RESULT link_result)
        set(link_details "${link_result}")
    else()
        file(CREATE_LINK "${source}" "${destination}" RESULT link_result)
        set(link_details "${link_result}")
    endif()

    if(NOT link_result STREQUAL "0")
        message(FATAL_ERROR
            "Failed to create runtime link:\n"
            "  Source: ${source}\n"
            "  Destination: ${destination}\n"
            "  Reason: ${link_details}"
        )
    endif()

    message(STATUS "Runtime link: ${destination} -> ${source}")
endfunction()
