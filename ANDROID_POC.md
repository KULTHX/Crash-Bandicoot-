# NIZAR CRASH 3 — Android Runtime Proof of Concept

This branch contains an isolated, non-commercial engineering test for the Android port of the PSXRecomp runtime.

## Scope

The first test compiles the exact PSXRecomp fiber backend pinned by the Warped project and calls it from an Android Activity through JNI. It checks that a fiber can be created, switched to repeatedly, preserve a local stack value, and return to the caller.

Pinned upstream source:
- Repository: https://github.com/Alexbeav/psxrecomp
- Commit: 578bd3bedd48e2403a5d3de1cdc3eb260065ed7d
- Source: runtime/src/psx_fiber.c
- Header: runtime/include/psx_fiber.h

## What a passing test would prove

- Android NDK can compile the pinned PSXRecomp fiber source for arm64-v8a.
- The POSIX ucontext backend can execute on the Android runtime used by the CI emulator.
- JNI can call the fiber backend from an Android Activity.

## What it would NOT prove

- It does not build the complete PSXRecomp runtime.
- It does not render PS1 graphics or play audio.
- It does not load Crash Bandicoot 3.
- It does not include any game disc, BIOS, copyrighted game code, or game assets.
- It does not establish performance or compatibility on every Android device.

The next engineering milestone after this test is an SDL3 + GLES Android host and a small original PS1 test program, followed by integrating the full PSXRecomp runtime.
