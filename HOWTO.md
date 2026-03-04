# How To

## Prerequisites

- **Visual Studio 2022** with C++ desktop development workload
- **CMake 3.20+**
- **SDL2** development libraries for Windows
  - Download from https://github.com/libsdl-org/SDL/releases
  - Extract to a known location (e.g. `C:\libs\SDL2`)

## Build

```bash
# Configure (point to SDL2 if not in system path)
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\libs\SDL2"

# Build
cmake --build build --config Release

# Output binary will be in build/Release/
```

## Run

```bash
# From the build output directory
NESticle.exe

# Load a ROM via the GUI, or pass on command line
NESticle.exe path/to/game.nes
```

### Required Data Files
- `gui.vol` — GUI graphics resource (not included in source repo)
  - Without this file, the GUI will not render but the emulator may still work in maximized mode

### Controls
| Key | Action |
|---|---|
| Arrow keys | D-pad |
| Z | B button |
| X | A button |
| Tab | Select |
| Enter | Start |
| Escape | Toggle GUI |
| Space | Maximize/restore emulator window |

## Project Structure

```
Source/
  *.CPP, *.H         Core emulator source
  m6502/              6502 CPU (fake6502 + shim)
  apu/                Blargg's NES APU
  asm_replacements.cpp  C versions of ASM blitters
  platform_sdl2.cpp   SDL2 platform backend
  BACK/               Backup files (ignore)
  *.ZIP               Historical source snapshots
```

## Adding New Mappers

1. Add a new `MMC_XXX` define in `MMC.H`
2. Create a new class derived from `memorymapper` in `MMC.CPP`
3. Implement `reset()`, `write()`, `ppuread()` at minimum
4. Register in `newmemorymapper()` factory function
