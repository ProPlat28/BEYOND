cmake_minimum_required(VERSION 3.16)
project(Beyond CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/out)

# CMake 4.x refuses very old third-party cmake_minimum_required values unless told otherwise
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# GLFW (windowing / input / fullscreen)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
# BEYOND_PORTABLE: a release build that runs on other Linux PCs (GLFW built in, C++ runtime linked statically)
option(BEYOND_PORTABLE "Build a self-contained binary for release" OFF)
if(NOT BEYOND_PORTABLE)
  find_package(glfw3 3.3 QUIET)
endif()
if(NOT glfw3_FOUND)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz)
  FetchContent_MakeAvailable(glfw)
endif()

# header-only helpers: plain file downloads (no nested CMake project, works on every CMake version)
set(THIRD_PARTY ${CMAKE_BINARY_DIR}/third_party)
file(MAKE_DIRECTORY ${THIRD_PARTY})
function(fetch_header url name)
  if(NOT EXISTS ${THIRD_PARTY}/${name})
    message(STATUS "Downloading ${name}")
    file(DOWNLOAD ${url} ${THIRD_PARTY}/${name} STATUS st TLS_VERIFY ON)
    list(GET st 0 code)
    if(NOT code EQUAL 0)
      file(REMOVE ${THIRD_PARTY}/${name})
      message(FATAL_ERROR "Could not download ${name} from ${url}: ${st}")
    endif()
  endif()
endfunction()
fetch_header(https://raw.githubusercontent.com/mackron/miniaudio/0.11.21/miniaudio.h miniaudio.h)
fetch_header(https://raw.githubusercontent.com/nothings/stb/master/stb_easy_font.h stb_easy_font.h)
fetch_header(https://raw.githubusercontent.com/nothings/stb/master/stb_vorbis.c stb_vorbis.c)

find_package(Threads REQUIRED)

add_executable(beyond
  src/main.cpp
  src/gfx.cpp
  src/planets.cpp
  src/controls.cpp
  src/flight.cpp
  src/story.cpp
  src/surface.cpp
  src/save.cpp
  src/render.cpp
  src/worlds.cpp
  src/ui.cpp
  src/menu.cpp
  src/audio.cpp
  src/vorbis.cpp
  src/aliens.cpp
  src/houses.cpp
  src/economy.cpp
  src/panels.cpp
  src/items.cpp
  src/ending.cpp
  src/devtools.cpp)
target_include_directories(beyond PRIVATE ${THIRD_PARTY})
# music, footsteps and voices are loaded at runtime from assets/audio next to the executable
add_custom_command(TARGET beyond POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/audio $<TARGET_FILE_DIR:beyond>/assets/audio)
# the icon images and, on Linux, the script that adds BEYOND to the app menu with its icon
add_custom_command(TARGET beyond POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/icon $<TARGET_FILE_DIR:beyond>/assets/icon)
if(UNIX AND NOT APPLE)
  add_custom_command(TARGET beyond POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tools/install-launcher.sh $<TARGET_FILE_DIR:beyond>/install-launcher.sh)
endif()
target_compile_definitions(beyond PRIVATE GL_SILENCE_DEPRECATION)
# developer tools (F1 in the one signed developer world) exist only in local builds, never in release packages
if(NOT BEYOND_PORTABLE)
  target_compile_definitions(beyond PRIVATE BEYOND_DEVTOOLS)
endif()
if(WIN32)
  target_compile_definitions(beyond PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
  target_sources(beyond PRIVATE src/beyond.rc)   # the icon of beyond.exe in Explorer
endif()
if(TARGET glfw)
  set(GLFW_TARGET glfw)
else()
  set(GLFW_TARGET glfw)
endif()
target_link_libraries(beyond PRIVATE ${GLFW_TARGET} Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX AND NOT APPLE)
  target_link_libraries(beyond PRIVATE m)
  if(BEYOND_PORTABLE)
    target_link_options(beyond PRIVATE -static-libstdc++ -static-libgcc)
  endif()
endif()
if(MSVC)
  target_compile_options(beyond PRIVATE /W3 /utf-8)
  set_target_properties(beyond PROPERTIES WIN32_EXECUTABLE OFF)
endif()
