# Project State

## Current Phase
**Phase 8: GUI Modernization** - Adapting legacy 320x200 GUI to 640x480, fixing layout/crash bugs.

## Build Status
- **Compiles**: Yes
- **Runs**: Yes (v0.4.2 — UI dialogs fixed for 640x480, NES Timing crash resolved)

## Key Metrics
- Original source files analyzed: ~25 C++, 4 ASM, ~20 headers
- Missing source recovered from SOUND.ZIP: 6502 CPU, Win95 backend, DOS backend
- Mappers supported: ~8 (MMC types 0-7)
- APU channels (original): 3/5 (square x2, triangle only)
- APU channels (target with Blargg): 5/5

## Key Components
| Component | Status | Notes |
|---|---|---|
| CMake build system | Done | Cross-platform build with SDL2 |
| fake6502 integration | Done | Public domain 6502 CPU via m6502_shim |
| Blargg APU integration | Done | LGPL NES sound (5 channels) |
| ASM-to-C blitters | Done | asm_replacements.cpp |
| SDL2 platform backend | Done | platform_sdl2.cpp, replaces DirectDraw/DirectSound |
| MSVC compatibility | Done | Borland/Watcom -> MSVC fixes applied |
| GUI widget positioning | Done | Absolute coord fixes, button bar layout |
| 640x480 dialog layouts | In Progress | About, NES Timing, ROM loader done; other dialogs pending |

## Dependencies
- SDL2 (zlib license)
- fake6502 (public domain)
- Blargg's Nes_Snd_Emu (LGPL)
