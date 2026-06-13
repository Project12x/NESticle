# How To

## Prerequisites

- **Visual Studio 2022** with C++ desktop development workload
- **CMake 3.20+**
- Vendored **SDL3** source under `external/SDL3-3.4.10/`

## Build

```bash
# Configure
cmake -B build-sdl3 -G "Visual Studio 17 2022"

# Build
cmake --build build-sdl3 --config Release

# Output binary will be in build-sdl3/Release/
```

## Package

```bash
cmake --build build-sdl3 --config Release --target package_release
```

The package target creates `releases/NESticle-sdl3-Release.zip` with `NESticle.exe`, `SDL3.dll`, `gui.vol`, `NESticle.pal`, `README.md`, verification docs, `LICENSE`, and third-party license/provenance notes.

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
- `NESticle.pal` -- default palette file
- `SDL3.dll` -- SDL3 runtime copied beside `NESticle.exe` by the build

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

### Gamepad (SDL3 Gamepad)
- D-pad and left stick for directional input
- Default button mapping: South=NES B, East=NES A, Back=Select, Start=Start
- Remap buttons and tune left-stick deadzone via Settings -> Redefine input -> select gamepad

### Remapping Controls
- **Keyboard**: Settings -> Redefine input -> select keyboard -> "Redefine keys..." -> press new key for each direction/button
- **Gamepad**: Settings -> Redefine input -> select gamepad -> adjust "Deadzone" if needed, or choose "Redefine buttons..." -> click a row, press the gamepad button to assign

## Project Structure

```
Source/
  *.CPP, *.H           Core emulator source
  m6502/                6502 CPU (fake6502 + shim)
  asm_replacements.cpp  C versions of ASM blitters
  platform_sdl3.cpp     SDL3 platform backend
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

## Manual Verification

Use `docs/verification.md` before tagging or sharing a release package.

## Compatibility Roadmap Workflow

1. Pick a known ROM or NES test ROM and record its mapper, mirroring, trainer, battery, and region details.
2. Run with `--dev-log` when diagnosing mapper, timing, audio, input, or package behavior.
3. Classify any failure before changing code: mapper, CPU, PPU, APU, input, timing, save, or host-platform.
4. For non-trivial mapper/timing work, inspect permissively licensed reference implementations at pinned commits and record the reuse mode.
5. Keep fixes scoped to the existing seams: `MMC.CPP/.H` for mappers, lowercase shim files for host work, and uppercase original files only when the behavior fix requires it.

## Developer Diagnostics

Run with `--dev-log` or `--diagnostics` to print startup, GUI, ROM header/mapper, CPU reset, mixer mode, SDL3 audio, timing, gamepad counters, and duplicate button-map diagnostics to stderr. Diagnostics stay out of the retro UI; normal launches only print failure details.

## Save States

Save states are validated before restore. See `docs/save-states.md` for the header, captured payload, and compatibility rules.

## ROM Dialog

The retro ROM dialog remembers the last visited ROM directory and includes recent successful ROM loads in the list.
