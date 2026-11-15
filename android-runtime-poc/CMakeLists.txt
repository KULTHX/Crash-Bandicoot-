cmake_minimum_required(VERSION 3.22.1)
project(nizar_crash3_psx_runtime_android LANGUAGES C CXX)

if(NOT ANDROID)
    message(FATAL_ERROR "This build entry is intended for the Android NDK.")
endif()

if(NOT DEFINED PSXRECOMP_ROOT OR
   NOT EXISTS "${PSXRECOMP_ROOT}/runtime/runtime.cmake")
    message(FATAL_ERROR
        "PSXRECOMP_ROOT must point to the pinned PSXRecomp framework checkout.")
endif()

# The upstream runtime target is an executable on desktop. SDL's Android
# Activity loads a shared library named libmain.so, so adapt only the temporary
# CI checkout used by this build. No upstream repository or source branch is
# modified by this project.
set(_runtime_cmake "${PSXRECOMP_ROOT}/runtime/runtime.cmake")
file(READ "${_runtime_cmake}" _runtime_source)
set(_desktop_target [=[
    add_executable(${target}
        ${PSXRECOMP_RUNTIME_SOURCES}
        ${mode_source}
        ${generated_sources}
        ${PSXRT_EXTRAS_SOURCES}
    )
]=])
set(_android_target [=[
    if(ANDROID)
        add_library(${target} SHARED
            ${PSXRECOMP_RUNTIME_SOURCES}
            ${mode_source}
            ${generated_sources}
            ${PSXRT_EXTRAS_SOURCES}
        )
    else()
        add_executable(${target}
            ${PSXRECOMP_RUNTIME_SOURCES}
            ${mode_source}
            ${generated_sources}
            ${PSXRT_EXTRAS_SOURCES}
        )
    endif()
]=])
string(FIND "${_runtime_source}" "${_desktop_target}" _target_patch_position)
if(_target_patch_position LESS 0)
    message(FATAL_ERROR
        "Could not locate the expected PSXRecomp runtime target declaration. "
        "The pinned framework source has changed; review the Android patch.")
endif()
string(REPLACE "${_desktop_target}" "${_android_target}"
    _runtime_source "${_runtime_source}")
file(WRITE "${_runtime_cmake}" "${_runtime_source}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# The native renderer uses SDL's GL entry-point loader. Android exports GLESv3
# and EGL instead of desktop OpenGL; make the framework's existing GL resolver
# choose those system libraries.
function(recomp_resolve_gl out_var)
    find_library(_android_gles GLESv3 REQUIRED)
    find_library(_android_egl EGL REQUIRED)
    set(${out_var} "${_android_gles};${_android_egl}" PARENT_SCOPE)
endfunction()

# Keep this first port focused on the runtime itself. The desktop launcher,
# runtime compilation wizard, Vulkan, and rewind backend are not Android-ready.
set(PSX_RECOMP_UI OFF CACHE BOOL "" FORCE)
set(PSX_SETUP_WIZARD OFF CACHE BOOL "" FORCE)
set(PSX_REWIND OFF CACHE BOOL "" FORCE)
set(PSX_ENABLE_VULKAN OFF CACHE BOOL "" FORCE)
set(PSX_DEBUG_TOOLS OFF CACHE BOOL "" FORCE)
set(PSX_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(PSX_SDL_BACKEND SDL3 CACHE STRING "" FORCE)
set(PSX_SDL3_FETCH ON CACHE BOOL "" FORCE)
set(PSXRECOMP_BIOS_STEMS OpenBIOS CACHE STRING "" FORCE)
set(PSXRECOMP_BIOS_STEM OpenBIOS CACHE STRING "" FORCE)
set(PSXRECOMP_BIOS_PROFILE
    "${PSXRECOMP_ROOT}/bios/OpenBIOS.toml" CACHE FILEPATH "" FORCE)

include("${PSXRECOMP_ROOT}/runtime/runtime.cmake")

# BIOS-only runtime build: this compiles the real PSXRecomp CPU, memory,
# scheduler, GPU, audio and SDL runtime without redistributing Crash game data.
# The Crash-specific generated translation units can only be produced from the
# user's own disc image and are intentionally not present in public CI.
psxrecomp_add_runtime_target(psx-runtime
    EXE_NAME "main"
    WINDOW_TITLE "Nizar Crash 3 Android Runtime"
    DEFAULT_BIOS_PATH "bios/openbios.bin"
)
