# GBA Emulator (Learning Project)

This is a learning-focused Game Boy Advance emulator in C++.

Current status:
- SDL2 desktop frontend opens a window and renders a 240x160 test pattern.
- Core scaffolding for CPU/Bus is stubbed.

## Build (Desktop)

Requirements:
- CMake 3.20+
- A C++20 compiler (MSVC, Clang, or GCC)
- Git

Steps:
1. Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
2. Build: `cmake --build build --config Debug --parallel`
3. Run: `build/Debug/gba_sdl.exe` (Windows) or `build/gba_sdl` (others)

SDL2 is fetched via CMake FetchContent and built from source by default.

## Next steps
- Implement a proper scheduler and the ARM7TDMI core (Thumb first).
- Add a 240x160 15-bit (BGR555) framebuffer path matching GBA Mode 3.
- Introduce cartridge loading for homebrew ROMs.
- Plan Android target using SDL2's Android template, sharing core sources.

## Notes
- Do not use proprietary BIOS or ROMs. Use homebrew and public domain test ROMs.
