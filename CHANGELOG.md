# Changelog

All notable changes to this project will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Planned
- GUI Modernization (Resizable frames, button clipping fixes)

## [0.4.1] - 2026-03-09

### Fixed
- **GUIVOL struct alignment**: Added missing `sfont` and 12 other fields to match original gui.vol entry order; all icons (scrollbar arrows, close/maximize buttons, checkmarks) now display correctly
- **GUI constructor coordinates**: Fixed 5 widget constructors using relative coordinates where absolute were required (close button, accept bar, scrollbar, number edit arrows)
- **Button bar overlap**: GUIonebuttonbox/GUItwobuttonbox now extend box height by 18px for the bottom button bar
- **Popup menu crash**: Fixed use-after-free in GUIpopupmenu::domenuitem() — mouse capture pointer cleared before popup self-deletion

## [0.4.0] - 2026-03-09

### Added
- **True Fullscreen**: ALT+ENTER toggles between 640x480 windowed GUI and 256x224 native NES fullscreen
- **SDL Logical Scaling**: `SDL_RenderSetLogicalSize` handles aspect-ratio-correct scaling and letterboxing
- **Command-line ROM loading**: Fixed `cmd_runrom`/`cmd_loadrom`/`cmd_restorerom` to handle file paths with spaces

### Fixed
- **Mouse mapping**: Uses SDL logical coordinate space instead of manual window-to-buffer scaling

## [0.3.1] - 2026-03-08

### Fixed
- **SMB1 Freeze**: Fixed an emulation freeze where SMB1 infinitely spun on the title screen waiting for Sprite 0 Hit.
- **Clock Synchronization**: Mended `m6502_shim.cpp` to accurately synthesize cycle counts mid-instruction from `fake6502` to NESticle's PPU engine (`m6502clockticks = clockticks6502;`), ensuring hardware scanlines compute accurately for CPU-PPU traps.
- **Clear-On-Read**: Properly hooked up `NES.CPP` `$2002` reads to clear the VBlank flag (`ram[0x2002] &= ~0x80;`), fixing NMI runaway hazards in SMB1.

## [0.3.0] - 2026-03-07

### Fixed
- **Sprite Flipping**: Repaired 8-bit signed integer casts for sprite `flipx`/`flipy`, restoring correct orientation in SMB1/SMB3.
- **Palette Bleeding**: Masked `attrib` byte with `& 3` so higher bits don't corrupt the sprite palette rendering function.
- **Noise Channel timing**: Adjusted LFSR duty/volume and fixed unipolar offset bugs causing SMB1/SMB3 sound effects to sound corrupted.
- **Input System**: Removed dirty mapping and restored standard Win95 keyboard scanning via SDL2.

### Added
- Animated cursor rendering bypassing the 8-bit palette via ARGB framebuffer overlay.

### Discovered
- Complete missing source code recovered from SOUND.ZIP archive:
  - 6502 CPU core (C++ portable version + x86 ASM version)
  - Win95 DirectDraw/DirectSound platform backend
  - DOS platform backend
  - Win95 keyboard handler

### Planned
- CMake build system with SDL2
- fake6502 CPU core (public domain, replacing restrictive original)
- Blargg's nes_apu (LGPL, replacing incomplete 3-channel original)
- C replacements for x86 ASM tile/sprite blitters
- SDL2 platform backend replacing DirectDraw/DirectSound
- MSVC compatibility fixes for Borland/Watcom C++ idioms
