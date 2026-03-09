# Project State

## Current Phase
**Phase 8: GUI Modernization** - Fixing legacy GUI frame boundaries, clipping, and resolution constraints.

## Build Status
- **Compiles**: Yes
- **Runs**: Yes (v0.4.1 — GUI icons correct, widget positioning fixed)

## Key Metrics
- Original source files analyzed: ~25 C++, 4 ASM, ~20 headers
- Missing source recovered from SOUND.ZIP: 6502 CPU, Win95 backend, DOS backend
- Mappers supported: ~8 (MMC types 0-7)
- APU channels (original): 3/5 (square x2, triangle only)
- APU channels (target with Blargg): 5/5

## Key Components
| Component | Status | Notes |
|---|---|---|
| CMake build system | Not started | |
| fake6502 integration | Not started | Public domain 6502 CPU |
| Blargg APU integration | Not started | LGPL NES sound |
| ASM-to-C blitters | Not started | 4 ASM files to convert |
| SDL2 platform backend | Not started | Replaces DirectDraw/DirectSound |
| MSVC compatibility | Not started | Borland/Watcom -> MSVC fixes |

## Dependencies
- SDL2 (zlib license)
- fake6502 (public domain)
- Blargg's Nes_Snd_Emu (LGPL)
