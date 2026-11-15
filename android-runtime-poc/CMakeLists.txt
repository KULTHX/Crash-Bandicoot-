cmake_minimum_required(VERSION 3.22.1)
project(nizar_crash3_psx_runtime_android LANGUAGES C CXX ASM)

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

# Android Bionic hides POSIX ucontext switching calls. Swap only the fiber
# translation unit in this disposable framework checkout for the local
# AAPCS64 implementation. Desktop framework sources remain untouched.
set(_upstream_fiber_source [=[
    ${PSXRECOMP_ROOT}/runtime/src/psx_fiber.c
]=])
set(_android_fiber_source
    "    ${CMAKE_CURRENT_LIST_DIR}/android_fiber.c\n    ${CMAKE_CURRENT_LIST_DIR}/android_fiber_aarch64.S\n")
string(FIND "${_runtime_source}" "${_upstream_fiber_source}" _fiber_patch_position)
if(_fiber_patch_position LESS 0)
    message(FATAL_ERROR "Could not locate the pinned PSXRecomp fiber source in runtime.cmake.")
endif()
string(REPLACE "${_upstream_fiber_source}" "${_android_fiber_source}"
    _runtime_source "${_runtime_source}")
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

# Bionic does not expose C11 timespec_get in the NDK headers used by this
# build. The framework uses it only for host-side DMA diagnostic timing.
# clock_gettime(CLOCK_REALTIME) supplies the same wall-clock timespec here.
set(_dma_source "${PSXRECOMP_ROOT}/runtime/src/dma.c")
file(READ "${_dma_source}" _dma_text)
set(_old_timespec_call "timespec_get(&ts, TIME_UTC);")
set(_android_timespec_call "clock_gettime(CLOCK_REALTIME, &ts);")
string(FIND "${_dma_text}" "${_old_timespec_call}" _timespec_patch_position)
if(_timespec_patch_position LESS 0)
    message(FATAL_ERROR "Could not find expected DMA diagnostic timespec_get call.")
endif()
string(REPLACE "${_old_timespec_call}" "${_android_timespec_call}"
    _dma_text "${_dma_text}")
file(WRITE "${_dma_source}" "${_dma_text}")

# The desktop GL renderer includes optional texture-dump diagnostics using
# glGetTexImage (not present in GLES). These dumps are not part of gameplay;
# on Android keep their output buffers deterministic without reading textures.
# glClearDepth maps directly to the GLES glClearDepthf entry point.
set(_gl_renderer_source "${PSXRECOMP_ROOT}/runtime/src/gpu_gl_renderer.c")
file(READ "${_gl_renderer_source}" _gl_renderer_text)
set(_old_gl_depth "glClearDepth(1.0);")
set(_new_gl_depth "glClearDepthf(1.0f);")
string(FIND "${_gl_renderer_text}" "${_old_gl_depth}" _gl_depth_position)
if(_gl_depth_position LESS 0)
    message(FATAL_ERROR "Could not find the expected desktop glClearDepth calls.")
endif()
string(REPLACE "${_old_gl_depth}" "${_new_gl_depth}"
    _gl_renderer_text "${_gl_renderer_text}")
set(_old_stereo_read
    "glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb + eye * n);")
set(_new_stereo_read
    "memset(rgb + eye * n, 0, n); /* Android GLES: stereo texture dump disabled. */")
set(_old_pass_read
    "glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);")
set(_new_pass_read
    "memset(rgb, 0, n); /* Android GLES: pass texture dump disabled. */")
string(FIND "${_gl_renderer_text}" "${_old_stereo_read}" _stereo_read_position)
string(FIND "${_gl_renderer_text}" "${_old_pass_read}" _pass_read_position)
if(_stereo_read_position LESS 0 OR _pass_read_position LESS 0)
    message(FATAL_ERROR "Could not find the expected optional GLES-incompatible texture dump calls.")
endif()
string(REPLACE "${_old_stereo_read}" "${_new_stereo_read}"
    _gl_renderer_text "${_gl_renderer_text}")
string(REPLACE "${_old_pass_read}" "${_new_pass_read}"
    _gl_renderer_text "${_gl_renderer_text}")
file(WRITE "${_gl_renderer_source}" "${_gl_renderer_text}")

# The shared runtime currently requests desktop OpenGL 3.3 Core. Android uses
# an OpenGL ES context; request GLES 3.0 only for the Android build and preserve
# the upstream desktop profile everywhere else.
set(_main_cpp_source "${PSXRECOMP_ROOT}/runtime/src/main.cpp")
file(READ "${_main_cpp_source}" _main_cpp_text)
set(_old_context_profile [=[
static void configure_core_gl_context_attributes() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    int major = 3, minor = 3;
]=])
set(_android_context_profile [=[
static void configure_core_gl_context_attributes() {
#if defined(__ANDROID__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    int major = 3, minor = 0;
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    int major = 3, minor = 3;
#endif
]=])
string(FIND "${_main_cpp_text}" "${_old_context_profile}" _context_profile_position)
if(_context_profile_position LESS 0)
    message(FATAL_ERROR "Could not locate the pinned runtime GL context profile setup.")
endif()
string(REPLACE "${_old_context_profile}" "${_android_context_profile}"
    _main_cpp_text "${_main_cpp_text}")
file(WRITE "${_main_cpp_source}" "${_main_cpp_text}")

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
