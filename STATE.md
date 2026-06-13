# Project State

## Current Phase
**Next: Compatibility Characterization** — The SDL3 improvement branch is feature-complete for packaging, reliability, settings, diagnostics, input, timing/audio stability, and save-state hardening. The next roadmap focus is measured compatibility work: known-ROM matrices, mapper priorities, timing/PPU evidence, and release-candidate discipline while preserving NESticle's retro UI, sound, palette look, timing feel, and visible quirks.

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
| Release packaging | Done | Deterministic SDL3 zip with assets, docs, and license notes |
| Compatibility matrix | Planned | Manual known-ROM evidence, no bundled ROMs |

## Preservation Boundary
NESticle's 1990s behavior is part of the product. Do not chase hardware-perfect audio, palette, timing, or UI behavior as a goal by itself. Treat odd sound, color, and glitch signatures as expected unless they are tied to a crash, corruption, host-platform failure, or a documented game-running compatibility blocker.

## Dependencies
- SDL3 (zlib license)
- fake6502 (public domain)

## Current Roadmap Focus
- Phase 19: Compatibility characterization
- Phase 20: Mapper coverage expansion based on evidence
- Phase 21: Timing and PPU regression workbench
- Phase 22: Audio and input verification depth
- Phase 23: Release-candidate discipline
