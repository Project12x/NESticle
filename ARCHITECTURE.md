# Architecture

## Overview

NESticle is a Nintendo Entertainment System (NES) emulator originally created in 1997. The architecture follows a classic emulator pattern: CPU -> bus -> PPU/APU/mappers, with a custom GUI and platform abstraction layer.

```
+------------------+     +------------------+
|  Platform Layer  |     |    GUI System     |
|  (SDL2)          |     |  (8-bit widgets)  |
+--------+---------+     +--------+---------+
         |                         |
         v                         v
+------------------------------------------+
|              Main Loop (MAIN.CPP)        |
|  initgame -> gametimer -> updatescreen   |
+--------+---------+-----------+-----------+
         |         |           |
         v         v           v
+--------+  +------+----+  +--+----------+
|CPU.CPP |  |NESVIDEO   |  |NESSOUND    |
|Scheduler|  |PPU render |  |NES DAC mix |
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
- `CPU.CPP` -- Frame/scanline scheduler, handles VBlank/IRQ timing
- `m6502/` -- 6502 CPU core (fake6502, public domain)
- `m6502_shim.cpp` -- Bridges fake6502 API to NESticle's `m6502.h` interface

### PPU (`NESVIDEO.CPP/.H`, `PPU.H`)
- Pattern tables (8x8 tile bitmaps, 1K banks)
- Name table caching with dirty-rectangle tracking
- Sprite rendering (8x8 and 8x16 modes)
- Scanline event list for mid-frame register changes
- Palette management (NES 64-color -> 8-bit indexed)

### APU (`NESSOUND.CPP/.H`)
- Original NESticle sound engine with all 5 channels
- Square x2, triangle, noise (LFSR), DMC
- NES hardware nonlinear mixer using DAC formulas:
  - Pulse DAC: `pulse_out = 95.88 / (8128/p + 100)`
  - TND DAC: `tnd_out = 159.79 / (1/(t/8227 + n/12241 + d/22638) + 100)`
- DC blocking high-pass filter (~7 Hz cutoff) removes unipolar offset
- Channels output raw DAC levels (0-15 / 0-127) via `mixraw()`
- Fallback to original linear mixer available

### Memory Mappers (`MMC.CPP/.H`)
- Base `memorymapper` class with virtual `write()`/`reset()`/`ppuread()`
- Supports ~8 mapper types (MMC_NONE through MMC_F5xxx)
- ROM/VROM bank switching

### GUI (`GUI.H`, `GUIRECT.H`, `GUIROOT.H`)
- Custom 8-bit palettized widget system
- Tree-based hierarchy: ROOT -> GUIroot -> GUIbox -> GUIcontents
- Widgets: buttons, checkboxes, listboxes, scrollbars, edit controls
- Cascading popup menus with proper click behavior
- Graphics loaded from `gui.vol` resource file

### Input (`INPUT.CPP/.H`, `INPUTW.CPP`)
- Keyboard input via SDL2 scancode mapping
- SDL GameController support with configurable button remapping
- `padmap` struct stores NES button -> SDL button assignments
- `redefinepad` dialog for interactive button picker (frame-based polling)
- Settings persisted in `inputdevicesettings`

### Platform (`platform_sdl2.cpp`)
- SDL2 window, 8-bit surface, palette
- Audio ring buffer with SDL callback
- Keyboard, mouse, GameController input
- ALT+ENTER fullscreen toggle with SDL logical scaling
- Replaces original DirectDraw/DirectSound/Win32 backend

### ROM Loading (`ROM.CPP/.H`)
- iNES format parsing (.nes files)
- Trainer support, battery-backed RAM
- Save states (snapshot)

## Data Flow (Per Frame)
1. `gametimer()` called at 60Hz
2. `tickemu()` runs VBlank cycles -> frame cycles via `m6502exec()`
3. Mid-frame events tracked in scanline event list
4. `updatescreen()` renders PPU state to backbuffer
5. SDL2 flips surface to window

## Audio Data Flow
1. `nessound::update()` called per frame
2. Each channel fills `rawbuf[]` with raw NES DAC levels via `mixraw()`
3. Master mixer applies NES DAC formulas per sample
4. DC blocking filter removes unipolar offset
5. Samples written to ring buffer, consumed by SDL audio callback
