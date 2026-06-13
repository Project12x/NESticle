# Project State

## Current Phase
**v0.5.0 — Feature Complete** — NESticle runs on modern Windows with full GUI, gamepad support, and hardware-accurate audio mixing.

## Build Status
- **Compiles**: Yes (MSVC 2022, CMake, SDL3)
- **Runs**: Yes (Windows 10/11)

## Key Metrics
- Original source files: ~25 C++, 4 ASM, ~20 headers
- Missing source recovered from SOUND.ZIP: 6502 CPU, Win95 backend, DOS backend
- Mappers supported: ~8 (MMC types 0-7)
- APU channels: 5/5 (square x2, triangle, noise, DMC)
- Input devices: Keyboard (2 players) + SDL Gamepad with remapping

## Key Components
| Component | Status | Notes |
|---|---|---|
| CMake build system | Done | Cross-platform build with vendored SDL3 |
| fake6502 integration | Done | Public domain 6502 CPU via m6502_shim |
| NES APU (5 channels) | Done | Original NESticle sound core with nonlinear mixer |
| ASM-to-C blitters | Done | asm_replacements.cpp |
| SDL3 platform backend | Done | platform_sdl3.cpp (video, audio, input, timer) |
| MSVC compatibility | Done | Borland/Watcom -> MSVC fixes applied |
| GUI system | Done | Full widget system, menus, dialogs at 640x480 |
| Input system | Done | Keyboard + SDL Gamepad w/ button remapping |
| NES nonlinear mixer | Done | Hardware DAC formulas with DC blocking filter |
| Fullscreen | Done | ALT+ENTER toggle, SDL logical scaling |

## Dependencies
- SDL3 (zlib license)
- fake6502 (public domain)
