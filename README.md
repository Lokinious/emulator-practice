# GBA Emulator (Learning Project)

This is a learning-focused Game Boy Advance emulator in C++20 using CMake and SDL2.

## Current status
- SDL2 desktop frontend renders a 240x160 framebuffer (matches GBA Mode 3 resolution).
- PPU: simple Mode 3 VRAM path (BGR555 -> ARGB8888 conversion for display).
- Bus: basic mapping for WRAM (0x02000000), VRAM (0x06000000), and cartridge ROM (0x08000000).
- Cartridge: loads a ROM file into memory.
- CPU: Thumb-only skeleton that executes a useful subset of Thumb instructions (loads/stores, ALU, branches). The main loop steps the CPU when a ROM is present.
- Tools: a tiny C++ ROM generator (`romgen`) to produce a minimal homebrew test ROM without an Arm toolchain.

## Build (Desktop)
Requirements:
- CMake 3.20+
- A C++20 compiler (MSVC, Clang, or GCC)
- Git
- Windows notes: Visual Studio 2022 Build Tools recommended. CMake should be installed (e.g., `C:\Program Files\CMake\bin\cmake.exe`).

### Using VS Code tasks (recommended)
- Ctrl+Shift+P → “Tasks: Run Task” → run in order:
  1) CMake configure
  2) CMake build
  3) Run romgen (optional; generates `test_rom.gba` in the workspace)
  4) Run gba_sdl or Run gba_sdl (test_rom.gba)

The tasks are set up to work on Windows. They also extend PATH for CMake and SDL’s debug DLLs when needed.

### Command line (PowerShell)
- Configure:
  cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
- Build:
  cmake --build build --config Debug --parallel
- Run (no ROM):
  .\build\Debug\gba_sdl.exe

## Generate a tiny test ROM
You can create a small Thumb homebrew ROM that writes colored pixels to Mode 3 VRAM.

- VS Code task: “Run romgen” → creates `test_rom.gba` in the repo root.
- Or via terminal:
  .\build\Debug\romgen.exe .\test_rom.gba 0x7FFF 200
  - Arg2: color in BGR555 (e.g., 0x7FFF white, 0x001F blue, 0x03E0 green, 0x7C00 red)
  - Arg3: pixel count (1–255)

## Run the emulator with a ROM
- VS Code task: “Run gba_sdl (test_rom.gba)"
- Or terminal:
  .\build\Debug\gba_sdl.exe .\test_rom.gba

## Troubleshooting
- “cmake is not recognized”: Ensure CMake is installed and on PATH. You can adjust the tasks’ PATH entry to the folder that contains `cmake.exe` (e.g., `C:\\Program Files\\CMake\\bin`).
- “SDL2d.dll not found”: Debug builds use SDL2d.dll. The tasks set PATH to the SDL build folder; alternatively, copy `build\_deps\sdl2-build\Debug\SDL2d.dll` next to `build\Debug\gba_sdl.exe`. Release builds use SDL2.dll.
- Windows asks “what app to open .gba?”: `.gba` files are ROMs; open them with this emulator or a third‑party emulator (e.g., mGBA). In this repo use: `.\build\Debug\gba_sdl.exe .\your. gba`.

## Next steps
- CPU: complete Thumb coverage (PUSH/POP, LDM/STM, high-register ops, BX) and add ARM state.
- Timing/MMIO: DISPCNT/DISPSTAT/VCOUNT, KEYINPUT, basic scanline/VBlank timing.
- DMA, timers, interrupts, audio.
- Cartridge backup (SRAM/Flash/EEPROM).
- Android project scaffolding (SDL2 template) sharing the `gba_core` library.

## Notes
- Only use homebrew or public domain ROMs. Do not use proprietary BIOS or game ROMs.
