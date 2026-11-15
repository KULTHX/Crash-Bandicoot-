# NIZAR CRASH 3 — Android Runtime Proof of Concept

This branch contains an isolated, non-commercial engineering test for an Android AArch64 fiber backend compatible with the PSXRecomp fiber API.

## Scope

The first test calls a local Android-specific implementation through JNI. Its public API matches the pinned PSXRecomp `psx_fiber.h`, while its AArch64 assembly saves and restores the AAPCS64 callee-saved registers, stack pointer, and floating-point callee-saved registers.

Pinned upstream reference:
- Repository: https://github.com/Alexbeav/psxrecomp
- Commit used by Warped: `578bd3bedd48e2403a5d3de1cdc3eb260065ed7d`
- API header: `runtime/include/psx_fiber.h`
- Existing desktop implementation: `runtime/src/psx_fiber.c`

The initial attempt to compile the upstream POSIX backend exposed that Android NDK/Bionic does not declare the `getcontext`, `makecontext`, and `swapcontext` functions used by that source. The local Android backend is a targeted alternative, not a modification of upstream PSXRecomp.

## What a passing test would prove

- Android NDK can build a native ARM64 library containing the fiber backend.
- The AArch64 stack/register switch can run on the Android ARM64 emulator.
- JNI can call the backend from an Android Activity.
- A fiber can yield and resume while preserving a local stack value.

## What it would NOT prove

- It does not build the complete PSXRecomp runtime.
- It does not prove the backend is correct for every PS1 workload or every Android device.
- It does not render PS1 graphics or play audio.
- It does not load Crash Bandicoot 3.
- It does not include any game disc, BIOS, copyrighted game code, or game assets.
- It does not establish game performance or compatibility.

The next engineering milestone after this test is an SDL3 + GLES Android host and an original, redistributable PS1 test program, followed by integrating the full PSXRecomp runtime.
