# Architecture

## Overview

NESticle is a Nintendo Entertainment System (NES) emulator. The architecture follows a classic emulator pattern: CPU → bus → PPU/APU/mappers, with a custom GUI and platform abstraction layer.

```
+------------------+     +------------------+
|  Platform Layer  |     |    GUI System     |
|  (SDL2)          |     |  (8-bit widgets)  |
+--------+---------+     +--------+---------+
         |                         |
         v                         v
+------------------------------------------+
|              Main Loop (MAIN.CPP)        |
|  initgame → gametimer → updatescreen    |
+--------+---------+-----------+-----------+
         |         |           |
         v         v           v
+--------+  +------+----+  +--+----------+
|CPU.CPP |  |NESVIDEO   |  |NESSOUND    |
|Scheduler|  |PPU render |  |APU (Blargg)|
+----+---+  +-----+-----+  +-----+------+
     |            |               |
     v            v               v
+----+---+  +-----+-----+  +-----+------+
|fake6502|  |PPU memory  |  |5 channels  |
|6502 CPU|  |sprites     |  |sq/tri/noise|
+--------+  |patterns    |  |DMC         |
            +-----+------+  +------------+
                  |
            +-----+------+
            |MMC.CPP     |
            |Bank mapper |
            +------------+
```

## Key Components

### CPU (`CPU.CPP` + `m6502/`)
- `CPU.CPP` — Frame/scanline scheduler, handles VBlank/IRQ timing
- `m6502/` — 6502 CPU core (fake6502, public domain)
- `m6502_shim.cpp` — Bridges fake6502 API to NESticle's `m6502.h` interface

### PPU (`NESVIDEO.CPP/.H`, `PPU.H`)
- Pattern tables (8x8 tile bitmaps, 1K banks)
- Name table caching with dirty-rectangle tracking
- Sprite rendering (8x8 and 8x16 modes)
- Scanline event list for mid-frame register changes
- Palette management (NES 64-color → 8-bit indexed)

### APU (`NESSOUND.CPP/.H`)
- Wraps Blargg's Nes_Snd_Emu for accurate sound
- 5 channels: square x2, triangle, noise, DMC
- Mixed to SDL2 audio callback

### Memory Mappers (`MMC.CPP/.H`)
- Base `memorymapper` class with virtual `write()`/`reset()`/`ppuread()`
- Supports ~8 mapper types (MMC_NONE through MMC_F5xxx)
- ROM/VROM bank switching

### GUI (`GUI.H`, `GUIRECT.H`, `GUIROOT.H`)
- Custom 8-bit palettized widget system
- Tree-based hierarchy: ROOT → GUIroot → GUIbox → GUIcontents
- Widgets: buttons, checkboxes, listboxes, scrollbars, edit controls
- Graphics loaded from `gui.vol` resource file

### Platform (`platform_sdl2.cpp`)
- SDL2 window, 8-bit surface, palette
- Audio, keyboard, mouse, timer
- Replaces original DirectDraw/DirectSound/Win32 backend

### ROM Loading (`ROM.CPP/.H`)
- iNES format parsing (.nes files)
- Trainer support, battery-backed RAM
- Save states (snapshot)

## Data Flow (Per Frame)
1. `gametimer()` called at 60Hz
2. `tickemu()` runs VBlank cycles → frame cycles via `m6502exec()`
3. Mid-frame events tracked in scanline event list
4. `updatescreen()` renders PPU state to backbuffer
5. SDL2 flips surface to window
