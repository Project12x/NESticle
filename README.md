# NESticle (Modernized)

A modernization of the NESticle NES emulator (v0.2) originally created by Icer Addis / Bloodlust Software (~1997). This project ports the leaked source code to compile and run on modern Windows using contemporary tools and libraries.

## Background

NESticle was one of the earliest and most popular NES emulators for DOS and Windows 95. The source code was leaked in 1997 and later made publicly available. This project preserves the original architecture while replacing obsolete platform dependencies with modern equivalents.

## Modernization Approach

| Component | Original (1997) | Modern Replacement |
|---|---|---|
| Compiler | Borland C++ / Watcom | MSVC (Visual Studio 2022) |
| Platform | DOS / Win95 DirectDraw+DirectSound | SDL2 |
| 6502 CPU | x86 ASM + restrictive C++ core | fake6502 (public domain) |
| NES APU | Incomplete (3/5 channels) | Blargg's nes_apu (LGPL) |
| Rendering ASM | TASM/MASM x86 assembly | Portable C implementations |

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

## Status

**Work in Progress** - See [STATE.md](STATE.md) and [ROADMAP.md](ROADMAP.md) for current progress.

## License

NESticle is (c) Bloodlust Software. The original source was leaked and has no clear open-source license. Modern replacement components (fake6502, Blargg's APU, SDL2) carry their own permissive licenses.
