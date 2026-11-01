cmake_minimum_required(VERSION 3.16)

project(Beyond LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/out)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# GLFW (windowing / input / fullscreen)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

find_package(glfw3 3.3 QUIET)

if(NOT glfw3_FOUND)
    FetchContent_Declare(
        glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(glfw)
endif()

# GLEW
# Use the system version when available, otherwise build static GLEW.
find_package(GLEW QUIET)

if(GLEW_FOUND)
    set(GLEW_TARGET GLEW::GLEW)
else()
    set(ONLY_LIBS ON CACHE BOOL "" FORCE)
    set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glew
        URL https://github.com/Perlmint/glew-cmake/archive/refs/tags/glew-cmake-2.2.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_MakeAvailable(glew)
    unset(CMAKE_POLICY_VERSION_MINIMUM)

    set(GLEW_TARGET libglew_static)
endif()

# Header-only libraries
FetchContent_Declare(
    miniaudio
    URL https://github.com/mackron/miniaudio/archive/refs/tags/0.11.21.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR _none
)

FetchContent_Declare(
    stb
    URL https://github.com/nothings/stb/archive/refs/heads/master.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR _none
)

FetchContent_MakeAvailable(miniaudio stb)

# OpenGL and threading
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# Main executable
add_executable(
    beyond
    src/main.cpp
    src/audio.cpp
)

target_include_directories(
    beyond
    PRIVATE
    ${miniaudio_SOURCE_DIR}
    ${stb_SOURCE_DIR}
)

target_compile_definitions(
    beyond
    PRIVATE
    GL_SILENCE_DEPRECATION
)

if(NOT GLEW_FOUND)
    target_compile_definitions(beyond PRIVATE GLEW_STATIC)
endif()

# GLFW target
set(GLFW_TARGET glfw)

# Link libraries
target_link_libraries(
    beyond
    PRIVATE
    ${GLFW_TARGET}
    ${GLEW_TARGET}
    OpenGL::GL
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(beyond PRIVATE m)
endif()

# Platform-specific compiler settings
if(MSVC)
    target_compile_options(beyond PRIVATE /W3 /utf-8)
    set_target_properties(
        beyond
        PROPERTIES
        WIN32_EXECUTABLE OFF
    )
endif()

if(APPLE)
    target_compile_definitions(beyond PRIVATE GL_SILENCE_DEPRECATION)
endif()

# Keep executable output consistent across platforms
set_target_properties(
    beyond
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/out
)
