# NIZAR CRASH 3 — SDL3 Android runtime proof of concept

This is a separate, original-code-only Android host test. It does not contain
Crash Bandicoot 3, a PS1 disc image, a BIOS, or any game-derived code/assets.

## What it tests

- SDL3's official Android Java activity glue.
- SDL3 native library build for Android NDK.
- arm64-v8a and x86_64 packaging.
- SDL window creation and lifecycle entry.
- An OpenGL ES 3.0 context.
- A short GL clear/swap render loop.

## Build inputs

The workflow checks out the official SDL repository at tag release-3.4.20
into a temporary sibling directory named SDL. SDL is built from source by
the Android NDK. SDL source is not copied into this repository.

## What a passing test proves

It proves SDL3 can be built and started as an Android application and can
create an OpenGL ES 3 context with a working frame-swap loop on the tested
emulator ABI.

It does not prove that the full PSXRecomp runtime or Crash Bandicoot 3 works
on Android. Those remain separate milestones.
