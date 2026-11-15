cmake_minimum_required(VERSION 3.22.1)
project(psx_fiber_android_poc C)

if(NOT DEFINED PSXRECOMP_ROOT OR NOT EXISTS "${PSXRECOMP_ROOT}/runtime/src/psx_fiber.c")
    message(FATAL_ERROR "PSXRECOMP_ROOT must point to the checked-out PSXRecomp source tree.")
endif()

if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "This proof of concept currently supports arm64-v8a only.")
endif()

add_library(psx_fiber_poc SHARED
    fiber_poc.c
    psx_fiber_android.c
    psx_fiber_aarch64.S
)

target_include_directories(psx_fiber_poc PRIVATE
    "${PSXRECOMP_ROOT}/runtime/include"
)

target_compile_features(psx_fiber_poc PRIVATE c_std_11)

find_library(android_log log REQUIRED)
target_link_libraries(psx_fiber_poc PRIVATE ${android_log})
