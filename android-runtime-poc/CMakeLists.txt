cmake_minimum_required(VERSION 3.22.1)
project(nizar_crash3_android LANGUAGES C CXX)

if(NOT ANDROID)
    message(FATAL_ERROR "This build is intended for the Android NDK.")
endif()

set(PSXRECOMP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../psxrecomp" CACHE PATH "PSXRecomp Android framework")
if(NOT EXISTS "${PSXRECOMP_ROOT}/runtime/runtime.cmake")
    message(FATAL_ERROR "PSXRecomp Android framework checkout is missing: ${PSXRECOMP_ROOT}")
endif()

# Use the Android-supported framework branch. It provides Android paths,
# OpenGL ES integration, JNI/logcat support, and the deterministic HLE thread
# scheduler. Do not replace psx_fiber.c with a custom native-fiber backend.
set(PSX_RECOMP_UI OFF CACHE BOOL "" FORCE)
set(PSX_SETUP_WIZARD OFF CACHE BOOL "" FORCE)
set(PSX_DEBUG_TOOLS OFF CACHE BOOL "" FORCE)
set(PSX_REWIND OFF CACHE BOOL "" FORCE)
set(PSX_ENABLE_VULKAN OFF CACHE BOOL "" FORCE)
set(PSX_NETPLAY OFF CACHE BOOL "" FORCE)
set(PSX_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
set(PSX_SDL_BACKEND SDL3 CACHE STRING "" FORCE)
set(PSX_SDL3_FETCH ON CACHE BOOL "" FORCE)

# Only the redistributable BIOS is present in this launcher/runtime smoke build.
set(PSXRECOMP_BIOS_STEMS "OpenBIOS" CACHE STRING "" FORCE)
set(PSXRECOMP_BIOS_STEM "OpenBIOS" CACHE STRING "" FORCE)
set(PSXRECOMP_BIOS_PROFILE
    "${PSXRECOMP_ROOT}/bios/OpenBIOS.toml" CACHE FILEPATH "" FORCE)

# Keep libmain.so self-contained and suitable for SDLActivity.
add_compile_options(-fno-semantic-interposition -fvisibility=hidden
    $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>)
add_link_options(-Wl,-Bsymbolic)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include("${PSXRECOMP_ROOT}/runtime/runtime.cmake")

# Launcher/runtime smoke build only. No Crash-generated C code or commercial
# disc/BIOS data is included in this public repository.
psxrecomp_add_runtime_target(psx-runtime
    EXE_NAME "main"
    WINDOW_TITLE "Crash Bandicoot 3: Warped"
    DEFAULT_BIOS_PATH "bios/openbios.bin"
)
if(ANDROID)
    set_target_properties(psx-runtime PROPERTIES OUTPUT_NAME main)
endif()
