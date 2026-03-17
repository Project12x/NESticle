# Changelog

All notable changes to this project will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [0.5.0] - 2026-03-17

### Added
- **Gamepad button remapping**: New "Redefine buttons..." dialog lets users remap NES B/A/Select/Start to any SDL GameController button. Accessible from the input device dialog when a gamepad is selected.
- **`padmap` struct**: Stores per-player gamepad button assignments, persisted in `inputdevicesettings`.
- **NES hardware nonlinear mixer**: Implements the actual NES DAC formulas (`pulse_out = 95.88/(8128/p+100)`, `tnd_out = 159.79/(1/(t/8227+n/12241+d/22638)+100)`) for hardware-accurate channel mixing. Togglable via `nessound::nonlinear_mixing`.
- **DC blocking high-pass filter**: Removes DC offset from the nonlinear mixer's unipolar [0,1] output, preserving only the AC audio signal (~7 Hz cutoff).
- **Mixer mode diagnostic**: Startup log confirms which mixer mode is active.

### Fixed
- **Input dialog buttons**: "Redefine keys..." and "Save settings" buttons no longer disappear or spawn outside the dialog window when changing input device. Buttons are created once and updated via `settext()`/`moveto()` instead of delete/recreate.
- **Gamepad button polling**: Switched from keyboard-triggered `keyhit()` to frame-based polling in `drawdata()` with `SDL_GameControllerUpdate()`.
- **Uninitialized `raw_vol`**: All `neschannel` state (`vol`, `raw_vol`, `count`, `K`, `freq`) now zero-initialized in `reset()`, preventing garbage noise output.

## [0.4.2] - 2026-03-17

### Fixed
- **Load button positioning**: Moved from title bar to bottom of dialog (absolute y-coord bug in GUIonebuttonbox/GUItwobuttonbox)
- **Filename text clipping**: Listbox text truncated to fit within scrollbar bounds using `font->getwidth()`
- **NES Timing crash**: Guarded `GUIedit::draw` drawrect against negative width when prompt text exceeds edit control width at 640x480 fonts
- **NES Timing layout**: Widened dialog from 140x80 to 250x80, edit controls from xw=30 to xw=180
- **About dialog layout**: Widened from 290x65 to 500x90, text pushed to x=140 to clear buddy portrait, proper line spacing
- **Maximize button icon**: Now uses `guivol.wmmark` image resource
- **Escape from fullscreen**: Added keyhit handler to GUImaximizebox for Escape key restore
- **ROM loader width**: Increased content from 140px to 280px

## [0.4.1] - 2026-03-09

### Fixed
- **GUIVOL struct alignment**: Added missing `sfont` and 12 other fields to match original gui.vol entry order; all icons now display correctly
- **GUI constructor coordinates**: Fixed 5 widget constructors using relative coordinates where absolute were required
- **Button bar overlap**: GUIonebuttonbox/GUItwobuttonbox now extend box height by 18px for the bottom button bar
- **Popup menu crash**: Fixed use-after-free in GUIpopupmenu::domenuitem()

## [0.4.0] - 2026-03-09

### Added
- **True Fullscreen**: ALT+ENTER toggles between 640x480 windowed GUI and 256x224 native NES fullscreen
- **SDL Logical Scaling**: Aspect-ratio-correct scaling and letterboxing
- **Command-line ROM loading**: Fixed path handling for files with spaces

## [0.3.1] - 2026-03-08

### Fixed
- **SMB1 Freeze**: Fixed emulation freeze (Sprite 0 Hit CPU starvation)
- **Clock Synchronization**: Accurate cycle count synthesis from fake6502 to NESticle PPU
- **Clear-On-Read**: $2002 VBlank flag properly cleared on read

## [0.3.0] - 2026-03-07

### Fixed
- **Sprite Flipping**: Repaired 8-bit signed integer casts for flipx/flipy
- **Palette Bleeding**: Masked attrib byte with `& 3`
- **Noise Channel timing**: LFSR duty/volume fixes
- **Input System**: Restored standard keyboard scanning via SDL2

### Added
- Animated cursor rendering via ARGB framebuffer overlay

### Discovered
- Complete missing source code recovered from SOUND.ZIP archive
