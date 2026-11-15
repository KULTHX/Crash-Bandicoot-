cmake_minimum_required(VERSION 3.22.1)
project(nizar_crash3_sdl_poc C)

if(NOT ANDROID)
    message(FATAL_ERROR "This project is intended to be built with the Android NDK.")
endif()
if(NOT DEFINED SDL_ROOT OR NOT EXISTS "${SDL_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "SDL_ROOT must point to the checked-out SDL3 source tree.")
endif()

set(SDL_SHARED ON CACHE BOOL "Build SDL3 as a shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Do not build SDL3 as a static library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Disable SDL test library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Disable SDL tests" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Disable SDL examples" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "Disable SDL install targets" FORCE)

add_subdirectory("${SDL_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/SDL3")

add_library(main SHARED main.c)
target_compile_features(main PRIVATE c_std_11)

find_library(ANDROID_LOG_LIBRARY log REQUIRED)
find_library(ANDROID_LIBRARY android REQUIRED)
find_library(GLESV3_LIBRARY GLESv3 REQUIRED)

target_link_libraries(main PRIVATE
    SDL3::SDL3
    ${ANDROID_LOG_LIBRARY}
    ${ANDROID_LIBRARY}
    ${GLESV3_LIBRARY}
)
