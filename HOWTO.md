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
# From the project root directory (needs gui.vol)
NESticle.exe

# Load a ROM via the GUI, or pass on command line
NESticle.exe path/to/game.nes
```

### Required Data Files
- `gui.vol` -- GUI graphics resource (must be in working directory)
  - Without this file, the GUI will not render

## Controls

### Keyboard (Default)
| Key | Action |
|---|---|
| Arrow keys | D-pad |
| Z | B button |
| X | A button |
| Tab | Select |
| Enter | Start |
| ALT+ENTER | Toggle fullscreen |
| Escape | Exit fullscreen / Toggle GUI |
| Space | Maximize/restore emulator window |

### Gamepad (SDL GameController)
- D-pad and left stick for directional input
- Default button mapping: A=NES B, B=NES A, Back=Select, Start=Start
- Remap buttons via Settings -> Redefine input -> select gamepad -> "Redefine buttons..."

### Remapping Controls
- **Keyboard**: Settings -> Redefine input -> select keyboard -> "Redefine keys..." -> press new key for each direction/button
- **Gamepad**: Settings -> Redefine input -> select gamepad -> "Redefine buttons..." -> click a row, press the gamepad button to assign

## Project Structure

```
Source/
  *.CPP, *.H           Core emulator source
  m6502/                6502 CPU (fake6502 + shim)
  asm_replacements.cpp  C versions of ASM blitters
  platform_sdl2.cpp     SDL2 platform backend
  INPUT.CPP/H           Input device management + gamepad
  INPUTW.CPP            Input config dialogs + button remapping UI
  NESSOUND.CPP/H        APU with nonlinear mixer
  BACK/                 Backup files (ignore)
```

## Adding New Mappers

1. Add a new `MMC_XXX` define in `MMC.H`
2. Create a new class derived from `memorymapper` in `MMC.CPP`
3. Implement `reset()`, `write()`, `ppuread()` at minimum
4. Register in `newmemorymapper()` factory function

## Audio Mixer Toggle

The default mixer uses NES hardware DAC formulas (nonlinear). To switch to the original NESticle linear mixer, set `nessound::nonlinear_mixing = 0` in `NESSOUND.CPP`.
