# NESticle (Modernized)

A restoration of the legendary NESticle NES emulator (v0.2) originally created by Icer Addis / Bloodlust Software in 1997. Nearly 30 years later, this project brings the iconic emulator back to life on modern Windows with full GUI, gamepad support, and improved audio mixing.

## What is NESticle?

NESticle was one of the first widely-used NES emulators, famous for its distinctive GUI, the bloody glove cursor, and making NES emulation accessible on Pentium-class PCs. It was a landmark piece of software in the emulation scene of the late 1990s.

This is not meant to compete with modern, cycle-accurate emulators like Mesen or FCEUX. This is a **preservation project** -- the original NESticle experience, running natively on modern hardware.

## What Changed

| Component | Original (1997) | Modern Replacement |
|---|---|---|
| Compiler | Borland C++ / Watcom | MSVC (Visual Studio 2022) |
| Platform | DOS / Win95 DirectDraw+DirectSound | SDL2 |
| 6502 CPU | x86 ASM + restrictive C++ core | fake6502 (public domain) |
| Rendering | TASM/MASM x86 assembly blitters | Portable C implementations |
| Audio Mixer | 3-channel linear mixer | 5-channel NES hardware DAC formulas |
| Input | Win95 keyboard only | SDL2 keyboard + GameController |

## What Stayed the Same

- The original NESticle GUI system (8-bit palettized widgets)
- The PPU rendering engine
- The mapper implementations
- The ROM loader and save state format
- The bloody glove cursor
- The overall "feel" of NESticle circa 1997

## Building

### Prerequisites
- Visual Studio 2022 (MSVC)
- CMake 3.20+
- SDL2 development libraries

### Build Steps
```bash
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

## Quick Start

1. Place `gui.vol` in the same directory as the executable
2. Run `NESticle.exe`
3. Load a ROM via the File menu or drag-and-drop
4. Play with keyboard (arrows + Z/X/Tab/Enter) or any SDL-compatible gamepad

## Controls

| Input | Action |
|---|---|
| Arrow keys | D-pad |
| Z / X | B / A |
| Tab / Enter | Select / Start |
| ALT+ENTER | Toggle fullscreen |
| Gamepad | Automatic (remappable via Settings) |

## Status

**v0.5.0 -- Feature Complete.** See [CHANGELOG.md](CHANGELOG.md) for details.

## Documentation

- [HOWTO.md](HOWTO.md) -- Build instructions, controls, configuration
- [ARCHITECTURE.md](ARCHITECTURE.md) -- System design and component overview
- [STATE.md](STATE.md) -- Current project status
- [ROADMAP.md](ROADMAP.md) -- Development phases
- [CHANGELOG.md](CHANGELOG.md) -- Version history

## License

NESticle is (c) Bloodlust Software / Icer Addis. The original source code has no clear open-source license. This project is a restoration effort made in good faith for preservation purposes. If the original author objects, this repository will be taken down.

Modern replacement components carry their own permissive licenses:
- fake6502 (public domain)
- SDL2 (zlib license)
